/**
 * lbreloc_bridge.cpp — PORT replacement for src/lb/lbreloc.c
 *
 * Replaces ROM DMA-based file loading with LUS ResourceManager calls.
 * Uses the token-based pointer indirection system: the relocation logic
 * computes real 64-bit pointers but stores 32-bit tokens in the 4-byte
 * data slots. Game code resolves tokens via RELOC_RESOLVE().
 *
 * This file is compiled as C++ (needs LUS headers) but exports C-linkage
 * functions matching the signatures in src/lb/lbreloc.h.
 */

#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>  // _exit
#endif

#include "resource/RelocFile.h"
#include "resource/RelocFileTable.h"
#include "resource/RelocPointerTable.h"
#include "resource/SyntheticReloc.h"
#include "ssb64_reloc_rebuild.h"
#include "bridge/lbreloc_byteswap.h"

extern "C" void port_aobj_register_halfswapped_range(void *base, unsigned long size);

// Bridge-local type definitions.
// These MUST be ABI-compatible with the decomp definitions in lbtypes.h.
// We define them here to avoid including the decomp's include/ directory
// (which shadows system headers and breaks C++ standard library includes).
#include "bridge/port_types.h"

#define LBRELOC_CACHE_ALIGN(x) (((x) + 0xF) & ~0xF)

enum {
	nLBFileLocationExtern  = 0,
	nLBFileLocationDefault = 1,
	nLBFileLocationForce   = 2,
};

struct LBFileNode
{
	u32 id;
	void *addr;
};

struct LBRelocSetup
{
	uintptr_t table_addr;
	u32 table_files_num;
	void *file_heap;
	size_t file_heap_size;
	LBFileNode *status_buffer;
	size_t status_buffer_size;
	LBFileNode *force_status_buffer;
	size_t force_status_buffer_size;
};

struct LBInternBuffer
{
	uintptr_t rom_table_lo;
	u32 total_files_num;
	uintptr_t rom_table_hi;
	void *heap_start;
	void *heap_ptr;
	void *heap_end;
	s32 status_buffer_num;
	s32 status_buffer_max;
	LBFileNode *status_buffer;
	s32 force_status_buffer_num;
	s32 force_status_buffer_max;
	LBFileNode *force_status_buffer;
};

// Same size as the N64 LBTableEntry (12 bytes) — used for size calculation
struct LBTableEntry
{
	ub32 is_compressed : 1;
	u32 data_offset : 31;
	u16 reloc_intern_offset;
	u16 compressed_size;
	u16 reloc_extern_offset;
	u16 decompressed_size;
};

// Forward declarations (C linkage)
extern "C" {
extern void syDebugPrintf(const char *fmt, ...);
extern void scManagerRunPrintGObjStatus(void);
extern void portResetPackedDisplayListCache(void);
extern void port_log(const char *fmt, ...);
extern void gmColScriptsLinkRelocTargets(void);

// DL-source range registry (see port/port_dl_ranges.h).
void port_dl_range_register(const void *base, size_t size, const char *label);
void port_dl_range_unregister(const void *base);

// Forward declarations for functions used in mutual recursion
void* lbRelocGetExternBufferFile(u32 id);
void* lbRelocGetInternBufferFile(u32 id);
void* lbRelocGetForceExternBufferFile(u32 id);
}

// // // // // // // // // // // //
//                               //
//   GLOBAL / STATIC VARIABLES   //
//                               //
// // // // // // // // // // // //

static LBInternBuffer sLBRelocInternBuffer;

static u32 *sLBRelocExternFileIDs;
static s32 sLBRelocExternFileIDsNum;
static s32 sLBRelocExternFileIDsMax;
static void *sLBRelocExternFileHeap;

struct PortRelocFileRange
{
	uintptr_t base;
	size_t size;
	u32 file_id;
	const char *path;
};

static std::vector<PortRelocFileRange> sPortRelocFileRanges;

static void portRelocEvictFileRangesInRange(void *base, size_t size)
{
	if ((base == nullptr) || (size == 0))
	{
		return;
	}

	uintptr_t begin = reinterpret_cast<uintptr_t>(base);
	uintptr_t end = begin + size;

	/* Unregister matching ranges from the DL-range registry so a stale
	 * reloc-file pointer in cmd_stack is rejected on next gfx_step (rather
	 * than dereferencing into now-recycled memory). Done before the erase
	 * so we still have the base pointers. */
	for (const auto &r : sPortRelocFileRanges) {
		uintptr_t rb = r.base;
		uintptr_t re = rb + r.size;
		if ((r.size != 0) && (rb < end) && (begin < re)) {
			port_dl_range_unregister(reinterpret_cast<const void*>(rb));
		}
	}

	sPortRelocFileRanges.erase(
		std::remove_if(sPortRelocFileRanges.begin(), sPortRelocFileRanges.end(),
			[begin, end](const PortRelocFileRange &range) {
				uintptr_t range_begin = range.base;
				uintptr_t range_end = range_begin + range.size;

				return (range.size != 0) && (range_begin < end) && (begin < range_end);
			}),
		sPortRelocFileRanges.end());
}

/* Walk a status-buffer array, drop every entry whose .addr falls inside
 * [lo, hi). Uses unordered swap-with-last + decrement-count erase: lookup
 * iterates the whole list linearly so order doesn't matter. After this
 * runs, lbRelocFindStatusBufferFile(id) for an evicted file returns NULL,
 * which makes ftManagerSetupFilesKind's *p_file_* assignments produce
 * NULL globals and the existing `*file_head == NULL` defensive guard in
 * efManagerMakeEffect bail cleanly. The eventual file-reload path
 * re-allocates fresh memory and re-adds the entry on next request. */
static void portStatusBufferEvictRange(LBFileNode *entries, s32 *p_num,
                                        uintptr_t lo, uintptr_t hi)
{
	if (entries == nullptr || p_num == nullptr) return;
	s32 i = 0;
	while (i < *p_num)
	{
		uintptr_t a = reinterpret_cast<uintptr_t>(entries[i].addr);
		if (a >= lo && a < hi)
		{
			entries[i] = entries[*p_num - 1];
			(*p_num)--;
		}
		else
		{
			i++;
		}
	}
}

/* Called by syTaskmanStartTask before reusing the scene arena for the next
 * scene. Evicts every port-side cache that may hold a pointer into the old
 * arena's contents (DL widening, texture upload, struct fixup, reloc file
 * ranges) so a stale lookup can't resolve through prior-scene state. Same
 * eviction APIs run per-relocFile-load in port_reloc_lb_load_request. */
extern "C" void port_taskman_evict_arena_caches(const void *base, size_t size)
{
	if ((base == nullptr) || (size == 0)) return;
	extern void portPackedDisplayListCacheDeleteRange(const void *base, size_t size);
	extern void portTextureCacheDeleteRange(const void *base, size_t size);
	extern void portEvictStructFixupsInRange(const void *base, size_t size);
	portPackedDisplayListCacheDeleteRange(base, size);
	portTextureCacheDeleteRange(base, size);
	portEvictStructFixupsInRange(base, size);
	portRelocEvictFileRangesInRange(const_cast<void *>(base), size);
	/* Structural fix: invalidate tokens whose pointers fall in the recycled
	 * scene arena. With the per-slot generational model in RelocPointerTable
	 * .cpp, this NULLs only the affected slots (bumping their generation)
	 * and pushes their indices onto a free list for reuse. Tokens pointing
	 * at intern-buffer files (mainmotion, submotion, model, special1-4,
	 * shieldpose) — which persist across scenes — remain valid because
	 * they don't intersect this range. This is THE structural fix that
	 * eliminates the variant-1/2/3 stale-data crash family — see
	 * docs/bugs/linux_stale_scene_data_family_2026-05-11.md. */
	portRelocInvalidateRange(base, size);

	/* Variant 6 fix (silent particle/sprite corruption on cross-mode scene
	 * transitions, e.g. Training → quit → VS with overlapping fighters):
	 * sLBRelocInternBuffer.status_buffer[] is a flat decomp-side cache of
	 * {file_id → host addr} that lbRelocGetStatusBufferFile() reads. It is
	 * only reset in lbRelocInitSetup, which fires at mode entry, NOT per
	 * scene. Within a mode the cache accumulates; across modes that don't
	 * happen to traverse a mode-init path, entries from a prior mode
	 * survive — and when their .addr lives in the recycled scene arena,
	 * the cache returns a pointer to bytes the upcoming memset(0) is
	 * about to zero.
	 *
	 * Subsequent ftManagerSetupFilesPlayablesAll → ftManagerSetupFilesKind
	 * calls then assign that stale pointer into the per-character gFTData*
	 * globals (e.g. gFTDataMarioSpecial2). Downstream readers dereference
	 * them, read zero bytes, MObjSub.sprites resolves to NULL, the
	 * defensive `current_sprite != NULL` guard in gcDrawMObjForDObj skips
	 * gDPSetTextureImage, and particle quads emit without a texture bound
	 * — rendering as flat boxes. Same shape touches stage-sprite elements
	 * (clouds, bushes, foreground decoration) that use the particle draw
	 * path. Fighter mesh draws are unaffected because their model file
	 * lives in the intern buffer (out of arena range).
	 *
	 * Surgical fix: evict status-buffer entries whose .addr falls in the
	 * arena range, exactly mirroring the existing token-invalidation
	 * scope. Intern-buffer entries (out of arena range) survive untouched.
	 * After this, ftManagerSetupFilesKind's lookup for an evicted id
	 * returns NULL, the global is set NULL, and the file-load path runs
	 * fresh on next demand. */
	{
		uintptr_t lo = reinterpret_cast<uintptr_t>(base);
		uintptr_t hi = lo + size;
		portStatusBufferEvictRange(sLBRelocInternBuffer.status_buffer,
		                            &sLBRelocInternBuffer.status_buffer_num, lo, hi);
		portStatusBufferEvictRange(sLBRelocInternBuffer.force_status_buffer,
		                            &sLBRelocInternBuffer.force_status_buffer_num, lo, hi);
	}
}

static bool portRelocIsFighterFigatreeFile(u32 file_id)
{
	static const char sFighterAnimPrefix[] = "reloc_animations/FT";
	static const char sFighterSubmotionPrefix[] = "reloc_submotions/FT";
	/* SCExplainMain contains arrays of FTKeyEvent u16s (per-player
	 * tutorial button inputs) and SCExplainPhase u16/u8 fields.
	 * The u16-halfswap applied by portRelocFixupFighterFigatree is
	 * what makes u16 pair reads produce the BE value on LE.
	 * Without it, u16[0] and u16[1] are swapped per u32, so the
	 * first FTKeyEvent reads from what should be the SECOND u16 of
	 * the pair — typically 0x0000, which parses as End and kills the
	 * tutorial's button scripting. */
	static const char sSCExplainMainPath[] = "reloc_scene/SCExplainMain";

	if (file_id >= RELOC_FILE_COUNT || gRelocFileTable[file_id] == NULL) return false;
	const char *path = gRelocFileTable[file_id];
	return (std::strncmp(path, sFighterAnimPrefix, sizeof(sFighterAnimPrefix) - 1) == 0) ||
	       (std::strncmp(path, sFighterSubmotionPrefix, sizeof(sFighterSubmotionPrefix) - 1) == 0) ||
	       (std::strcmp(path, sSCExplainMainPath) == 0);
}

static void portRelocFixupFighterFigatree(void *ram_dst, size_t copy_size, const std::vector<uint8_t> &reloc_words)
{
	u32 *words;
	size_t word_count;
	size_t i;

	if ((ram_dst == NULL) || (copy_size < sizeof(u32)))
	{
		return;
	}
	words = (u32*)ram_dst;
	word_count = copy_size / sizeof(u32);

	if (reloc_words.size() < word_count)
	{
		word_count = reloc_words.size();
	}
	for (i = 0; i < word_count; i++)
	{
		if (reloc_words[i] == 0)
		{
			words[i] = (words[i] << 16) | (words[i] >> 16);
		}
	}
}

// // // // // // // // // // // //
//                               //
//       RESOURCE LOADING        //
//                               //
// // // // // // // // // // // //

static std::shared_ptr<RelocFile> portLoadRelocResource(u32 file_id)
{
	if (file_id >= RELOC_FILE_COUNT || gRelocFileTable[file_id] == NULL)
	{
		spdlog::error("lbReloc bridge: invalid file_id {} (0x{:08X})", file_id, file_id);
		return nullptr;
	}

	auto ctx = Ship::Context::GetInstance();
	if (!ctx)
	{
		spdlog::error("lbReloc bridge: no Ship::Context");
		return nullptr;
	}

	// Deblobbed bundles are synthesized from their typed o2r slices
	// (docs/deblob.md). Parents ship with archive:false, so on synthesis
	// failure the LoadResource below misses and the stale-archive handler
	// prints its actionable delete-and-relaunch message — the synthesis
	// error above it names the exact slice and invariant.
	if (portGetSyntheticRelocSpec(file_id) != nullptr)
	{
		auto synthetic = portBuildSyntheticRelocResource(file_id);
		if (synthetic)
		{
			return synthetic;
		}
		spdlog::error("[deblob] synthesis failed for file_id {} — no archived parent"
		              " ships for deblobbed bundles; see the slice error above",
		              file_id);
	}

	std::string path(gRelocFileTable[file_id]);
	auto resource = ctx->GetResourceManager()->LoadResource(path);

	if (!resource)
	{
		/* Catch the "stale BattleShip.o2r" failure mode visibly before the
		 * caller dereferences this NULL and SIGSEGVs deep in portFixupSprite.
		 *
		 * When a user upgrades the port across an asset-format bump (e.g.
		 * factory version 0 vs 3), their cached BattleShip.o2r is silently
		 * incompatible: every RELO resource fails to load with "GetFactory
		 * failed to find an import factory" deep inside libultraship, but
		 * we only see NULL here. A healthy run loads thousands of reloc
		 * resources — the first one failing means the archive is unusable.
		 *
		 * Print an actionable message and exit cleanly rather than crash
		 * minutes later in a non-obvious place. */
		static bool sExitedOnStaleArchive = false;
		if (!sExitedOnStaleArchive)
		{
			sExitedOnStaleArchive = true;
			std::string dataDirO2r =
			    Ship::Context::GetPathRelativeToAppDirectory("BattleShip.o2r");
			fprintf(stderr,
			    "\n"
			    "============================================================\n"
			    "SSB64 PC port: failed to load asset from BattleShip.o2r\n"
			    "  resource: %s\n"
			    "  file_id:  %u\n"
			    "============================================================\n"
			    "\n"
			    "This almost always means your cached BattleShip.o2r was\n"
			    "packed by an older build whose resource format doesn't\n"
			    "match this binary's registered factory version.\n"
			    "\n"
			    "Fix: delete the stale archive and re-launch — the binary\n"
			    "will re-extract assets from your baserom on first run.\n"
			    "\n"
			    "    rm \"%s\"\n"
			    "\n"
			    "If that doesn't help, check ssb64.log for the underlying\n"
			    "ResourceLoader 'GetFactory failed' error which names the\n"
			    "exact Format/Version mismatch.\n"
			    "============================================================\n"
			    "\n",
			    path.c_str(), file_id, dataDirO2r.c_str());
			port_log("SSB64: STALE BattleShip.o2r detected — first reloc load failed "
			         "(path=%s file_id=%u). Delete '%s' and re-launch to re-extract.\n",
			         path.c_str(), file_id, dataDirO2r.c_str());
			std::fflush(stderr);
			/* _exit(0): skip C++ static destructors and spdlog flush so the
			 * Linux desktop doesn't show a "fatal error" popup. The exit
			 * code is intentionally 0 — the meaningful failure signal is
			 * the message we just printed; a non-zero code triggers
			 * crash-reporter UI on some desktops (Fedora/GNOME). Use POSIX
			 * _exit on POSIX, _Exit (stdlib) as a portable fallback. */
#if defined(__unix__) || defined(__APPLE__)
			_exit(0);
#else
			std::_Exit(0);
#endif
		}
		spdlog::error("lbReloc bridge: failed to load '{}' (file_id={})", path, file_id);
		return nullptr;
	}

	auto relocFile = std::dynamic_pointer_cast<RelocFile>(resource);
	if (!relocFile)
	{
		spdlog::error("lbReloc bridge: '{}' is not a RelocFile", path);
		return nullptr;
	}

	return relocFile;
}

// All game-facing functions have C linkage
extern "C" {

// // // // // // // // // // // //
//                               //
//      STATUS BUFFER FUNCS      //
//                               //
// // // // // // // // // // // //

void* lbRelocFindStatusBufferFile(u32 id)
{
	s32 i;

	if (sLBRelocInternBuffer.status_buffer_num == 0)
	{
		return NULL;
	}
	else for (i = 0; i < sLBRelocInternBuffer.status_buffer_num; i++)
	{
		if (id == sLBRelocInternBuffer.status_buffer[i].id)
		{
			return sLBRelocInternBuffer.status_buffer[i].addr;
		}
	}
	return NULL;
}

void* lbRelocGetStatusBufferFile(u32 id)
{
	return lbRelocFindStatusBufferFile(id);
}

void* lbRelocFindForceStatusBufferFile(u32 id)
{
	s32 i;

	if (sLBRelocInternBuffer.force_status_buffer_num != 0)
	{
		for (i = 0; i < sLBRelocInternBuffer.force_status_buffer_num; i++)
		{
			if (id == sLBRelocInternBuffer.force_status_buffer[i].id)
			{
				return sLBRelocInternBuffer.force_status_buffer[i].addr;
			}
		}
	}
	return lbRelocFindStatusBufferFile(id);
}

void* lbRelocGetForceStatusBufferFile(u32 id)
{
	return lbRelocFindForceStatusBufferFile(id);
}

void lbRelocAddStatusBufferFile(u32 id, void *addr)
{
	u32 num = sLBRelocInternBuffer.status_buffer_num;

	if (num >= (u32)sLBRelocInternBuffer.status_buffer_max)
	{
		while (TRUE)
		{
			syDebugPrintf("Relocatable Data Manager: Status Buffer is full !!\n");
			scManagerRunPrintGObjStatus();
		}
	}
	sLBRelocInternBuffer.status_buffer[num].id = id;
	sLBRelocInternBuffer.status_buffer[num].addr = addr;
	sLBRelocInternBuffer.status_buffer_num++;
}

void lbRelocAddForceStatusBufferFile(u32 id, void *addr)
{
	u32 num = sLBRelocInternBuffer.force_status_buffer_num;

	if (num >= (u32)sLBRelocInternBuffer.force_status_buffer_max)
	{
		while (TRUE)
		{
			syDebugPrintf("Relocatable Data Manager: Force Status Buffer is full !!\n");
			scManagerRunPrintGObjStatus();
		}
	}
	sLBRelocInternBuffer.force_status_buffer[num].id = id;
	sLBRelocInternBuffer.force_status_buffer[num].addr = addr;
	sLBRelocInternBuffer.force_status_buffer_num++;
}

// // // // // // // // // // // //
//                               //
//   BRIDGE: LOAD & RELOCATE     //
//                               //
// // // // // // // // // // // //

/**
 * Copy `src_bytes` into `ram_dst` and perform the full reloc-fixup
 * pipeline against it: cache evictions, byte-swap, status-buffer
 * registration, internal/external pointer chain walks (token-based),
 * figatree-specific cleanup, audit emission.
 *
 * Exposed for mod-loader use: a mod that ships its own reloc-format
 * file bytes (e.g., new-character assets bundled inside a TCC mod)
 * calls this directly instead of routing through portLoadRelocResource,
 * which only sees files inside BattleShip.o2r. Mod file_ids may sit
 * outside the vanilla 0..RELOC_FILE_COUNT-1 range; we gate operations
 * that need a valid gRelocFileTable[] entry on the usual bounds check.
 *
 * Token-based relocation: instead of writing 64-bit void* into 4-byte
 * slots (which would corrupt adjacent data on LP64), each target
 * pointer is registered in the token table and the 32-bit token is
 * written into the 4-byte slot.
 */

/* Explicit relocation input for synthesized (deblobbed) bundles: their
 * chain descriptor words were replaced with real content at extraction, so
 * the loader applies these flat, chain-ordered lists instead of walking
 * the in-data chains (docs/deblob.md, RelocFile.h). */
struct PortRelocExplicitRelocation
{
	const RelocExplicitInternSlot *intern_slots;
	size_t intern_count;
	const RelocExplicitExternSlot *extern_slots;
	size_t extern_count;
};

/* One intern relocation slot, shared by the chain walk, the explicit flat
 * loop, and the mod-private path so all three tokenize identically:
 * optional SETTIMG/G_VTX texture fixup, target pointer -> token, optional
 * figatree slot marking, token stamped into the slot, optional chain-slot
 * note for the generational eviction bookkeeping. */
static inline void portRelocApplyInternSlot(
	void *ram_dst, size_t copySize,
	uint32_t slot_byte_off, uint32_t target_byte_off,
	bool texture_fixup, bool note_chain,
	std::vector<uint8_t> *figatree_words)
{
	if (texture_fixup)
	{
		portRelocFixupTextureFromChain(ram_dst, copySize,
		                               slot_byte_off, target_byte_off);
	}

	void *target = (void *)((uintptr_t)ram_dst + target_byte_off);
	u32 token = portRelocRegisterPointer(target);

	uint32_t slot_word = slot_byte_off / (uint32_t)sizeof(u32);
	if (figatree_words != nullptr && slot_word < figatree_words->size())
	{
		(*figatree_words)[slot_word] = 1;
	}

	u32 *slot = (u32 *)((uintptr_t)ram_dst + slot_byte_off);
	*slot = token;
	if (note_chain)
	{
		portRelocNoteChainSlot(slot);
	}
}

static void portRelocLoadFileFromBytesImpl(
	unsigned int    file_id,
	void           *ram_dst,
	unsigned int    bytes_num,
	int             loc,
	const void     *src_bytes,
	unsigned int    src_size,
	unsigned short  reloc_intern_offset,
	unsigned short  reloc_extern_offset,
	const unsigned short *extern_file_ids,
	unsigned int    extern_count,
	int             force_figatree_fixup,
	const PortRelocExplicitRelocation *explicit_reloc)
{
	/* portRelocIsFighterFigatreeFile looks up gRelocFileTable[file_id]
	 * to match the "reloc_animations/" path prefix. Mod-registered file
	 * IDs sit outside the vanilla table range so the path lookup fails
	 * and the halfswap step is skipped, leaving anim-joint tokens in
	 * chain-entry form. The mod can override via force_figatree_fixup
	 * for its own animation files. */
	bool is_fighter_figatree =
		force_figatree_fixup || portRelocIsFighterFigatreeFile(file_id);
	std::vector<uint8_t> figatree_reloc_words;
	const char *table_path = (file_id < RELOC_FILE_COUNT) ? gRelocFileTable[file_id] : nullptr;

	// Gated: SSB64_LOG_LBRELOC_LOAD=1 logs every file load. Helpful when
	// tracing which reloc files are loaded per scene.
	if (getenv("SSB64_LOG_LBRELOC_LOAD") != nullptr) {
		static int s_load_log_count = 0;
		if (s_load_log_count < 512) {
			s_load_log_count++;
			port_log("SSB64: lbReloc LOAD file_id=%u loc=%d path=%s fig=%d\n",
				file_id, loc,
				table_path ? table_path : "(null)",
				(int)is_fighter_figatree);
		}
	}

	if (src_bytes == nullptr || src_size == 0)
	{
		spdlog::error("portRelocLoadFileFromBytes: NULL/empty src for file_id {}", file_id);
		return;
	}

	// Copy decompressed data into the game's heap allocation
	size_t copySize = src_size;
	if (copySize > bytes_num && bytes_num > 0)
	{
		spdlog::warn("lbReloc bridge: file_id {} data ({} bytes) exceeds "
		             "buffer ({} bytes), truncating", file_id, copySize, bytes_num);
		copySize = bytes_num;
	}
	if (is_fighter_figatree)
	{
		figatree_reloc_words.resize(copySize / sizeof(u32), 0);
	}
	// Invalidate fixup idempotency state that keyed on addresses inside the
	// region we're about to overwrite. Needed because bump-reset heaps (e.g.
	// the stage-select wallpaper heaps rewound by lbRelocGetForceExternHeapFile
	// on every cursor tick) reuse the same addresses across stages; without
	// this, portFixupSprite/Bitmap/SpriteBitmapData wrongly skip the new load
	// and the BSWAP texel loop later walks past the texture on bogus sizes.
	portEvictStructFixupsInRange(ram_dst, copySize);
	// Evict any libultraship texture-cache entries whose origAddr falls in
	// the heap range we're about to overwrite. The Fast3D cache key is
	// {addr, fmt, siz, sizeBytes, masks, maskt, w, h} — same-shape textures
	// (e.g. the 300x6 RGBA16 wallpaper tile rows used by every stage) at a
	// reused heap address would otherwise hit a stale cached upload from
	// the prior file. Symptom: scene 45 (DK+Samus Kongo Jungle) renders
	// the prior scene's wallpaper. See docs/dk_intro_wallpaper_*.md
	extern void portTextureCacheDeleteRange(const void *base, size_t size);
	portTextureCacheDeleteRange(ram_dst, copySize);
	// Evict cached packed-DL widenings whose source pointer falls in the
	// range we're about to overwrite. Without this, the widening cache
	// hands back a vector with stale fileBase/fileSize, segment-0E sub-DL
	// references resolve to the prior file's address window, and the
	// interpreter walks garbage — fingerprint of issue #103/#128.
	extern void portPackedDisplayListCacheDeleteRange(const void *base, size_t size);
	portPackedDisplayListCacheDeleteRange(ram_dst, copySize);
	portRelocEvictFileRangesInRange(ram_dst, copySize);
	memcpy(ram_dst, src_bytes, copySize);

	/* lbRelocGetExternHeapFile sets sLBRelocExternFileHeap = heap so that
	 * subsequent dep loads (triggered by the chain walk via
	 * lbRelocGetExternBufferFile) bump-allocate sequentially past this
	 * file's buffer. Mod-loaded files bypass that entry point entirely,
	 * leaving the bump pointer at whatever the last vanilla load left it
	 * at -- often pointing *into* the buffer we just memcpy'd. When a
	 * cache-miss dep load fires inside the chain walk it then writes its
	 * bytes through this file's region, corrupting slots before the walk
	 * has a chance to tokenize them. Mirror the vanilla setup by
	 * advancing the bump pointer past this file's end so dep allocations
	 * land in clean memory. Done for Extern and Force; Default uses the
	 * intern-buffer bump heap which mod paths never enter. */
	if (loc == nLBFileLocationExtern || loc == nLBFileLocationForce) {
		sLBRelocExternFileHeap = (void *)LBRELOC_CACHE_ALIGN(
			(uintptr_t)ram_dst + copySize);
	}

	// One-shot raw dump for verification against ROM extraction.
	// Set SSB64_DUMP_FILE_ID env var to a file_id; this writes the post-memcpy,
	// pre-byteswap bytes to debug_traces/port_file_<id>.bin.  Compare against
	// debug_tools/reloc_extract/reloc_extract.py output for the same id.
	//
	// If SSB64_DUMP_ALL_FIGATREE=1, dump every fighter figatree file loaded
	// (pre-byteswap) for bulk comparison.
	{
		const char *dump_id_env = getenv("SSB64_DUMP_FILE_ID");
		const char *dump_all_env = getenv("SSB64_DUMP_ALL_FIGATREE");
		bool do_dump = false;
		if (dump_id_env != nullptr) {
			unsigned long target_id = strtoul(dump_id_env, nullptr, 0);
			if ((unsigned long)file_id == target_id) do_dump = true;
		}
		if (dump_all_env != nullptr && dump_all_env[0] == '1' && is_fighter_figatree) {
			do_dump = true;
		}
		if (do_dump) {
			char path[256];
			snprintf(path, sizeof(path), "debug_traces/port_file_%u.bin", file_id);
			FILE *df = fopen(path, "wb");
			if (df != nullptr) {
				fwrite(ram_dst, 1, copySize, df);
				fclose(df);
				spdlog::info("[port-dump] wrote {} bytes for file_id={} ({}) to {}",
				             copySize, file_id, table_path ? table_path : "(mod)", path);
			}
		}
	}

	// Byte-swap from N64 big-endian to native little-endian.
	// Must happen BEFORE the reloc chain walk (which reads u16 fields
	// from u32 words using bit shifts that assume native byte order).
	portRelocByteSwapBlob(ram_dst, copySize, (unsigned int)file_id);

	// Register in status buffer
	if (loc == nLBFileLocationForce)
	{
		lbRelocAddForceStatusBufferFile(file_id, ram_dst);
	}
	else
	{
		lbRelocAddStatusBufferFile(file_id, ram_dst);
	}

	sPortRelocFileRanges.push_back({ reinterpret_cast<uintptr_t>(ram_dst), copySize, file_id, table_path });

	/* Mirror this range into the DL-range registry so gfx_step's bounds
	 * check accepts DLs resolved through reloc files. Path string from
	 * gRelocFileTable is static (linker-emitted), safe to retain. */
	port_dl_range_register(ram_dst, copySize, table_path ? table_path : "reloc?");

	// --- Internal pointer relocation (token-based) ---
	//
	// Each reloc descriptor is a 4-byte word in the data:
	//   bits [31:16] = next descriptor offset (in words), 0xFFFF = end
	//   bits [15:0]  = target offset within this file (in words)
	//
	// On N64, the code overwrites this 4-byte word with a void* pointer.
	// In the port, we compute the pointer, register it as a token, and
	// write the 32-bit token into the 4-byte word.

	if (explicit_reloc != nullptr)
	{
		// --- Explicit relocation (synthesized/deblobbed bundles) ---
		//
		// The chain descriptor words were replaced with real content at
		// extraction, so the in-data chains are unwalkable; apply the
		// spec's flat, chain-ordered slot lists instead (docs/deblob.md).
		// Offsets arrive in the bundle's CURRENT layout, so this path is
		// also what makes relayouted (resized-mod) bundles relocatable.
		for (size_t i = 0; i < explicit_reloc->intern_count; i++)
		{
			const RelocExplicitInternSlot &s = explicit_reloc->intern_slots[i];
			if ((size_t)s.SlotByteOff + sizeof(u32) > copySize ||
			    (size_t)s.TargetByteOff >= copySize)
			{
				spdlog::error("[deblob] I9 violated: explicit intern slot {} OOB "
				              "(file_id {}, slot=0x{:X} target=0x{:X} size=0x{:X})"
				              " — diagnose: SSB64_SYNTH_INSPECT={}",
				              i, file_id, s.SlotByteOff, s.TargetByteOff, copySize, file_id);
				break;
			}
			portRelocApplyInternSlot(ram_dst, copySize,
			                         s.SlotByteOff, s.TargetByteOff,
			                         /* texture_fixup */ true, /* note_chain */ true,
			                         is_fighter_figatree ? &figatree_reloc_words : nullptr);
		}

		for (size_t i = 0; i < explicit_reloc->extern_count; i++)
		{
			const RelocExplicitExternSlot &s = explicit_reloc->extern_slots[i];
			if ((size_t)s.SlotByteOff + sizeof(u32) > copySize)
			{
				spdlog::error("[deblob] I9 violated: explicit extern slot {} OOB "
				              "(file_id {}, slot=0x{:X} size=0x{:X})"
				              " — diagnose: SSB64_SYNTH_INSPECT={}",
				              i, file_id, s.SlotByteOff, copySize, file_id);
				break;
			}

			// Same dependency resolution as the chain walk below, but the
			// dep file id comes from the slot itself instead of the
			// positional extern id list.
			void *vaddr_extern;
			if (loc == nLBFileLocationForce)
			{
				vaddr_extern = lbRelocFindForceStatusBufferFile(s.DepFileId);
			}
			else
			{
				vaddr_extern = lbRelocFindStatusBufferFile(s.DepFileId);
			}
			if (vaddr_extern == NULL)
			{
				switch (loc)
				{
				case nLBFileLocationExtern:
					vaddr_extern = lbRelocGetExternBufferFile(s.DepFileId);
					break;
				case nLBFileLocationDefault:
					vaddr_extern = lbRelocGetInternBufferFile(s.DepFileId);
					break;
				case nLBFileLocationForce:
					vaddr_extern = lbRelocGetForceExternBufferFile(s.DepFileId);
					break;
				}
			}

			void *target = (void *)((uintptr_t)vaddr_extern +
			                        ((size_t)s.DepWordOff * sizeof(u32)));
			u32 token = portRelocRegisterPointer(target);

			uint32_t slot_word = s.SlotByteOff / (uint32_t)sizeof(u32);
			if (is_fighter_figatree && slot_word < figatree_reloc_words.size())
			{
				figatree_reloc_words[slot_word] = 1;
			}
			u32 *slot = (u32 *)((uintptr_t)ram_dst + s.SlotByteOff);
			*slot = token;
			portRelocNoteChainSlot(slot);
		}
	}
	else
	{

	u16 reloc_intern = reloc_intern_offset;
	u32 intern_steps = 0;

	while (reloc_intern != 0xFFFF)
	{
		/* Containment: a pristine chain only references word offsets inside
		 * its own file and visits each slot once, so a legit walk can never
		 * take more steps than the file has words nor step outside it. A
		 * corrupted chain word (observed 2026-07-31: gen-16 token misparsed
		 * as G_VTX rot16'd live descriptors) used to send this walk on an
		 * unbounded wander that exhausted the token table — abort() in
		 * portRelocRegisterPointer. Log + stop instead: the file keeps every
		 * fixup applied up to the bad word, and the scene keeps running. */
		if ((size_t)reloc_intern * sizeof(u32) + sizeof(u32) > copySize ||
		    ++intern_steps > (u32)(copySize / sizeof(u32)))
		{
			spdlog::error("lbReloc bridge: file_id {} intern chain walk STOPPED "
			              "(off={} steps={} copySize={}) — corrupt chain word",
			              file_id, reloc_intern, intern_steps, copySize);
			port_log("SSB64: chainWalk STOP intern file=%u off=0x%x steps=%u size=0x%zx\n",
			         file_id, (unsigned)reloc_intern, (unsigned)intern_steps, copySize);
			break;
		}
		u32 *slot = (u32 *)((uintptr_t)ram_dst + (reloc_intern * sizeof(u32)));

		// Read the reloc descriptor before we overwrite the slot.
		// After the blanket u32 byte-swap, native u32 reads produce the
		// same values as on the N64 (big-endian).  The struct layout is:
		//   bits [31:16] = next descriptor offset (word index), 0xFFFF = end
		//   bits [15:0]  = target offset within this file (word index)
		u16 next_reloc = (u16)(*slot >> 16);
		u16 words_num  = (u16)(*slot & 0xFFFF);

		/* An intern target outside the file means this chain word is not a
		 * chain word (corruption, or the walk already left the rails). */
		if ((size_t)words_num * sizeof(u32) >= copySize)
		{
			spdlog::error("lbReloc bridge: file_id {} intern chain target OOB "
			              "(slot_off={} target_words={} copySize={}) — stopping walk",
			              file_id, reloc_intern, words_num, copySize);
			port_log("SSB64: chainWalk STOP intern-target-oob file=%u slot=0x%x tgt=0x%x size=0x%zx\n",
			         file_id, (unsigned)reloc_intern, (unsigned)words_num, copySize);
			break;
		}

		// All reloc chain entries are intra-file pointers.  Tokenize them
		// normally so the resource system can resolve them to PC addresses.
		//
		// Note: G_DL commands that reference segment 0x0E are NOT in the reloc
		// chain — they exist as raw 0x0Exxxxxx values in the ROM data.
		// These are intra-file sub-DL references resolved to absolute
		// addresses by portNormalizeDisplayListPointer at widening time.
		//
		// Texture fixup happens inside the helper: if this slot is the w1
		// of a G_SETTIMG cmd, the chain encoding gives us the in-file
		// target offset where the texture bytes live; pass2's seg==0x0E
		// scan can't see these. Idempotent.
		portRelocApplyInternSlot(ram_dst, copySize,
		                         (uint32_t)(reloc_intern * sizeof(u32)),
		                         (uint32_t)(words_num * sizeof(u32)),
		                         /* texture_fixup */ true, /* note_chain */ true,
		                         is_fighter_figatree ? &figatree_reloc_words : nullptr);

		reloc_intern = next_reloc;
	}

	// --- External pointer relocation (token-based) ---
	//
	// Same chain structure, but the target is in a DIFFERENT file.
	// The extern file IDs come from the RelocFile metadata (extracted
	// by Torch at ROM-extraction time), not from ROM DMA.

	u16 reloc_extern = reloc_extern_offset;
	u32 extern_idx = 0;
	u32 extern_steps = 0;

	while (reloc_extern != 0xFFFF)
	{
		/* Same containment as the intern walk: slots must lie inside this
		 * file and a legit chain can't have more entries than file words.
		 * (words_num indexes the DEPENDENCY file, so it can't be bounds-
		 * checked here; extern_idx >= extern_count below covers the rest.) */
		if ((size_t)reloc_extern * sizeof(u32) + sizeof(u32) > copySize ||
		    ++extern_steps > (u32)(copySize / sizeof(u32)))
		{
			spdlog::error("lbReloc bridge: file_id {} extern chain walk STOPPED "
			              "(off={} steps={} copySize={}) — corrupt chain word",
			              file_id, reloc_extern, extern_steps, copySize);
			port_log("SSB64: chainWalk STOP extern file=%u off=0x%x steps=%u size=0x%zx\n",
			         file_id, (unsigned)reloc_extern, (unsigned)extern_steps, copySize);
			break;
		}
		u32 *slot = (u32 *)((uintptr_t)ram_dst + (reloc_extern * sizeof(u32)));

		u16 next_reloc = (u16)(*slot >> 16);
		u16 words_num  = (u16)(*slot & 0xFFFF);

		if (extern_idx >= extern_count)
		{
			spdlog::error("lbReloc bridge: file_id {} extern reloc overrun "
			              "(idx={}, count={})", file_id, extern_idx,
			              extern_count);
			break;
		}

		u16 dep_file_id = extern_file_ids[extern_idx];
		void *vaddr_extern;

		// Check if dependency is already loaded
		if (loc == nLBFileLocationForce)
		{
			vaddr_extern = lbRelocFindForceStatusBufferFile(dep_file_id);
		}
		else
		{
			vaddr_extern = lbRelocFindStatusBufferFile(dep_file_id);
		}

		// Load dependency if not cached
		if (vaddr_extern == NULL)
		{
			switch (loc)
			{
			case nLBFileLocationExtern:
				vaddr_extern = lbRelocGetExternBufferFile(dep_file_id);
				break;
			case nLBFileLocationDefault:
				vaddr_extern = lbRelocGetInternBufferFile(dep_file_id);
				break;
			case nLBFileLocationForce:
				vaddr_extern = lbRelocGetForceExternBufferFile(dep_file_id);
				break;
			}
		}

		// Compute target pointer (offset into the dependency file's data)
		void *target = (void *)((uintptr_t)vaddr_extern + (words_num * sizeof(u32)));

		u32 token = portRelocRegisterPointer(target);

		if (is_fighter_figatree && (reloc_extern < figatree_reloc_words.size()))
		{
			figatree_reloc_words[reloc_extern] = 1;
		}
		*slot = token;
		portRelocNoteChainSlot(slot);

		extern_idx++;
		reloc_extern = next_reloc;
	}

	} // explicit_reloc == nullptr (chain walks)

	if (is_fighter_figatree)
	{
		portRelocFixupFighterFigatree(ram_dst, copySize, figatree_reloc_words);
		/* Register the halfswapped range so port_aobj_event32_unhalfswap_stream
		 * knows to only touch EVENT32 streams inside this file's memory. */
		port_aobj_register_halfswapped_range(ram_dst, (unsigned long)copySize);
	}

	{
		extern void portStageAuditEmitLoadSummary(unsigned int file_id, const char *path, size_t size);
		extern void portStageAuditEmitOpcodeCensus(unsigned int file_id, const char *path, const void *data, size_t size);
		portStageAuditEmitLoadSummary(file_id, table_path, copySize);
		portStageAuditEmitOpcodeCensus(file_id, table_path, ram_dst, copySize);
	}
}

/* Stable C ABI (mod-facing, see doc above): chain-walk relocation only. */
extern "C" void portRelocLoadFileFromBytes(
	unsigned int    file_id,
	void           *ram_dst,
	unsigned int    bytes_num,
	int             loc,
	const void     *src_bytes,
	unsigned int    src_size,
	unsigned short  reloc_intern_offset,
	unsigned short  reloc_extern_offset,
	const unsigned short *extern_file_ids,
	unsigned int    extern_count,
	int             force_figatree_fixup)
{
	portRelocLoadFileFromBytesImpl(file_id, ram_dst, bytes_num, loc,
	                               src_bytes, src_size,
	                               reloc_intern_offset, reloc_extern_offset,
	                               extern_file_ids, extern_count,
	                               force_figatree_fixup,
	                               /* explicit_reloc */ nullptr);
}

/**
 * Mod-private variant of portRelocLoadFileFromBytes: copies src_bytes
 * into a mod-owned buffer, byte-swaps to native endian, and walks the
 * intern reloc chain to register pointer tokens. Skips status-buffer
 * registration, extern chain walks, cache evictions, and audit emission
 * because the caller's buffer isn't shared with the engine's per-scene
 * heap (so engine cache invariants don't apply, and consuming a slot in
 * the per-scene status buffer would steal it from the engine's own
 * file loads). Used by mods (via ce_load_reloc_blob) to host long-lived
 * reloc files (e.g., SR-extended menu sprite blobs) that need to flow
 * through the engine's reloc pipeline so internal pointer tokens
 * resolve, but don't belong in the engine's per-scene file table.
 *
 * Re-entrant safety: callable from inside lbRelocInitSetup's tail (e.g.,
 * via the CharacterEngine post-reset callback) because nothing here
 * touches lbRelocInternBuffer or other per-scene engine state.
 */
/* Extended form: `explicit_intern_pairs` is a flat u32 array of
 * (slot_byte_off, target_byte_off) pairs (chain order) for mod bytes whose
 * chain was pre-relocated away — i.e. a mod re-serving a deblobbed
 * bundle's synthesized data (docs/deblob.md). Pass nullptr/0 for ordinary
 * chain-intact reloc bytes; C-flat layout so libtcc mods can call it. */
extern "C" void portRelocLoadFileFromBytesPrivateEx(
	void           *ram_dst,
	unsigned int    dst_size,
	const void     *src_bytes,
	unsigned int    src_size,
	unsigned short  reloc_intern_offset,
	const unsigned int *explicit_intern_pairs,
	unsigned int    explicit_pair_count)
{
	if (src_bytes == nullptr || src_size == 0 || ram_dst == nullptr) {
		return;
	}

	size_t copySize = src_size;
	if (copySize > dst_size && dst_size > 0) {
		spdlog::warn("portRelocLoadFileFromBytesPrivate: src ({} bytes) > dst ({} bytes), truncating",
		             copySize, dst_size);
		copySize = dst_size;
	}

	// Invalidate any prior fixup state / cached uploads that still reference
	// the buffer we're about to overwrite. Each call must redo
	// portFixupSprite / portFixupBitmapArray / portFixupSpriteBitmapData on
	// the freshly-loaded data; without these evictions the fixup tracker
	// thinks "already done" and skips the BE-restore + TMEM swizzle, leaving
	// the texel data in the wrong byte order for the RDP.
	portEvictStructFixupsInRange(ram_dst, copySize);
	extern void portTextureCacheDeleteRange(const void *base, size_t size);
	portTextureCacheDeleteRange(ram_dst, copySize);
	extern void portPackedDisplayListCacheDeleteRange(const void *base, size_t size);
	portPackedDisplayListCacheDeleteRange(ram_dst, copySize);

	memcpy(ram_dst, src_bytes, copySize);

	// Byte-swap from N64 BE to native LE; same call portRelocLoadFileFromBytes
	// makes inside its public path. file_id is unused by the swap logic for
	// non-figatree files but the API requires a value; pass 0 since this is
	// a mod-private buffer that has no engine file_id.
	portRelocByteSwapBlob(ram_dst, copySize, /* file_id = */ 0u);

	// Register the buffer's address range so portRelocFindContainingFile can
	// see it. Display lists inside a reloc file encode their vertex / matrix /
	// sub-DL pointers as segment-0x0E offsets (0x0E0xxxxx). Those only get
	// rewritten to fileBase+offset by portNormalizeDisplayListPointer, which
	// bails immediately unless the DL pointer resolves to a registered range.
	// The public loader pushes the range above; the private path used
	// to skip it, so a mod-private DL (e.g. KirbyHatEngine's 0xA8D custom-hat
	// models) was never widened — the interpreter read the packed 8-byte
	// commands as native 16-byte and fed the raw 0x0Exxxxxx vertex pointer to
	// GfxSpVertex, AV'ing on a garbage host address. Re-loads reuse the same
	// buffer, so evict any prior range for this region before re-registering.
	portRelocEvictFileRangesInRange(ram_dst, copySize);
	sPortRelocFileRanges.push_back({ reinterpret_cast<uintptr_t>(ram_dst),
	                                 copySize, /* file_id = */ 0xFFFFFFFFu,
	                                 "(mod-private)" });

	/* Mirror into the DL-range registry as the public path does, so
	 * gfx_step's runaway-walker bounds check accepts mod-private DLs
	 * (portRelocEvictFileRangesInRange above also unregisters any prior
	 * DL range for this buffer, so re-loads must re-register). */
	port_dl_range_register(ram_dst, copySize, "(mod-private)");

	// Intern relocation. Same per-slot logic as the public loader (shared
	// helper) but without texture-fixup (no SETTIMG-driven texture cache
	// for the mod-private buffer), figatree halfswap, or chain-slot notes.
	if (explicit_intern_pairs != nullptr)
	{
		// Explicit flat list: (slot, target) byte-offset pairs, for
		// pre-relocated (deblobbed-bundle) bytes whose chain is gone.
		for (unsigned int i = 0; i < explicit_pair_count; i++)
		{
			uint32_t slot_off = explicit_intern_pairs[i * 2];
			uint32_t target_off = explicit_intern_pairs[i * 2 + 1];
			if ((size_t)slot_off + sizeof(u32) > copySize ||
			    (size_t)target_off >= copySize)
			{
				spdlog::error("portRelocLoadFileFromBytesPrivateEx: explicit slot {} OOB "
				              "(slot=0x{:X} target=0x{:X} size=0x{:X})",
				              i, slot_off, target_off, copySize);
				break;
			}
			portRelocApplyInternSlot(ram_dst, copySize, slot_off, target_off,
			                         /* texture_fixup */ false,
			                         /* note_chain */ false, nullptr);
		}
		return;
	}

	u16 reloc_intern = reloc_intern_offset;
	u32 intern_steps = 0;
	while (reloc_intern != 0xFFFF) {
		/* Same containment as the public intern walk (see there). */
		if ((size_t)reloc_intern * sizeof(u32) + sizeof(u32) > copySize ||
		    ++intern_steps > (u32)(copySize / sizeof(u32))) {
			spdlog::error("portRelocLoadFileFromBytesPrivate: chain walk STOPPED "
			              "(off={} steps={} copySize={}) — corrupt chain word",
			              reloc_intern, intern_steps, copySize);
			break;
		}
		u32 *slot = (u32 *)((uintptr_t)ram_dst + (reloc_intern * sizeof(u32)));
		u16 next_reloc = (u16)(*slot >> 16);
		u16 words_num  = (u16)(*slot & 0xFFFF);

		if ((size_t)words_num * sizeof(u32) >= copySize) {
			spdlog::error("portRelocLoadFileFromBytesPrivate: chain target OOB "
			              "(slot_off={} target_words={} copySize={}) — stopping walk",
			              reloc_intern, words_num, copySize);
			break;
		}

		portRelocApplyInternSlot(ram_dst, copySize,
		                         (uint32_t)(reloc_intern * sizeof(u32)),
		                         (uint32_t)(words_num * sizeof(u32)),
		                         /* texture_fixup */ false,
		                         /* note_chain */ false, nullptr);

		reloc_intern = next_reloc;
	}
}

/* Stable C ABI kept for already-shipped TCC mods (ce_load_reloc_blob). */
extern "C" void portRelocLoadFileFromBytesPrivate(
	void           *ram_dst,
	unsigned int    dst_size,
	const void     *src_bytes,
	unsigned int    src_size,
	unsigned short  reloc_intern_offset)
{
	portRelocLoadFileFromBytesPrivateEx(ram_dst, dst_size, src_bytes, src_size,
	                                    reloc_intern_offset, nullptr, 0);
}

/**
 * Load a file from the .o2r archive, copy into ram_dst, and perform
 * token-based internal + external pointer relocation. Thin wrapper
 * around portRelocLoadFileFromBytes that sources its bytes via the
 * libultraship ResourceManager (i.e., the BattleShip.o2r /
 * BattleShip.fromsource.o2r pipeline).
 */
void lbRelocLoadAndRelocFile(u32 file_id, void *ram_dst, u32 bytes_num, s32 loc)
{
	auto relocFile = portLoadRelocResource(file_id);
	if (!relocFile)
	{
		spdlog::error("lbReloc bridge: cannot load file_id {} — halting", file_id);
		return;
	}

	// Synthesized (deblobbed) bundles carry explicit slot lists instead of
	// walkable in-data chains (docs/deblob.md).
	PortRelocExplicitRelocation explicit_reloc;
	const PortRelocExplicitRelocation *explicit_ptr = nullptr;
	if (relocFile->HasExplicitRelocation)
	{
		explicit_reloc.intern_slots = relocFile->ExplicitInternSlots.data();
		explicit_reloc.intern_count = relocFile->ExplicitInternSlots.size();
		explicit_reloc.extern_slots = relocFile->ExplicitExternSlots.data();
		explicit_reloc.extern_count = relocFile->ExplicitExternSlots.size();
		explicit_ptr = &explicit_reloc;
	}

	portRelocLoadFileFromBytesImpl(
		(unsigned int)file_id,
		ram_dst,
		bytes_num,
		(int)loc,
		relocFile->Data.data(),
		(unsigned int)relocFile->Data.size(),
		relocFile->RelocInternOffset,
		relocFile->RelocExternOffset,
		relocFile->ExternFileIds.data(),
		(unsigned int)relocFile->ExternFileIds.size(),
		/* force_figatree_fixup = */ 0,
		explicit_ptr);
}

// // // // // // // // // // // //
//                               //
//    BRIDGE: SIZE CALCULATION   //
//                               //
// // // // // // // // // // // //

size_t lbRelocGetExternBytesNum(u32 file_id)
{
	s32 i;

	if (lbRelocFindStatusBufferFile(file_id) != NULL)
	{
		return 0;
	}

	for (i = 0; i < sLBRelocExternFileIDsNum; i++)
	{
		if (file_id == sLBRelocExternFileIDs[i])
		{
			return 0;
		}
	}

	if (sLBRelocExternFileIDsNum >= sLBRelocExternFileIDsMax)
	{
		while (TRUE)
		{
			syDebugPrintf("Relocatable Data Manager: External Data is over %d!!\n",
			              sLBRelocExternFileIDsMax);
			scManagerRunPrintGObjStatus();
		}
	}

	auto relocFile = portLoadRelocResource(file_id);
	if (!relocFile) { return 0; }

	size_t bytes_read = (u32)LBRELOC_CACHE_ALIGN(relocFile->Data.size());
	sLBRelocExternFileIDs[sLBRelocExternFileIDsNum++] = file_id;

	for (u16 dep_id : relocFile->ExternFileIds)
	{
		bytes_read += lbRelocGetExternBytesNum(dep_id);
	}

	return bytes_read;
}

size_t lbRelocGetFileSize(u32 id)
{
	u32 file_ids_extern_buf[50];

	sLBRelocExternFileIDs = file_ids_extern_buf;
	sLBRelocExternFileIDsNum = 0;
	sLBRelocExternFileIDsMax = ARRAY_COUNT(file_ids_extern_buf);

	return lbRelocGetExternBytesNum(id);
}

// // // // // // // // // // // //
//                               //
//     BRIDGE: FILE LOADING      //
//                               //
// // // // // // // // // // // //

void* lbRelocGetExternBufferFile(u32 id)
{
	void *file = lbRelocFindStatusBufferFile(id);
	if (file != NULL) { return file; }

	auto relocFile = portLoadRelocResource(id);
	if (!relocFile) { return NULL; }

	void *file_alloc = (void *)LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocExternFileHeap);
	size_t file_size = relocFile->Data.size();
	sLBRelocExternFileHeap = (void *)((uintptr_t)file_alloc + file_size);

	lbRelocLoadAndRelocFile(id, file_alloc, (u32)file_size, nLBFileLocationExtern);
	return file_alloc;
}

void* lbRelocGetExternHeapFile(u32 id, void *heap)
{
	sLBRelocExternFileHeap = heap;
	return lbRelocGetExternBufferFile(id);
}

void* lbRelocGetInternBufferFile(u32 id)
{
	void *file = lbRelocFindStatusBufferFile(id);
	if (file != NULL) { return file; }

	auto relocFile = portLoadRelocResource(id);
	if (!relocFile) { return NULL; }

	void *file_alloc = (void *)LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocInternBuffer.heap_ptr);
	size_t file_size = relocFile->Data.size();

	if (((uintptr_t)file_alloc + file_size) > (uintptr_t)sLBRelocInternBuffer.heap_end)
	{
		while (TRUE)
		{
			syDebugPrintf("Relocatable Data Manager: Buffer is full !!\n");
			scManagerRunPrintGObjStatus();
		}
	}
	sLBRelocInternBuffer.heap_ptr = (void *)((uintptr_t)file_alloc + file_size);

	lbRelocLoadAndRelocFile(id, file_alloc, (u32)file_size, nLBFileLocationDefault);
	return file_alloc;
}

void* lbRelocGetForceExternBufferFile(u32 id)
{
	void *file = lbRelocFindForceStatusBufferFile(id);
	if (file != NULL) { return file; }

	auto relocFile = portLoadRelocResource(id);
	if (!relocFile) { return NULL; }

	void *file_alloc = (void *)LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocExternFileHeap);
	size_t file_size = relocFile->Data.size();
	sLBRelocExternFileHeap = (void *)((uintptr_t)file_alloc + file_size);

	lbRelocLoadAndRelocFile(id, file_alloc, (u32)file_size, nLBFileLocationForce);
	return file_alloc;
}

void* lbRelocGetForceExternHeapFile(u32 id, void *heap)
{
	/* NOTE: rewinding abandons prior loads above `heap`, and their corpse
	 * entries linger in the address-keyed registries until a same-range
	 * load evicts them (a smaller reload leaves the old tail's entries).
	 * Do NOT try to evict [heap, sLBRelocExternFileHeap) here:
	 * sLBRelocExternFileHeap is shared with the extern-heap bump path, so
	 * that span can contain LIVE extern files — evicting it invalidates
	 * their tokens and crashes scene 28 (verified regression). Corpse
	 * chain-slot entries are instead detected at use: the runtime texture
	 * fixup validates a candidate slot's word via
	 * portRelocTryResolvePointer before clamping. */
	sLBRelocExternFileHeap = heap;
	sLBRelocInternBuffer.force_status_buffer_num = 0;
	return lbRelocGetForceExternBufferFile(id);
}

/* Mod helper: same force-status-buffer reset that
 * lbRelocGetForceExternHeapFile does on entry. Mods that route mod-
 * registered anim file IDs through portRelocLoadFileFromBytes (skipping
 * the orig wrapper because the file isn't in the engine's file table)
 * still need this reset, otherwise the lbRelocAddForceStatusBufferFile
 * call inside portRelocLoadFileFromBytes accumulates entries across
 * fighter status changes until the per-scene cap is exceeded and the
 * engine enters its "Force Status Buffer is full !!" panic loop. */
extern "C" void portRelocResetForceStatusBuffer(void)
{
	sLBRelocInternBuffer.force_status_buffer_num = 0;
}

/* Accessors for the extern-file bump cursor. portRelocLoadFileFromBytes
 * resets sLBRelocExternFileHeap to point past the file it just loaded
 * so that file's own extern deps bump-allocate contiguously
 * after it. That's correct when the destination is the engine's shared
 * extern heap, but when a mod loads a file into its own malloc'd buffer
 * (CharacterEngine's private-buffer hooks) the reset leaves the cursor
 * pointing *inside* that private buffer. If the OUTER file's chain walk
 * then loads a later vanilla dep, it bump-allocates into the small
 * private buffer and overruns it. CharacterEngine brackets each private
 * mod-file load with get/set so the outer walk's cursor survives the
 * nested load. */
extern "C" void *portRelocGetExternFileHeap(void)
{
	return sLBRelocExternFileHeap;
}

extern "C" void portRelocSetExternFileHeap(void *heap)
{
	sLBRelocExternFileHeap = heap;
}

// // // // // // // // // // // //
//                               //
//     BRIDGE: BATCH LOADING     //
//                               //
// // // // // // // // // // // //

size_t lbRelocLoadFilesExtern(u32 *ids, u32 len, void **files, void *heap)
{
	sLBRelocExternFileHeap = heap;

	while (len != 0)
	{
		*files = lbRelocGetExternBufferFile(*ids);
		ids++;
		files++;
		len--;
	}

	return (size_t)((uintptr_t)sLBRelocExternFileHeap - (uintptr_t)heap);
}

size_t lbRelocLoadFilesIntern(u32 *ids, u32 len, void **files)
{
	void *heap = sLBRelocInternBuffer.heap_ptr;

	while (len)
	{
		*files = lbRelocGetInternBufferFile(*ids);
		ids++;
		files++;
		len--;
	}

	return (size_t)((uintptr_t)sLBRelocInternBuffer.heap_ptr - (uintptr_t)heap);
}

size_t lbRelocGetAllocSize(u32 *ids, u32 len)
{
	u32 file_ids[50];
	size_t allocated = 0;

	sLBRelocExternFileIDs = file_ids;
	sLBRelocExternFileIDsNum = 0;
	sLBRelocExternFileIDsMax = ARRAY_COUNT(file_ids);

	while (len != 0)
	{
		allocated = LBRELOC_CACHE_ALIGN(allocated);
		allocated += lbRelocGetExternBytesNum(*ids);
		ids++;
		len--;
	}
	return allocated;
}

bool portRelocFindContainingFile(const void *ptr, uintptr_t *out_base, size_t *out_size)
{
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

	for (const auto &range : sPortRelocFileRanges)
	{
		uintptr_t start = range.base;
		size_t size = range.size;

		if ((addr >= start) && (size != 0) && ((addr - start) < size))
		{
			if (out_base != NULL)
			{
				*out_base = start;
			}
			if (out_size != NULL)
			{
				*out_size = size;
			}
			return TRUE;
		}
	}
	return FALSE;
}

/* C-callable helper: find both file_id and base for a pointer.
 * Returns -1 if not in any reloc file range. */
extern "C" int portRelocFindFileIdAndBase(const void *ptr, uintptr_t *out_base)
{
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	for (const auto &range : sPortRelocFileRanges)
	{
		if ((addr >= range.base) && (range.size != 0) && ((addr - range.base) < range.size))
		{
			if (out_base != nullptr) *out_base = range.base;
			return (int)range.file_id;
		}
	}
	return -1;
}

/* Classify a pointer for diagnostic output. Writes a short human-readable
 * label into buf (e.g. "scene_arena+0x4528", "reloc[523]+0x120 path",
 * "n64_seg", "low_brk", "high_heap"). Used by the GFX diag dump
 * (libultraship interpreter.cpp diagDumpAll) to identify the upstream
 * holder behind a stale-pointer crash in gfx_step. Never derefs ptr —
 * uses only registered range metadata. */
extern void *gPortSceneHeap;
extern const size_t gPortSceneHeapSize;
extern "C" void port_classify_dl_ptr(uintptr_t addr, char *buf, size_t buf_size)
{
	if (buf == nullptr || buf_size == 0) return;
	if (addr == 0) {
		std::snprintf(buf, buf_size, "null");
		return;
	}
	if (addr <= 0x0FFFFFFFu) {
		std::snprintf(buf, buf_size, "n64_seg[%u]+0x%x",
		              (unsigned)((addr >> 24) & 0xFF),
		              (unsigned)(addr & 0x00FFFFFFu));
		return;
	}
	if (gPortSceneHeap != nullptr) {
		uintptr_t arena_base = reinterpret_cast<uintptr_t>(gPortSceneHeap);
		if (addr >= arena_base && (addr - arena_base) < gPortSceneHeapSize) {
			std::snprintf(buf, buf_size, "scene_arena+0x%lx",
			              (unsigned long)(addr - arena_base));
			return;
		}
	}
	uintptr_t reloc_base = 0;
	int file_id = portRelocFindFileIdAndBase(reinterpret_cast<const void*>(addr), &reloc_base);
	if (file_id >= 0) {
		const char *path = (file_id < (int)RELOC_FILE_COUNT && gRelocFileTable[file_id])
		                   ? gRelocFileTable[file_id] : "?";
		std::snprintf(buf, buf_size, "reloc[%d]+0x%lx %s",
		              file_id, (unsigned long)(addr - reloc_base), path);
		return;
	}
	/* No registered range. Tag by host-address heuristic. */
	if (addr < 0x100000000ull) {
		std::snprintf(buf, buf_size, "low_brk@0x%lx", (unsigned long)addr);
	} else if (addr >= 0x7f0000000000ull) {
		std::snprintf(buf, buf_size, "high_heap@0x%lx", (unsigned long)addr);
	} else {
		std::snprintf(buf, buf_size, "other@0x%lx", (unsigned long)addr);
	}
}

void *portRelocResolveArrayEntry(const void *array_ptr, unsigned int index)
{
	if (array_ptr == nullptr)
	{
		return nullptr;
	}

	uintptr_t base = 0;
	size_t size = 0;

	if (portRelocFindContainingFile(array_ptr, &base, &size))
	{
		uintptr_t addr = reinterpret_cast<uintptr_t>(array_ptr);
		size_t byte_offset = static_cast<size_t>(addr - base);
		size_t entry_offset = byte_offset + (static_cast<size_t>(index) * sizeof(uint32_t));

		if ((entry_offset > size) || ((size - entry_offset) < sizeof(uint32_t)))
		{
			return nullptr;
		}

		return portRelocResolvePointer(reinterpret_cast<const uint32_t *>(array_ptr)[index]);
	}

	return reinterpret_cast<void *const *>(array_ptr)[index];
}

bool portRelocDescribePointer(const void *ptr, uintptr_t *out_base, size_t *out_size, u32 *out_file_id, const char **out_path)
{
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

	for (const auto &range : sPortRelocFileRanges)
	{
		uintptr_t start = range.base;
		size_t size = range.size;

		if ((addr >= start) && (size != 0) && ((addr - start) < size))
		{
			if (out_base != NULL)
			{
				*out_base = start;
			}
			if (out_size != NULL)
			{
				*out_size = size;
			}
			if (out_file_id != NULL)
			{
				*out_file_id = range.file_id;
			}
			if (out_path != NULL)
			{
				*out_path = range.path;
			}
			return TRUE;
		}
	}
	return FALSE;
}

// // // // // // // // // // // //
//                               //
//    BRIDGE: INITIALIZATION     //
//                               //
// // // // // // // // // // // //

void lbRelocInitSetup(LBRelocSetup *setup)
{
	/* Note: the historical portRelocResetPointerTable() call used to live
	 * here. It wholesale-invalidated every token in the table on every
	 * scene init, which is what created the variant-1/2/3 stale-data crash
	 * family — tokens pointing at INTERN-BUFFER files (mainmotion, sub-
	 * motion, model, special1-4, shieldpose; all persistent across scenes)
	 * would suddenly fail to resolve at scene N+1 even though their
	 * underlying memory was still live. The new per-slot generational
	 * model in RelocPointerTable.cpp handles invalidation surgically:
	 * port_taskman_evict_arena_caches() calls portRelocInvalidateRange()
	 * with the scene-arena range, so ONLY arena-backed tokens go stale.
	 * Persistent file tokens stay valid across scene cycles. No wholesale
	 * reset here. */
	gmColScriptsLinkRelocTargets();
	portResetPackedDisplayListCache();
	sPortRelocFileRanges.clear();

	// Clear u16 struct fixup tracking — addresses from the old heap are stale
	portResetStructFixups();

	// Clear FB-mirror registrations — see port/bridge/framebuffer_capture.h.
	// The 1P stage-clear wallpaper buf and the lbtransition photo heap both
	// register their CPU pointer as a mirror of a snapshot FB; on scene change
	// the bump-reset heaps free those addresses and a fresh load could land
	// at the same address. Without this, the new asset would render the prior
	// scene's snapshot instead of its own pixels.
	extern void port_capture_release_all(void);
	port_capture_release_all();

	// ROM addresses (unused in port but stored for completeness)
	sLBRelocInternBuffer.rom_table_lo = setup->table_addr;
	sLBRelocInternBuffer.total_files_num = setup->table_files_num;
	sLBRelocInternBuffer.rom_table_hi = setup->table_addr +
		((setup->table_files_num + 1) * sizeof(LBTableEntry));

	// Heap management (still used — callers allocate and pass heaps)
	sLBRelocInternBuffer.heap_start = sLBRelocInternBuffer.heap_ptr = setup->file_heap;
	sLBRelocInternBuffer.heap_end = (void *)((uintptr_t)setup->file_heap + setup->file_heap_size);

	// Status buffers (file caching)
	sLBRelocInternBuffer.status_buffer_num = 0;
	sLBRelocInternBuffer.status_buffer_max = setup->status_buffer_size;
	sLBRelocInternBuffer.status_buffer = setup->status_buffer;

	sLBRelocInternBuffer.force_status_buffer_max = setup->force_status_buffer_size;
	sLBRelocInternBuffer.force_status_buffer_num = 0;
	sLBRelocInternBuffer.force_status_buffer = setup->force_status_buffer;
}

} // extern "C"

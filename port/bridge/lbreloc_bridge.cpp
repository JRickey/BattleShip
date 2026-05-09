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
#include <ship/resource/type/Blob.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include "resource/RelocFile.h"
#include "resource/RelocFileTable.h"
#include "resource/RelocPointerTable.h"
#include "bridge/lbreloc_bridge_testing.h"
#include "bridge/lbreloc_byteswap.h"

extern "C" void port_aobj_register_halfswapped_range(void *base, unsigned long size);

// ----------------------------------------------------------------------------
//  Sub-resource override (Stage 10)
// ----------------------------------------------------------------------------
//
// When a mod .o2r is added to the ArchiveManager *after* BattleShip.o2r,
// LUS's last-archive-wins resolution claims any matching CRC64 hashes from
// the mod. The container's SubResourceHashes table records (hash, byte_off,
// size, kind) per sub-resource at extraction time, so the runtime can ask
// LoadResource for each hash and overlay the resulting bytes into a mutable
// copy of `relocFile->Data` before memcpy'ing into the heap.
//
// `sAnyOverridesActive` short-circuits the whole scan when no mods are
// loaded — without it, every reloc-file load would issue N LoadResource
// calls against BattleShip.o2r itself just to confirm "nothing differs".
// Set to true by `portRelocSetOverridesActive(true)` from the mods/
// directory scanner (see port.cpp).
//
// `relocFile->Data` is preserved unchanged on disk and as the in-memory
// resource — overlays land in a transient std::vector built per-load. This
// keeps subsequent loads of the same file_id (e.g. force-extern stage
// reloads) deterministic and means the test harness's drift gate sees the
// raw container bytes, not the overlay output.
static std::atomic<bool> sAnyOverridesActive{false};

extern "C" void portRelocSetOverridesActive(int active) {
    sAnyOverridesActive.store(active != 0, std::memory_order_release);
}

// LUS's BlobFactory pads each Blob with 16 zero bytes after the recorded
// data length so N64 readers that overshoot by a few bytes (compressed-MIDI
// parsers, etc.) don't run off the end of an allocation. We trim those when
// computing the actual override bytes to memcmp / memcpy.
static constexpr size_t kBlobTrailingPadding = 16;

// Builds an overlay copy of `relocFile->Data` with each sub-resource override
// (if any) memcpy'd in at its recorded byte offset. Returns an empty vector
// when no overrides apply — the caller falls back to `relocFile->Data` and
// avoids the copy. Bounds-checked: out-of-range entries (size 0, offset past
// end, etc.) are skipped silently to keep a malformed mod from crashing.
//
// Identity check: if a sub-resource resolves to bytes that exactly match the
// container's existing slice, that's the base archive's own copy of the
// sub-resource, NOT an override — skip it. Without this check, the test
// harness (which loads only BattleShip.o2r — no mods) would still allocate
// + memcpy on every load even though nothing changed.
static std::vector<uint8_t> portRelocBuildOverlay(const RelocFile& relocFile) {
    if (!sAnyOverridesActive.load(std::memory_order_acquire)) {
        return {};
    }
    if ((relocFile.ProcessingFlags & PROC_SUBRESOURCES_EMITTED) == 0) {
        return {};
    }
    if (relocFile.SubResourceHashes.empty()) {
        return {};
    }

    auto context = Ship::Context::GetInstance();
    if (!context) return {};
    auto resMgr = context->GetResourceManager();
    if (!resMgr) return {};

    std::vector<uint8_t> overlay;
    bool overlayBuilt = false;

    for (const auto& sr : relocFile.SubResourceHashes) {
        if (sr.Size == 0) continue;
        if (sr.ByteOffset > relocFile.Data.size()) continue;
        if (sr.Size > relocFile.Data.size() - sr.ByteOffset) continue;

        auto resource = resMgr->LoadResource(sr.Hash);
        if (!resource) continue;
        auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
        if (!blob) continue;

        const size_t paddedSize = blob->Data.size();
        const size_t blobSize   = paddedSize > kBlobTrailingPadding
                                      ? paddedSize - kBlobTrailingPadding
                                      : 0;
        if (blobSize == 0) continue;

        // Copy at most `sr.Size` bytes (the recorded extent) — never overrun
        // the container slot regardless of mod-blob length. Smaller blobs
        // get truncated to fit; larger ones get truncated to the slot.
        const size_t copyBytes = std::min<size_t>(sr.Size, blobSize);

        // Identity short-circuit: bytes match → base archive's own copy →
        // no override → don't allocate.
        const uint8_t* containerSlice = relocFile.Data.data() + sr.ByteOffset;
        if (copyBytes == sr.Size
            && std::memcmp(containerSlice, blob->Data.data(), copyBytes) == 0) {
            continue;
        }

        // First real override: lazily clone Data so subsequent overrides
        // mutate the same buffer.
        if (!overlayBuilt) {
            overlay.assign(relocFile.Data.begin(), relocFile.Data.end());
            overlayBuilt = true;
        }
        std::memcpy(overlay.data() + sr.ByteOffset, blob->Data.data(), copyBytes);
    }

    return overlay;
}

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
	uint32_t proc_flags;   // RelocFile::ProcessingFlags at load time
};

static std::vector<PortRelocFileRange> sPortRelocFileRanges;

// // // // // // // // // // // //
//                               //
//   TEST INJECTION HOOKS        //
//                               //
// // // // // // // // // // // //

// Resource-loader override (set by tests). When nullptr, portLoadRelocResource
// falls through to the production Ship::Context lookup.
static std::shared_ptr<RelocFile> (*sRelocLoaderOverride)(uint32_t file_id) = nullptr;

// Chain-walk observer (set by tests). Invoked once per registered chain target.
static PortRelocChainObserver sChainObserver = nullptr;

extern "C" void portRelocSetResourceLoader(std::shared_ptr<RelocFile> (*loader)(uint32_t file_id))
{
	sRelocLoaderOverride = loader;
}

extern "C" void portRelocClearResourceLoader(void)
{
	sRelocLoaderOverride = nullptr;
}

extern "C" void portRelocSetChainObserver(PortRelocChainObserver obs)
{
	sChainObserver = obs;
}

extern "C" void portRelocClearChainObserver(void)
{
	sChainObserver = nullptr;
}

extern "C" void portRelocBridgeForgetFileRanges(void)
{
	sPortRelocFileRanges.clear();
}

// Test-only: status buffer storage. Owned by the bridge so tests don't need
// to know LBFileNode's layout. Two arrays — main + force.
static std::vector<LBFileNode> sTestStatusBuf;
static std::vector<LBFileNode> sTestForceStatusBuf;

extern "C" void portRelocBridgeSetupForTest(
    void *intern_heap_base, size_t intern_heap_size,
    void *extern_heap_base, size_t extern_heap_size,
    size_t status_buffer_count,
    size_t force_status_buffer_count)
{
	sTestStatusBuf.assign(status_buffer_count, LBFileNode{0u, nullptr});
	sTestForceStatusBuf.assign(force_status_buffer_count, LBFileNode{0u, nullptr});

	sLBRelocInternBuffer.rom_table_lo = 0;
	sLBRelocInternBuffer.rom_table_hi = 0;
	sLBRelocInternBuffer.total_files_num = 0;
	sLBRelocInternBuffer.heap_start = intern_heap_base;
	sLBRelocInternBuffer.heap_ptr = intern_heap_base;
	sLBRelocInternBuffer.heap_end = (void*)((uintptr_t)intern_heap_base + intern_heap_size);
	sLBRelocInternBuffer.status_buffer_num = 0;
	sLBRelocInternBuffer.status_buffer_max = static_cast<s32>(status_buffer_count);
	sLBRelocInternBuffer.status_buffer = sTestStatusBuf.data();
	sLBRelocInternBuffer.force_status_buffer_num = 0;
	sLBRelocInternBuffer.force_status_buffer_max = static_cast<s32>(force_status_buffer_count);
	sLBRelocInternBuffer.force_status_buffer = sTestForceStatusBuf.data();

	sLBRelocExternFileHeap = extern_heap_base;
	(void)extern_heap_size;  /* The bridge's bump alloc has no end-check on extern heap; tests size the buffer */
}

static void portRelocEvictFileRangesInRange(void *base, size_t size)
{
	if ((base == nullptr) || (size == 0))
	{
		return;
	}

	uintptr_t begin = reinterpret_cast<uintptr_t>(base);
	uintptr_t end = begin + size;

	sPortRelocFileRanges.erase(
		std::remove_if(sPortRelocFileRanges.begin(), sPortRelocFileRanges.end(),
			[begin, end](const PortRelocFileRange &range) {
				uintptr_t range_begin = range.base;
				uintptr_t range_end = range_begin + range.size;

				return (range.size != 0) && (range_begin < end) && (begin < range_end);
			}),
		sPortRelocFileRanges.end());
}

/* Fighter animation / submotion files plus SCExplainMain go through the
 * halfswap pipeline at extraction (PROC_HALFSWAP_DONE on every v3 archive).
 * The runtime path here only registers their loaded heap range with the
 * AObjEvent32 walker (port_aobj_register_halfswapped_range), so the lazy
 * un-halfswap walker knows which streams to undo when an opcode reader
 * touches them. */
static bool portRelocIsFighterFigatreeFile(u32 file_id)
{
	static const char sFighterAnimPrefix[] = "reloc_animations/FT";
	static const char sFighterSubmotionPrefix[] = "reloc_submotions/FT";
	static const char sSCExplainMainPath[] = "reloc_scene/SCExplainMain";

	if (file_id >= RELOC_FILE_COUNT || gRelocFileTable[file_id] == NULL) return false;
	const char *path = gRelocFileTable[file_id];
	return (std::strncmp(path, sFighterAnimPrefix, sizeof(sFighterAnimPrefix) - 1) == 0) ||
	       (std::strncmp(path, sFighterSubmotionPrefix, sizeof(sFighterSubmotionPrefix) - 1) == 0) ||
	       (std::strcmp(path, sSCExplainMainPath) == 0);
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

	// Test-only injection point: tests install a loader so we don't need a
	// fully-initialized Ship::Context. Production never sets sRelocLoaderOverride.
	if (sRelocLoaderOverride != nullptr)
	{
		auto rf = sRelocLoaderOverride(file_id);
		if (!rf)
		{
			spdlog::error("lbReloc bridge: test loader returned null for file_id {}", file_id);
		}
		return rf;
	}

	auto ctx = Ship::Context::GetInstance();
	if (!ctx)
	{
		spdlog::error("lbReloc bridge: no Ship::Context");
		return nullptr;
	}

	std::string path(gRelocFileTable[file_id]);
	auto resource = ctx->GetResourceManager()->LoadResource(path);

	if (!resource)
	{
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
 * Load a file from the .o2r archive, copy into ram_dst, and perform
 * token-based internal + external pointer relocation.
 *
 * Instead of writing 64-bit void* into 4-byte slots (which would corrupt
 * adjacent data), we register each target pointer in the token table and
 * write the 32-bit token into the slot.
 */
void lbRelocLoadAndRelocFile(u32 file_id, void *ram_dst, u32 bytes_num, s32 loc)
{
	auto relocFile = portLoadRelocResource(file_id);
	bool is_fighter_figatree = portRelocIsFighterFigatreeFile(file_id);

	// Gated: SSB64_LOG_LBRELOC_LOAD=1 logs every file load. Helpful when
	// tracing which reloc files are loaded per scene.
	if (getenv("SSB64_LOG_LBRELOC_LOAD") != nullptr) {
		static int s_load_log_count = 0;
		if (s_load_log_count < 512) {
			s_load_log_count++;
			port_log("SSB64: lbReloc LOAD file_id=%u loc=%d path=%s fig=%d\n",
				file_id, loc,
				(file_id < RELOC_FILE_COUNT && gRelocFileTable[file_id]) ? gRelocFileTable[file_id] : "(null)",
				(int)is_fighter_figatree);
		}
	}

	if (!relocFile)
	{
		spdlog::error("lbReloc bridge: cannot load file_id {} — halting", file_id);
		return;
	}

	// Copy decompressed data into the game's heap allocation
	size_t copySize = relocFile->Data.size();
	if (copySize > bytes_num && bytes_num > 0)
	{
		spdlog::warn("lbReloc bridge: file_id {} data ({} bytes) exceeds "
		             "buffer ({} bytes), truncating", file_id, copySize, bytes_num);
		copySize = bytes_num;
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

	// Stage-10 sub-resource overlay. When a mod .o2r in the ArchiveManager
	// last-archive-wins claims any of this container's sub-resource hashes,
	// portRelocBuildOverlay returns a mutable copy of relocFile->Data with
	// the override bytes spliced in at their recorded offsets. Empty vector
	// = no overrides applied (the common case — short-circuit avoids the
	// copy + memcmp scan unless a mod is actually loaded). Override bytes
	// land in `ram_dst` via the same memcpy path; chain registration below
	// runs against the post-overlay bytes since the chain-slot encodings
	// and chain-target byte offsets are stable across overrides (sub-asset
	// regions never overlap chain slots, by construction in torch).
	std::vector<uint8_t> overlay = portRelocBuildOverlay(*relocFile);
	const uint8_t* memcpySrc = !overlay.empty()
	                              ? overlay.data()
	                              : relocFile->Data.data();
	memcpy(ram_dst, memcpySrc, copySize);

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
				             copySize, file_id, gRelocFileTable[file_id], path);
			}
		}
	}

	// Byte-swap from N64 big-endian to native little-endian.
	// Must happen BEFORE the reloc chain walk (which reads u16 fields
	// from u32 words using bit shifts that assume native byte order).
	// Each transform torch already ran at extraction time is advertised
	// via ProcessingFlags so we skip the duplicate runtime work.
	portRelocByteSwapBlob(ram_dst, copySize, (unsigned int)file_id,
	                      (unsigned int)relocFile->ProcessingFlags);

	// Register in status buffer
	if (loc == nLBFileLocationForce)
	{
		lbRelocAddForceStatusBufferFile(file_id, ram_dst);
	}
	else
	{
		lbRelocAddStatusBufferFile(file_id, ram_dst);
	}

	sPortRelocFileRanges.push_back({ reinterpret_cast<uintptr_t>(ram_dst), copySize, file_id, gRelocFileTable[file_id], (uint32_t)relocFile->ProcessingFlags });

	// --- Reloc chain processing ---
	//
	// Two source paths produce the same per-entry side effects:
	//   (a) PROC_CHAIN_FLATTENED is set: torch pre-walked the chain and
	//       emitted relocFile->InternChain / ExternChain. We iterate those
	//       lists directly.
	//   (b) PROC_CHAIN_FLATTENED is unset: walk the encoded chain in-place
	//       (each slot is a u32 of [next_word_idx<<16 | target_word_idx],
	//       0xFFFF terminates).
	//
	// Per-entry side effects (must be identical across both paths so the
	// PROC_CHAIN_FLATTENED gate is purely a sourcing change):
	//   - Internal: portRelocFixupTextureFromChain on the slot/target byte
	//     offsets (catches G_SETTIMG/G_VTX byte fixups for chain-targeted
	//     loads that pass2's in-DL walker can't see), then register the
	//     resolved pointer as a token, write the token into the slot, mark
	//     the figatree halfswap mask if applicable, fire the test observer.
	//   - External: same shape, but the target lives in a dependency file
	//     loaded via lbRelocGet*BufferFile. dep_file_id comes from the flat
	//     list entry or from ExternFileIds[idx] (chain-insertion order).
	//
	// G_DL commands that reference segment 0x0E are NOT in the reloc chain —
	// they exist as raw 0x0Exxxxxx values in the data and are resolved by
	// portNormalizeDisplayListPointer at widening time.
	auto applyInternEntry = [&](uint32_t slot_byte_off, uint32_t target_byte_off) {
		portRelocFixupTextureFromChain(ram_dst, copySize, slot_byte_off, target_byte_off);
		u32 *slot = (u32 *)((uintptr_t)ram_dst + slot_byte_off);
		void *target = (void *)((uintptr_t)ram_dst + target_byte_off);
		*slot = portRelocRegisterPointer(target);
		if (sChainObserver != nullptr) {
			sChainObserver(file_id, slot_byte_off, /*dep_file_id=*/-1, target_byte_off);
		}
	};

	auto applyExternEntry = [&](uint32_t slot_byte_off, uint32_t target_byte_off, u16 dep_file_id) {
		void *vaddr_extern = (loc == nLBFileLocationForce)
			? lbRelocFindForceStatusBufferFile(dep_file_id)
			: lbRelocFindStatusBufferFile(dep_file_id);
		if (vaddr_extern == NULL) {
			switch (loc) {
			case nLBFileLocationExtern:
				vaddr_extern = lbRelocGetExternBufferFile(dep_file_id); break;
			case nLBFileLocationDefault:
				vaddr_extern = lbRelocGetInternBufferFile(dep_file_id); break;
			case nLBFileLocationForce:
				vaddr_extern = lbRelocGetForceExternBufferFile(dep_file_id); break;
			}
		}
		u32 *slot = (u32 *)((uintptr_t)ram_dst + slot_byte_off);
		void *target = (void *)((uintptr_t)vaddr_extern + target_byte_off);
		*slot = portRelocRegisterPointer(target);
		if (sChainObserver != nullptr) {
			sChainObserver(file_id, slot_byte_off, (int32_t)dep_file_id, target_byte_off);
		}
	};

	if (relocFile->ProcessingFlags & PROC_CHAIN_FLATTENED)
	{
		// Path (a): flat-list iteration — torch's chain walker output.
		for (const auto &e : relocFile->InternChain)
			applyInternEntry(e.SlotByteOffset, e.TargetByteOffset);
		for (const auto &e : relocFile->ExternChain)
			applyExternEntry(e.SlotByteOffset, e.TargetByteOffset, e.DepFileId);
	}
	else
	{
		// Path (b): encoded-chain walk on the data slots in-place.
		u16 reloc_intern = relocFile->RelocInternOffset;
		while (reloc_intern != 0xFFFF)
		{
			u32 *slot = (u32 *)((uintptr_t)ram_dst + (reloc_intern * sizeof(u32)));
			u16 next_reloc = (u16)(*slot >> 16);
			u16 words_num  = (u16)(*slot & 0xFFFF);
			applyInternEntry((uint32_t)(reloc_intern * sizeof(u32)),
			                 (uint32_t)(words_num   * sizeof(u32)));
			reloc_intern = next_reloc;
		}

		u16 reloc_extern = relocFile->RelocExternOffset;
		u32 extern_idx = 0;
		while (reloc_extern != 0xFFFF)
		{
			u32 *slot = (u32 *)((uintptr_t)ram_dst + (reloc_extern * sizeof(u32)));
			u16 next_reloc = (u16)(*slot >> 16);
			u16 words_num  = (u16)(*slot & 0xFFFF);
			if (extern_idx >= relocFile->ExternFileIds.size())
			{
				spdlog::error("lbReloc bridge: file_id {} extern reloc overrun "
				              "(idx={}, count={})", file_id, extern_idx,
				              relocFile->ExternFileIds.size());
				break;
			}
			u16 dep_file_id = relocFile->ExternFileIds[extern_idx];
			applyExternEntry((uint32_t)(reloc_extern * sizeof(u32)),
			                 (uint32_t)(words_num   * sizeof(u32)),
			                 dep_file_id);
			extern_idx++;
			reloc_extern = next_reloc;
		}
	}

	/* Heap-absolute side effect: tell the lazy AObjEvent32 walker
	 * (port_aobj_fixup) and the FTKeyEvent reader (ftkey.c) that this region
	 * came in with halfswap applied at extraction. The byte transform itself
	 * is now always done by torch (PROC_HALFSWAP_DONE on every v3 archive). */
	if (is_fighter_figatree)
	{
		port_aobj_register_halfswapped_range(ram_dst, (unsigned long)copySize);
	}

	{
		extern void portStageAuditEmitLoadSummary(unsigned int file_id, const char *path, size_t size);
		extern void portStageAuditEmitOpcodeCensus(unsigned int file_id, const char *path, const void *data, size_t size);
		const char *audit_path = (file_id < RELOC_FILE_COUNT) ? gRelocFileTable[file_id] : nullptr;
		portStageAuditEmitLoadSummary(file_id, audit_path, copySize);
		portStageAuditEmitOpcodeCensus(file_id, audit_path, ram_dst, copySize);
	}
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
	sLBRelocExternFileHeap = heap;
	sLBRelocInternBuffer.force_status_buffer_num = 0;
	return lbRelocGetForceExternBufferFile(id);
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

/* C-callable helper: read RelocFile::ProcessingFlags for a currently-loaded
 * file. Returns 0 (no torch transforms applied) if the file is not loaded.
 * Used by the portFixup* helpers to skip the byte-transform body when
 * (a) the file's PROC_<family>_DONE bit is set AND (b) the catalog has a
 * matching entry — see portStructFixupCatalogHasEntry. */
extern "C" uint32_t portRelocGetProcessingFlags(int file_id)
{
	if (file_id < 0)
		return 0;
	for (const auto &range : sPortRelocFileRanges)
	{
		if ((int)range.file_id == file_id)
			return range.proc_flags;
	}
	return 0;
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
	// Clear token table from previous scene — prevents unbounded growth
	// and stale tokens pointing to freed heap memory
	portRelocResetPointerTable();
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

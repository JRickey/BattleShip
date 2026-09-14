// Deblob synthesis engine — see SyntheticReloc.h and docs/deblob.md.
//
// Architecture (patch-list, relayout-ready from day one):
//   pass 0  load every slice resource; derive its ACTUAL byte length from
//           the parsed payload (never the o2r entry size); un-OTR display
//           lists into per-slice scratch words, recording every reference
//           as {word_index, target_slice, addend} instead of writing an
//           offset — resolution to a concrete pointer value is deferred.
//   layout  fast path: every actual size == spec vanilla size -> vanilla
//           offsets verbatim (I5: output equals the relocated view
//           byte-for-byte). Relayout path (mods, Phase 6): reassign.
//   pass 2  blit slices at their layout offsets; apply DL patches with the
//           TARGET slice's layout offset; emit explicit intern/extern slot
//           lists (RelocFile) in layout coordinates for the loader.
//
// Every failure cites its invariant (docs/deblob.md) and the diagnosis
// command. SSB64_SYNTH_INSPECT=<id> dumps the build's decisions as JSON
// shaped to diff against the generator manifest (the L2<->L5 gate).

#include "SyntheticReloc.h"
#include "RelocFile.h"
#include "RelocFileTable.h"
#include "../bridge/ssb64_reloc_rebuild.h"

#include <cstring>
#include <algorithm>
#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <ship/resource/type/Blob.h>
#include <ship/utils/StrHash64.h>
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Texture.h>
#include <fast/resource/type/Vertex.h>

// ---------------------------------------------------------------------------
// Spec lookup: direct-index table over the generated file_id-sorted array.
// ---------------------------------------------------------------------------

static const SSB64SyntheticRelocSpec **sSpecByFileId = nullptr;
static std::once_flag sSpecTableOnce;

const SSB64SyntheticRelocSpec *portGetSyntheticRelocSpec(uint32_t file_id)
{
	std::call_once(sSpecTableOnce, [] {
		sSpecByFileId = new const SSB64SyntheticRelocSpec *[RELOC_FILE_COUNT]();
		for (uint32_t i = 0; i < gSyntheticRelocSpecCount; i++)
		{
			const auto &spec = gSyntheticRelocSpecs[i];
			if (spec.file_id < RELOC_FILE_COUNT)
			{
				sSpecByFileId[spec.file_id] = &spec;
			}
		}
	});
	if (file_id >= RELOC_FILE_COUNT)
	{
		return nullptr;
	}
	return sSpecByFileId[file_id];
}

// ---------------------------------------------------------------------------
// OTR display-list decoding (pass 0)
// ---------------------------------------------------------------------------

// Torch's binary DL exporter replaces pointer-carrying commands with OTR
// pseudo-opcodes followed by one CRC64-of-path word pair. An "injected"
// pair (both words equal to the command's own) marks an unresolved literal
// — the original word is preserved verbatim (this is how raw seg-0x0E refs
// and extern-slot descriptor words survive byte-exactly).
static constexpr uint8_t kOTRGSetTImgHash = 0x20;
static constexpr uint8_t kOTRGDlHash = 0x31;
static constexpr uint8_t kOTRGVtxHash = 0x32;
static constexpr uint8_t kOTRGMarker = 0x33;
static constexpr uint8_t kOTRGMoveMemHash = 0x42;

static constexpr uint8_t kSeg0E = 0x0E;
static constexpr int32_t kPatchLiteral = -1;

struct SynthDlPatch
{
	uint32_t word_index;   // index into SynthDl::words (the w1 word)
	int32_t target_slice;  // slice index, or kPatchLiteral
	uint32_t addend;       // byte offset added to the target slice's layout offset
	bool seg0e;            // reconstruct with the 0x0E segment prefix
	uint32_t read_size = 1; // full referenced range, not just its first byte
};

struct SynthDl
{
	std::vector<uint32_t> words;   // real F3DEX2 words; patched w1s hold 0
	std::vector<SynthDlPatch> patches;
};

static uint64_t portReadHash64(const Gfx &cmd)
{
	return (static_cast<uint64_t>(static_cast<uint32_t>(cmd.words.w0)) << 32) |
	       static_cast<uint32_t>(cmd.words.w1);
}

// Torch's unresolved-reference encoding (DisplayListFactory export): the
// OTR pseudo-command is followed by the ORIGINAL command pair verbatim
// (its top byte is the real opcode — 0xFD/0xDE/0xDC), not a CRC64 hash.
// A real hash's top byte colliding with the expected opcode is the
// residual I7 hazard documented in docs/deblob.md.
static bool portIsLiteralPair(const Gfx &pair, uint8_t real_opcode)
{
	return static_cast<uint8_t>(static_cast<uint32_t>(pair.words.w0) >> 24) == real_opcode;
}

// PR #271: a binary mod may move a command while retaining its exact
// external-chain descriptor. Recover its identity, never its old slot.
// Match before interpreting segment prefixes: a chain-next word can start
// with 0x0E too. Ambiguous descriptors are rejected.
static const SSB64RelocExternSlot *portFindExternalDescriptor(
    const SSB64SyntheticRelocSpec &spec, const char *path, uint32_t value)
{
	const SSB64RelocExternSlot *match = nullptr;
	for (uint32_t e = 0; e < spec.extern_slot_count; e++)
	{
		const auto &slot = spec.extern_slots[e];
		if (std::strcmp(spec.slices[slot.slot_slice].path, path) != 0) continue;
		uint32_t next = 0xFFFF;
		if (e + 1 < spec.extern_slot_count)
		{
			const auto &n = spec.extern_slots[e + 1];
			next = (spec.slices[n.slot_slice].vanilla_offset + n.slot_offset_in_slice) / 4;
		}
		if (value != ((next << 16) | slot.dep_word_offset)) continue;
		if (match && (match->dep_file_id != slot.dep_file_id || match->dep_word_offset != slot.dep_word_offset))
			return nullptr;
		match = &slot;
	}
	return match;
}

// Map a vanilla byte offset to its containing slice (binary search over the
// spec's sorted vanilla layout). Used to make literal seg-0x0E references
// relayout-safe: they become slice+delta patches instead of raw offsets.
static int32_t portFindVanillaSlice(const SSB64SyntheticRelocSpec &spec, uint32_t vanilla_off)
{
	uint32_t lo = 0, hi = spec.slice_count;
	while (lo < hi)
	{
		uint32_t mid = (lo + hi) / 2;
		const auto &s = spec.slices[mid];
		if (vanilla_off < s.vanilla_offset)
		{
			hi = mid;
		}
		else if (vanilla_off >= s.vanilla_offset + s.vanilla_size)
		{
			lo = mid + 1;
		}
		else
		{
			return (int32_t)mid;
		}
	}
	return kPatchLiteral;
}

// Push a literal (unresolved-at-extraction) w1. Seg-0x0E words become
// slice+delta patches so relayout can remap them; everything else (extern
// descriptors, plain literals) passes through untouched.
static void portPushLiteralRef(const SSB64SyntheticRelocSpec &spec, SynthDl &out, uint32_t w1)
{
	if ((w1 >> 24) == kSeg0E)
	{
		int32_t slice = portFindVanillaSlice(spec, w1 & 0x00FFFFFF);
		if (slice != kPatchLiteral)
		{
			out.patches.push_back({(uint32_t)out.words.size(), slice,
			                       (w1 & 0x00FFFFFF) - spec.slices[slice].vanilla_offset,
			                       /* seg0e */ true});
			out.words.push_back(0);
			return;
		}
	}
	out.words.push_back(w1);
}

static bool portUnOtrDisplayList(const Fast::DisplayList &dl,
                                 const SSB64SyntheticRelocSpec &spec,
                                 const std::function<bool(uint64_t, int32_t &)> &resolveReference,
                                 const char *slice_path, uint32_t vanilla_size,
                                 bool vanilla,
                                 SynthDl &out)
{
	out.words.reserve(dl.Instructions.size() * 2);
	bool skipArtificialBranchEnd = false;

	auto resolveHash = [&](const Gfx &hashCmd, int32_t &slice_out) -> bool {
		if (!resolveReference(portReadHash64(hashCmd), slice_out))
		{
			// I7: resolution must be total — an unresolved non-injected
			// hash is a fatal load error, never a silent passthrough.
			spdlog::error("[deblob] I7 violated: unresolved reference hash 0x{:016X} in '{}'"
			              " — diagnose: compare SSB64_SYNTH_INSPECT output with the manifest",
			              portReadHash64(hashCmd), slice_path);
			return false;
		}
		return true;
	};

	for (size_t i = 0; i < dl.Instructions.size(); i++)
	{
		const auto &cmd = dl.Instructions[i];
		const uint32_t w0 = static_cast<uint32_t>(cmd.words.w0);
		const uint32_t w1 = static_cast<uint32_t>(cmd.words.w1);
		const uint8_t opcode = static_cast<uint8_t>(w0 >> 24);

		// Fast64 XML uses LUS-owned path strings, unlike Torch's binary
		// hash pairs. Never truncate a host string pointer into the N64 image.
		if (opcode == 0x24 || opcode == 0x25 || opcode == 0x27)
		{
			const char *path = reinterpret_cast<const char *>(cmd.words.w1);
			if (std::find(dl.Strings.begin(), dl.Strings.end(), path) == dl.Strings.end())
			{
				spdlog::error("[deblob] invalid filepath command in '{}'", slice_path);
				return false;
			}
			int32_t target;
			if (!resolveReference(CRC64(path), target))
			{
				spdlog::error("[deblob] missing or unsupported asset '{}' referenced by '{}'", path, slice_path);
				return false;
			}
			uint32_t addend = 0, width = 1;
			if (opcode == 0x24)
			{
				if (++i >= dl.Instructions.size()) return false;
				const auto &args = dl.Instructions[i];
				const uint32_t count = static_cast<uint32_t>(args.words.w0);
				const uint32_t index = static_cast<uint32_t>(args.words.w1) >> 16;
				if (count == 0 || count > 32 || index > 32 - count) return false;
				addend = (static_cast<uint32_t>(args.words.w1) & 0xFFFF) * sizeof(Vtx);
				width = count * sizeof(Vtx);
				out.words.push_back(0x01000000u | (count << 12) | ((index + count) << 1));
			}
			else
			{
				out.words.push_back(((opcode == 0x25 ? 0xFDu : 0xDEu) << 24) | (w0 & 0x00FFFFFFu));
			}
			out.patches.push_back({(uint32_t)out.words.size(), target, addend, false, width});
			out.words.push_back(0);
			continue;
		}
		if (opcode == 0x26) // XML Triangle1 uses LUS's wide-index encoding
		{
			const uint32_t a = w0 & 0xFFFFFF, b = w1 >> 16, c = w1 & 0xFFFF;
			if (a >= 32 || b >= 32 || c >= 32) return false;
			out.words.push_back(0x05000000u | (a << 17) | (b << 9) | (c << 1));
			out.words.push_back(0);
			continue;
		}

		if (skipArtificialBranchEnd && opcode == 0xDF)
		{
			skipArtificialBranchEnd = false;
			continue;
		}
		skipArtificialBranchEnd = false;

		if (opcode == kOTRGMarker)
		{
			if (i + 1 >= dl.Instructions.size()) return false;
			i++;
			continue;
		}

		if (opcode == kOTRGVtxHash)
		{
			// Unresolved G_VTX refs never get the OTR opcode (torch emits
			// the plain original command instead), so 0x32 is always
			// followed by a real hash pair.
			if ((i + 1) >= dl.Instructions.size())
			{
				spdlog::error("[deblob] truncated VTX OTR command in '{}'", slice_path);
				return false;
			}
			const auto &hashCmd = dl.Instructions[++i];
			const uint32_t nvtx = (w0 >> 12) & 0xFF;
			const uint32_t end = (w0 >> 1) & 0x7F;
			if (nvtx == 0 || nvtx > 32 || end < nvtx || end > 32 || w1 % sizeof(Vtx) != 0)
			{
				spdlog::error("[deblob] invalid vertex count, buffer index or alignment in '{}'", slice_path);
				return false;
			}
			out.words.push_back((0x01u << 24) | (nvtx << 12) | (end << 1));
			int32_t slice;
			if (!resolveHash(hashCmd, slice))
			{
				return false;
			}
			// w1 carries the byte delta within the vertex slice
			out.patches.push_back({(uint32_t)out.words.size(), slice, w1, false,
			                       nvtx * static_cast<uint32_t>(sizeof(Vtx))});
			out.words.push_back(0);
			continue;
		}

		if (opcode == kOTRGDlHash || opcode == kOTRGSetTImgHash || opcode == kOTRGMoveMemHash)
		{
			if ((i + 1) >= dl.Instructions.size())
			{
				spdlog::error("[deblob] truncated OTR ref command 0x{:02X} in '{}'",
				              opcode, slice_path);
				return false;
			}
			const uint8_t real_opcode =
				(opcode == kOTRGDlHash) ? 0xDE :
				(opcode == kOTRGSetTImgHash) ? 0xFD : 0xDC;
			const auto &pairCmd = dl.Instructions[++i];

			// Torch's unresolved encodings differ per opcode: G_DL and
			// G_MOVEMEM duplicate the OTR command as the pair; G_SETTIMG
			// writes the ORIGINAL command as the pair (real opcode in the
			// top byte). Cover both shapes.
			const bool duplicated =
				(pairCmd.words.w0 == cmd.words.w0 && pairCmd.words.w1 == cmd.words.w1);
			int32_t resolved_slice;
			const bool named_reference = resolveReference(portReadHash64(pairCmd), resolved_slice);
			if (!named_reference && (duplicated || portIsLiteralPair(pairCmd, real_opcode)))
			{
				const uint32_t literal = duplicated ? w1 : static_cast<uint32_t>(pairCmd.words.w1);
				const bool external = !vanilla && portFindExternalDescriptor(spec, slice_path, literal);
				if (!vanilla && !external && ((literal >> 24) == 0 || (literal >> 24) > 15))
				{
					spdlog::error("[deblob] unresolved vanilla relocation descriptor in override '{}'; export a named asset reference", slice_path);
					return false;
				}
				if (opcode == kOTRGMoveMemHash)
				{
					// The OTR MOVEMEM rewrite destroys the original
					// pointer word — an unresolved one cannot be
					// reconstructed. Generator-side node coverage must
					// prevent this (I7).
					spdlog::error("[deblob] I7 violated: unresolved MOVEMEM in '{}'"
					              " (pointer lost at extraction) — diagnose:"
					              " compare SSB64_SYNTH_INSPECT output with the manifest",
					              slice_path);
					return false;
				}
				if (duplicated)
				{
					// OTR words duplicated: reconstruct the real command
					// (branch bits were zeroed by the OTR macro; the I5
					// gate catches any vanilla DL that actually used them)
					out.words.push_back(((uint32_t)real_opcode << 24) |
					                    (opcode == kOTRGSetTImgHash ? (w0 & 0x00FFFFFFu) : 0u));
					if (external) out.words.push_back(w1);
					else portPushLiteralRef(spec, out, w1);
				}
				else
				{
					// pair IS the original command — emit verbatim
					out.words.push_back(static_cast<uint32_t>(pairCmd.words.w0));
					if (external) out.words.push_back(literal);
					else portPushLiteralRef(spec, out, literal);
				}
			}
			else
			{
				if (!named_reference)
				{
					spdlog::error("[deblob] I7 violated: unresolved reference hash 0x{:016X} in '{}'",
					              portReadHash64(pairCmd), slice_path);
					return false;
				}
				const int32_t slice = resolved_slice;
				if (opcode == kOTRGDlHash)
				{
					const uint32_t branch = (w0 >> 16) & 0xFF;
					out.words.push_back((0xDEu << 24) | (branch << 16));
					out.patches.push_back({(uint32_t)out.words.size(), slice, 0, false});
				}
				else if (opcode == kOTRGSetTImgHash)
				{
					out.words.push_back((0xFDu << 24) | (w0 & 0x00FFFFFFu));
					out.patches.push_back({(uint32_t)out.words.size(), slice, 0, false});
				}
				else
				{
					const bool hasOffset = ((w1 >> 8) & 0xFF) != 0;
					out.words.push_back((0xDCu << 24) | (w0 & 0x00FFFFFFu));
					out.patches.push_back({(uint32_t)out.words.size(), slice,
					                       hasOffset ? 8u : 0u, false});
				}
				out.words.push_back(0);
			}
			if (opcode == kOTRGDlHash)
			{
				skipArtificialBranchEnd = (((w0 >> 16) & 0xFF) & G_DL_NOPUSH) != 0;
			}
			continue;
		}

		if (!vanilla && ((opcode >= 0x20 && opcode <= 0x45) ||
		    ((opcode == 0x01 || opcode == 0xDA || opcode == 0xDC || opcode == 0xDE || opcode == 0xFD) &&
		     ((w1 >> 24) == 0 || (w1 >> 24) > 15) &&
		     !portFindExternalDescriptor(spec, slice_path, w1))))
		{
			spdlog::error("[deblob] unsupported or unresolved pointer command 0x{:02X} in override '{}'", opcode, slice_path);
			return false;
		}
		out.words.push_back(w0);
		out.words.push_back(w1);
	}

	// Torch appends a G_ENDDL terminator to count-delimited joint DLs so
	// LUS's factory can read them; drop it when the reconstruction lands
	// exactly one command past the vanilla slice (byte-exact fast path).
	if (vanilla && out.words.size() >= 2 &&
	    out.words.size() * sizeof(uint32_t) == (size_t)vanilla_size + 8 &&
	    out.words[out.words.size() - 2] == 0xDF000000u &&
	    out.words.back() == 0)
	{
		out.words.resize(out.words.size() - 2);
	}
	if (!vanilla && (out.words.size() < 2 ||
	    (out.words[out.words.size() - 2] != 0xDF000000u &&
	     out.words[out.words.size() - 2] != 0xDE010000u)))
	{
		spdlog::error("[deblob] replacement DL '{}' must end with EndDisplayList or JumpToDisplayList", slice_path);
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Build
// ---------------------------------------------------------------------------

struct SynthCacheEntry
{
	std::shared_ptr<RelocFile> reloc;
	std::vector<uint32_t> layout_offsets;   // per-slice layout offsets
	std::vector<uint32_t> intern_pairs;     // flat (slot,target) pairs, layout coords
	std::vector<uint32_t> actual_sizes;
};

// Intentionally leaked: the cache holds Ship::IResource shared_ptrs, and
// running their destructors during static teardown (after spdlog's own
// statics are gone) SIGABRTs — same pattern PortShutdown documents for
// the context. Process exit reclaims the memory.
static auto &sSynthCache = *new std::unordered_map<uint32_t, SynthCacheEntry>();
static std::mutex sSynthCacheMutex;
static std::weak_ptr<Ship::Archive> sBaseArchive;

void portSyntheticRelocSetBaseArchive(std::shared_ptr<Ship::Archive> archive)
{
	std::lock_guard<std::mutex> lock(sSynthCacheMutex);
	sBaseArchive = archive;
	sSynthCache.clear();
}

void portSyntheticRelocEvictAll()
{
	std::lock_guard<std::mutex> lock(sSynthCacheMutex);
	sSynthCache.clear();
}

// CRC32-IEEE (matches zlib's crc32(), which the generator's zlib.crc32
// uses for spec.relocated_view_crc32). Self-contained to keep the ssb64
// link line free of a direct zlib dependency.
static uint32_t portCrc32(const uint8_t *data, size_t size)
{
	static uint32_t table[256];
	static std::once_flag once;
	std::call_once(once, [] {
		for (uint32_t i = 0; i < 256; i++)
		{
			uint32_t c = i;
			for (int k = 0; k < 8; k++)
			{
				c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
			}
			table[i] = c;
		}
	});
	uint32_t crc = 0xFFFFFFFFu;
	for (size_t i = 0; i < size; i++)
	{
		crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFFu;
}

static void portWriteBE32(std::vector<uint8_t> &out, size_t offset, uint32_t value)
{
	out[offset + 0] = static_cast<uint8_t>((value >> 24) & 0xFF);
	out[offset + 1] = static_cast<uint8_t>((value >> 16) & 0xFF);
	out[offset + 2] = static_cast<uint8_t>((value >> 8) & 0xFF);
	out[offset + 3] = static_cast<uint8_t>(value & 0xFF);
}

static void portWriteBE16(std::vector<uint8_t> &out, size_t offset, uint16_t value)
{
	out[offset + 0] = static_cast<uint8_t>((value >> 8) & 0xFF);
	out[offset + 1] = static_cast<uint8_t>(value & 0xFF);
}

static void portDumpSynthInspect(const SSB64SyntheticRelocSpec &spec,
                                 const SynthCacheEntry &entry,
                                 const std::vector<uint32_t> &actual_sizes,
                                 const std::vector<SynthDl> &dl_scratch)
{
	const char *inspect = getenv("SSB64_SYNTH_INSPECT");
	if (inspect == nullptr || strtoul(inspect, nullptr, 0) != spec.file_id)
	{
		return;
	}

	char path[256];
	snprintf(path, sizeof(path), "debug_traces/synth_inspect_%u.json", spec.file_id);
	FILE *f = fopen(path, "wb");
	if (f == nullptr)
	{
		return;
	}
	fprintf(f, "{\n \"file_id\": %u,\n \"parent_path\": \"%s\",\n \"data_size\": %zu,\n \"slices\": [\n",
	        spec.file_id, spec.parent_path, entry.reloc->Data.size());
	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		const auto &s = spec.slices[i];
		fprintf(f, "  {\"path\": \"%s\", \"vanilla_offset\": %u, \"vanilla_size\": %u,"
		           " \"layout_offset\": %u, \"actual_size\": %u}%s\n",
		        s.path, s.vanilla_offset, s.vanilla_size,
		        entry.layout_offsets[i], actual_sizes[i],
		        (i + 1 < spec.slice_count) ? "," : "");
	}
	fprintf(f, " ],\n \"dl_patches\": [\n");
	bool first = true;
	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		for (const auto &p : dl_scratch[i].patches)
		{
			fprintf(f, "%s  {\"dl\": \"%s\", \"word\": %u, \"target_slice\": %d,"
			           " \"addend\": %u, \"seg0e\": %s}",
			        first ? "" : ",\n", spec.slices[i].path, p.word_index,
			        p.target_slice, p.addend, p.seg0e ? "true" : "false");
			first = false;
		}
	}
	fprintf(f, "\n ],\n \"intern_slots\": [");
	first = true;
	for (const auto &slot : entry.reloc->ExplicitInternSlots)
	{
		fprintf(f, "%s{\"slot\": %u, \"target\": %u}", first ? "" : ",",
		        slot.SlotByteOff, slot.TargetByteOff);
		first = false;
	}
	fprintf(f, "],\n \"extern_slots\": [");
	first = true;
	for (const auto &slot : entry.reloc->ExplicitExternSlots)
	{
		fprintf(f, "%s{\"slot\": %u, \"file\": %u, \"word\": %u}", first ? "" : ",",
		        slot.SlotByteOff, slot.DepFileId, slot.DepWordOff);
		first = false;
	}
	fprintf(f, "],\n \"intern_slot_count\": %zu,\n \"extern_slot_count\": %zu\n}\n",
	        entry.reloc->ExplicitInternSlots.size(), entry.reloc->ExplicitExternSlots.size());
	fclose(f);
	spdlog::info("[deblob] wrote synthesis inspect JSON to {}", path);
}

static void portDumpSynthBytesIfRequested(const SSB64SyntheticRelocSpec &spec, const RelocFile &relocFile)
{
	const char *dumpAll = getenv("SSB64_DUMP_SYNTH_RELOC");
	const char *dumpOne = getenv("SSB64_DUMP_SYNTH_RELOC_FILE_ID");
	bool shouldDump = (dumpAll != nullptr && dumpAll[0] == '1');
	if (dumpOne != nullptr && strtoul(dumpOne, nullptr, 0) == spec.file_id)
	{
		shouldDump = true;
	}
	if (!shouldDump)
	{
		return;
	}
	char path[256];
	snprintf(path, sizeof(path), "debug_traces/synth_reloc_%u.bin", spec.file_id);
	FILE *df = fopen(path, "wb");
	if (df != nullptr)
	{
		fwrite(relocFile.Data.data(), 1, relocFile.Data.size(), df);
		fclose(df);
		spdlog::info("[deblob] wrote synthesized bundle for '{}' to {}", spec.parent_path, path);
	}
}

// Remap a vanilla u32-word offset into dependency file `dep_file_id` to
// that file's CURRENT layout. Identity unless the dep is a deblobbed
// bundle that relayouted (a mod resized one of its slices) — then the
// vanilla offset is located in the dep's vanilla slice map and re-based
// on the slice's new layout offset. Builds the dep first so its layout
// exists (fighter dep graphs are acyclic: Main -> Model -> extern banks).
static thread_local std::vector<uint32_t> sSynthBuildStack;

static bool portRemapDepWordOffset(uint16_t dep_file_id, uint32_t dep_word_off, uint32_t &remapped)
{
	remapped = dep_word_off;
	const auto *depSpec = portGetSyntheticRelocSpec(dep_file_id);
	if (depSpec == nullptr)
	{
		return true;
	}
	for (uint32_t in_progress : sSynthBuildStack)
	{
		if (in_progress == dep_file_id)
		{
			spdlog::error("[deblob] cyclic bundle dependency at file_id {}", dep_file_id);
			return false;
		}
	}
	if (!portBuildSyntheticRelocResource(dep_file_id))
	{
		spdlog::error("[deblob] dependency file_id {} failed synthesis", dep_file_id);
		return false;
	}
	std::lock_guard<std::mutex> lock(sSynthCacheMutex);
	auto it = sSynthCache.find(dep_file_id);
	if (it == sSynthCache.end())
	{
		return false;
	}
	const uint32_t vanilla_byte = dep_word_off * 4;
	int32_t slice = portFindVanillaSlice(*depSpec, vanilla_byte);
	if (slice == kPatchLiteral)
	{
		spdlog::error("[deblob] dependency reference has no slice in file_id {}", dep_file_id);
		return false;
	}
	const uint32_t delta = vanilla_byte - depSpec->slices[slice].vanilla_offset;
	if (delta >= it->second.actual_sizes[slice])
	{
		spdlog::error("[deblob] dependency reference +0x{:X} exceeds replacement '{}'", delta, depSpec->slices[slice].path);
		return false;
	}
	remapped = (it->second.layout_offsets[slice] + delta) / 4;
	return true;
}

std::shared_ptr<RelocFile> portBuildSyntheticRelocResource(uint32_t file_id)
{
	const auto *specPtr = portGetSyntheticRelocSpec(file_id);
	if (specPtr == nullptr)
	{
		return nullptr;
	}
	// Extend the original slices with the transitive closure of new mod
	// assets. These have no vanilla address and are packed after the base.
	auto spec = *specPtr;
	std::vector<SSB64SyntheticSlice> slices(spec.slices, spec.slices + spec.slice_count);
	std::deque<std::string> extra_paths; // c_str pointers survive append
	spec.slices = slices.data();

	{
		std::lock_guard<std::mutex> lock(sSynthCacheMutex);
		auto it = sSynthCache.find(file_id);
		if (it != sSynthCache.end())
		{
			return it->second.reloc;
		}
	}

	auto ctx = Ship::Context::GetInstance();
	if (!ctx)
	{
		spdlog::error("[deblob] no Ship::Context for synthetic file_id {}", file_id);
		return nullptr;
	}
	auto rm = ctx->GetResourceManager();
	auto am = rm->GetArchiveManager();
	auto baseArchive = sBaseArchive.lock();
	if (!baseArchive)
	{
		spdlog::error("[deblob] base archive provenance was not registered");
		return nullptr;
	}

	// Recursion guard for cross-file remaps (portRemapDepWordOffset may
	// build a dependency bundle mid-build).
	struct BuildStackGuard
	{
		BuildStackGuard(uint32_t id) { sSynthBuildStack.push_back(id); }
		~BuildStackGuard() { sSynthBuildStack.pop_back(); }
	} build_guard(file_id);

	// hash -> slice index map for DL reference resolution
	std::unordered_map<uint64_t, uint32_t> hashToSlice;
	hashToSlice.reserve(spec.slice_count);
	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		hashToSlice.emplace(CRC64(spec.slices[i].path), i);
	}

	// --- pass 0: load slices, derive actual sizes, un-OTR display lists ---
	std::vector<std::shared_ptr<Ship::IResource>> resources(spec.slice_count);
	std::vector<uint32_t> actual_sizes(spec.slice_count, 0);
	std::vector<SynthDl> dl_scratch(spec.slice_count);
	std::vector<bool> overridden(spec.slice_count, false);
	bool any_override = false;
	auto resolveReference = [&](uint64_t hash, int32_t &target) -> bool {
		const auto found = hashToSlice.find(hash);
		if (found != hashToSlice.end())
		{
			target = static_cast<int32_t>(found->second);
			return true;
		}
		const auto path = am->HashToString(hash);
		if (!path || slices.size() >= specPtr->slice_count + 4096) return false;
		std::shared_ptr<Ship::IResource> resource;
		try { resource = rm->LoadResource(*path); }
		catch (const std::exception &e)
		{
			spdlog::error("[deblob] new asset '{}' failed to parse: {}", *path, e.what());
			return false;
		}
		SSB64SyntheticSliceKind kind;
		if (std::dynamic_pointer_cast<Fast::DisplayList>(resource)) kind = SSB64SyntheticSliceKind::DisplayList;
		else if (std::dynamic_pointer_cast<Fast::Vertex>(resource)) kind = SSB64SyntheticSliceKind::Vertex;
		else if (std::dynamic_pointer_cast<Fast::Texture>(resource)) kind = SSB64SyntheticSliceKind::Texture;
		else if (std::dynamic_pointer_cast<Ship::Blob>(resource)) kind = SSB64SyntheticSliceKind::Blob;
		else return false;
		target = static_cast<int32_t>(slices.size());
		extra_paths.push_back(*path);
		slices.push_back({extra_paths.back().c_str(), spec.vanilla_data_size, 0, kind});
		spec.slices = slices.data();
		spec.slice_count = static_cast<uint32_t>(slices.size());
		hashToSlice.emplace(hash, target);
		resources.push_back(resource);
		actual_sizes.push_back(0);
		dl_scratch.emplace_back();
		overridden.push_back(true);
		any_override = true;
		return true;
	};

	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		const auto slice = spec.slices[i];
		std::shared_ptr<Ship::IResource> resource = resources[i];
		try
		{
			if (!resource) resource = rm->LoadResource(slice.path);
		}
		catch (const std::exception &e)
		{
			// A factory parse failure surfaces here through the async
			// loader's future; one bad slice must degrade this bundle,
			// never kill the process.
			spdlog::error("[deblob] slice '{}' failed to parse: {} (parent '{}')",
			              slice.path, e.what(), spec.parent_path);
			return nullptr;
		}
		if (!resource)
		{
			spdlog::error("[deblob] missing slice '{}' for parent '{}'"
			              " — diagnose: unzip -l BattleShip.o2r | grep {}",
			              slice.path, spec.parent_path, spec.parent_path);
			return nullptr;
		}
		resources[i] = resource;
		// Pinned LUS does not populate ResourceInitData::Parent. Resolve the
		// actual factory path (including alt/ and .meta redirects) in the VFS.
		const auto init = resource->GetInitData();
		const std::string resource_path = init ? init->Path : slice.path;
		const auto meta_archive = am->GetArchiveFromFile(std::string(slice.path) + ".meta");
		overridden[i] = i >= specPtr->slice_count || am->GetArchiveFromFile(resource_path) != baseArchive ||
		                (meta_archive && meta_archive != baseArchive);
		any_override = any_override || overridden[i];

		switch (slice.kind)
		{
		case SSB64SyntheticSliceKind::Blob:
		{
			auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
			if (!blob)
			{
				spdlog::error("[deblob] slice '{}' is not a Blob", slice.path);
				return nullptr;
			}
			// LUS's Blob factory appends kBlobPadding (16) zero bytes for
			// N64 overread tolerance (ship BlobFactory.cpp) — the payload
			// is Data.size() minus that pad.
			constexpr uint32_t kLusBlobPadding = 16;
			actual_sizes[i] = (uint32_t)blob->Data.size() >= kLusBlobPadding
			                      ? (uint32_t)blob->Data.size() - kLusBlobPadding
			                      : (uint32_t)blob->Data.size();
			break;
		}
		case SSB64SyntheticSliceKind::Texture:
		{
			auto texture = std::dynamic_pointer_cast<Fast::Texture>(resource);
			if (!texture)
			{
				spdlog::error("[deblob] slice '{}' is not a Texture", slice.path);
				return nullptr;
			}
			actual_sizes[i] = texture->ImageDataSize;
			break;
		}
		case SSB64SyntheticSliceKind::Vertex:
		{
			auto vertex = std::dynamic_pointer_cast<Fast::Vertex>(resource);
			if (!vertex)
			{
				spdlog::error("[deblob] slice '{}' is not a Vertex", slice.path);
				return nullptr;
			}
			actual_sizes[i] = (uint32_t)(vertex->VertexList.size() * sizeof(Vtx));
			break;
		}
		case SSB64SyntheticSliceKind::DisplayList:
		{
			auto dl = std::dynamic_pointer_cast<Fast::DisplayList>(resource);
			if (!dl)
			{
				spdlog::error("[deblob] slice '{}' is not a DisplayList", slice.path);
				return nullptr;
			}
			SynthDl scratch;
			if (!portUnOtrDisplayList(*dl, spec, resolveReference, slice.path,
			                          slice.vanilla_size, !overridden[i], scratch))
			{
				return nullptr;
			}
			dl_scratch[i] = std::move(scratch);
			actual_sizes[i] = (uint32_t)(dl_scratch[i].words.size() * sizeof(uint32_t));
			break;
		}
		}
	}
	// Bound the packed image before offset arithmetic or allocation. The
	// original game uses 24-bit segmented addresses for file-local data.
	uint64_t total_size = spec.vanilla_data_size;
	for (uint32_t size : actual_sizes) total_size += static_cast<uint64_t>(size) + 16;
	if (total_size > 0x01000000)
	{
		spdlog::error("[deblob] bundle '{}' exceeds the 24-bit address space", spec.parent_path);
		return nullptr;
	}

	// --- layout ---
	// Fast path: every actual size matches the vanilla layout -> vanilla
	// offsets verbatim (I5 byte-exact). Otherwise RELAYOUT: keep vanilla
	// offsets up to the first resized slice, then pack forward with
	// 16-byte alignment (satisfies Gfx=8, Vtx=16, TMEM DMA sources, and
	// word-granular chain targets). Determinism note: vanilla inter-slice
	// padding before the first resize is preserved so the forced-relayout
	// gate (I6) can compare against the fast path.
	SynthCacheEntry entry;
	entry.layout_offsets.resize(spec.slice_count);
	entry.actual_sizes = actual_sizes;
	bool relayout = false;
	uint32_t cursor = 0;
	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		if (!relayout)
		{
			entry.layout_offsets[i] = spec.slices[i].vanilla_offset;
			if (actual_sizes[i] != spec.slices[i].vanilla_size)
			{
				relayout = true;
				spdlog::info("[deblob] relayout: '{}' size 0x{:X} != vanilla 0x{:X} — "
				             "repacking '{}' from slice {}",
				             spec.slices[i].path, actual_sizes[i],
				             spec.slices[i].vanilla_size, spec.parent_path, i);
			}
			cursor = entry.layout_offsets[i] + actual_sizes[i];
		}
		else
		{
			cursor = (cursor + 15u) & ~15u;
			entry.layout_offsets[i] = cursor;
			cursor += actual_sizes[i];
		}
	}
	const uint32_t data_size = relayout ? ((cursor + 7u) & ~7u)
	                                    : spec.vanilla_data_size;

	// I9 structural policy for resized slices: a reference or slot that
	// lands at delta > 0 inside a slice is only meaningful when the
	// replacement preserved that prefix — allow with a warning while the
	// delta still fits, fail loudly when it does not.
	auto check_delta = [&](uint32_t slice_idx, uint32_t delta, const char *what, uint32_t width = 1) -> bool {
		if (delta < actual_sizes[slice_idx] && width <= actual_sizes[slice_idx] - delta)
		{
			if (relayout && delta != 0 && spec.slices[slice_idx].vanilla_size != 0 &&
			    actual_sizes[slice_idx] != spec.slices[slice_idx].vanilla_size)
			{
				spdlog::warn("[deblob] {} at +0x{:X} into resized slice '{}' — "
				             "only valid for prefix-preserving replacements",
				             what, delta, spec.slices[slice_idx].path);
			}
			return true;
		}
		spdlog::error("[deblob] I9 violated: {} at +0x{:X} exceeds resized slice"
		              " '{}' (new size 0x{:X}) — diagnose: SSB64_SYNTH_INSPECT={}",
		              what, delta, spec.slices[slice_idx].path,
		              actual_sizes[slice_idx], spec.file_id);
		return false;
	};

	// --- pass 2: blit + patch ---
	auto relocFile = std::make_shared<RelocFile>(std::shared_ptr<Ship::ResourceInitData>());
	relocFile->FileId = spec.file_id;
	relocFile->RelocInternOffset = 0xFFFF;  // chains are gone; explicit lists only
	relocFile->RelocExternOffset = 0xFFFF;
	relocFile->Data.assign(data_size, 0);
	relocFile->HasExplicitRelocation = true;

	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		const auto &slice = spec.slices[i];
		const uint32_t off = entry.layout_offsets[i];

		switch (slice.kind)
		{
		case SSB64SyntheticSliceKind::Blob:
		{
			auto blob = std::static_pointer_cast<Ship::Blob>(resources[i]);
			std::memcpy(relocFile->Data.data() + off, blob->Data.data(), actual_sizes[i]);
			break;
		}
		case SSB64SyntheticSliceKind::Texture:
		{
			auto texture = std::static_pointer_cast<Fast::Texture>(resources[i]);
			std::memcpy(relocFile->Data.data() + off, texture->ImageData, actual_sizes[i]);
			break;
		}
		case SSB64SyntheticSliceKind::Vertex:
		{
			auto vertex = std::static_pointer_cast<Fast::Vertex>(resources[i]);
			size_t cursor = off;
			for (const auto &v : vertex->VertexList)
			{
				portWriteBE16(relocFile->Data, cursor + 0x0, (uint16_t)v.v.ob[0]);
				portWriteBE16(relocFile->Data, cursor + 0x2, (uint16_t)v.v.ob[1]);
				portWriteBE16(relocFile->Data, cursor + 0x4, (uint16_t)v.v.ob[2]);
				portWriteBE16(relocFile->Data, cursor + 0x6, v.v.flag);
				portWriteBE16(relocFile->Data, cursor + 0x8, (uint16_t)v.v.tc[0]);
				portWriteBE16(relocFile->Data, cursor + 0xA, (uint16_t)v.v.tc[1]);
				relocFile->Data[cursor + 0xC] = v.v.cn[0];
				relocFile->Data[cursor + 0xD] = v.v.cn[1];
				relocFile->Data[cursor + 0xE] = v.v.cn[2];
				relocFile->Data[cursor + 0xF] = v.v.cn[3];
				cursor += sizeof(Vtx);
			}
			break;
		}
		case SSB64SyntheticSliceKind::DisplayList:
		{
			auto &scratch = dl_scratch[i];
			// apply patches: target slice layout offset (+ addend), with
			// the seg-0x0E prefix restored for literal segment refs
			for (const auto &p : scratch.patches)
			{
				const uint8_t opcode = scratch.words[p.word_index - 1] >> 24;
				const auto kind = spec.slices[p.target_slice].kind;
				if (overridden[i] &&
				    ((opcode == 0x01 && kind != SSB64SyntheticSliceKind::Vertex) ||
				     (opcode == 0xDE && kind != SSB64SyntheticSliceKind::DisplayList) ||
				     (opcode == 0xFD && kind != SSB64SyntheticSliceKind::Texture && kind != SSB64SyntheticSliceKind::Blob)))
				{
					spdlog::error("[deblob] wrong resource type for command 0x{:02X} in '{}'", opcode, slice.path);
					return nullptr;
				}
				if (!check_delta((uint32_t)p.target_slice, p.addend, "DL reference",
				                 opcode == 0xDE ? 8 : p.read_size))
				{
					return nullptr;
				}
				uint32_t value = entry.layout_offsets[p.target_slice] + p.addend;
				if (p.seg0e)
				{
					value = ((uint32_t)kSeg0E << 24) | (value & 0x00FFFFFF);
				}
				scratch.words[p.word_index] = value;
			}
			for (size_t w = 0; w < scratch.words.size(); w++)
			{
				portWriteBE32(relocFile->Data, off + w * sizeof(uint32_t), scratch.words[w]);
			}
			break;
		}
		}
	}
	// A named DL can reference another new DL. Reject cycles before those
	// references become an endless renderer call/branch chain.
	std::vector<uint8_t> visiting(spec.slice_count, 0);
	std::function<bool(uint32_t)> visit = [&](uint32_t i) {
		if (visiting[i] == 1) return false;
		if (visiting[i] == 2) return true;
		visiting[i] = 1;
		for (const auto &p : dl_scratch[i].patches)
		{
			if (!p.seg0e && dl_scratch[i].words[p.word_index - 1] >> 24 == 0xDE &&
			    !visit(static_cast<uint32_t>(p.target_slice))) return false;
		}
		visiting[i] = 2;
		return true;
	};
	if (any_override)
	{
		for (uint32_t i = 0; i < spec.slice_count; i++)
		{
			if (!visit(i))
			{
				spdlog::error("[deblob] cyclic display-list references in '{}'", spec.parent_path);
				return nullptr;
			}
		}
	}

	// explicit slot lists in layout coordinates
	relocFile->ExplicitInternSlots.reserve(spec.intern_slot_count);
	for (uint32_t i = 0; i < spec.intern_slot_count; i++)
	{
		const auto &s = spec.intern_slots[i];
		// A replacement DL owns its command stream. Vanilla slot positions
		// describe the old program and must never be applied to the new one.
		if (overridden[s.slot_slice] &&
		    spec.slices[s.slot_slice].kind == SSB64SyntheticSliceKind::DisplayList) continue;
		if (!check_delta(s.slot_slice, s.slot_offset_in_slice, "intern slot position", 4) ||
		    !check_delta(s.target_slice, s.target_offset_in_slice, "intern slot target"))
		{
			return nullptr;
		}
		RelocExplicitInternSlot slot;
		slot.SlotByteOff = entry.layout_offsets[s.slot_slice] + s.slot_offset_in_slice;
		slot.TargetByteOff = entry.layout_offsets[s.target_slice] + s.target_offset_in_slice;
		relocFile->ExplicitInternSlots.push_back(slot);
		entry.intern_pairs.push_back(slot.SlotByteOff);
		entry.intern_pairs.push_back(slot.TargetByteOff);
	}
	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		if (!overridden[i]) continue;
		for (const auto &p : dl_scratch[i].patches)
		{
			if (p.seg0e) continue; // segmented references are resolved by the renderer
			RelocExplicitInternSlot slot{
				entry.layout_offsets[i] + p.word_index * 4,
				entry.layout_offsets[p.target_slice] + p.addend};
			relocFile->ExplicitInternSlots.push_back(slot);
			entry.intern_pairs.push_back(slot.SlotByteOff);
			entry.intern_pairs.push_back(slot.TargetByteOff);
		}
	}
	relocFile->ExplicitExternSlots.reserve(spec.extern_slot_count);
	relocFile->ExternFileIds.reserve(spec.extern_slot_count);
	for (uint32_t i = 0; i < spec.extern_slot_count; i++)
	{
		const auto &s = spec.extern_slots[i];
		if (overridden[s.slot_slice] &&
		    spec.slices[s.slot_slice].kind == SSB64SyntheticSliceKind::DisplayList) continue;
		if (!check_delta(s.slot_slice, s.slot_offset_in_slice, "extern slot position", 4))
		{
			return nullptr;
		}
		RelocExplicitExternSlot slot;
		slot.SlotByteOff = entry.layout_offsets[s.slot_slice] + s.slot_offset_in_slice;
		slot.DepFileId = s.dep_file_id;
		// Cross-file fidelity: a dependency that is itself a relayouted
		// deblobbed bundle moved its contents — remap the vanilla word
		// offset through the dep's layout. Identity for unmodded deps.
		if (!portRemapDepWordOffset(s.dep_file_id, s.dep_word_offset, slot.DepWordOff)) return nullptr;
		relocFile->ExplicitExternSlots.push_back(slot);
		// chain order == extern id order: lbRelocGetExternBytesNum walks
		// this list to predict dependency allocation sizes
		relocFile->ExternFileIds.push_back(s.dep_file_id);
	}

	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		if (!overridden[i] || spec.slices[i].kind != SSB64SyntheticSliceKind::DisplayList) continue;
		const auto &scratch = dl_scratch[i];
		for (uint32_t w = 1; w < scratch.words.size(); w += 2)
		{
			const uint32_t op = scratch.words[w - 1] >> 24;
			if (op != 0x01 && op != 0xDE && op != 0xFD && op != 0xDC) continue;
			if (std::any_of(scratch.patches.begin(), scratch.patches.end(),
			                [w](const auto &p) { return p.word_index == w; })) continue;
			const auto *external = portFindExternalDescriptor(spec, spec.slices[i].path, scratch.words[w]);
			if (!external) continue; // validated segmented references need no relocation
			RelocExplicitExternSlot slot;
			slot.SlotByteOff = entry.layout_offsets[i] + w * 4;
			slot.DepFileId = external->dep_file_id;
			if (!portRemapDepWordOffset(slot.DepFileId, external->dep_word_offset, slot.DepWordOff)) return nullptr;
			relocFile->ExplicitExternSlots.push_back(slot);
			relocFile->ExternFileIds.push_back(slot.DepFileId);
		}
	}

	// I5 applies to resources from the extracted archive. An override can
	// legitimately change content while preserving every size and offset.
	entry.reloc = relocFile;
	portDumpSynthInspect(spec, entry, actual_sizes, dl_scratch);
	portDumpSynthBytesIfRequested(spec, *relocFile);

	if (!any_override)
	{
		// Overridden bundles are validated structurally above.
		uint32_t crc = portCrc32(relocFile->Data.data(), relocFile->Data.size());
		if (crc != spec.relocated_view_crc32)
		{
			// Wrong bytes corrupt rendering and gameplay downstream; fail
			// the build so the caller's fallback (archived parent) runs.
			spdlog::error("[deblob] I5 violated: synthesized '{}' crc 0x{:08X} != spec 0x{:08X}"
			              " — diagnose: SSB64_DUMP_SYNTH_RELOC_FILE_ID={} then compare with"
			              " tools/generate_fighter_slices.py --only <Symbol> --check",
			              spec.parent_path, crc, spec.relocated_view_crc32, spec.file_id);
			return nullptr;
		}
	}

	{
		std::lock_guard<std::mutex> lock(sSynthCacheMutex);
		sSynthCache.emplace(file_id, std::move(entry));
	}
	spdlog::info("[deblob] synthesized '{}' from {} slices ({} bytes{})",
	             spec.parent_path, spec.slice_count, data_size,
	             any_override ? ", override" : ", crc ok");
	return relocFile;
}

// synth_verify harness (docs/deblob.md): build every spec'd bundle through
// the real archive/factory/synthesis path; each build enforces I5/I7/I9
// internally. Emits per-file verdicts as JSON for machine consumption and
// returns the failure count.
int portSyntheticRelocSelfTest()
{
	uint32_t pass = 0, fail = 0;
	std::vector<std::pair<const char *, bool>> verdicts;
	verdicts.reserve(gSyntheticRelocSpecCount);

	for (uint32_t i = 0; i < gSyntheticRelocSpecCount; i++)
	{
		const auto &spec = gSyntheticRelocSpecs[i];
		bool ok = portBuildSyntheticRelocResource(spec.file_id) != nullptr;
		verdicts.emplace_back(spec.parent_path, ok);
		ok ? pass++ : fail++;
	}

	FILE *f = fopen("debug_traces/synth_verify_results.json", "wb");
	if (f == nullptr)
	{
		// debug_traces/ may not exist in a fresh working dir
#ifdef _WIN32
		(void)system("mkdir debug_traces >NUL 2>&1");
#else
		(void)system("mkdir -p debug_traces");
#endif
		f = fopen("debug_traces/synth_verify_results.json", "wb");
	}
	if (f != nullptr)
	{
		fprintf(f, "{\n \"pass\": %u,\n \"fail\": %u,\n \"files\": [\n", pass, fail);
		for (size_t i = 0; i < verdicts.size(); i++)
		{
			fprintf(f, "  {\"path\": \"%s\", \"ok\": %s}%s\n",
			        verdicts[i].first, verdicts[i].second ? "true" : "false",
			        (i + 1 < verdicts.size()) ? "," : "");
		}
		fprintf(f, " ]\n}\n");
		fclose(f);
	}

	spdlog::info("[deblob] synth_verify: {} pass, {} fail of {} specs",
	             pass, fail, gSyntheticRelocSpecCount);
	fprintf(stderr, "[deblob] synth_verify: %u pass, %u fail of %u specs\n",
	        pass, fail, gSyntheticRelocSpecCount);
	for (const auto &[path, ok] : verdicts)
	{
		if (!ok)
		{
			fprintf(stderr, "[deblob]   FAIL %s\n", path);
		}
	}
	return (int)fail;
}

extern "C" int port_synth_reloc_get_intern_pairs(uint32_t file_id,
                                                 const uint32_t **pairs_out,
                                                 uint32_t *pair_count_out)
{
	if (pairs_out == nullptr || pair_count_out == nullptr)
	{
		return 1;
	}
	// ensure the bundle is built so the layout is current
	if (!portBuildSyntheticRelocResource(file_id))
	{
		return 1;
	}
	std::lock_guard<std::mutex> lock(sSynthCacheMutex);
	auto it = sSynthCache.find(file_id);
	if (it == sSynthCache.end())
	{
		return 1;
	}
	*pairs_out = it->second.intern_pairs.data();
	*pair_count_out = (uint32_t)(it->second.intern_pairs.size() / 2);
	return 0;
}

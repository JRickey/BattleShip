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
#include <mutex>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
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
                                 const std::unordered_map<uint64_t, uint32_t> &hashToSlice,
                                 const char *slice_path, uint32_t vanilla_size,
                                 SynthDl &out)
{
	out.words.reserve(dl.Instructions.size() * 2);
	bool skipArtificialBranchEnd = false;

	auto resolveHash = [&](const Gfx &hashCmd, int32_t &slice_out) -> bool {
		auto it = hashToSlice.find(portReadHash64(hashCmd));
		if (it == hashToSlice.end())
		{
			// I7: resolution must be total — an unresolved non-injected
			// hash is a fatal load error, never a silent passthrough.
			spdlog::error("[deblob] I7 violated: unresolved reference hash 0x{:016X} in '{}'"
			              " — diagnose: compare SSB64_SYNTH_INSPECT output with the manifest",
			              portReadHash64(hashCmd), slice_path);
			return false;
		}
		slice_out = (int32_t)it->second;
		return true;
	};

	for (size_t i = 0; i < dl.Instructions.size(); i++)
	{
		const auto &cmd = dl.Instructions[i];
		const uint32_t w0 = static_cast<uint32_t>(cmd.words.w0);
		const uint32_t w1 = static_cast<uint32_t>(cmd.words.w1);
		const uint8_t opcode = static_cast<uint8_t>(w0 >> 24);

		if (skipArtificialBranchEnd && opcode == 0xDF)
		{
			skipArtificialBranchEnd = false;
			continue;
		}
		skipArtificialBranchEnd = false;

		if (opcode == kOTRGMarker)
		{
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
			const uint32_t didx = ((w0 >> 1) & 0x7F) - nvtx;
			out.words.push_back((0x01u << 24) | (nvtx << 12) | ((didx + nvtx) << 1));
			int32_t slice;
			if (!resolveHash(hashCmd, slice))
			{
				return false;
			}
			// w1 carries the byte delta within the vertex slice
			out.patches.push_back({(uint32_t)out.words.size(), slice, w1, false});
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
			if (duplicated || portIsLiteralPair(pairCmd, real_opcode))
			{
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
					portPushLiteralRef(spec, out, w1);
				}
				else
				{
					// pair IS the original command — emit verbatim
					out.words.push_back(static_cast<uint32_t>(pairCmd.words.w0));
					portPushLiteralRef(spec, out, static_cast<uint32_t>(pairCmd.words.w1));
				}
			}
			else
			{
				int32_t slice;
				if (!resolveHash(pairCmd, slice))
				{
					return false;
				}
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

		out.words.push_back(w0);
		out.words.push_back(w1);
	}

	// Torch appends a G_ENDDL terminator to count-delimited joint DLs so
	// LUS's factory can read them; drop it when the reconstruction lands
	// exactly one command past the vanilla slice (byte-exact fast path).
	if (out.words.size() >= 2 &&
	    out.words.size() * sizeof(uint32_t) == (size_t)vanilla_size + 8 &&
	    out.words[out.words.size() - 2] == 0xDF000000u &&
	    out.words.back() == 0)
	{
		out.words.resize(out.words.size() - 2);
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
};

static std::unordered_map<uint32_t, SynthCacheEntry> sSynthCache;
static std::mutex sSynthCacheMutex;

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
	fprintf(f, "\n ],\n \"intern_slot_count\": %u,\n \"extern_slot_count\": %u\n}\n",
	        spec.intern_slot_count, spec.extern_slot_count);
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

std::shared_ptr<RelocFile> portBuildSyntheticRelocResource(uint32_t file_id)
{
	const auto *specPtr = portGetSyntheticRelocSpec(file_id);
	if (specPtr == nullptr)
	{
		return nullptr;
	}
	const auto &spec = *specPtr;

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

	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		const auto &slice = spec.slices[i];
		std::shared_ptr<Ship::IResource> resource;
		try
		{
			resource = rm->LoadResource(slice.path);
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
			if (!portUnOtrDisplayList(*dl, spec, hashToSlice, slice.path,
			                          slice.vanilla_size, dl_scratch[i]))
			{
				return nullptr;
			}
			actual_sizes[i] = (uint32_t)(dl_scratch[i].words.size() * sizeof(uint32_t));
			break;
		}
		}
	}

	// --- layout ---
	// Fast path requires every actual size to match the vanilla layout.
	// The relayout path (variable-size mod slices) lands in Phase 6; until
	// then a size mismatch is a hard, well-labeled failure.
	SynthCacheEntry entry;
	entry.layout_offsets.resize(spec.slice_count);
	bool all_vanilla = true;
	for (uint32_t i = 0; i < spec.slice_count; i++)
	{
		entry.layout_offsets[i] = spec.slices[i].vanilla_offset;
		if (actual_sizes[i] != spec.slices[i].vanilla_size)
		{
			all_vanilla = false;
			spdlog::error("[deblob] slice '{}' size 0x{:X} != vanilla 0x{:X}"
			              " (variable-size replacement not yet supported — relayout lands next)",
			              spec.slices[i].path, actual_sizes[i], spec.slices[i].vanilla_size);
		}
	}
	if (!all_vanilla)
	{
		return nullptr;
	}
	const uint32_t data_size = spec.vanilla_data_size;

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

	// explicit slot lists in layout coordinates
	relocFile->ExplicitInternSlots.reserve(spec.intern_slot_count);
	for (uint32_t i = 0; i < spec.intern_slot_count; i++)
	{
		const auto &s = spec.intern_slots[i];
		RelocExplicitInternSlot slot;
		slot.SlotByteOff = entry.layout_offsets[s.slot_slice] + s.slot_offset_in_slice;
		slot.TargetByteOff = entry.layout_offsets[s.target_slice] + s.target_offset_in_slice;
		relocFile->ExplicitInternSlots.push_back(slot);
		entry.intern_pairs.push_back(slot.SlotByteOff);
		entry.intern_pairs.push_back(slot.TargetByteOff);
	}
	relocFile->ExplicitExternSlots.reserve(spec.extern_slot_count);
	relocFile->ExternFileIds.reserve(spec.extern_slot_count);
	for (uint32_t i = 0; i < spec.extern_slot_count; i++)
	{
		const auto &s = spec.extern_slots[i];
		RelocExplicitExternSlot slot;
		slot.SlotByteOff = entry.layout_offsets[s.slot_slice] + s.slot_offset_in_slice;
		slot.DepFileId = s.dep_file_id;
		slot.DepWordOff = s.dep_word_offset;
		relocFile->ExplicitExternSlots.push_back(slot);
		// chain order == extern id order: lbRelocGetExternBytesNum walks
		// this list to predict dependency allocation sizes
		relocFile->ExternFileIds.push_back(s.dep_file_id);
	}

	// I5 vanilla gate (debug-grade here; synth_verify is the full gate):
	// the fast-path output must equal the relocated view the generator
	// hashed at extraction time.
	uint32_t crc = portCrc32(relocFile->Data.data(), relocFile->Data.size());
	entry.reloc = relocFile;
	portDumpSynthInspect(spec, entry, actual_sizes, dl_scratch);
	portDumpSynthBytesIfRequested(spec, *relocFile);

	if (crc != spec.relocated_view_crc32)
	{
		// A wrong-byte bundle corrupts rendering and gameplay downstream;
		// fail the build so the caller's fallback (archived parent) runs.
		spdlog::error("[deblob] I5 violated: synthesized '{}' crc 0x{:08X} != spec 0x{:08X}"
		              " — diagnose: SSB64_DUMP_SYNTH_RELOC_FILE_ID={} then compare with"
		              " tools/generate_fighter_slices.py --only <Symbol> --check",
		              spec.parent_path, crc, spec.relocated_view_crc32, spec.file_id);
		return nullptr;
	}

	{
		std::lock_guard<std::mutex> lock(sSynthCacheMutex);
		sSynthCache.emplace(file_id, std::move(entry));
	}
	spdlog::info("[deblob] synthesized '{}' from {} slices ({} bytes, crc ok)",
	             spec.parent_path, spec.slice_count, data_size);
	return relocFile;
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

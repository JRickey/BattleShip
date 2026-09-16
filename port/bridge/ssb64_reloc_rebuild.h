#pragma once

// Deblob rebuild specs — the generator <-> runtime contract (docs/deblob.md).
//
// Data lives in the generated ssb64_reloc_rebuild.<version>.cpp (one per ROM
// region, selected by CMake like RelocFileTable.<version>.cpp; regenerate
// with `python tools/generate_fighter_slices.py --version <v>`). Every spec
// describes one deblobbed bundle's VANILLA layout: offsets and sizes are
// lookup keys and fast-path values, never invariants — mod-replaced slices
// may be any size, and the runtime re-layouts the bundle around them.
//
// Slot metadata is slice-relative and emitted in CHAIN ORDER: extern slots
// consume extern file ids positionally, and the chain-next links baked into
// the bundle bytes do not survive relayout, so the runtime rebuilds all
// relocation from these lists (invariants I2/I3 guarantee containment).

#include <cstdint>

enum class SSB64SyntheticSliceKind : uint8_t
{
	Blob = 0,
	Vertex = 1,
	Texture = 2,
	DisplayList = 3,
};

struct SSB64SyntheticSlice
{
	const char *path;            // o2r path, e.g. "reloc_fighters_main/LinkModel/dLinkModel_Gfx_0x1D88"
	uint32_t vanilla_offset;     // byte offset in the vanilla bundle
	uint32_t vanilla_size;       // byte size in the vanilla bundle
	SSB64SyntheticSliceKind kind;
};

// One intern relocation slot (chain order). The slot word, at
// slices[slot_slice].vanilla_offset + slot_offset_in_slice, holds the
// pre-relocated plain byte offset of its target on the relocated view.
struct SSB64RelocInternSlot
{
	uint16_t slot_slice;
	uint16_t target_slice;
	uint32_t slot_offset_in_slice;
	uint32_t target_offset_in_slice;
};

// One extern relocation slot (chain order == extern file id order). The
// slot word keeps its chain-descriptor form in the bundle bytes;
// dep_word_offset is the word offset into the dependency file and is
// layout-invariant for non-deblobbed deps.
struct SSB64RelocExternSlot
{
	uint16_t slot_slice;
	uint16_t dep_file_id;
	uint32_t slot_offset_in_slice;
	uint32_t dep_word_offset;
};

struct SSB64SyntheticRelocSpec
{
	uint32_t file_id;
	const char *parent_path;         // gRelocFileTable[file_id] string
	uint32_t vanilla_data_size;
	uint32_t relocated_view_crc32;   // CRC32 of the vanilla relocated view (I5 debug assert)
	const char *manifest_hash;       // content hash of the manifest this spec was emitted with (I10)
	const SSB64SyntheticSlice *slices;
	uint32_t slice_count;
	const SSB64RelocInternSlot *intern_slots;
	uint32_t intern_slot_count;
	const SSB64RelocExternSlot *extern_slots;
	uint32_t extern_slot_count;
};

// Generated, file_id-sorted (binary-searchable).
extern const SSB64SyntheticRelocSpec gSyntheticRelocSpecs[];
extern const uint32_t gSyntheticRelocSpecCount;

// O(1) lookup (direct-index table built on first use); nullptr when the
// file is not deblobbed. Implemented by the synthesis engine.
const SSB64SyntheticRelocSpec *portGetSyntheticRelocSpec(uint32_t file_id);

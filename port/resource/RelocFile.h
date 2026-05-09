#pragma once

#include <ship/resource/Resource.h>
#include <vector>
#include <cstdint>

/**
 * SSB64 relocatable file resource.
 *
 * Loaded from .o2r archives at runtime. Contains decompressed file data plus
 * relocation metadata needed for pointer patching.
 *
 * `ProcessingFlags` (added in resource version 1) advertises which byte
 * transformations torch already applied at extraction time. The lbReloc
 * bridge inspects them to skip the equivalent runtime work. v0 archives
 * read with flags=0, meaning the runtime applies every step itself.
 */

/* When set, torch already applied pass1 BSWAP32 to `Data`. The bytes are in
 * post-pass1 form (each 4-byte group reversed) and the runtime must NOT
 * apply pass1 again. */
constexpr uint32_t PROC_PASS1_BSWAP_DONE = 1u << 0;

/* When set, torch already applied the pass2 DL-walk byte transforms (per-Vtx
 * rotate16+BSWAP32, per-format texture/palette BSWAP32) to `Data`. The runtime
 * still re-runs the scan to populate idempotency trackers (sStructU16Fixups
 * for vertices) — those are absolute-address state that lives in the runtime
 * heap, not in the file — but skips the actual byte-transform helpers. */
constexpr uint32_t PROC_PASS2_DONE = 1u << 1;

/* When the matching bit is set, torch already applied the corresponding
 * portFixup* family's byte transform. The runtime helper still records the
 * call in its idempotency tracker (sStructU16Fixups, heap-absolute state)
 * but skips the byte-transform body iff (a) this flag is set AND
 * (b) the StructFixupCatalog has a matching (file_id, byte_offset_in_file)
 * entry for the family. The catalog gate exists so a future torch SHA that
 * dropped a tuple does not silently corrupt data — without a catalog hit the
 * runtime falls back to the transform regardless of the flag. */
constexpr uint32_t PROC_STRUCT_U16_DONE     = 1u << 2;
constexpr uint32_t PROC_STRUCT_U32_DONE     = 1u << 3;  // also covers portFixupRawTextureBSWAP32
constexpr uint32_t PROC_SPRITE_DONE         = 1u << 4;
constexpr uint32_t PROC_BITMAP_DONE         = 1u << 5;
constexpr uint32_t PROC_MOBJSUB_DONE        = 1u << 6;
constexpr uint32_t PROC_FTATTRIBUTES_DONE   = 1u << 7;

/* When set, torch already applied the u16-halfswap to fighter
 * animation/submotion files (and reloc_scene/SCExplainMain). The runtime skips
 * the portRelocFixupFighterFigatree byte transform but still calls
 * port_aobj_register_halfswapped_range, since the halfswapped-range registry
 * is heap-absolute state that the lazy AObjEvent32 walker / FTKeyEvent reader
 * consult and must be populated every load. */
constexpr uint32_t PROC_HALFSWAP_DONE       = 1u << 8;

/* Bit 9 is reserved-unused per the Stage 7 closeout: the AObjEvent32 unhalfswap
 * walker stays runtime-side permanently. See
 * docs/decision_aobjevent32_runtime_walker_2026-05-09.md. */

/* When set, torch pre-walked the relocation chains and emitted a flat list
 * of (slot_byte_offset, target_byte_offset) entries — split into intern and
 * extern sublists. The runtime iterates that list directly, skipping the
 * encoded-chain walk in `lbreloc_bridge.cpp`. The encoded slot bytes are
 * still present in `Data` (untouched by torch) so v2 archives without the
 * runtime gate enabled can fall back to the encoded walker. The flat-list
 * iterator preserves every per-entry side effect of the encoded walker:
 * portRelocFixupTextureFromChain on internal entries, dep-file lazy-load
 * for externals, portRelocRegisterPointer + slot overwrite, optional
 * figatree halfswap-mask population, and the chain-observer callback used
 * by the fixture harness. */
constexpr uint32_t PROC_CHAIN_FLATTENED     = 1u << 10;

/* When set, torch emitted per-sub-asset Blob resources alongside the
 * container at "<entryOtrPath>__sub/<kind>/<offset>" and recorded
 * (hash, byte_offset, size, kind) for each in `SubResourceHashes`. The
 * lbReloc bridge resolves each hash via the resource manager before memcpy
 * — when a side-loaded mod .o2r overrides one (last-archive-wins), the
 * override bytes are overlaid into a mutable copy of `Data` at the recorded
 * offset, and the resulting buffer is what gets memcpy'd into the heap.
 * Containers without overrides take the no-op path (no allocation). */
constexpr uint32_t PROC_SUBRESOURCES_EMITTED = 1u << 11;

/* One pre-walked chain entry. byte_offset values are relative to the start
 * of `Data`. dep_file_id == 0xFFFF marks an internal entry; for externals it
 * matches ExternFileIds[i] indexed in chain insertion order. */
struct RelocChainEntry {
    uint32_t SlotByteOffset;
    uint32_t TargetByteOffset;
    uint16_t DepFileId;             // 0xFFFF = internal
};

/* Sub-resource kind tags. Mirrors torch/src/factories/ssb64/RelocFactory.cpp's
 * SubResKind enum. The runtime treats them as opaque labels — overlay logic
 * in lbreloc_bridge does not switch on them — but external tooling (the
 * Phase C example_mod scripts, a future mod-manager UI) reads them when
 * deciding which sub-resources a mod should target. */
enum class SubResKind : uint8_t {
    Vtx          = 0,
    Tex          = 1,
    Tlut         = 2,
    Sprite       = 3,
    Bitmap       = 4,
    MObjSub      = 5,
    StructU16    = 6,
    StructU32    = 7,
    FTAttributes = 8,
};

/* One sub-resource record: hash of the OTR path, byte offset/size into
 * `Data` for overlay, and a kind tag. Populated in v3 archives when
 * PROC_SUBRESOURCES_EMITTED is set; empty otherwise. */
struct RelocSubResource {
    uint64_t   Hash;
    uint32_t   ByteOffset;
    uint32_t   Size;
    SubResKind Kind;
};

class RelocFile final : public Ship::Resource<void> {
public:
    using Resource::Resource;

    void* GetPointer() override;
    size_t GetPointerSize() override;

    uint32_t FileId;
    uint16_t RelocInternOffset;     // internal reloc chain start (in u32 words), 0xFFFF = none
    uint16_t RelocExternOffset;     // external reloc chain start (in u32 words), 0xFFFF = none
    std::vector<uint16_t> ExternFileIds;   // file IDs referenced by external relocations
    uint32_t ProcessingFlags = 0;          // PROC_* bitmask; v0 archives: 0
    std::vector<RelocChainEntry> InternChain;  // populated when PROC_CHAIN_FLATTENED
    std::vector<RelocChainEntry> ExternChain;  // populated when PROC_CHAIN_FLATTENED
    std::vector<RelocSubResource> SubResourceHashes;  // populated when PROC_SUBRESOURCES_EMITTED
    std::vector<uint8_t> Data;             // post-`ProcessingFlags` bytes
};

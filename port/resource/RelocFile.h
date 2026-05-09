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
    std::vector<uint8_t> Data;             // post-`ProcessingFlags` bytes
};

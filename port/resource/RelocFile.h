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

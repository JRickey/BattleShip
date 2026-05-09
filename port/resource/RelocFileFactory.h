#pragma once

#include <ship/resource/ResourceFactoryBinary.h>

/**
 * LUS resource factory for SSB64 relocatable files.
 *
 * Reads the SSB64Reloc resource type (0x52454C4F / "RELO") from .o2r archives.
 *
 * Two on-disk versions exist; both factories are registered so old archives
 * keep loading after a torch bump:
 *
 *   v0 (legacy, registered for backward compat):
 *     u32  file_id
 *     u16  reloc_intern_offset
 *     u16  reloc_extern_offset
 *     u32  num_extern_file_ids
 *     u16[num_extern_file_ids]  extern_file_ids
 *     u32  decompressed_data_size
 *     u8[decompressed_data_size]  decompressed_data
 *
 *   v1 (current; introduced when Pass 1 BSWAP32 moved into torch):
 *     ... same prefix ...
 *     u32  processing_flags        // PROC_* bitmask in RelocFile.h
 *     u32  decompressed_data_size
 *     u8[decompressed_data_size]  decompressed_data   // already post-Pass1
 */
class ResourceFactoryBinaryRelocFileV0 final : public Ship::ResourceFactoryBinary {
public:
    std::shared_ptr<Ship::IResource> ReadResource(
        std::shared_ptr<Ship::File> file,
        std::shared_ptr<Ship::ResourceInitData> initData) override;
};

class ResourceFactoryBinaryRelocFileV1 final : public Ship::ResourceFactoryBinary {
public:
    std::shared_ptr<Ship::IResource> ReadResource(
        std::shared_ptr<Ship::File> file,
        std::shared_ptr<Ship::ResourceInitData> initData) override;
};

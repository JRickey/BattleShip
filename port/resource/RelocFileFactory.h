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
 *   v1 (introduced when Pass 1 BSWAP32 moved into torch):
 *     ... same prefix ...
 *     u32  processing_flags        // PROC_* bitmask in RelocFile.h
 *     u32  decompressed_data_size
 *     u8[decompressed_data_size]  decompressed_data   // already post-Pass1
 *
 *   v2 (Stage 9 — chain-flatten):
 *     ... same prefix ...
 *     u32  processing_flags
 *     u32  num_intern_chain_entries
 *     { u32 slot_byte_offset, u32 target_byte_offset }[num_intern_chain_entries]
 *     u32  num_extern_chain_entries
 *     { u32 slot_byte_offset, u32 target_byte_offset }[num_extern_chain_entries]
 *     u32  decompressed_data_size
 *     u8[decompressed_data_size]  decompressed_data
 *
 *   v3 (current; Stage 10 — sub-resource emission):
 *     ... v2 prefix through extern chain entries ...
 *     u32  num_sub_resource_hashes
 *     { u64 hash, u32 byte_offset, u32 size, u8 kind, u8[3] pad }[num_sub_resource_hashes]
 *     u32  decompressed_data_size
 *     u8[decompressed_data_size]  decompressed_data
 *
 * v2/v3 archives may have PROC_CHAIN_FLATTENED set (runtime iterates the flat
 * lists) or unset (defensive — runtime falls through to the encoded walker on
 * the `decompressed_data` slots, which torch leaves intact regardless).
 *
 * v3 archives may have PROC_SUBRESOURCES_EMITTED set (override-overlay path
 * checks the resource manager for each hash and overlays bytes into a
 * mutable copy of Data before memcpy) or unset (no overrides possible —
 * skip the entire overlay path).
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

class ResourceFactoryBinaryRelocFileV2 final : public Ship::ResourceFactoryBinary {
public:
    std::shared_ptr<Ship::IResource> ReadResource(
        std::shared_ptr<Ship::File> file,
        std::shared_ptr<Ship::ResourceInitData> initData) override;
};

class ResourceFactoryBinaryRelocFileV3 final : public Ship::ResourceFactoryBinary {
public:
    std::shared_ptr<Ship::IResource> ReadResource(
        std::shared_ptr<Ship::File> file,
        std::shared_ptr<Ship::ResourceInitData> initData) override;
};

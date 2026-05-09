#pragma once

#include <ship/resource/ResourceFactoryBinary.h>

/**
 * LUS resource factory for SSB64 relocatable files.
 *
 * Reads the SSB64Reloc resource type (0x52454C4F / "RELO") from .o2r archives.
 * Only the v3 layout is supported — Stage 11 dropped the V0/V1/V2 readers
 * along with the runtime byteswap/halfswap helpers they relied on. The
 * asset/torch coupling already forced re-extraction on every torch SHA bump,
 * so older archives are de-facto unsupported.
 *
 *   v3 layout:
 *     u32  file_id
 *     u16  reloc_intern_offset
 *     u16  reloc_extern_offset
 *     u32  num_extern_file_ids
 *     u16[num_extern_file_ids]  extern_file_ids
 *     u32  processing_flags        // PROC_* bitmask in RelocFile.h
 *     u32  num_intern_chain_entries
 *     { u32 slot_byte_offset, u32 target_byte_offset }[num_intern_chain_entries]
 *     u32  num_extern_chain_entries
 *     { u32 slot_byte_offset, u32 target_byte_offset }[num_extern_chain_entries]
 *     u32  num_sub_resource_hashes
 *     { u64 hash, u32 byte_offset, u32 size, u8 kind, u8[3] pad }[num_sub_resource_hashes]
 *     u32  decompressed_data_size
 *     u8[decompressed_data_size]  decompressed_data
 *
 * v3 archives may have PROC_SUBRESOURCES_EMITTED set (override-overlay path
 * checks the resource manager for each hash and overlays bytes into a
 * mutable copy of Data before memcpy) or unset (no overrides possible —
 * skip the entire overlay path).
 */
class ResourceFactoryBinaryRelocFileV3 final : public Ship::ResourceFactoryBinary {
public:
    std::shared_ptr<Ship::IResource> ReadResource(
        std::shared_ptr<Ship::File> file,
        std::shared_ptr<Ship::ResourceInitData> initData) override;
};

#pragma once

/**
 * fixture_format.h — Binary format for committed reloc-pipeline fixtures.
 *
 * Each fixture file pins one reloc file's post-load runtime state for byte-
 * for-byte comparison against future harness invocations. The format is
 * fixed-layout little-endian (M1 / x86-64 are LE; if we ever target a BE
 * test host, byte-swap on read).
 *
 * Layout:
 *
 *   Header (32 bytes):
 *     [0..3]   magic         u8[4]  = "RFX0"
 *     [4..7]   version       u32    = 1
 *     [8..11]  file_id       u32
 *     [12..15] section_flags u32    (see kSection* constants)
 *     [16..19] body_size     u32    bytes after header, excluding the crc trailer
 *     [20..27] body_crc64    u64    crc64 over body_size bytes
 *     [28..31] reserved      u32    = 0
 *
 *   Body sections in fixed order, only those whose flag bit is set are
 *   present:
 *
 *     DATA            u32 size + `size` bytes (post-fixup contents of the file)
 *     HALFSWAP_RANGES u32 count + (u32 base_offset, u32 size)[count]
 *     TRACKERS        u32 count + (u32 byte_offset, u8 family, u8 pad[3])[count]
 *     CHAIN_INTERN    u32 count + (u32 slot_off, u32 target_off)[count]
 *     CHAIN_EXTERN    u32 count + (u32 slot_off, u32 dep_file_id, u32 target_off)[count]
 *
 * Snapshot.path is NOT serialized — the fixture filename carries it.
 *
 * On read, body_crc64 is recomputed and compared. Mismatch is a hard error.
 */

#include "snapshot.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace harness {

constexpr uint32_t kFixtureMagic   = 0x30584652;   /* "RFX0" little-endian */
constexpr uint32_t kFixtureVersion = 1;

constexpr uint32_t kSectionData          = 1u << 0;
constexpr uint32_t kSectionHalfswap      = 1u << 1;
constexpr uint32_t kSectionTrackers      = 1u << 2;
constexpr uint32_t kSectionChainIntern   = 1u << 3;
constexpr uint32_t kSectionChainExtern   = 1u << 4;

/** Compose the canonical fixture filename for a given file_id + path.
 *  Format: <id_zero_padded>_<sanitized_path>.fixt
 *  Sanitization: '/' -> '_', strip leading "reloc_" prefix.
 *  Example: file_id=500, path="reloc_animations/FTMarioAnim001"
 *           -> "0500_animations_FTMarioAnim001.fixt"
 */
std::string FixtureFilename(uint32_t file_id, const std::string &path);

/** Write `snap` to `path`. Overwrites if exists.
 *  Returns true on success. */
bool WriteFixture(const std::filesystem::path &path, const Snapshot &snap);

/** Load a fixture file into a Snapshot. Verifies magic, version, body crc.
 *  Returns false (snap unmodified) on any header / crc mismatch. */
bool ReadFixture(const std::filesystem::path &path, Snapshot &out);

} // namespace harness

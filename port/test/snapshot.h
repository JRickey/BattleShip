#pragma once

/**
 * Snapshot — serialized capture of the post-load state of one reloc file.
 *
 * Comparable byte-for-byte: equality across runs of the same file_id is the
 * core invariant tests assert. Lists are stored sorted by their primary key
 * (slot offset for chains, byte offset for tracker entries) so the bridge's
 * underlying iteration order does not matter for equality.
 *
 * Snapshot fields are file-relative: every offset is a byte offset into the
 * buffer the bridge was handed for `file_id`. Token tables and absolute
 * heap addresses are NOT captured — they leak harness implementation
 * details into fixtures.
 *
 * Stage 2 will spill these to disk as `.fixt` files; for Stage 1 the
 * harness keeps them in memory only (snapshot diff via gtest output).
 */

#include <cstdint>
#include <string>
#include <vector>

namespace harness {

struct ChainTarget {
    uint32_t slot_offset;     /* byte offset of the chain slot inside the loading file */
    int32_t  dep_file_id;     /* -1 = internal target inside the same file */
    uint32_t target_offset;   /* byte offset of the target within (dep ? dep file : self) */

    bool operator==(const ChainTarget &o) const {
        return slot_offset == o.slot_offset && dep_file_id == o.dep_file_id && target_offset == o.target_offset;
    }
};

struct TrackerEntry {
    uint32_t byte_offset;     /* file-relative byte offset of the tracker key */
    uint8_t  family;          /* 0=StructU16, 1=TexFixupWord, 2=Deswizzle4c — see snapshot.cpp */

    bool operator==(const TrackerEntry &o) const {
        return byte_offset == o.byte_offset && family == o.family;
    }
};

struct HalfswapRange {
    uint32_t base_offset;     /* file-relative byte offset of the registered halfswap range start */
    uint32_t size;
    bool operator==(const HalfswapRange &o) const {
        return base_offset == o.base_offset && size == o.size;
    }
};

struct Snapshot {
    uint32_t file_id      = 0;
    std::string path;                       /* gRelocFileTable entry, for human-readable diffs */
    std::vector<uint8_t> data;              /* post-fixup bytes */
    std::vector<ChainTarget> chain_targets; /* sorted by slot_offset */
    std::vector<TrackerEntry> trackers;     /* sorted by byte_offset */
    std::vector<HalfswapRange> halfswap_ranges; /* sorted by base_offset (single-entry today) */

    bool operator==(const Snapshot &o) const {
        return file_id == o.file_id
            && data == o.data
            && chain_targets == o.chain_targets
            && trackers == o.trackers
            && halfswap_ranges == o.halfswap_ranges;
        /* path intentionally NOT compared — it's a human-readable label that
         * could legitimately drift if a yaml renames a file. */
    }

    /** Sort all list fields into canonical order. */
    void Canonicalize();

    /** Render a human-readable summary (sizes + first divergence vs. other).
     *  Used by the GTest assertion macro to localize fixture mismatches. */
    std::string Diff(const Snapshot &other) const;
};

constexpr uint8_t kFamilyStructU16    = 0;
constexpr uint8_t kFamilyTexFixupWord = 1;
constexpr uint8_t kFamilyDeswizzle4c  = 2;

} // namespace harness

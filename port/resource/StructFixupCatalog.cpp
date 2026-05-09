/**
 * StructFixupCatalog.cpp - Auto-generated portFixup* dispatch catalog.
 *
 * Source: port/test/struct_fixup_catalog.json
 * Generator: tools/generate_struct_fixup_catalog.py
 * DO NOT EDIT BY HAND.
 *
 * Family histogram (PROC_<FAMILY>_DONE bit → entry count):
 *   2 STRUCT_U16          0
 *   3 STRUCT_U32          0
 *   4 SPRITE              0
 *   5 BITMAP              0
 *   6 MOBJSUB             0
 *   7 FTATTRIBUTES        0
 * Total entries: 0
 */

#include "StructFixupCatalog.h"

#include <algorithm>
#include <cstdint>

namespace {

// Each entry packs (family<<56)|(file_id<<32)|byte_offset_in_file.
// Sorted ascending so the lookup uses std::binary_search.
constexpr uint64_t kEntries[] = {
    /* empty catalog — runtime applies all fixup byte transforms */
};

constexpr size_t kEntryCount = 0;
static_assert(sizeof(kEntries) / sizeof(kEntries[0]) == kEntryCount,
              "kEntries length must match kEntryCount");

}  // namespace

extern "C" bool portStructFixupCatalogHasEntry(int file_id, uint32_t byte_offset, uint8_t family)
{
    if (file_id < 0 || file_id >= (1 << 16) || family < 2 || family > 7)
        return false;
    const uint64_t key = (uint64_t(family) << 56)
                       | (uint64_t(uint32_t(file_id)) << 32)
                       | uint64_t(byte_offset);
    return std::binary_search(kEntries, kEntries + kEntryCount, key);
}

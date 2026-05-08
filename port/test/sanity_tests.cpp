/**
 * sanity_tests.cpp — Eight hand-picked reloc files, one per category, that
 * validate the harness + snapshot machinery before Stage 2 fans out across
 * all 2,132 files.
 *
 * Each test asserts:
 *   1. The load returns a non-empty snapshot (resource present, factory wired).
 *   2. Re-running the harness 3 times produces byte-identical snapshots
 *      (the determinism gate the prior two refactor attempts skipped).
 *
 * If any of these fails, the harness has a determinism bug that must be
 * fixed before Stage 2's fixture generation.
 */

#include "reloc_harness.h"

#include <gtest/gtest.h>

namespace {

struct SanityCase {
    uint32_t    file_id;
    const char *category;
    const char *expected_path_substr;
};

static const SanityCase kSanityCases[] = {
    {  0u, "menu_common",          "MNCommon"        },
    {  8u, "menu_sprite",          "MNVSItemSwitch"  },
    { 55u, "mv_opening",           "MVOpeningRun"    },
    { 83u, "effect_bank",          "EFCommonEffects1"},
    { 88u, "stage_geometry",       "StageDreamLand"  },
    {203u, "fighter_main",         "MarioMain"       },
    {251u, "item_common",          "ITCommonData"    },
    {500u, "fighter_animation",    "FTMarioAnim001"  },
};

class SanityFixture : public ::testing::TestWithParam<SanityCase> {};

TEST_P(SanityFixture, LoadIsDeterministic)
{
    const SanityCase &tc = GetParam();
    harness::MockHeap heap;

    auto first = harness::LoadAndProcess(tc.file_id, heap);
    ASSERT_GT(first.data.size(), 0u)
        << "category=" << tc.category << " file_id=" << tc.file_id
        << " path=" << first.path
        << " — bridge returned an empty buffer (resource missing or load failed?)";
    EXPECT_NE(first.path.find(tc.expected_path_substr), std::string::npos)
        << "category=" << tc.category << " expected substring '"
        << tc.expected_path_substr << "' in path '" << first.path << "'";

    /* Determinism: 2nd and 3rd loads must produce snapshots equal to the first. */
    auto second = harness::LoadAndProcess(tc.file_id, heap);
    EXPECT_EQ(first, second)
        << "category=" << tc.category << " file_id=" << tc.file_id
        << " — load #2 differs from load #1\n"
        << first.Diff(second);

    auto third = harness::LoadAndProcess(tc.file_id, heap);
    EXPECT_EQ(first, third)
        << "category=" << tc.category << " file_id=" << tc.file_id
        << " — load #3 differs from load #1\n"
        << first.Diff(third);
}

INSTANTIATE_TEST_SUITE_P(
    AcrossCategories, SanityFixture,
    ::testing::ValuesIn(kSanityCases),
    [](const ::testing::TestParamInfo<SanityCase> &info) {
        return std::string(info.param.category);
    });

/* The fighter animation case (FTMarioAnim001 = file_id 500) is special: it
 * goes through portRelocFixupFighterFigatree, which registers the file as a
 * halfswap range. The snapshot should reflect that. */
TEST(FighterAnimation, RegistersHalfswapRange)
{
    harness::MockHeap heap;
    auto snap = harness::LoadAndProcess(500u, heap);
    ASSERT_GT(snap.data.size(), 0u);
    EXPECT_EQ(snap.halfswap_ranges.size(), 1u)
        << "FTMarioAnim001 should register exactly one halfswap range";
    if (!snap.halfswap_ranges.empty()) {
        EXPECT_EQ(snap.halfswap_ranges[0].base_offset, 0u);
        EXPECT_EQ(snap.halfswap_ranges[0].size, snap.data.size());
    }
}

/* Conversely, a non-fighter file (StageDreamLand = file_id 88) must NOT
 * register a halfswap range — that would be a false positive and would
 * silently corrupt non-figatree data on a future migration step. */
TEST(StageGeometry, DoesNotRegisterHalfswapRange)
{
    harness::MockHeap heap;
    auto snap = harness::LoadAndProcess(88u, heap);
    ASSERT_GT(snap.data.size(), 0u);
    EXPECT_EQ(snap.halfswap_ranges.size(), 0u)
        << "StageDreamLand should not be classified as fighter-figatree";
}

} // namespace

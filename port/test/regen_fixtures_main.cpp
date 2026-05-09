/**
 * regen_fixtures_main.cpp — CLI to regenerate the .fixt fixtures.
 *
 * Single process, single archive open. For each requested file_id:
 *   1. LoadAndProcess → Snapshot.
 *   2. WriteFixture to <out_dir>/<canonical-name>.fixt.
 *   3. (Optional --check) compare against existing fixture; report diffs
 *      without writing.
 *
 * Usage:
 *   port_reloc_regen --all                       # write all 2,132 fixtures
 *   port_reloc_regen --ids 0,1,2,500             # write listed IDs only
 *   port_reloc_regen --all --check               # diff vs. committed, no writes
 *   port_reloc_regen --all --out-dir <path>      # custom output directory
 *   port_reloc_regen --all --force               # overwrite without diff check
 *
 * Default --out-dir is <repo>/port/test/fixtures, located by walking up
 * from cwd. SSB64_FIXTURES_DIR env var overrides.
 */

#include "fixture_format.h"
#include "mock_heap.h"
#include "reloc_harness.h"
#include "snapshot.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
extern const char *const gRelocFileTable[];
}
constexpr uint32_t kRelocFileCount = 2132;

namespace {

struct Args {
    std::vector<uint32_t> ids;       /* explicit list */
    bool                  all = false;
    bool                  check = false;
    bool                  force = false;
    bool                  quiet = false;
    std::filesystem::path out_dir;
};

void PrintUsage()
{
    std::fprintf(stderr,
        "Usage: port_reloc_regen --all | --ids id1,id2,... [options]\n"
        "  --out-dir <path>    output directory (default <repo>/port/test/fixtures)\n"
        "  --check             diff against existing fixtures, do not write\n"
        "  --force             overwrite without comparison\n"
        "  --quiet             only print summary + errors\n");
}

bool ParseIdList(const char *arg, std::vector<uint32_t> &out)
{
    const char *p = arg;
    while (*p) {
        char *end = nullptr;
        unsigned long v = std::strtoul(p, &end, 10);
        if (end == p) return false;
        if (v >= kRelocFileCount) {
            std::fprintf(stderr, "regen: id %lu out of range\n", v);
            return false;
        }
        out.push_back(static_cast<uint32_t>(v));
        if (*end == ',') p = end + 1;
        else if (*end == '\0') break;
        else return false;
    }
    return true;
}

std::filesystem::path FindDefaultFixturesDir()
{
    if (const char *env = std::getenv("SSB64_FIXTURES_DIR"); env != nullptr && *env != '\0') {
        return std::filesystem::path(env);
    }
    /* Walk up from cwd looking for the SOURCE tree's port/test/. We
     * disambiguate from the build dir's port/test/ (which also exists
     * after a build) by checking for sanity_tests.cpp — only the source
     * tree carries that file. */
    std::filesystem::path cwd = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i) {
        if (std::filesystem::exists(cwd / "port" / "test" / "sanity_tests.cpp")) {
            auto out = cwd / "port" / "test" / "fixtures";
            std::filesystem::create_directories(out);
            return out;
        }
        cwd = cwd.parent_path();
    }
    /* Fall back: cwd-relative. */
    auto p = std::filesystem::current_path() / "port" / "test" / "fixtures";
    std::filesystem::create_directories(p);
    return p;
}

bool ParseArgs(int argc, char **argv, Args &args)
{
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--all") {
            args.all = true;
        } else if (a == "--check") {
            args.check = true;
        } else if (a == "--force") {
            args.force = true;
        } else if (a == "--quiet") {
            args.quiet = true;
        } else if (a == "--ids" && i + 1 < argc) {
            if (!ParseIdList(argv[++i], args.ids)) return false;
        } else if (a == "--out-dir" && i + 1 < argc) {
            args.out_dir = argv[++i];
        } else if (a == "--help" || a == "-h") {
            PrintUsage();
            std::exit(0);
        } else {
            std::fprintf(stderr, "regen: unknown arg '%s'\n", a.c_str());
            return false;
        }
    }
    if (!args.all && args.ids.empty()) {
        std::fprintf(stderr, "regen: must specify --all or --ids\n");
        return false;
    }
    if (args.out_dir.empty()) {
        args.out_dir = FindDefaultFixturesDir();
    } else {
        std::filesystem::create_directories(args.out_dir);
    }
    return true;
}

struct Stats {
    uint32_t written = 0;
    uint32_t skipped_empty = 0;
    uint32_t failed = 0;
    uint32_t diff_match = 0;
    uint32_t diff_mismatch = 0;
    uint32_t diff_missing = 0;
    uint64_t total_bytes = 0;
};

bool ProcessOne(uint32_t file_id, harness::MockHeap &heap,
                const Args &args, Stats &stats)
{
    const char *path_cstr = (file_id < kRelocFileCount) ? gRelocFileTable[file_id] : nullptr;
    if (path_cstr == nullptr) {
        std::fprintf(stderr, "regen: file_id %u has null path entry\n", file_id);
        stats.failed++;
        return false;
    }

    auto snap = harness::LoadAndProcess(file_id, heap);
    if (snap.data.empty()) {
        if (!args.quiet) {
            std::fprintf(stderr, "regen: file_id %u (%s) — empty data, skipping\n",
                         file_id, path_cstr);
        }
        stats.skipped_empty++;
        return false;
    }

    auto fname = harness::FixtureFilename(file_id, path_cstr);
    auto fixture_path = args.out_dir / fname;

    if (args.check) {
        harness::Snapshot existing;
        if (!harness::ReadFixture(fixture_path, existing)) {
            std::fprintf(stderr, "regen: file_id %u (%s) — fixture missing or unreadable: %s\n",
                         file_id, path_cstr, fixture_path.string().c_str());
            stats.diff_missing++;
            return false;
        }
        existing.path = path_cstr;     /* path field was not serialized */
        snap.path = path_cstr;
        if (snap == existing) {
            stats.diff_match++;
            return true;
        }
        std::fprintf(stderr, "regen: file_id %u (%s) — DIFF\n%s",
                     file_id, path_cstr, snap.Diff(existing).c_str());
        stats.diff_mismatch++;
        return false;
    }

    /* Refuse to silently overwrite committed fixtures unless --force. */
    if (!args.force && std::filesystem::exists(fixture_path)) {
        harness::Snapshot existing;
        if (harness::ReadFixture(fixture_path, existing)) {
            existing.path = path_cstr;
            snap.path = path_cstr;
            if (snap == existing) {
                if (!args.quiet) {
                    std::fprintf(stdout, "regen: file_id %u (%s) — unchanged\n",
                                 file_id, path_cstr);
                }
                stats.diff_match++;
                return true;
            }
            std::fprintf(stderr,
                "regen: file_id %u (%s) — would overwrite an existing fixture with different content; "
                "pass --force to confirm.\n%s",
                file_id, path_cstr, snap.Diff(existing).c_str());
            stats.failed++;
            return false;
        }
    }

    if (!harness::WriteFixture(fixture_path, snap)) {
        std::fprintf(stderr, "regen: file_id %u (%s) — write failed\n",
                     file_id, path_cstr);
        stats.failed++;
        return false;
    }

    auto sz = std::filesystem::file_size(fixture_path);
    stats.total_bytes += sz;
    stats.written++;
    if (!args.quiet) {
        std::fprintf(stdout, "regen: %4u  %-50s  %8zu B\n",
                     file_id, path_cstr, static_cast<size_t>(sz));
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    Args args;
    if (!ParseArgs(argc, argv, args)) {
        PrintUsage();
        return 2;
    }

    if (args.all) {
        args.ids.clear();
        for (uint32_t i = 0; i < kRelocFileCount; ++i) {
            if (gRelocFileTable[i] != nullptr) args.ids.push_back(i);
        }
    } else {
        /* Dedup + drop duplicates. */
        std::unordered_set<uint32_t> seen;
        std::vector<uint32_t> uniq;
        for (uint32_t id : args.ids) {
            if (seen.insert(id).second) uniq.push_back(id);
        }
        args.ids = std::move(uniq);
    }

    std::fprintf(stderr, "regen: out_dir=%s ids=%zu mode=%s\n",
                 args.out_dir.string().c_str(), args.ids.size(),
                 args.check ? "check" : (args.force ? "write-force" : "write"));

    auto t0 = std::chrono::steady_clock::now();

    /* Pre-warm the archive session so the first per-file load doesn't have
     * to pay the archive open cost (about 2.5s on a fresh process). */
    harness::ArchiveSession::Get();

    Stats stats;
    harness::MockHeap heap;

    for (uint32_t file_id : args.ids) {
        ProcessOne(file_id, heap, args, stats);
    }

    auto t1 = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::fprintf(stderr,
        "regen: done in %lld ms — written=%u, unchanged=%u, missing=%u, mismatch=%u, "
        "skipped_empty=%u, failed=%u, total_bytes=%llu\n",
        static_cast<long long>(elapsed_ms),
        stats.written, stats.diff_match, stats.diff_missing, stats.diff_mismatch,
        stats.skipped_empty, stats.failed,
        static_cast<unsigned long long>(stats.total_bytes));

    /* TearDown ArchiveSession before main returns (mirrors test_main.cpp's
     * GoogleTest Environment teardown rationale). */
    harness::ArchiveSession::Shutdown();

    if (args.check) {
        return (stats.diff_mismatch > 0 || stats.diff_missing > 0) ? 1 : 0;
    }
    return (stats.failed > 0) ? 1 : 0;
}

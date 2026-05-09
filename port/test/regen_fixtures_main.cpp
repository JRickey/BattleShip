/**
 * regen_fixtures_main.cpp — CLI to regenerate the .fixt fixtures.
 *
 * Single process, single archive open. For each requested file_id:
 *   1. LoadAndProcess → Snapshot.
 *   2. WriteFixture to <out_dir>/<canonical-name>.fixt.
 *   3. (Optional --check) compare against existing fixture; report diffs
 *      without writing.
 *   4. (Optional --check-subres) validate that every entry in
 *      RelocFile::SubResourceHashes resolves to a loadable Blob whose
 *      bytes equal `Data[byte_offset..+size]` (the byte-identity invariant
 *      the runtime overlay path relies on for its short-circuit) and that
 *      no two sub-resources share a CRC64 hash. Stage 10 follow-up B.
 *
 * Usage:
 *   port_reloc_regen --all                       # write all 2,132 fixtures
 *   port_reloc_regen --ids 0,1,2,500             # write listed IDs only
 *   port_reloc_regen --all --check               # diff vs. committed, no writes
 *   port_reloc_regen --all --check-subres        # validate v3 sub-resources
 *   port_reloc_regen --all --check --check-subres   # both gates in one run
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

#include "resource/RelocFile.h"

#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/type/Blob.h>
#include <ship/utils/StrHash64.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
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
    bool                  check_subres = false;
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
        "  --check-subres      validate v3 sub-resource bytes + hash uniqueness\n"
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
        } else if (a == "--check-subres") {
            args.check_subres = true;
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
    /* --check-subres counters (Stage 10 follow-up B). */
    uint32_t subres_validated     = 0;  /* OK on every gate */
    uint32_t subres_blob_missing  = 0;  /* LoadResource(hash) returned null */
    uint32_t subres_blob_cast_failed = 0;  /* Loaded but not a Ship::Blob */
    uint32_t subres_oob           = 0;  /* (byte_offset, size) escapes Data */
    uint32_t subres_size_mismatch = 0;  /* blob size != recorded size */
    uint32_t subres_byte_mismatch = 0;  /* memcmp(blob, Data+off, size) != 0 */
    uint32_t subres_hash_mismatch = 0;  /* CRC64(rebuilt path) != recorded hash */
    uint32_t subres_collisions    = 0;  /* same hash claimed by ≥2 sub-resources */
};

/* Mirrors port/resource/RelocFile.h::SubResKind path-fragment names — kept
 * in step with torch's SubResourceOtrPath() so we can recompute the hash
 * deterministically from (entryOtrPath, kind, byte_offset) and verify it
 * equals the value torch wrote into the v3 container header. */
const char *SubResKindStr(SubResKind k)
{
    switch (k) {
    case SubResKind::Vtx:          return "vtx";
    case SubResKind::Tex:          return "tex";
    case SubResKind::Tlut:         return "tlut";
    case SubResKind::Sprite:       return "sprite";
    case SubResKind::Bitmap:       return "bitmap";
    case SubResKind::MObjSub:      return "mobjsub";
    case SubResKind::StructU16:    return "struct_u16";
    case SubResKind::StructU32:    return "struct_u32";
    case SubResKind::FTAttributes: return "ftattributes";
    }
    return "?";
}

/* LUS's BlobFactory pads each Blob's payload with 16 trailing zero bytes
 * (mirrors `kBlobTrailingPadding` in port/bridge/lbreloc_bridge.cpp). The
 * recorded sub-resource size excludes the padding; subtract here so the
 * size compare is apples-to-apples. */
constexpr size_t kBlobTrailingPadding = 16;

/* Validates a single RelocFile's SubResourceHashes:
 *   - Each entry's blob loads via the resource manager.
 *   - blob.Data has at least size + 16 bytes (recorded size excludes the
 *     LUS Blob trailing padding), and the first `size` bytes equal
 *     `relocFile.Data[byte_offset..+size]`.
 *   - (byte_offset, size) is in-bounds for relocFile.Data.
 *   - CRC64("<entryOtrPath>__sub/<kind>/<byte_offset>") == sr.Hash, so the
 *     path scheme torch uses still matches the recorded hash.
 *   - sr.Hash hasn't been claimed by another (file_id, sub-resource) pair
 *     across the entire archive. Tracked via `globalHashes` (file_id +
 *     sub-resource location of the first occurrence).
 *
 * `entryOtrPath` is the container's OTR path (e.g. "reloc_fighters_main/MarioMain")
 * — same string torch passed to SubResourceOtrPath at extraction. Read from
 * gRelocFileTable so we don't depend on Resource::GetInitData(). */
struct GlobalHashEntry {
    uint32_t file_id;
    uint32_t byte_offset;
    SubResKind kind;
};

void ValidateSubResources(uint32_t file_id,
                          const char *entryOtrPath,
                          const RelocFile &relocFile,
                          Ship::ResourceManager &resMgr,
                          std::unordered_map<uint64_t, GlobalHashEntry> &globalHashes,
                          Stats &stats,
                          bool quiet)
{
    /* Files extracted with no sub-resources (PROC_SUBRESOURCES_EMITTED unset)
     * are skipped silently — they pre-date Stage 10 or were extracted by an
     * older torch. The bit is informational; a false negative here would
     * matter only if a stale file mixed into a v3 archive. */
    if ((relocFile.ProcessingFlags & PROC_SUBRESOURCES_EMITTED) == 0) {
        return;
    }

    const std::string container = entryOtrPath ? entryOtrPath : "";

    for (const auto &sr : relocFile.SubResourceHashes) {
        bool ok = true;

        /* (1) In-bounds. */
        if (sr.Size == 0
            || sr.ByteOffset > relocFile.Data.size()
            || sr.Size > relocFile.Data.size() - sr.ByteOffset) {
            stats.subres_oob++;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource oob: kind=%s off=%u size=%u (file=%zu B)\n",
                    file_id, container.c_str(), SubResKindStr(sr.Kind),
                    sr.ByteOffset, sr.Size, relocFile.Data.size());
            }
            continue;
        }

        /* (2) Resolve via resource manager. */
        auto resource = resMgr.LoadResource(sr.Hash);
        if (!resource) {
            stats.subres_blob_missing++;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource missing: kind=%s off=%u hash=0x%016llx\n",
                    file_id, container.c_str(), SubResKindStr(sr.Kind), sr.ByteOffset,
                    static_cast<unsigned long long>(sr.Hash));
            }
            continue;
        }
        auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
        if (!blob) {
            stats.subres_blob_cast_failed++;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource not a Blob: kind=%s off=%u\n",
                    file_id, container.c_str(), SubResKindStr(sr.Kind), sr.ByteOffset);
            }
            continue;
        }

        /* (3) Logical size = padded - trailing padding. The factory may
         * round up further (e.g. for alignment); accept >= recorded size. */
        const size_t padded = blob->Data.size();
        const size_t logical = padded > kBlobTrailingPadding
                                   ? padded - kBlobTrailingPadding
                                   : 0;
        if (logical < sr.Size) {
            stats.subres_size_mismatch++;
            ok = false;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource short: kind=%s off=%u recorded=%u "
                    "blob_logical=%zu padded=%zu\n",
                    file_id, container.c_str(), SubResKindStr(sr.Kind), sr.ByteOffset,
                    sr.Size, logical, padded);
            }
        }

        /* (4) Byte-identity invariant: the runtime overlay path short-
         * circuits when blob.Data == container slice; if they ever drift,
         * "no override" mods would silently get treated as overrides. */
        if (logical >= sr.Size
            && std::memcmp(blob->Data.data(),
                           relocFile.Data.data() + sr.ByteOffset,
                           sr.Size) != 0) {
            stats.subres_byte_mismatch++;
            ok = false;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource bytes differ: kind=%s off=%u size=%u\n",
                    file_id, container.c_str(), SubResKindStr(sr.Kind),
                    sr.ByteOffset, sr.Size);
            }
        }

        /* (5) Recompute CRC64(path) and verify it matches the stored hash.
         * Catches torch path-scheme drift (e.g. delimiter change, kind
         * rename) that would silently break overrides while keeping the
         * drift gate green. */
        std::string path;
        path.reserve(container.size() + 32);
        path  = container;
        path += "__sub/";
        path += SubResKindStr(sr.Kind);
        path += '/';
        path += std::to_string(sr.ByteOffset);
        const uint64_t recomputed = CRC64(path.c_str());
        if (recomputed != sr.Hash) {
            stats.subres_hash_mismatch++;
            ok = false;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource hash mismatch: path=\"%s\" "
                    "recorded=0x%016llx recomputed=0x%016llx\n",
                    file_id, container.c_str(), path.c_str(),
                    static_cast<unsigned long long>(sr.Hash),
                    static_cast<unsigned long long>(recomputed));
            }
        }

        /* (6) Global uniqueness: a CRC64 collision across distinct sub-
         * resources is theoretically ~2⁻⁶⁴ per pair, but a misformatted
         * path bug (e.g. accidentally identical paths) would reproduce the
         * same hash deterministically. */
        auto [it, inserted] = globalHashes.try_emplace(
            sr.Hash, GlobalHashEntry{file_id, sr.ByteOffset, sr.Kind});
        if (!inserted) {
            stats.subres_collisions++;
            ok = false;
            if (!quiet) {
                std::fprintf(stderr,
                    "regen: file %u (%s) sub-resource hash 0x%016llx already claimed by "
                    "file %u kind=%s off=%u\n",
                    file_id, container.c_str(),
                    static_cast<unsigned long long>(sr.Hash),
                    it->second.file_id, SubResKindStr(it->second.kind),
                    it->second.byte_offset);
            }
        }

        if (ok) stats.subres_validated++;
    }
}

bool ProcessOne(uint32_t file_id, harness::MockHeap &heap,
                const Args &args, Stats &stats,
                Ship::ResourceManager *resMgr,
                std::unordered_map<uint64_t, GlobalHashEntry> *globalHashes)
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

    /* Sub-resource validation runs against the RelocFile (which carries
     * SubResourceHashes) loaded fresh from the archive — not the post-
     * runtime snap.data, whose chain slots have been overwritten with
     * heap tokens. The harness's ArchiveSession holds the open archive;
     * LoadFile() returns the cached resource without re-parsing. */
    if (args.check_subres && resMgr && globalHashes) {
        auto rf = harness::ArchiveSession::Get().LoadFile(file_id);
        if (rf) {
            ValidateSubResources(file_id, path_cstr, *rf, *resMgr,
                                 *globalHashes, stats, args.quiet);
        } else if (!args.quiet) {
            std::fprintf(stderr, "regen: file_id %u (%s) — LoadFile returned null, "
                                 "sub-resource validation skipped\n",
                         file_id, path_cstr);
        }
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

    /* --check-subres without --check: the file was loaded above; no
     * fixture work to do. Treat as success. */
    if (args.check_subres) {
        return true;
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

    /* Mode label combines the gates so the summary line tells you which
     * checks ran without re-deriving from the args list. --check-subres
     * standalone short-circuits before any fixture write, so the label
     * reads "subres" rather than "write+subres" in that case. */
    std::string mode;
    if (args.check)               mode = "check";
    else if (args.check_subres)   mode = "subres";
    else if (args.force)          mode = "write-force";
    else                          mode = "write";
    if (args.check && args.check_subres) mode = "check+subres";

    std::fprintf(stderr, "regen: out_dir=%s ids=%zu mode=%s\n",
                 args.out_dir.string().c_str(), args.ids.size(), mode.c_str());

    auto t0 = std::chrono::steady_clock::now();

    /* Pre-warm the archive session so the first per-file load doesn't have
     * to pay the archive open cost (about 2.5s on a fresh process). */
    harness::ArchiveSession::Get();

    /* Resolve the resource manager once. Sub-resource validation reuses
     * the same archive the harness opened — Ship::Context::GetInstance()
     * returns the singleton ArchiveSession set up. */
    Ship::ResourceManager *resMgr = nullptr;
    if (args.check_subres) {
        if (auto ctx = Ship::Context::GetInstance()) {
            if (auto rm = ctx->GetResourceManager()) {
                resMgr = rm.get();
            }
        }
        if (!resMgr) {
            std::fprintf(stderr, "regen: --check-subres requested but ResourceManager "
                                 "is not available; aborting\n");
            harness::ArchiveSession::Shutdown();
            return 2;
        }
    }

    Stats stats;
    harness::MockHeap heap;
    std::unordered_map<uint64_t, GlobalHashEntry> globalHashes;

    for (uint32_t file_id : args.ids) {
        ProcessOne(file_id, heap, args, stats, resMgr, &globalHashes);
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

    if (args.check_subres) {
        const uint32_t subres_failed = stats.subres_blob_missing + stats.subres_blob_cast_failed
            + stats.subres_oob + stats.subres_size_mismatch + stats.subres_byte_mismatch
            + stats.subres_hash_mismatch + stats.subres_collisions;
        std::fprintf(stderr,
            "regen: subres — validated=%u, failed=%u (missing=%u cast_failed=%u oob=%u "
            "size_mismatch=%u byte_mismatch=%u hash_mismatch=%u collisions=%u), "
            "unique_hashes=%zu\n",
            stats.subres_validated, subres_failed,
            stats.subres_blob_missing, stats.subres_blob_cast_failed, stats.subres_oob,
            stats.subres_size_mismatch, stats.subres_byte_mismatch,
            stats.subres_hash_mismatch, stats.subres_collisions,
            globalHashes.size());
    }

    /* TearDown ArchiveSession before main returns (mirrors test_main.cpp's
     * GoogleTest Environment teardown rationale). */
    harness::ArchiveSession::Shutdown();

    int exit_code = 0;
    if (args.check && (stats.diff_mismatch > 0 || stats.diff_missing > 0)) {
        exit_code = 1;
    }
    if (args.check_subres
        && (stats.subres_blob_missing + stats.subres_blob_cast_failed + stats.subres_oob
            + stats.subres_size_mismatch + stats.subres_byte_mismatch
            + stats.subres_hash_mismatch + stats.subres_collisions) > 0) {
        exit_code = 1;
    }
    if (!args.check && !args.check_subres && stats.failed > 0) {
        exit_code = 1;
    }
    return exit_code;
}

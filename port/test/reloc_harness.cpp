#include "reloc_harness.h"

#include "resource/RelocFile.h"
#include "resource/RelocFileFactory.h"
#include "resource/RelocFileTable.h"
#include "resource/RelocPointerTable.h"
#include "resource/ResourceType.h"

#include "bridge/lbreloc_bridge_testing.h"
#include "bridge/lbreloc_byteswap.h"

#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/ResourceLoader.h>
#include <ship/resource/Resource.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <unordered_set>
#include <vector>

extern "C" {
/* Bridge-private hooks the harness needs. */
void portRelocResetPointerTableForTest(void);
void portResetStructFixups(void);
void port_aobj_event32_unhalfswap_reset(void);
void lbRelocLoadAndRelocFile(uint32_t file_id, void *ram_dst, uint32_t bytes_num, int32_t loc);

/* Forward decl: bridge defines this. */
int portRelocFindFileIdAndBase(const void *ptr, uintptr_t *out_base);
}

namespace harness {

namespace {

constexpr int32_t kBridgeLocDefault = 1;  /* nLBFileLocationDefault */

std::filesystem::path FindO2RArchive()
{
    if (const char *env = std::getenv("SSB64_TEST_O2R_PATH"); env != nullptr && *env != '\0') {
        std::filesystem::path p(env);
        if (std::filesystem::exists(p)) return p;
    }
    /* Fall back: walk up from cwd looking for build/BattleShip.o2r. The
     * default ctest WORKING_DIRECTORY is CMAKE_BINARY_DIR which has the
     * archive; this is a belt-and-suspenders for ad-hoc invocations. */
    std::filesystem::path cwd = std::filesystem::current_path();
    for (int i = 0; i < 5; ++i) {
        std::filesystem::path candidate = cwd / "BattleShip.o2r";
        if (std::filesystem::exists(candidate)) return candidate;
        candidate = cwd / "build" / "BattleShip.o2r";
        if (std::filesystem::exists(candidate)) return candidate;
        cwd = cwd.parent_path();
    }
    throw std::runtime_error("BattleShip.o2r not found — set SSB64_TEST_O2R_PATH or run from build dir");
}

/* Per-load capture state. */
struct LoadCapture {
    uint32_t loading_file_id;
    uintptr_t loading_base;
    size_t    loading_size;
    std::vector<ChainTarget> chains;
};

LoadCapture *gActiveCapture = nullptr;

extern "C" void ChainObserverCb(uint32_t file_id, uint32_t slot_offset,
                                int32_t dep_file_id, uint32_t target_offset)
{
    if (gActiveCapture == nullptr) return;
    if (file_id != gActiveCapture->loading_file_id) {
        /* Recursive dep load — bridge fires the observer for the dep too,
         * but we only snapshot the requested file. */
        return;
    }
    gActiveCapture->chains.push_back({slot_offset, dep_file_id, target_offset});
}

struct TrackerCollect {
    uintptr_t base;
    size_t size;
    std::vector<TrackerEntry> entries;
};

extern "C" void TrackerCollectCb(uint8_t family, uint64_t addr, void *user)
{
    auto *tc = static_cast<TrackerCollect *>(user);
    if (addr < tc->base) return;
    uint64_t off = addr - tc->base;
    if (off >= tc->size) return;
    tc->entries.push_back({static_cast<uint32_t>(off), family});
}

} // namespace

// ============================================================
//   ArchiveSession
// ============================================================

struct ArchiveSession::Impl {
    std::shared_ptr<Ship::Context> ctx;
};

/* Hold the singleton in a heap-allocated unique_ptr so we can release it
 * deterministically via Shutdown() before main returns — sidesteps the
 * function-local-static destruction-order race against spdlog. */
namespace {
std::unique_ptr<ArchiveSession> &SessionSlot()
{
    static std::unique_ptr<ArchiveSession> slot;
    return slot;
}
} // namespace

ArchiveSession &ArchiveSession::Get()
{
    auto &slot = SessionSlot();
    if (!slot) {
        slot.reset(new ArchiveSession());
    }
    return *slot;
}

void ArchiveSession::Shutdown()
{
    SessionSlot().reset();
}

ArchiveSession::~ArchiveSession() = default;

ArchiveSession::ArchiveSession() : mImpl(std::make_unique<Impl>())
{
    /* Silence spdlog so error logs from negative tests don't pollute stdout. */
    auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
    auto null_logger = std::make_shared<spdlog::logger>("null", sink);
    spdlog::set_default_logger(null_logger);

    auto archivePath = FindO2RArchive();

    /* O2rArchive::LoadFile(hash) calls Ship::Context::GetInstance()->...,
     * so the bare ResourceManager path is not viable. We construct a minimal
     * Ship::Context with only Configuration + ConsoleVariables + Resource
     * Manager initialized — Window/Audio/ControlDeck stay null.
     *
     * The config filename gets wrapped by Ship::Context::GetPathRelativeToAppDirectory
     * inside InitConfiguration; the resulting "<app_dir>/port_reloc_tests.cfg.json"
     * lands in the build directory (cwd) when the test runs, which is gitignored. */
    mImpl->ctx = Ship::Context::CreateUninitializedInstance(
        "ssb64-port-reloc-tests", "ssb64-port-reloc-tests",
        /*configFilePath=*/"port_reloc_tests.cfg.json");
    if (!mImpl->ctx->InitConfiguration()) {
        throw std::runtime_error("InitConfiguration failed in test harness");
    }
    if (!mImpl->ctx->InitConsoleVariables()) {
        throw std::runtime_error("InitConsoleVariables failed in test harness");
    }
    if (!mImpl->ctx->InitResourceManager({archivePath.string()}, {}, /*reservedThreadCount=*/1)) {
        throw std::runtime_error("InitResourceManager failed in test harness");
    }

    auto loader = mImpl->ctx->GetResourceManager()->GetResourceLoader();
    loader->RegisterResourceFactory(
        std::make_shared<ResourceFactoryBinaryRelocFileV0>(),
        RESOURCE_FORMAT_BINARY,
        "SSB64Reloc",
        static_cast<uint32_t>(SSB64::ResourceType::SSB64Reloc),
        0);
    loader->RegisterResourceFactory(
        std::make_shared<ResourceFactoryBinaryRelocFileV1>(),
        RESOURCE_FORMAT_BINARY,
        "SSB64Reloc",
        static_cast<uint32_t>(SSB64::ResourceType::SSB64Reloc),
        1);
    loader->RegisterResourceFactory(
        std::make_shared<ResourceFactoryBinaryRelocFileV2>(),
        RESOURCE_FORMAT_BINARY,
        "SSB64Reloc",
        static_cast<uint32_t>(SSB64::ResourceType::SSB64Reloc),
        2);
    loader->RegisterResourceFactory(
        std::make_shared<ResourceFactoryBinaryRelocFileV3>(),
        RESOURCE_FORMAT_BINARY,
        "SSB64Reloc",
        static_cast<uint32_t>(SSB64::ResourceType::SSB64Reloc),
        3);
}

std::shared_ptr<RelocFile> ArchiveSession::LoadFile(uint32_t file_id)
{
    if (file_id >= RELOC_FILE_COUNT || gRelocFileTable[file_id] == nullptr) {
        return nullptr;
    }
    auto res = mImpl->ctx->GetResourceManager()->LoadResource(gRelocFileTable[file_id]);
    if (!res) return nullptr;
    return std::dynamic_pointer_cast<RelocFile>(res);
}

// ============================================================
//   Bridge state lifecycle
// ============================================================

/* Per-process heap buffers for the bridge's recursive dependency loads.
 * Two distinct buffers — the bridge uses different bump pointers and the
 * intern path enforces an end-of-heap check. Sized for an entire test
 * session; the OS demand-pages so RSS stays modest. */
namespace {
std::vector<uint8_t> &InternHeapBuffer()
{
    static std::vector<uint8_t> buf(256ull * 1024 * 1024, 0);
    return buf;
}
std::vector<uint8_t> &ExternHeapBuffer()
{
    static std::vector<uint8_t> buf(256ull * 1024 * 1024, 0);
    return buf;
}
} // namespace

void ResetBridgeGlobals()
{
    portRelocResetPointerTableForTest();
    portResetStructFixups();
    port_aobj_event32_unhalfswap_reset();
    portRelocBridgeForgetFileRanges();
    /* Re-arm the bridge's status buffers + heap pointers for a fresh test. */
    auto &intern = InternHeapBuffer();
    auto &extern_buf = ExternHeapBuffer();
    portRelocBridgeSetupForTest(intern.data(), intern.size(),
                                extern_buf.data(), extern_buf.size(),
                                /*status_buffer_count=*/256,
                                /*force_status_buffer_count=*/64);
}

// ============================================================
//   LoadAndProcess
// ============================================================

/* port_aobj_is_in_halfswapped_range is the only port_aobj_fixup hook the
 * harness consumes (used to capture whether the bridge registered the loaded
 * file as a halfswap range). */
extern "C" int port_aobj_is_in_halfswapped_range(const void *p);

Snapshot LoadAndProcess(uint32_t file_id, MockHeap &heap)
{
    Snapshot snap;
    if (file_id >= RELOC_FILE_COUNT || gRelocFileTable[file_id] == nullptr) {
        return snap;
    }
    snap.path = gRelocFileTable[file_id];
    snap.file_id = file_id;

    /* Reset bridge state so this load starts clean. */
    ResetBridgeGlobals();
    heap.Reset();

    /* Install the test-only resource loader. */
    portRelocSetResourceLoader(
        +[](uint32_t fid) -> std::shared_ptr<RelocFile> {
            return ArchiveSession::Get().LoadFile(fid);
        });

    /* Probe the file's size by issuing a resource load. The harness needs
     * the size to allocate a MockHeap slot before lbRelocLoadAndRelocFile
     * is called (it expects a pre-allocated ram_dst). */
    size_t alloc_size = 0;
    {
        auto rf = ArchiveSession::Get().LoadFile(file_id);
        if (!rf) {
            portRelocClearResourceLoader();
            return snap;
        }
        alloc_size = rf->Data.size();
    }
    if (alloc_size == 0) {
        portRelocClearResourceLoader();
        return snap;
    }

    void *ram_dst = heap.Alloc(file_id, alloc_size);

    /* Install observers, do the load. */
    LoadCapture cap{file_id, reinterpret_cast<uintptr_t>(ram_dst), alloc_size, {}};
    gActiveCapture = &cap;
    portRelocSetChainObserver(&ChainObserverCb);

    lbRelocLoadAndRelocFile(file_id, ram_dst, static_cast<uint32_t>(alloc_size), kBridgeLocDefault);

    portRelocClearChainObserver();
    gActiveCapture = nullptr;
    portRelocClearResourceLoader();

    /* Capture post-fixup bytes. */
    snap.data.resize(alloc_size);
    std::memcpy(snap.data.data(), ram_dst, alloc_size);
    snap.chain_targets = std::move(cap.chains);

    /* Capture tracker entries inside this file's range. */
    TrackerCollect tc{cap.loading_base, cap.loading_size, {}};
    portRelocForEachTrackerEntry(&TrackerCollectCb, &tc);
    snap.trackers = std::move(tc.entries);

    /* Capture halfswap range. The bridge registers fighter-figatree files;
     * the byteswap layer doesn't expose the registered range list directly,
     * so we probe with port_aobj_is_in_halfswapped_range at the buffer base.
     * Since each load registers at most one range covering the whole file,
     * this is sufficient. */
    if (port_aobj_is_in_halfswapped_range(ram_dst)) {
        snap.halfswap_ranges.push_back({0u, static_cast<uint32_t>(alloc_size)});
    }

    snap.Canonicalize();
    return snap;
}

} // namespace harness

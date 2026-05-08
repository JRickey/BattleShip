#pragma once

/**
 * lbreloc_bridge_testing.h — Test-only injection points for lbreloc_bridge.
 *
 * The bridge's production resource lookup goes through
 * Ship::Context::GetInstance()->GetResourceManager()->LoadResource(). For unit
 * tests we want determinism without setting up a full Ship::Context, and we
 * also want a per-test snapshot of the bridge's mutable state (chain target
 * lists, the file-range registry).
 *
 * This header exposes:
 *   - PortRelocResourceLoader: the production code path is ALWAYS used unless
 *     a test installs a custom loader via portRelocSetResourceLoader. Tests
 *     normally point this at a wrapper around a single open Ship::Archive so
 *     no Ship::Context exists.
 *   - PortRelocChainObserver: the bridge calls this on every internal/external
 *     chain target it registers, exposing slot_offset / target_offset (file-
 *     relative) plus the dependency file_id for external entries. Tests use
 *     it to capture chain target lists for snapshot equality.
 *   - portRelocBridgeReset / portRelocBridgeForgetFileRanges: hooks for tests
 *     to drop the bridge's internal sPortRelocFileRanges between iterations.
 *
 * No production code calls into this header. Including it from the game build
 * is harmless (the symbols compile in always) but the test-only setters/lambdas
 * are zeroed at process start and stay there.
 */

#include <cstdint>
#include <memory>

class RelocFile;

extern "C" {

/**
 * Resource-loader hook. When non-null, lbreloc_bridge uses it instead of the
 * Ship::Context::GetInstance() resource manager lookup. Production never
 * installs one; tests do.
 *
 * Implementation must be thread-safe enough for the test driver (the bridge
 * itself is single-threaded today). Returns nullptr on miss; bridge logs and
 * fails the load.
 */
struct PortRelocResourceLoader; // opaque

void portRelocSetResourceLoader(std::shared_ptr<RelocFile> (*loader)(uint32_t file_id));
void portRelocClearResourceLoader(void);

/**
 * Chain-walk observer. Invoked once per registered chain target (internal +
 * external). file_id is the file the load is being performed FOR; slot_offset
 * and target_offset are byte offsets within that file's blob (i.e., the same
 * coordinate system as Snapshot::data). For external chains, dep_file_id
 * names the dependency file; target_offset is into the dependency, not the
 * loading file.
 *
 * Observer is invoked while the bridge is mid-walk; it MUST NOT call back into
 * lbReloc*. Tests typically just push to a vector.
 */
typedef void (*PortRelocChainObserver)(
    uint32_t file_id,
    uint32_t slot_offset,
    int32_t  dep_file_id,    /* -1 for internal targets */
    uint32_t target_offset);

void portRelocSetChainObserver(PortRelocChainObserver obs);
void portRelocClearChainObserver(void);

/**
 * Drop the bridge's internal sPortRelocFileRanges registry. Tests call this
 * between iterations so a fresh load doesn't accumulate ranges from prior
 * tests, which would distort portRelocFindContainingFile lookups invoked by
 * the byteswap layer. Production never calls this; lbRelocInitSetup clears
 * the registry as a normal scene-change effect.
 */
void portRelocBridgeForgetFileRanges(void);

/**
 * Initialize bridge-internal status buffers and the heap pointers suitable
 * for a test load. Call once per test before lbRelocLoadAndRelocFile.
 *
 * `intern_heap_base` / `intern_heap_size` back lbRelocGetInternBufferFile —
 * the path used when the load location is nLBFileLocationDefault (the
 * harness default). Bounded — the bridge bails to syDebugPrintf if exceeded.
 *
 * `extern_heap_base` / `extern_heap_size` back lbRelocGetExternBufferFile —
 * the path used for nLBFileLocationExtern. Unbounded inside the bridge but
 * tests should still size it to fit their dependency tree.
 *
 * Both heaps must be DISTINCT buffers — they use different bump pointers
 * with no shared bookkeeping.
 *
 * Status buffer sizes are passed by COUNT (number of LBFileNode entries),
 * not bytes. The bridge allocates the LBFileNode arrays itself.
 *
 * Replaces lbRelocInitSetup for tests so we don't pull in game-side hooks
 * (gmColScriptsLinkRelocTargets etc.) the harness doesn't need.
 */
void portRelocBridgeSetupForTest(
    void *intern_heap_base, size_t intern_heap_size,
    void *extern_heap_base, size_t extern_heap_size,
    size_t status_buffer_count,
    size_t force_status_buffer_count);

/**
 * Walk every idempotency-tracker entry currently held by lbreloc_byteswap.
 * Invoked AFTER a load to capture tracker state into the snapshot, without
 * exposing the byteswap layer's container types. Families:
 *   family=0  → sStructU16Fixups (portFixupStructU16/U32, sprite/bitmap, etc.)
 *   family=1  → sTexFixupWords   (per-word texture/TLUT fixup entries)
 *   family=2  → sDeswizzle4cFixups (post-decode 4c sprite deswizzle)
 *
 * `addr` is the absolute uintptr_t key. Tests filter by file range and convert
 * to file-relative offsets before recording in the snapshot.
 */
void portRelocForEachTrackerEntry(
    void (*cb)(uint8_t family, uint64_t addr, void *user), void *user);

} // extern "C"

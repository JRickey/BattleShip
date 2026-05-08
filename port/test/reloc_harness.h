#pragma once

/**
 * reloc_harness — Drives the runtime reloc pipeline against MockHeap and
 * captures a Snapshot for comparison.
 *
 * Architecture:
 *
 *   1. ArchiveSession (process-wide singleton): opens BattleShip.o2r once
 *      via Ship::ResourceManager (NOT Ship::Context — we don't need
 *      Window/Audio/etc.) and registers the RelocFile factory. Reused
 *      across every TEST so we don't pay the archive-open cost 2,132 times.
 *
 *   2. LoadAndProcess(file_id, heap):
 *        - Resets bridge globals (token table, struct fixups, port_aobj).
 *        - Installs the resource-loader override to delegate to
 *          ArchiveSession's ResourceManager. Drops the override on return.
 *        - Installs a chain observer that records (file_id, slot, dep,
 *          target) tuples for the loading file (and only the loading file —
 *          recursive dependency loads do NOT contribute to this snapshot).
 *        - Asks MockHeap for a buffer sized to the file's data, calls
 *          lbRelocLoadAndRelocFile, and captures:
 *            * the post-fixup bytes of THAT buffer
 *            * the chain target list collected by the observer
 *            * tracker entries (struct-fixup / tex-word / 4c-deswizzle)
 *              whose absolute address falls inside that buffer
 *            * the halfswap range registered for that buffer (if any)
 *
 *   3. Returns a canonicalized Snapshot (sorted lists, deterministic).
 *
 * The harness is single-threaded by design — the production bridge is too,
 * and the static observer hooks are global state.
 */

#include "mock_heap.h"
#include "snapshot.h"

#include <cstdint>
#include <memory>

class RelocFile;  /* global namespace, defined in port/resource/RelocFile.h */

namespace harness {

/** Process-wide handle to the open BattleShip.o2r archive + registered
 *  RelocFile factory. Constructed on first use; tests don't need to touch
 *  this directly — LoadAndProcess uses it internally.
 *
 *  Lifecycle: Get() lazily creates the instance. Shutdown() is called by the
 *  GoogleTest environment before process exit so Ship::Context tears down
 *  while spdlog is still alive (mirroring port.cpp's PortShutdown rationale).
 */
class ArchiveSession {
public:
    ArchiveSession();
    ~ArchiveSession();
    ArchiveSession(const ArchiveSession &) = delete;
    ArchiveSession &operator=(const ArchiveSession &) = delete;

    static ArchiveSession &Get();
    static void Shutdown();   /* Drops the singleton. Safe to call multiple times. */

    /** Internal helper used by LoadAndProcess via the bridge's loader hook.
     *  Tests should not call this directly. */
    std::shared_ptr<::RelocFile> LoadFile(uint32_t file_id);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

/**
 * Drive a single load of `file_id` through the bridge against `heap` and
 * return the resulting snapshot. On failure (resource missing, factory
 * mismatch, internal assertion) returns a snapshot with file_id == 0 and
 * empty data — tests should EXPECT_GT(snap.data.size(), 0u) as a sanity
 * gate.
 */
Snapshot LoadAndProcess(uint32_t file_id, MockHeap &heap);

/**
 * Reset all bridge-global state and re-arm the bridge's status buffers +
 * extern heap. Called by LoadAndProcess at entry; exposed so tests can
 * reset between explicit operations (e.g., loading the same file three
 * times for a determinism check).
 */
void ResetBridgeGlobals();

} // namespace harness

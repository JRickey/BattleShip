#pragma once

/**
 * MockHeap — deterministic bump allocator for the reloc-pipeline test harness.
 *
 * The bridge takes ram_dst as a parameter, so heap allocation lives in the
 * caller. The harness uses MockHeap to back every per-file allocation needed
 * during a single LoadAndProcess call (the requested file plus any external
 * dependencies the bridge recursively loads).
 *
 * Determinism guarantees:
 *   - Within one MockHeap instance, file_id N is always handed back the same
 *     base address. Re-loading N is a no-op alloc (no double-mapping).
 *   - Allocation order is recorded so Snapshot can convert tracker keys
 *     (which the byteswap layer stores as raw uintptr_t) back to file-
 *     relative offsets per file.
 *   - The backing buffer is a single std::vector<uint8_t> sized to
 *     kPerFileStride * kMaxFiles. Stride is wide enough that the largest
 *     reloc file plus a 16-byte alignment gap fits comfortably; observed
 *     max is ~700 KiB so 1 MiB is generous.
 *
 * Tests typically construct one MockHeap per TEST and tear it down at exit;
 * a fresh instance for every test is the simplest way to guarantee
 * cross-test isolation. (The token table + struct-fixup trackers in
 * lbreloc_byteswap are reset separately by the harness.)
 */

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace harness {

class MockHeap {
public:
    static constexpr size_t kPerFileStride = 0x100000;     /* 1 MiB per slot */
    static constexpr size_t kMaxFiles      = 64;           /* one test loads ≤ ~30 deps in practice */

    MockHeap();

    /** Get (allocating if needed) a buffer for `file_id` of `size` bytes.
     *  Idempotent — same file_id returns the same address regardless of
     *  whether the size matches. Asserts if size > kPerFileStride. */
    void *Alloc(uint32_t file_id, size_t size);

    /** Address handed back by Alloc(file_id, ...). Returns 0 if file_id has
     *  not been allocated yet. */
    uintptr_t AddressFor(uint32_t file_id) const;

    /** Number of bytes Alloc was called with for this file. Returns 0 if
     *  not allocated. */
    size_t SizeFor(uint32_t file_id) const;

    /** All currently allocated file_ids — order preserved by allocation. */
    const std::vector<uint32_t> &AllocatedFileIds() const { return mAllocOrder; }

    /** Look up which file_id owns an arbitrary uintptr_t (e.g. a tracker key
     *  inserted by the byteswap layer). Returns -1 if outside any allocated
     *  region. */
    int32_t FileIdForAddress(uintptr_t addr) const;

    /** Reset all allocation state. Backing buffer is retained but considered
     *  uninitialized; Alloc-then-Reset-then-Alloc may give different bytes. */
    void Reset();

    /** Backing buffer base — useful for asserting all allocations land inside
     *  the same VA range. Tests shouldn't index into this directly. */
    uint8_t *BufferBase() { return mBuffer.data(); }
    size_t   BufferSize() const { return mBuffer.size(); }

private:
    struct Slot { uintptr_t base; size_t size; size_t slot_index; };
    std::vector<uint8_t> mBuffer;
    std::unordered_map<uint32_t, Slot> mSlots;
    std::vector<uint32_t> mAllocOrder;
    size_t mNextSlot = 0;
};

} // namespace harness

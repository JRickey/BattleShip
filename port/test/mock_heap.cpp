#include "mock_heap.h"

#include <cassert>
#include <cstring>

namespace harness {

MockHeap::MockHeap() : mBuffer(kPerFileStride * kMaxFiles, 0) {}

void *MockHeap::Alloc(uint32_t file_id, size_t size)
{
    assert(size <= kPerFileStride && "single reloc file exceeds MockHeap stride");
    auto it = mSlots.find(file_id);
    if (it != mSlots.end()) {
        return reinterpret_cast<void *>(it->second.base);
    }
    assert(mNextSlot < kMaxFiles && "MockHeap slot exhaustion — bump kMaxFiles");
    size_t slot_index = mNextSlot++;
    uintptr_t base = reinterpret_cast<uintptr_t>(mBuffer.data()) + slot_index * kPerFileStride;
    /* Zero the slot so a re-allocation after Reset() doesn't expose stale
     * bytes from a prior load. */
    std::memset(reinterpret_cast<void *>(base), 0, kPerFileStride);
    mSlots.emplace(file_id, Slot{base, size, slot_index});
    mAllocOrder.push_back(file_id);
    return reinterpret_cast<void *>(base);
}

uintptr_t MockHeap::AddressFor(uint32_t file_id) const
{
    auto it = mSlots.find(file_id);
    return (it == mSlots.end()) ? 0 : it->second.base;
}

size_t MockHeap::SizeFor(uint32_t file_id) const
{
    auto it = mSlots.find(file_id);
    return (it == mSlots.end()) ? 0 : it->second.size;
}

int32_t MockHeap::FileIdForAddress(uintptr_t addr) const
{
    for (const auto &kv : mSlots) {
        const Slot &s = kv.second;
        if (addr >= s.base && addr < s.base + kPerFileStride) {
            return static_cast<int32_t>(kv.first);
        }
    }
    return -1;
}

void MockHeap::Reset()
{
    mSlots.clear();
    mAllocOrder.clear();
    mNextSlot = 0;
    /* Buffer bytes left in place — Alloc rezeros each slot when reused. */
}

} // namespace harness

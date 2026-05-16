/**
 * port_dl_ranges.cpp — registry of valid DL memory ranges for the GFX
 * walker's bounds check. See port_dl_ranges.h for the rationale.
 *
 * Wired into libultraship at startup via port_dl_ranges_init(), which
 * registers our port-side bounds-check and address classifier as the
 * callbacks libultraship's Fast::Interpreter uses to validate DL
 * pointers and label diagnostic dumps. libultraship has no compile-time
 * dependency on this file — both callbacks are optional.
 */

#include "port_dl_ranges.h"
#include "fast/interpreter.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace {

struct Entry {
    uintptr_t base;
    size_t size;
    const char *label;
};

/* Small registry (< 100 ranges typical). Linear scan is fine; mutex is
 * cheap. If profiling shows contention we can switch to a sorted vector
 * with binary search or a copy-on-write atomic snapshot. */
std::vector<Entry> sRanges;
std::mutex sRangesMtx;

/* "Walked past" threshold: how far past a registered range do we still
 * recognise as runaway-from-that-range? One 64 KiB window comfortably
 * covers the gap to the next mmap'd allocation while staying tight
 * enough to fire on the actual walk-off cases. */
constexpr size_t kWalkPastWindow = 0x10000;

} /* namespace */

extern "C" void port_dl_range_register(const void *base, size_t size, const char *label) {
    if ((base == nullptr) || (size == 0)) {
        return;
    }
    uintptr_t b = reinterpret_cast<uintptr_t>(base);
    std::lock_guard<std::mutex> lk(sRangesMtx);
    for (auto &e : sRanges) {
        if (e.base == b) {
            e.size = size;
            e.label = label;
            return;
        }
    }
    sRanges.push_back({b, size, label});
}

extern "C" void port_dl_range_unregister(const void *base) {
    if (base == nullptr) return;
    uintptr_t b = reinterpret_cast<uintptr_t>(base);
    std::lock_guard<std::mutex> lk(sRangesMtx);
    for (auto it = sRanges.begin(); it != sRanges.end(); ++it) {
        if (it->base == b) {
            sRanges.erase(it);
            return;
        }
    }
}

extern "C" int port_dl_check_addr(uintptr_t addr) {
    if (addr == 0) return PORT_DL_UNKNOWN;
    std::lock_guard<std::mutex> lk(sRangesMtx);
    bool walked_past = false;
    for (const auto &e : sRanges) {
        if ((addr >= e.base) && ((addr - e.base) < e.size)) {
            return PORT_DL_IN_RANGE;
        }
        if ((addr >= e.base + e.size) && ((addr - (e.base + e.size)) < kWalkPastWindow)) {
            walked_past = true;
            /* Don't return immediately — another registered range further
             * along could legitimately contain addr (overlapping ranges
             * are allowed at registration). Only commit to WALKED_PAST
             * after confirming addr isn't in any other range. */
        }
    }
    return walked_past ? PORT_DL_WALKED_PAST : PORT_DL_UNKNOWN;
}

extern "C" int port_dl_range_classify_str(uintptr_t addr, char *buf, size_t buf_size) {
    if (buf == nullptr || buf_size == 0) return 0;
    if (addr == 0) {
        std::snprintf(buf, buf_size, "null");
        return 0;
    }
    std::lock_guard<std::mutex> lk(sRangesMtx);
    for (const auto &e : sRanges) {
        if ((addr >= e.base) && ((addr - e.base) < e.size)) {
            std::snprintf(buf, buf_size, "%s+0x%lx",
                          e.label ? e.label : "?",
                          static_cast<unsigned long>(addr - e.base));
            return 1;
        }
    }
    /* Fall through to a heuristic label so the diag still tells us
     * something useful for unregistered ranges. */
    if (addr <= 0x0FFFFFFFu) {
        std::snprintf(buf, buf_size, "n64_seg[%u]+0x%x",
                      static_cast<unsigned>((addr >> 24) & 0xFF),
                      static_cast<unsigned>(addr & 0x00FFFFFFu));
    } else if (addr < 0x100000000ull) {
        std::snprintf(buf, buf_size, "low_brk@0x%lx", static_cast<unsigned long>(addr));
    } else {
        std::snprintf(buf, buf_size, "other@0x%lx", static_cast<unsigned long>(addr));
    }
    return 0;
}

/* Called once at port startup.
 *
 * Historical note: older libultraship revisions exposed
 * Fast::RegisterDLBoundsCheck / Fast::RegisterAddressClassifier hooks.
 * The current API no longer exposes those registration points, so this
 * remains as a compatibility no-op until a replacement hook is added. */
extern "C" void port_dl_ranges_init(void) {
    /* No-op on current libultraship API. */
}

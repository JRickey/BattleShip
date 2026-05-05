#pragma once

#ifdef __SWITCH__

#include <cstdint>

namespace Ship {

enum SwitchCpuClock : uint32_t {
    Level1 = 1020000000,   // 1020 MHz — stock handheld/docked
    Level2 = 1581000000,   // 1581 MHz — safe overclock (cooling permitting)
    Level3 = 1785000000,   // 1785 MHz — max official boost (docked only)
};

// Convenience aliases
constexpr int SWITCH_CPU_STOCK = 1;
constexpr int SWITCH_CPU_SAFE  = 2;
constexpr int SWITCH_CPU_MAX   = 3;

} // namespace Ship

#endif // __SWITCH__

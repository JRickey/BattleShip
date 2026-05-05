#pragma once

#ifdef __SWITCH__

#include <string>
#include <cstdint>

namespace Ship {

class Switch {
  public:
    enum class OperationMode { Handheld, Docked, Unknown };

    /** Initialize Switch subsystems (ROMFS, applet, etc.). */
    static void Init();

    /** Clean up Switch subsystems. */
    static void Exit();

    /** Get the current display mode (handheld 1280x720 or docked 1920x1080). */
    static OperationMode GetOperationMode();

    /** Get the display size in pixels for the current mode. */
    static void GetDisplaySize(int* width, int* height);

    /** Load Nintendo system fonts (Standard + Extended) via pl service. */
    static void LoadSystemFonts();

    /** Open the on-screen virtual keyboard (Swkbd). Returns the entered text. */
    static std::string OpenVirtualKeyboard(const char* initialText = "");

    /** Set CPU boost level (1 = stock 1020MHz, 2 = 1785MHz max). */
    static void SetCpuBoost(int level);
};

} // namespace Ship

#endif // __SWITCH__

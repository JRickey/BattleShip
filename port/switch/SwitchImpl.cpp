/**
 * SwitchImpl.cpp — Nintendo Switch platform implementation.
 *
 * Uses libnx and standard Switch homebrew APIs. All subsystems accessed
 * through the devkitPro/libnx SDK headers (automatically available when
 * building with CMAKE_SYSTEM_NAME=NintendoSwitch).
 */
#ifdef __SWITCH__

#include "SwitchImpl.h"
#include "SwitchPerformanceProfiles.h"

#include <switch.h>
#include <SDL2/SDL.h>
#include <cstdio>
#include <unistd.h>

/* ImGui's Platform_OpenInShellFn_DefaultImpl uses execvp/waitpid,
 * which are not available on Switch. Provide no-op stubs. */
extern "C" int execvp(const char*, char* const*) { return -1; }
extern "C" int waitpid(int, int*, int)           { return -1; }

namespace Ship {

/* ========================================================================= */
/*  Init / Exit                                                              */
/* ========================================================================= */

void Switch::Init() {
    romfsInit();
    plInitialize(PlServiceType_User);
    appletInitialize();

    // Request max safe clock rate at boot for consistent 60fps.
    SetCpuBoost(SWITCH_CPU_MAX);
}

void Switch::Exit() {
    SetCpuBoost(SWITCH_CPU_STOCK);
    plExit();
    romfsExit();
}

/* ========================================================================= */
/*  Display                                                                  */
/* ========================================================================= */

Switch::OperationMode Switch::GetOperationMode() {
    switch (appletGetOperationMode()) {
        case AppletOperationMode_Handheld:
            return OperationMode::Handheld;
        case AppletOperationMode_Console:
            return OperationMode::Docked;
        default:
            return OperationMode::Unknown;
    }
}

void Switch::GetDisplaySize(int* width, int* height) {
    OperationMode mode = GetOperationMode();
    if (mode == OperationMode::Docked) {
        *width  = 1920;
        *height = 1080;
    } else {
        *width  = 1280;
        *height = 720;
    }
}

/* ========================================================================= */
/*  System Fonts                                                             */
/* ========================================================================= */

void Switch::LoadSystemFonts() {
    PlFontData standardFont;
    PlFontData extendedFont;

    Result rc = plGetSharedFontByType(&standardFont, PlSharedFontType_Standard);
    if (R_FAILED(rc)) {
        fprintf(stderr, "Switch: failed to load Standard system font\n");
        return;
    }

    rc = plGetSharedFontByType(&extendedFont, PlSharedFontType_NintendoExt);
    if (R_FAILED(rc)) {
        fprintf(stderr, "Switch: failed to load Extended system font\n");
        return;
    }

    // Font data is ready: standardFont.address/size, extendedFont.address/size.
    // ImGui integration requires building a custom atlas. This is deferred to
    // a later phase — for now, the default ImGui font is used.
}

/* ========================================================================= */
/*  Virtual Keyboard                                                         */
/* ========================================================================= */

std::string Switch::OpenVirtualKeyboard(const char* initialText) {
    // Swkbd requires applet resources. SDL_StartTextInput / SDL_StopTextInput
    // provide a simpler path through SDL2's switch port (it wraps Swkbd).
    // For now, return empty — keyboard integration is a Phase 6+ feature.
    (void)initialText;
    return {};
}

/* ========================================================================= */
/*  CPU Boost                                                                */
/* ========================================================================= */

void Switch::SetCpuBoost(int level) {
    if (level < 1) level = 1;
    if (level > 3) level = 3;

    uint32_t hz = (level == 1) ? SwitchCpuClock::Level1
                : (level == 2) ? SwitchCpuClock::Level2
                :               SwitchCpuClock::Level3;

    /* Try the old API first (firmware < 8.0.0). */
    if (hosversionBefore(8, 0, 0)) {
        pcvSetClockRate(PcvModule_CpuBus, hz);
        return;
    }

    /* Modern firmware (8.0.0+): use applet CPU boost mode.
     * ApmCpuBoostMode_FastLoad gives 1785 MHz docked/handheld
     * (where thermals allow). */
    ApmCpuBoostMode boost = (level >= 2)
        ? ApmCpuBoostMode_FastLoad
        : ApmCpuBoostMode_Normal;
    appletSetCpuBoostMode(boost);
}

} // namespace Ship

#endif // __SWITCH__

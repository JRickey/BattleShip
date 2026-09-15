#include <Windows.h>
#include <SDL2/SDL.h>

#include <cstdio>
#include <string>

extern "C" __declspec(dllimport) void* uwp_GetWindowReference();

namespace {

using BattleShipMain = int (*)(int, char**);

void WriteStartupMarker(const char* message, DWORD error = ERROR_SUCCESS) {
    char* prefPath = SDL_GetPrefPath(nullptr, "BattleShip");
    if (prefPath == nullptr) {
        return;
    }

    const std::string logPath = std::string(prefPath) + "wrapper.log";
    SDL_free(prefPath);
    if (SDL_RWops* log = SDL_RWFromFile(logPath.c_str(), "ab")) {
        char line[512]{};
        const int length = std::snprintf(line, sizeof(line), "%s (GetLastError=%lu)\r\n",
                                         message, static_cast<unsigned long>(error));
        if (length > 0) {
            SDL_RWwrite(log, line, 1, static_cast<size_t>(length));
        }
        SDL_RWclose(log);
    }
}

static int Bootstrap(int argc, char** argv) {
    WriteStartupMarker("Bootstrap entered");

    // Initializes the libuwp CoreWindow bridge before BattleShip creates its
    // DXGI backend on the SDL thread.
    uwp_GetWindowReference();
    WriteStartupMarker("libuwp CoreWindow bridge initialized");

    // Loading the game explicitly lets the wrapper start and leave a durable
    // diagnostic when Xbox cannot resolve one of BattleShip.dll's imports.
    // A static import fails before WinMain/Bootstrap and produces no useful
    // app-local log or Device Portal crash dump.
    HMODULE game = LoadPackagedLibrary(L"BattleShip.dll", 0);
    if (game == nullptr) {
        const DWORD error = GetLastError();
        WriteStartupMarker("LoadPackagedLibrary(BattleShip.dll) failed", error);
        char detail[192]{};
        std::snprintf(detail, sizeof(detail),
                      "BattleShip.dll could not be loaded (Windows error %lu). See wrapper.log.",
                      static_cast<unsigned long>(error));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "BattleShip startup failed", detail, nullptr);
        return static_cast<int>(error);
    }

    const auto battleShipMain = reinterpret_cast<BattleShipMain>(GetProcAddress(game, "SDL_main"));
    if (battleShipMain == nullptr) {
        const DWORD error = GetLastError();
        WriteStartupMarker("GetProcAddress(SDL_main) failed", error);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "BattleShip startup failed",
                                 "BattleShip.dll does not export SDL_main. See wrapper.log.", nullptr);
        FreeLibrary(game);
        return static_cast<int>(error);
    }

    WriteStartupMarker("BattleShip.dll loaded; calling SDL_main");
    const int result = battleShipMain(argc, argv);
    WriteStartupMarker("SDL_main returned", static_cast<DWORD>(result));
    FreeLibrary(game);
    return result;
}

} // namespace

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    WriteStartupMarker("WinMain entered");
    return SDL_WinRTRunApp(Bootstrap, nullptr);
}

#include <Windows.h>
#include <SDL2/SDL.h>

extern "C" __declspec(dllimport) void* uwp_GetWindowReference();

static int Bootstrap(int argc, char** argv) {
    // Initializes the libuwp CoreWindow bridge before BattleShip creates its
    // DXGI backend on the SDL thread.
    uwp_GetWindowReference();
    return SDL_main(argc, argv);
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return SDL_WinRTRunApp(Bootstrap, nullptr);
}

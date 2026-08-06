#include "enhancements.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include <cstdint>
#include <cstdlib>

constexpr const char* kMusicSelectionCVar = "gEnhancements.MusicSelection";

namespace {
    // Tracks whether the player is currently picking a song
    bool sIsSelectingMusic = false;

    // The same pool of playable stage tracks used in ShuffleMusic.cpp
    const uint32_t kShufflePool[] = {
        0, 1, 2, 4, 5, 6, 7, 8, 9, 36, 37, 25
    };
    constexpr size_t kShufflePoolSize = sizeof(kShufflePool) / sizeof(kShufflePool[0]);
}

extern "C" {
    // This variable lives in ShuffleMusic.cpp. We declare it here so we can update it.
    extern int32_t gManualMusicSelection;

    // Called when the stage select screen is loaded to reset the state
    void port_enhancement_music_select_reset(void) {
        sIsSelectingMusic = false;
        gManualMusicSelection = -1; // clear the track choice
    }

    // Returns:
    // 0 = CVar is off (Do normal behavior)
    // 1 = Phase 1 (Stage locked, wait for music selection)
    // 2 = Phase 2 (Music locked, proceed to battle)
    int32_t port_enhancement_music_select_handle_a(uint32_t bgm_id) {
        if (CVarGetInteger(kMusicSelectionCVar, 0) == 0) {
            return 0;
        }

        if (!sIsSelectingMusic) {
            sIsSelectingMusic = true;
            return 1;
        } else {
            // 0xFFFF is the flag sent by C if the player selects the "Random" stage slot
            if (bgm_id == 0xFFFF) {
                bgm_id = kShufflePool[std::rand() % kShufflePoolSize];
            }
            gManualMusicSelection = bgm_id;
            sIsSelectingMusic = false;
            return 2;
        }
    }

    // Returns: 1 if the B button canceled the music selection, 0 if normal behavior
    int32_t port_enhancement_music_select_handle_b(void) {
        if (sIsSelectingMusic) {
            sIsSelectingMusic = false;
            return 1;
        }
        return 0;
    }
}

namespace ssb64 {
    namespace enhancements {

        const char* MusicSelectionCVarName() {
            return kMusicSelectionCVar;
        }

    } // namespace enhancements
} // namespace ssb64

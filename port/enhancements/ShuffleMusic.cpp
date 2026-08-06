#include "enhancements.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>

constexpr const char* kShuffleMusicCVar = "gEnhancements.ShuffleMusic";

// Stage BGM integer mappings derived from gmMusicID
constexpr uint32_t nSYAudioBGMPupupu = 0;   // Dream Land
constexpr uint32_t nSYAudioBGMZebes = 1;    // Planet Zebes
constexpr uint32_t nSYAudioBGMInishie = 2;  // Mushroom Kingdom
constexpr uint32_t nSYAudioBGMSector = 4;   // Sector Z
constexpr uint32_t nSYAudioBGMJungle = 5;   // Kongo Jungle
constexpr uint32_t nSYAudioBGMCastle = 6;   // Peach's Castle
constexpr uint32_t nSYAudioBGMYamabuki = 7; // Saffron City
constexpr uint32_t nSYAudioBGMYoster = 8;   // Yoshi's Island
constexpr uint32_t nSYAudioBGMHyrule = 9;   // Hyrule Castle
constexpr uint32_t nSYAudioBGMLast = 25;    // Final Destination
constexpr uint32_t nSYAudioBGMZako = 36;    // Battlefield
constexpr uint32_t nSYAudioBGMMetal = 37;   // Meta Crystal

namespace {

    const uint32_t kShufflePool[] = {
        nSYAudioBGMPupupu,
        nSYAudioBGMZebes,
        nSYAudioBGMInishie,
        nSYAudioBGMSector,
        nSYAudioBGMJungle,
        nSYAudioBGMCastle,
        nSYAudioBGMYamabuki,
        nSYAudioBGMYoster,
        nSYAudioBGMHyrule,
        nSYAudioBGMZako,
        nSYAudioBGMMetal,
        nSYAudioBGMLast
    };

    constexpr size_t kShufflePoolSize = sizeof(kShufflePool) / sizeof(kShufflePool[0]);

    bool IsStageBGM(uint32_t bgm) {
        for (size_t i = 0; i < kShufflePoolSize; i++) {
            if (kShufflePool[i] == bgm) {
                return true;
            }
        }
        return false;
    }

}

// -1 means the user has not locked in a manual track
extern "C" {
    int32_t gManualMusicSelection = -1;
}

extern "C" uint32_t port_enhancement_shuffle_music(uint32_t requested_bgm) {
    // 1. highest priority: Manual Selection via Double-Tap UI
    if (gManualMusicSelection != -1 && IsStageBGM(requested_bgm)) {
        return (uint32_t)gManualMusicSelection;
    }

    // 2. middle priority: Shuffle Music
    if (CVarGetInteger(kShuffleMusicCVar, 0) != 0 && IsStageBGM(requested_bgm)) {
        static bool sIsSeeded = false;
        if (!sIsSeeded) {
            // Seed the generator with the current time
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            sIsSeeded = true;
        }
        uint32_t random_index = std::rand() % kShufflePoolSize;
        return kShufflePool[random_index];
    }

    // 3. lowest priority: Default Stage BGM
    return requested_bgm;
}

namespace ssb64 {
    namespace enhancements {
        const char* ShuffleMusicCVarName() {
            return kShuffleMusicCVar;
        }
    } // namespace enhancements
} // namespace ssb64

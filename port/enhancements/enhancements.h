#ifndef PORT_ENHANCEMENTS_H
#define PORT_ENHANCEMENTS_H

#ifdef __cplusplus
#include <string> // needed for the built-in updater
#include <vector> // needed for the shader-pack candidate list
extern "C" {
#endif

#define PORT_ENHANCEMENT_MAX_PLAYERS 4

int port_enhancement_tap_jump_disabled(int player_index);

// Hitbox-view debug overlay. Returns the dbObjectDisplayMode the caller should
// use for the current frame. When the cvar is off this is the entity's own
// stored display_mode; when on it overrides to a hitbox-display mode so
// existing fighters/items/weapons flip to hitbox view immediately without a
// match restart.
int port_enhancement_hitbox_display_override(int current_mode);

void port_enhancement_stage_hazards_tick(void);

// C-Stick → smash/aerial attack remap. When enabled for a player, the C-button
// directional inputs are translated into the corresponding smash/aerial attack
// inputs (mirroring later Smash games' right-stick behaviour) instead of the
// stock jump assignments. Active only during gameplay so CSS palette cycling
// with C buttons is unaffected.
//
// `tap_pre_remap` is a snapshot of `*button_tap` taken BEFORE this helper
// rewrites it, so callees can still ask "was this C-button tapped this frame
// (rising edge)?" after the C-bit has been cleared from the working tap mask.
// The caller passes the same value it copies into `*button_tap`.
void port_enhancement_c_stick_smash(int player_index, unsigned short* button_hold, unsigned short* button_tap, signed char* stick_x, signed char* stick_y, unsigned short tap_pre_remap);

// D-Pad → C-button remap. Companion to the C-Stick smash option for players
// who also have tap-jump disabled and would otherwise lose their jump input.
// `tap_pre_remap` has the same meaning as above.
void port_enhancement_dpad_jump(int player_index, unsigned short* button_hold, unsigned short* button_tap, unsigned short tap_pre_remap);

// Per-player NRage-style analog-stick remap. When the per-player toggle is on,
// rewrites *stick_x/*stick_y from libultraship's raw per-direction axis values
// using the verbatim per-axis deadzone+range formula from
// Ownasaurus/USBtoN64v2 (usbh_xpad.c), bypassing libultraship's circular
// deadzone, octagonal-gate clamp, and notch snap. When the toggle is off this
// is a no-op and libultraship's stock pipeline output flows through unchanged
// — both schemes coexist, the toggle picks which runs.
void port_enhancement_analog_remap(int player_index, signed char* stick_x, signed char* stick_y);

int port_cheat_unlock_all_active(void);

// competitive ruleset enhancements
int port_get_comp_ruleset(void);
void port_apply_comp_ruleset_overrides(void);
void port_cleanup_comp_ruleset(void);
void port_comp_ruleset_skip_stageselect(void);

// neutral spawn points on dreamland
int port_enhancement_neutral_spawns(void);

// z-cancel options
int port_enhancement_IsAutoZCancelEnabled(void);
int port_enhancement_IsFailedZCancelFlashEnabled(void);

// boot to CSS
int port_enhancement_boot_to_vs_css(void);

// skip results screen
int port_enhancement_skip_results_screen(void);

// cpu behavior
int port_enhancement_cpu_level_9(void);

// Classic mode 2-player co-op. The enabled flag is latched at boot
// (port_classic_coop_latch) — "(Needs reload)" semantics. The context flag
// marks the VS CSS overlay as currently serving Classic mode so its
// classic-only hooks (difficulty/stock selectors, exit redirects) engage.
void port_classic_coop_latch(void);
int port_enhancement_classic_coop(void);
int port_classic_coop_context(void);
void port_classic_coop_set_context(int active);
int port_classic_coop_friendly_fire(void);

#ifdef __cplusplus
}

namespace ssb64 {
namespace enhancements {
const char* TapJumpCVarName(int playerIndex);
const char* HitboxViewCVarName();
const char* StageHazardsDisabledCVarName();
const char* CStickSmashCVarName(int playerIndex);
const char* DPadJumpCVarName(int playerIndex);
const char* AnalogRemapCVarName(int playerIndex);
const char* AnalogRemapDeadzoneCVarName(int playerIndex);
const char* AnalogRemapRangeCVarName(int playerIndex);
const char* WidescreenCVarName();
const char* BonusStagesCVarName();
const char* CompRulesetCVarName();
const char* NeutralSpawnsCVarName();
const char* AutoZCancelCVarName();
const char* FailedZCancelCVarName();
const char* BootToVSCSSCVarName();
const char* SkipResultsScreenCVarName();
const char* CpuLevel9CVarName();
const char* ClassicCoopCVarName();
const char* ClassicCoopFriendlyFireCVarName();
const char* ShuffleMusicCVarName();
const char* MusicSelectionCVarName();
const char* DisableHUDCVarName();

// Discord Rich Presence
void UpdateDiscordPresence(const char* gameState, const char* matchDetails);
void TickDiscordPresence();

// Updater functions
void CheckForUpdatesAsync(bool force = false);
void StartGameUpdate();
bool IsUpdateAvailable();
bool DidUpdateCheckFail();
bool IsDownloading();
bool IsDownloadComplete();
bool IsCheckingForUpdates();
std::string GetUpdateStatus();
std::string GetDownloadStatus();
std::string GetLatestVersion();

// Libretro shader pack downloader. Two-phase background flow:
//
//   1. FetchShaderPackCatalogAsync() pulls libretro/glsl-shaders'
//      master.zip into a tempdir, validates every candidate against
//      the same normalize + transpile pipeline the picker uses,
//      and publishes the supported subset to GetShaderPackCandidates().
//      Phase advances Idle -> DownloadingCatalog -> ExtractingCatalog
//      -> EnumeratingCatalog -> AwaitingSelection.
//   2. InstallSelectedShaderPackAsync(stems) copies just the user-
//      picked candidates from the tempdir into <user-data>/shaders/
//      libretro/, writes per-shader sidecars, and tears the tempdir
//      down. Phase advances AwaitingSelection -> InstallingSelected
//      -> Done.
//
// CancelShaderPackFlow() returns to Idle from any phase and removes
// the tempdir. Sidecars are written with `compat=native` so the
// picker still warns about Low Resolution Mode.
//
// All UI state below is atomic so the menu doesn't need a lock per
// frame. Status strings + the candidate list are mutex-guarded but
// read at most once a frame from the picker.
enum class ShaderPackPhase {
    Idle,
    DownloadingCatalog,
    ExtractingCatalog,
    EnumeratingCatalog,
    AwaitingSelection,
    InstallingSelected,
    Done,
    Error,
};

// One installable candidate the catalog phase discovered. The UI
// shows `displayLabel`; `stem` is the stable identifier the user
// passes back to InstallSelectedShaderPackAsync.
struct ShaderPackCandidate {
    std::string stem;
    std::string displayLabel;
};

void                              FetchShaderPackCatalogAsync();
void                              InstallSelectedShaderPackAsync(
                                      const std::vector<std::string>& selectedStems);
void                              CancelShaderPackFlow();
ShaderPackPhase                   GetShaderPackPhase();
std::vector<ShaderPackCandidate>  GetShaderPackCandidates();
std::string                       GetShaderPackStatus();
int                               GetShaderPackInstalledCount();

// Back-compat boolean queries, derived from GetShaderPackPhase().
bool IsShaderPackDownloadInProgress();
bool IsShaderPackDownloadComplete();
}
}
#endif

#endif

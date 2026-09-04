// SPDX-License-Identifier: MIT
//
// Portions of this file are derived from the Ship of Harkinian (SoH) project
//   Copyright (c) The Harbour Masters
//   https://github.com/HarbourMasters/Shipwright
// Licensed under the MIT License; see LICENSE at repository root.

#include "PortMenu.h"

#include "Compat.h"
#include "../enhancements/enhancements.h"
#include "../interpolation/frame_interpolation.h"
#ifdef PORT_HIRES_ENABLED
#include "../hires/HiResPack.h"
#endif

#include <fast/backends/gfx_rendering_api.h>
#include <fast/postprocess/PostProcessSourceLoader.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#ifndef DISABLE_SCRIPTING
#include <ship/scripting/ScriptLoader.h>
#include "../mods/HookManager.h"
#include "../mods/ModRegistry.h"

namespace ssb64 { void MountModsDir(); void UnmountMissingMods(); }
#endif

#include <imgui.h>

#include <SDL2/SDL.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace ssb64 {

using namespace UIWidgets;

namespace {
static const std::map<int32_t, const char*> kMenuThemeOptions = {
    { UIWidgets::Colors::Red, "Red" },
    { UIWidgets::Colors::DarkRed, "Dark Red" },
    { UIWidgets::Colors::Orange, "Orange" },
    { UIWidgets::Colors::Green, "Green" },
    { UIWidgets::Colors::DarkGreen, "Dark Green" },
    { UIWidgets::Colors::LightBlue, "Light Blue" },
    { UIWidgets::Colors::Blue, "Blue" },
    { UIWidgets::Colors::DarkBlue, "Dark Blue" },
    { UIWidgets::Colors::Indigo, "Indigo" },
    { UIWidgets::Colors::Violet, "Violet" },
    { UIWidgets::Colors::Purple, "Purple" },
    { UIWidgets::Colors::Brown, "Brown" },
    { UIWidgets::Colors::Gray, "Gray" },
    { UIWidgets::Colors::DarkGray, "Dark Gray" },
};

static const std::map<int32_t, const char*> kTextureFilteringMap = {
    { Fast::FILTER_THREE_POINT, "Three-Point" },
    { Fast::FILTER_LINEAR, "Linear" },
    { Fast::FILTER_NONE, "None" },
};

// Enhanced framerate mode: CVar stores the target render fps; game logic
// always stays at 60 Hz (see port/interpolation/frame_interpolation.h).
static const std::map<int32_t, const char*> kEnhancedFpsMap = {
    { 0, "Off (60 FPS)" },
    { 120, "120 FPS" },
    { 180, "180 FPS" },
    { 240, "240 FPS" },
};

// Mirrors the LowResMode switch in libultraship Gui::CalculateGameViewport /
// Gui::DrawGame. 0 keeps the framebuffer at the window resolution; 1 forces a
// 320x240 4:3 framebuffer centered with side strips; 2/3 keep the window's
// aspect but lock vertical pixel count.
// Keys 0..3 hand off to libultraship's gLowResMode (Gui.cpp LowResMode switch);
// keys 4..7 take the PixelPerfectMode path of gAdvancedResolution and ignore
// gLowResMode. port.cpp's boot latch translates the pending menu value into
// the right combination of LUS cvars so libultraship sees a stable state for
// the whole session.
static const std::map<int32_t, const char*> kLowResModeMap = {
    { 0, "Off (window resolution)" },
    { 1, "N64 (320x240, stretched 4:3)" },
    { 2, "240p (window aspect, stretched)" },
    { 3, "480p (window aspect, stretched)" },
    { 4, "N64 integer-scaled (auto-fit, pixel-perfect)" },
    { 5, "N64 integer-scaled (2x, pixel-perfect)" },
    { 6, "N64 integer-scaled (3x, pixel-perfect)" },
    { 7, "N64 integer-scaled (4x, pixel-perfect)" },
};

// Mirrors dbObjectDisplayMode (src/sys/develop.h). 0 disables the override and
// returns the engine's normal rendering.
static const std::map<int32_t, const char*> kHitboxViewMap = {
    { 0, "Off" },
    { 1, "Filled (red cubes)" },
    { 2, "Outline + opaque hurtboxes" },
};

// Bundled (in-archive) post-process shaders get a friendly label
// in the picker. Names not in this map render with the shader's
// short name directly (the user-supplied taxonomy is good enough).
static const char* PostProcessShaderLabel(const std::string& name) {
    if (name == "scanlines")  return "Scanlines";
    if (name == "crt-lottes") return "CRT - Lottes";
    return nullptr;
}

// Draws the compat badge after a picker entry. `Any` shaders render
// at any resolution; `Native` shaders ratio against InputSize and
// only look right when the source FB is locked to ~240p (Low
// Resolution Mode in the Settings → Graphics menu). The badge text
// itself is colored, license hover-text comes from the sidecar.
static void DrawPostProcessCompatBadge(const Fast::PostProcessShaderInfo& info) {
    const char* text = info.compat == Fast::PostProcessCompat::Any ? "[any res]" : "[needs 240p]";
    const ImVec4 color = info.compat == Fast::PostProcessCompat::Any
        ? ImVec4(0.55f, 0.85f, 0.55f, 1.0f)
        : ImVec4(0.95f, 0.75f, 0.35f, 1.0f);
    ImGui::SameLine();
    ImGui::TextColored(color, "%s", text);
    if (!info.license.empty() && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("License: %s", info.license.c_str());
    }
}

// Selecting a `compat=native` shader while Low Resolution Mode is
// disabled is the common UX trap — the shader compiles and runs but
// produces a tiny render in the corner of the FB. We open this
// pending-modal token from the picker so the modal can be drawn at
// the top of the next frame outside any combo / popup context.
static std::string s_lowResWarnShader;
static bool s_lowResWarnNeedsOpen = false;

// Render the post-process shader picker as a single combo widget
// that opens onto a tree of bundled + user-installed shaders. Drives
// the two LUS cvars LUS::Run reads each frame:
//   gPostProcessEnabled (int, 0/1)
//   gPostProcessShader  (string, short name as accepted by
//                        Fast::LoadPostProcessShader)
// The combo's preview text reflects the current cvar state, so a
// fresh launch picks up the previously-selected shader without the
// picker needing to know about state-restore.
void RenderPostProcessShaderPicker(WidgetInfo& /*widget*/) {
    const bool enabled = CVarGetInteger("gPostProcessEnabled", 0) != 0;
    const char* currentRaw = CVarGetString("gPostProcessShader", "");
    const std::string current = currentRaw ? currentRaw : std::string();

    std::string preview = "Off";
    if (enabled && !current.empty()) {
        if (const char* friendly = PostProcessShaderLabel(current)) {
            preview = friendly;
        } else {
            preview = current;
        }
    }

    ImGui::TextUnformatted("Post-Process Shader");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (!ImGui::BeginCombo("##gPostProcessShaderPicker", preview.c_str())) {
        return;
    }

    // Filter input persists across frames inside the open popup
    // only — closing the combo clears it. ImGui draws the combo
    // content inside its own internal child window each frame, so
    // a static-local is the simplest persistence story without a
    // companion cvar.
    static char filterBuf[128] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##gPostProcessShaderFilter", "Filter...", filterBuf, sizeof(filterBuf));
    const std::string filter = filterBuf;

    auto matchesFilter = [&filter](const std::string& s) -> bool {
        if (filter.empty()) {
            return true;
        }
        // Case-insensitive substring match.
        std::string hay = s;
        std::string needle = filter;
        std::transform(hay.begin(), hay.end(), hay.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(needle.begin(), needle.end(), needle.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return hay.find(needle) != std::string::npos;
    };

    auto selectShader = [](const std::string& name) {
        if (name.empty()) {
            CVarSetInteger("gPostProcessEnabled", 0);
            return;
        }
        CVarSetString("gPostProcessShader", name.c_str());
        CVarSetInteger("gPostProcessEnabled", 1);

        // Trigger the Low Resolution Mode warn modal when the picked
        // shader expects a native-resolution source FB but the user's
        // current latched mode is "off". The shader still loads — we
        // never block selection — but the modal nudges them toward
        // enabling N64 mode + restart so the picture isn't a tiny
        // render in the corner of the window.
        const Fast::PostProcessShaderInfo info = Fast::GetPostProcessShaderInfo(name);
        if (info.compat == Fast::PostProcessCompat::Native &&
            CVarGetInteger("gLowResModePending", 0) == 0) {
            s_lowResWarnShader = name;
            s_lowResWarnNeedsOpen = true;
        }
    };

    // Per-frame compat-info cache. Filesystem stat + sidecar parse
    // happens here so each picker entry below can show a badge without
    // re-probing on every Selectable. The map's lifetime is the
    // visible combo body; rebuilt every frame the combo is open since
    // the user's shader set rarely changes mid-popup and the cost is
    // dwarfed by ImGui itself.
    std::map<std::string, Fast::PostProcessShaderInfo> infoCache;
    auto cachedInfo = [&infoCache](const std::string& name) -> const Fast::PostProcessShaderInfo& {
        auto it = infoCache.find(name);
        if (it != infoCache.end()) return it->second;
        Fast::PostProcessShaderInfo info = Fast::GetPostProcessShaderInfo(name);
        return infoCache.emplace(name, info).first->second;
    };

    // "Off" sits at the top regardless of filter — it's not a shader
    // name, just the disable affordance, and hiding it behind the
    // filter would leave the user with no way to turn the effect off
    // once they've narrowed the list.
    if (ImGui::Selectable("Off", !enabled)) {
        selectShader(std::string());
    }
    ImGui::Separator();

    // Bundled group: pulled from the LUS API rather than hardcoded
    // here so adding a builtin shader to f3d.o2r exposes it in the
    // picker without a port-side change.
    const std::vector<std::string> bundled = Fast::ListBuiltinPostProcessShaders();
    std::vector<std::string> bundledFiltered;
    bundledFiltered.reserve(bundled.size());
    for (const std::string& name : bundled) {
        const char* label = PostProcessShaderLabel(name);
        if (matchesFilter(name) || (label && matchesFilter(label))) {
            bundledFiltered.push_back(name);
        }
    }
    if (!bundledFiltered.empty()) {
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNodeEx("Bundled", ImGuiTreeNodeFlags_SpanAvailWidth)) {
            for (const std::string& name : bundledFiltered) {
                const char* friendly = PostProcessShaderLabel(name);
                const std::string label = friendly ? friendly : name;
                const bool active = enabled && current == name;
                if (ImGui::Selectable(label.c_str(), active)) {
                    selectShader(name);
                }
                DrawPostProcessCompatBadge(cachedInfo(name));
            }
            ImGui::TreePop();
        }
    }

    // User-installed shaders, one tree node per folder under the
    // per-user shaders dir. The "(loose)" bucket holds shaders that
    // live directly in the root.
    const std::vector<Fast::UserPostProcessShaderFolder> userFolders =
        Fast::ListUserPostProcessShaders();
    for (const auto& folder : userFolders) {
        std::vector<std::string> matching;
        matching.reserve(folder.shaderNames.size());
        for (const std::string& name : folder.shaderNames) {
            if (matchesFilter(name) || matchesFilter(folder.displayName)) {
                matching.push_back(name);
            }
        }
        if (matching.empty()) {
            continue;
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNodeEx(folder.displayName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth)) {
            for (const std::string& name : matching) {
                const Fast::PostProcessShaderInfo& info = cachedInfo(name);
                const std::string label = info.label.empty() ? name : info.label;
                const bool active = enabled && current == name;
                if (ImGui::Selectable(label.c_str(), active)) {
                    selectShader(name);
                }
                DrawPostProcessCompatBadge(info);
            }
            ImGui::TreePop();
        }
    }

    if (bundledFiltered.empty() && userFolders.empty() && filter.empty()) {
        ImGui::TextDisabled("No user shaders installed.");
        ImGui::TextDisabled("Drop .glsl / .glslp files into:");
        const std::string userDir = Ship::Context::GetPathRelativeToAppDirectory("shaders");
        ImGui::TextDisabled("%s", userDir.c_str());
    }

    ImGui::EndCombo();
}

// Live status panel for the post-process chain. Reads
// Fast::GetPostProcessRuntimeDiagnostics(), which the chain publishes
// on every load / unload. Lets the user confirm at a glance:
//   * which shader is actually active (matches the cvar)
//   * which load path produced it (legacy .glsl / .glslp, or .slang)
//   * how many passes compiled
//   * the last error if a load failed
// Pair the on-screen state with the matching SPDLOG_INFO lines in
// ssb64.log to walk through the load pipeline — see
// docs/crt_shader_testing_protocol.md.
void RenderPostProcessDiagnostics(WidgetInfo& /*widget*/) {
    const Fast::PostProcessRuntimeDiagnostics diag = Fast::GetPostProcessRuntimeDiagnostics();

    ImGui::TextDisabled("Shader runtime diagnostics");
    if (diag.active) {
        ImGui::Text("Active:  %s", diag.name.c_str());
        ImGui::Text("Flavor:  %s", diag.flavor.c_str());
        ImGui::Text("Passes:  %zu", diag.passCount);
    } else if (!diag.lastError.empty()) {
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Inactive — last error:");
        ImGui::TextWrapped("%s", diag.lastError.c_str());
    } else {
        ImGui::TextDisabled("No shader loaded.");
    }

    // Reload button — flips the enabled cvar off then on to force the
    // interpreter to redo the load (which re-runs all of the
    // SPDLOG_INFO instrumentation in the load path). Helpful when
    // tweaking sidecars or replacing files on disk during testing.
    if (ImGui::Button("Reload active shader")) {
        const char* currentRaw = CVarGetString("gPostProcessShader", "");
        const std::string current = currentRaw ? currentRaw : std::string();
        if (!current.empty()) {
            CVarSetInteger("gPostProcessEnabled", 0);
            // The interpreter's transition detection keys on the (name,
            // enabled) pair, so we re-write both even though the name
            // is unchanged — that re-arms the load path.
            CVarSetString("gPostProcessShader", current.c_str());
            CVarSetInteger("gPostProcessEnabled", 1);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Force the runtime to re-resolve, re-load, and re-compile the\n"
            "currently-selected shader. Watch ssb64.log for the matching\n"
            "Post-process: ... INFO lines. Useful after editing a shader\n"
            "file on disk or its .lus.json sidecar.");
    }
}

#if !defined(__ANDROID__)
// Shader-pack downloader UI is gated off on Android (the libretro
// catalog fetch + ZIP-extract pipeline doesn't fit Play Store distro;
// port/enhancements/ShaderDownloader.cpp is not compiled into libmain.so).
// The matching menu entry under Settings → Graphics is wrapped in the
// same guard, so this block is dead code on Android — gate the whole
// definition so the link doesn't reach for the missing symbols.

// State scoped to the shader-pack modal, mirroring the Low-Res-warn
// pattern. We keep a "needs to open this frame" flag so the
// RenderShaderPackModal hook (called every frame from
// RenderMenuTopLevel) can issue the OpenPopup against the correct
// ImGui ID — clicking the picker's "Get more shaders…" button only
// sets the flag; OpenPopup must run on the same stack as the
// matching BeginPopupModal.
static bool s_shaderPackModalNeedsOpen = false;

// User-side selection state. Vector<bool> packs the picks alongside
// the candidate list returned by GetShaderPackCandidates(); we keep
// the candidate copy here so toggles don't reach into the worker
// thread's data, and so cancel-then-reopen produces a fresh list
// instead of stale state.
static std::vector<ssb64::enhancements::ShaderPackCandidate> s_shaderPackCandidatesUi;
static std::vector<bool>                                     s_shaderPackSelected;
// Last phase we saw on the previous frame — used to detect the
// AwaitingSelection edge so we snapshot the candidate list once
// (instead of once per frame, which would also reset the user's
// checkbox state).
static ssb64::enhancements::ShaderPackPhase s_shaderPackLastPhase =
    ssb64::enhancements::ShaderPackPhase::Idle;
static char s_shaderPackFilter[128] = {};

// Renders the "Get more shaders…" affordance immediately beneath the
// picker. Click opens the modal; the modal handles the catalog fetch,
// the user's selection, and the install in sequence.
void RenderShaderPackDownloader(WidgetInfo& /*widget*/) {
    using namespace ssb64::enhancements;

    const ShaderPackPhase phase = GetShaderPackPhase();
    const bool busy =
        phase == ShaderPackPhase::DownloadingCatalog ||
        phase == ShaderPackPhase::ExtractingCatalog ||
        phase == ShaderPackPhase::EnumeratingCatalog ||
        phase == ShaderPackPhase::AwaitingSelection ||
        phase == ShaderPackPhase::InstallingSelected;

    if (UIWidgets::Button(
            "Get more shaders...",
            UIWidgets::ButtonOptions()
                .Color(UIWidgets::Colors::LightBlue)
                .Tooltip("Browse and install community CRT/scanline/NTSC shaders\n"
                         "from libretro/glsl-shaders. These shaders are GPL'd and\n"
                         "are installed by the user at runtime - the binary itself\n"
                         "stays MIT. Most expect Low Resolution Mode for correct\n"
                         "rendering scale."))) {
        SPDLOG_INFO("Shader pack: button clicked, current phase={} busy={}",
                    static_cast<int>(phase), busy);
        s_shaderPackModalNeedsOpen = true;
        if (!busy) {
            ssb64::enhancements::FetchShaderPackCatalogAsync();
        }
    }

    const std::string status = GetShaderPackStatus();
    if (!status.empty() && phase == ShaderPackPhase::Done) {
        // Persistent post-install summary directly under the button
        // (the modal closes on Install, so the user wouldn't see this
        // otherwise).
        ImGui::TextDisabled("%s", status.c_str());
    }
}

// Drawn from RenderMenuTopLevel each frame so the modal survives the
// picker combo collapsing. Six conceptual states laid out in one
// function because they share the same window+title+button layout
// scaffold; switching them out frame-to-frame is the entire UX:
//
//   1. Loading (DownloadingCatalog / ExtractingCatalog /
//      EnumeratingCatalog) — spinner + status string + Cancel.
//   2. Awaiting selection — checkbox list + filter + Select All /
//      None + Install + Cancel.
//   3. Installing — spinner + status + (no Cancel; copy is fast).
//   4. Done / Error — status line + OK.
void RenderShaderPackModal() {
    using namespace ssb64::enhancements;

    if (s_shaderPackModalNeedsOpen) {
        SPDLOG_INFO("Shader pack: opening modal");
        ImGui::OpenPopup("Shader pack##downloader");
        s_shaderPackModalNeedsOpen = false;
    }

    const ShaderPackPhase phase = GetShaderPackPhase();

    // Detect the just-arrived AwaitingSelection edge — when we cross
    // it we snapshot the candidate list and reset the selection
    // bitmap to "all on" so the user's first interaction is to
    // de-select rather than to re-tick everything.
    if (phase == ShaderPackPhase::AwaitingSelection &&
        s_shaderPackLastPhase != ShaderPackPhase::AwaitingSelection) {
        s_shaderPackCandidatesUi = GetShaderPackCandidates();
        s_shaderPackSelected.assign(s_shaderPackCandidatesUi.size(), true);
        s_shaderPackFilter[0] = '\0';
    }
    s_shaderPackLastPhase = phase;

    // Always re-pin position + size on every open. NoSavedSettings
    // prevents imgui.ini from restoring stale geometry that could
    // place the modal off-screen or at zero size — a known cause of
    // "modal looks stuck because it's invisible". Matches the flags
    // used by the Re-extract modal lower in this file.
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560.0f, 480.0f), ImGuiCond_Always);
    if (!ImGui::BeginPopupModal("Shader pack##downloader", nullptr,
                                ImGuiWindowFlags_NoSavedSettings)) {
        return;
    }

    const std::string status = GetShaderPackStatus();

    auto closeWithCancel = [&]() {
        CancelShaderPackFlow();
        s_shaderPackCandidatesUi.clear();
        s_shaderPackSelected.clear();
        s_shaderPackLastPhase = ShaderPackPhase::Idle;
        ImGui::CloseCurrentPopup();
    };

    // Fixed sizes for the action-row buttons so the modal layout
     // doesn't reflow as their labels change between phases. The
     // theme's default Fill size would consume the entire content
     // region and we want the rest of the modal (text, list) to keep
     // its space.
    const ImVec2 kSmallButton(120.0f, 0.0f);
    const ImVec2 kMediumButton(160.0f, 0.0f);

    switch (phase) {
        case ShaderPackPhase::Idle:
        case ShaderPackPhase::Error: {
            // Fallback for the rare case where the modal opens before
            // the catalog phase has actually fired. Offer to retry
            // the fetch rather than leaving the user with an empty
            // window.
            ImGui::TextWrapped("%s", !status.empty()
                ? status.c_str()
                : "No catalog loaded. Try fetching again.");
            ImGui::Spacing();
            if (UIWidgets::Button("Retry",
                                  UIWidgets::ButtonOptions()
                                      .Size(kSmallButton)
                                      .Color(UIWidgets::Colors::LightBlue))) {
                ssb64::enhancements::FetchShaderPackCatalogAsync();
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Close",
                                  UIWidgets::ButtonOptions()
                                      .Size(kSmallButton)
                                      .Color(UIWidgets::Colors::Gray))) {
                ImGui::CloseCurrentPopup();
            }
            break;
        }
        case ShaderPackPhase::DownloadingCatalog:
        case ShaderPackPhase::ExtractingCatalog:
        case ShaderPackPhase::EnumeratingCatalog: {
            ImGui::TextWrapped("%s", status.empty() ? "Working..." : status.c_str());
            ImGui::Spacing();
            ImGui::TextDisabled("Validating each shader against this build's\n"
                                "post-process pipeline. Takes a few seconds.");
            ImGui::Spacing();
            if (UIWidgets::Button("Cancel",
                                  UIWidgets::ButtonOptions()
                                      .Size(kSmallButton)
                                      .Color(UIWidgets::Colors::Gray))) {
                closeWithCancel();
            }
            break;
        }
        case ShaderPackPhase::AwaitingSelection: {
            ImGui::TextWrapped("%zu shaders available. Tick the ones you'd like to install.",
                               s_shaderPackCandidatesUi.size());
            ImGui::Spacing();

            if (UIWidgets::Button("Select all",
                                  UIWidgets::ButtonOptions()
                                      .Size(kMediumButton)
                                      .Color(UIWidgets::Colors::Gray))) {
                s_shaderPackSelected.assign(s_shaderPackCandidatesUi.size(), true);
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Select none",
                                  UIWidgets::ButtonOptions()
                                      .Size(kMediumButton)
                                      .Color(UIWidgets::Colors::Gray))) {
                s_shaderPackSelected.assign(s_shaderPackCandidatesUi.size(), false);
            }
            ImGui::SameLine();
            // The InputString widget has its own padded layout; let it
            // claim the remaining row width so the placeholder doesn't
            // crowd the buttons.
            std::string filterValue = s_shaderPackFilter;
            ImGui::SetNextItemWidth(-1.0f);
            if (UIWidgets::InputString(
                    "##shaderPackFilter", &filterValue,
                    UIWidgets::InputOptions()
                        .LabelPosition(UIWidgets::LabelPositions::None)
                        .PlaceholderText("Filter by name..."))) {
                std::strncpy(s_shaderPackFilter, filterValue.c_str(),
                             sizeof(s_shaderPackFilter) - 1);
                s_shaderPackFilter[sizeof(s_shaderPackFilter) - 1] = '\0';
            }

            // Lower-case copy of the filter — case-insensitive substring
            // match is what users expect from a name filter.
            std::string filterLower = s_shaderPackFilter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            // Reserve room above for the action row at the bottom so
            // the Install button doesn't shove the list off-screen on
            // the last frame. Themed checkboxes are taller than the
            // ImGui defaults; use the actual frame height + spacing
            // for the reservation.
            ImGui::BeginChild("##shaderPackList",
                              ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() - 8.0f),
                              true);
            int visibleCount = 0;
            for (size_t i = 0; i < s_shaderPackCandidatesUi.size(); ++i) {
                const auto& cand = s_shaderPackCandidatesUi[i];
                if (!filterLower.empty()) {
                    std::string labelLower = cand.displayLabel;
                    std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (labelLower.find(filterLower) == std::string::npos) {
                        continue;
                    }
                }
                ++visibleCount;
                bool checked = s_shaderPackSelected[i];
                if (UIWidgets::Checkbox(cand.displayLabel.c_str(), &checked,
                                        UIWidgets::CheckboxOptions().Color(
                                            UIWidgets::Colors::LightBlue))) {
                    s_shaderPackSelected[i] = checked;
                }
            }
            if (visibleCount == 0 && !filterLower.empty()) {
                ImGui::TextDisabled("No shaders match this filter.");
            }
            ImGui::EndChild();

            int totalChecked = 0;
            for (bool b : s_shaderPackSelected) {
                if (b) ++totalChecked;
            }

            const std::string installLabel =
                "Install " + std::to_string(totalChecked) + " shader" +
                (totalChecked == 1 ? "" : "s");
            const bool canInstall = totalChecked > 0;
            if (!canInstall) {
                ImGui::BeginDisabled();
            }
            if (UIWidgets::Button(installLabel.c_str(),
                                  UIWidgets::ButtonOptions()
                                      .Size(ImVec2(220.0f, 0.0f))
                                      .Color(UIWidgets::Colors::Green))) {
                std::vector<std::string> picks;
                picks.reserve(static_cast<size_t>(totalChecked));
                for (size_t i = 0; i < s_shaderPackCandidatesUi.size(); ++i) {
                    if (s_shaderPackSelected[i]) {
                        picks.push_back(s_shaderPackCandidatesUi[i].stem);
                    }
                }
                ssb64::enhancements::InstallSelectedShaderPackAsync(picks);
            }
            if (!canInstall) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Cancel",
                                  UIWidgets::ButtonOptions()
                                      .Size(kSmallButton)
                                      .Color(UIWidgets::Colors::Gray))) {
                closeWithCancel();
            }
            break;
        }
        case ShaderPackPhase::InstallingSelected: {
            ImGui::TextWrapped("%s", status.empty() ? "Installing..." : status.c_str());
            ImGui::Spacing();
            ImGui::TextDisabled("Just copying files now - this should take less than a second.");
            break;
        }
        case ShaderPackPhase::Done: {
            ImGui::TextWrapped("%s", status.empty() ? "Done." : status.c_str());
            ImGui::Spacing();
            if (UIWidgets::Button("OK",
                                  UIWidgets::ButtonOptions()
                                      .Size(kSmallButton)
                                      .Color(UIWidgets::Colors::Green))) {
                s_shaderPackCandidatesUi.clear();
                s_shaderPackSelected.clear();
                s_shaderPackLastPhase = ShaderPackPhase::Idle;
                ImGui::CloseCurrentPopup();
            }
            break;
        }
    }

    ImGui::EndPopup();
}
#endif // !__ANDROID__ (shader-pack downloader)

// Modal companion to RenderPostProcessShaderPicker. Drawn from
// RenderMenuTopLevel each frame so the popup survives the picker's
// combo closing. The picker sets s_lowResWarnNeedsOpen when the user
// selects a `compat=native` shader while Low Resolution Mode is off.
void RenderPostProcessLowResWarnModal() {
    if (s_lowResWarnNeedsOpen) {
        ImGui::OpenPopup("Low Resolution Mode##postprocess");
        s_lowResWarnNeedsOpen = false;
    }

    // Center the modal even though the picker may have triggered it
    // from a deep ImGui menu — the user shouldn't have to chase it
    // back to the picker's screen position.
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("Low Resolution Mode##postprocess",
                                nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::TextWrapped("\"%s\" was designed for the native 240p source resolution.",
                       s_lowResWarnShader.c_str());
    ImGui::Spacing();
    ImGui::TextWrapped("Low Resolution Mode is off, so the source framebuffer is the "
                       "full window size — the shader will render correctly, but the "
                       "picture will live in a small corner of the screen.");
    ImGui::Spacing();
    ImGui::TextWrapped("Enabling Low Resolution Mode forces the source FB to 320x240 "
                       "and requires a restart to take effect (it's latched at boot to "
                       "avoid mid-session Fast3D resize hazards).");
    ImGui::Spacing();

    if (ImGui::Button("Enable N64 mode (latched, restart required)")) {
        CVarSetInteger("gLowResModePending", 1);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Keep current settings")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable post-process")) {
        CVarSetInteger("gPostProcessEnabled", 0);
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

} // namespace

void PortMenu::AddSidebarEntry(std::string sectionName, std::string sidebarName, uint32_t columnCount) {
    assert(!sectionName.empty());
    assert(!sidebarName.empty());
    menuEntries.at(sectionName).sidebars.emplace(sidebarName, SidebarEntry{ .columnCount = columnCount });
    menuEntries.at(sectionName).sidebarOrder.push_back(sidebarName);
}

WidgetInfo& PortMenu::AddWidget(WidgetPath& pathInfo, std::string widgetName, WidgetType widgetType) {
    assert(!widgetName.empty());
    assert(menuEntries.contains(pathInfo.sectionName));
    assert(menuEntries.at(pathInfo.sectionName).sidebars.contains(pathInfo.sidebarName));

    auto& sidebars = menuEntries.at(pathInfo.sectionName).sidebars;
    const uint8_t column = pathInfo.column;
    while (sidebars.at(pathInfo.sidebarName).columnWidgets.size() < column + 1) {
        sidebars.at(pathInfo.sidebarName).columnWidgets.push_back({});
    }

    SidebarEntry& entry = sidebars.at(pathInfo.sidebarName);
    entry.columnWidgets.at(column).push_back({ .name = std::move(widgetName), .type = widgetType });
    WidgetInfo& widget = entry.columnWidgets.at(column).back();

    switch (widgetType) {
        case WIDGET_CHECKBOX:
        case WIDGET_CVAR_CHECKBOX:
            widget.options = std::make_shared<CheckboxOptions>();
            break;
        case WIDGET_SLIDER_FLOAT:
        case WIDGET_CVAR_SLIDER_FLOAT:
            widget.options = std::make_shared<FloatSliderOptions>();
            break;
        case WIDGET_CVAR_BTN_SELECTOR:
            widget.options = std::make_shared<BtnSelectorOptions>();
            break;
        case WIDGET_SLIDER_INT:
        case WIDGET_CVAR_SLIDER_INT:
            widget.options = std::make_shared<IntSliderOptions>();
            break;
        case WIDGET_COMBOBOX:
        case WIDGET_CVAR_COMBOBOX:
        case WIDGET_AUDIO_BACKEND:
        case WIDGET_VIDEO_BACKEND:
            widget.options = std::make_shared<ComboboxOptions>();
            break;
        case WIDGET_BUTTON:
            widget.options = std::make_shared<ButtonOptions>();
            break;
        case WIDGET_WINDOW_BUTTON:
            widget.options = std::make_shared<WindowButtonOptions>();
            break;
        case WIDGET_CVAR_COLOR_PICKER:
        case WIDGET_COLOR_PICKER:
            widget.options = std::make_shared<ColorPickerOptions>();
            break;
        case WIDGET_SEPARATOR_TEXT:
        case WIDGET_TEXT:
            widget.options = std::make_shared<TextOptions>();
            break;
        case WIDGET_SEARCH:
        case WIDGET_SEPARATOR:
        default:
            widget.options = std::make_shared<WidgetOptions>();
            break;
    }

    return widget;
}

PortMenu::PortMenu()
    : Menu("gOpenPortMenu", "Port Menu", 0, UIWidgets::Colors::LightBlue) {
}

void PortMenu::AddMenuSettings() {
    AddMenuEntry("Settings", CVAR_SETTING("Menu.SettingsSidebarSection"));
    AddSidebarEntry("Settings", "General", 2);

    WidgetPath path = { "Settings", "General", SECTION_COLUMN_1 };
    AddWidget(path, "Menu Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Menu Theme", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_SETTING("Menu.Theme"))
        .RaceDisable(false)
        .Options(ComboboxOptions().Tooltip("Changes the theme of the menu widgets.")
                     .ComboMap(kMenuThemeOptions)
                     .DefaultIndex(Colors::LightBlue));

    AddWidget(path, "Menu Controller Navigation", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_IMGUI_CONTROLLER_NAV)
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Allows controller navigation of the port menu.\nCAUTION: this disables game inputs while the menu is visible."));

    AddWidget(path, "Allow background inputs", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ALLOW_BACKGROUND_INPUTS)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
                        CVarGetInteger(CVAR_ALLOW_BACKGROUND_INPUTS, 1) ? "1" : "0");
        })
        .Options(CheckboxOptions().Tooltip(
                     "Allows controller input to be picked up by the game even when the window is not focused.")
                     .DefaultValue(true));

    AddWidget(path, "Menu Background Opacity", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_SETTING("Menu.BackgroundOpacity"))
        .RaceDisable(false)
        .Options(FloatSliderOptions().DefaultValue(0.85f).IsPercentage().Tooltip(
            "Sets the opacity of the background behind the port menu."));

    AddWidget(path, "General Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Cursor Always Visible", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("CursorVisibility"))
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            Ship::Context::GetInstance()->GetWindow()->SetForceCursorVisibility(
                CVarGetInteger(CVAR_SETTING("CursorVisibility"), 0));
        })
        .Options(CheckboxOptions().Tooltip("Makes the cursor always visible, even in fullscreen."));

    AddWidget(path, "Search In Sidebar", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("Menu.SidebarSearch"))
        .RaceDisable(false)
        .Callback([this](WidgetInfo&) {
            if (CVarGetInteger(CVAR_SETTING("Menu.SidebarSearch"), 0)) {
                InsertSidebarSearch();
            } else {
                RemoveSidebarSearch();
            }
        })
        .Options(CheckboxOptions().Tooltip(
            "Displays the Search page as a sidebar entry in Settings instead of in the header."));

    AddWidget(path, "Search Input Autofocus", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("Menu.SearchAutofocus"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Gives the search input focus when it becomes visible."));

#if !defined(__ANDROID__)
    // DRP — Android drops discord-rpc entirely (no curl + Play Store
    // distribution friction; the Discord SDK isn't built for mobile).
    AddWidget(path, "Enable Discord Rich Presence", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SETTING("Menu.EnableDRP"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Adds Discord Rich Presence (DRP)."));
#endif

    AddWidget(path, "Open App Files Folder", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            std::string filesPath = Ship::Context::GetInstance()->GetAppDirectoryPath();
            SDL_OpenURL(std::string("file:///" + fs::absolute(filesPath).string()).c_str());
        })
        .Options(ButtonOptions().Tooltip("Opens the folder that contains the save, config, and asset files."));

    path.sidebarName = "Graphics";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Graphics", 2);

    AddWidget(path, "Graphics Options", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Fullscreen", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) { Ship::Context::GetInstance()->GetWindow()->ToggleFullscreen(); })
        .Options(ButtonOptions().Tooltip("Toggles fullscreen on or off."));

    AddWidget(path, "Internal Resolution", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_INTERNAL_RESOLUTION)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            Ship::Context::GetInstance()->GetWindow()->SetResolutionMultiplier(
                CVarGetFloat(CVAR_INTERNAL_RESOLUTION, 1.0f));
        })
        .PreFunc([this](WidgetInfo& info) {
            if (disabledMap.at(DISABLE_FOR_ADVANCED_RESOLUTION_ON).active &&
                disabledMap.at(DISABLE_FOR_VERTICAL_RES_TOGGLE_ON).active) {
                info.activeDisables.push_back(DISABLE_FOR_ADVANCED_RESOLUTION_ON);
                info.activeDisables.push_back(DISABLE_FOR_VERTICAL_RES_TOGGLE_ON);
            } else if (disabledMap.at(DISABLE_FOR_LOW_RES_MODE_ON).active) {
                info.activeDisables.push_back(DISABLE_FOR_LOW_RES_MODE_ON);
            }
        })
        .Options(FloatSliderOptions()
                     .Tooltip("Multiplies the internal rendering resolution.")
                     .ShowButtons(false)
                     .IsPercentage()
                     .Min(0.5f)
                     .Max(2.0f));

    AddWidget(path, "Low Resolution Mode (Needs reload)", WIDGET_CVAR_COMBOBOX)
        .CVar("gLowResModePending")
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .Tooltip("Forces the internal framebuffer to a low resolution. "
                              "Stretched modes scale the framebuffer to fill the game viewport. "
                              "Pixel-perfect modes render N64-native 320x240 and draw it at an integer "
                              "factor, centred with black borders — sharper retro look, no sub-pixel blur. "
                              "Auto-fit picks the largest factor that still fits in the window. "
                              "Overrides Internal Resolution while active. "
                              "Latched at startup — changes take effect on next launch (toggling mid-session "
                              "would resize the framebuffer and race the Metal renderer).")
                     .ComboMap(kLowResModeMap)
                     .DefaultIndex(0));

#ifndef __WIIU__
    AddWidget(path, "Anti-aliasing (MSAA)", WIDGET_CVAR_SLIDER_INT)
        .CVar(CVAR_MSAA_VALUE)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            Ship::Context::GetInstance()->GetWindow()->SetMsaaLevel(CVarGetInteger(CVAR_MSAA_VALUE, 1));
        })
        .Options(IntSliderOptions()
                     .Tooltip("Sets the MSAA level for rendered geometry.")
                     .Min(1)
                     .Max(8)
                     .DefaultValue(1));
#endif

#if !defined(__ANDROID__)
    // Post-process / CRT shader stack — hidden on Android. The libretro
    // slang→SPIR-V→backend transpile path hasn't been validated against
    // GLES on mobile, and the libretro shader catalog download (curl +
    // ZIP extraction) is a poor fit for a Play Store distribution.
    AddWidget(path, "Post-Process Shader", WIDGET_CUSTOM)
        .RaceDisable(false)
        .CustomFunction(RenderPostProcessShaderPicker);

    AddWidget(path, "Shader Pack Downloader", WIDGET_CUSTOM)
        .RaceDisable(false)
        .CustomFunction(RenderShaderPackDownloader);

    AddWidget(path, "Shader Runtime Diagnostics", WIDGET_CUSTOM)
        .RaceDisable(false)
        .CustomFunction(RenderPostProcessDiagnostics);
#endif

    AddWidget(path, "Renderer API (Needs reload)", WIDGET_VIDEO_BACKEND).RaceDisable(false);
    AddWidget(path, "Enable Vsync", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_VSYNC_ENABLED)
        .RaceDisable(false)
        .PreFunc([this](WidgetInfo& info) { info.isHidden = disabledMap.at(DISABLE_FOR_NO_VSYNC).active; })
        .Options(CheckboxOptions().Tooltip("Removes tearing, but can cap the game to the display refresh rate.")
                     .DefaultValue(true));

    AddWidget(path, "Enhanced Framerate (Interpolated)", WIDGET_CVAR_COMBOBOX)
        .CVar(PORT_INTERP_CVAR_FPS)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) { portInterpApplyConfig(); })
        .Options(ComboboxOptions()
                     .Tooltip("Renders extra frames between 60 Hz game ticks by interpolating object "
                              "and camera motion. Game logic, physics, and input timing stay at exactly "
                              "60 Hz; only rendering is smoother. Adds up to one tick (16 ms) of visual "
                              "latency. Requires a display refresh rate at or above the chosen value "
                              "(or Vsync off) — if the host can't keep up, the game automatically steps "
                              "back down to protect game speed.")
                     .ComboMap(kEnhancedFpsMap)
                     .DefaultIndex(0));

#if !defined(__ANDROID__)
    AddWidget(path, "Windowed Fullscreen", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_SDL_WINDOWED_FULLSCREEN)
        .RaceDisable(false)
        .PreFunc([this](WidgetInfo& info) {
            info.isHidden = disabledMap.at(DISABLE_FOR_NO_WINDOWED_FULLSCREEN).active;
        })
        .Options(CheckboxOptions().Tooltip("Enables borderless windowed fullscreen mode."));
#endif

    AddWidget(path, "Allow multi-windows", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_ENABLE_MULTI_VIEWPORTS)
        .RaceDisable(false)
        .PreFunc([this](WidgetInfo& info) {
            info.isHidden = disabledMap.at(DISABLE_FOR_NO_MULTI_VIEWPORT).active;
        })
        .Options(CheckboxOptions()
                     .Tooltip("Allows multiple ImGui windows to be opened at once. Requires reload to take effect.")
                     .DefaultValue(true));

    AddWidget(path, "Texture Filter (Needs reload)", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_TEXTURE_FILTER)
        .RaceDisable(false)
        .Options(ComboboxOptions().Tooltip("Sets the active texture filtering mode.").ComboMap(kTextureFilteringMap));

    AddWidget(path, "Widescreen (Needs reload)", WIDGET_CVAR_CHECKBOX)
        .CVar(enhancements::WidescreenCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip(
                         "Renders battle scenes with a wider field of view to fill widescreen "
                         "windows. Title screen, menus, and intros stretch to fill the window "
                         "(no widescreen art). Some stages may show authored background edges "
                         "that were never visible in 4:3. Experimental — restart required for "
                         "the toggle to take effect.")
                     .DefaultValue(true));

    // Overscan crop. SSB64 bakes a 10px title-safe margin into its GBI scissor on
    // every side (decomp sys/rdp.c + sys/objdisplay.c); a CRT's overscan hid it,
    // but a desktop window shows it as a black border framing the whole image.
    // libultraship's Gui::DrawGame crops this proportional margin out of the
    // presented framebuffer so the safe area fills the viewport. Takes effect
    // immediately (read every frame at present time, no reload needed).
    AddWidget(path, "Crop Overscan Border", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_PREFIX_ADVANCED_RESOLUTION ".Overscan.Enabled")
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Removes the black border framing the image by cropping the game's built-in "
                              "title-safe margin, so the picture fills the window like it filled a CRT. "
                              "Disable to see the raw N64 framebuffer including its overscan border.")
                     .DefaultValue(true));

    AddWidget(path, "Overscan Crop — Horizontal", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_PREFIX_ADVANCED_RESOLUTION ".Overscan.Horizontal")
        .RaceDisable(false)
        .Options(FloatSliderOptions()
                     .Tooltip("How many N64 pixels (of 320 wide) to crop from each of the left and right "
                              "edges. The game's authored margin is 10.")
                     .Min(0.0f)
                     .Max(32.0f)
                     .DefaultValue(10.0f));

    AddWidget(path, "Overscan Crop — Vertical", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_PREFIX_ADVANCED_RESOLUTION ".Overscan.Vertical")
        .RaceDisable(false)
        .Options(FloatSliderOptions()
                     .Tooltip("How many N64 pixels (of 240 tall) to crop from each of the top and bottom "
                              "edges. The game's authored margin is 10.")
                     .Min(0.0f)
                     .Max(32.0f)
                     .DefaultValue(10.0f));

    path.sidebarName = "Gameplay";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Gameplay", 1);

    AddWidget(path, "Disable Stage Hazards", WIDGET_CVAR_CHECKBOX)
        .CVar(enhancements::StageHazardsDisabledCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Disables stage specific hazards such as moving platforms, tornadoes, Arwings, wind, etc."
            "Does not take effect mid-battle, only between battles.")
                     .DefaultValue(false));
    AddWidget(path, "Bonus Stages", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::BonusStagesCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Shows the port-added bonus stage page (Final Destination, "
                              "Metal Cavern, Battlefield) on the VS / Training stage select "
                              "screen. Turn off for the original stage roster. Takes effect "
                              "the next time the stage select screen opens.")
                     .DefaultValue(true));
    AddWidget(path, "Debug", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Hitbox View", WIDGET_CVAR_COMBOBOX)
        .CVar(enhancements::HitboxViewCVarName())
        .RaceDisable(false)
        .Options(ComboboxOptions()
                     .Tooltip("Replaces fighter/item/weapon rendering with the debug "
                              "hitbox visualization the decomp ships. Filled mode "
                              "draws red cubes for active hitboxes; Outline mode "
                              "draws hitbox edges and shows hurtboxes as solid "
                              "yellow/green/blue boxes (yellow=normal, green="
                              "invincible, blue=intangible).")
                     .ComboMap(kHitboxViewMap)
                     .DefaultIndex(0));

    // --- Competitive ruleset (still in Gameplay sidebar) ---
    AddWidget(path, "Competitive Rules", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Use Competitive Ruleset", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::CompRulesetCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Forces the following in VS Mode: \n"
                                           "- 4 Stocks \n"
                                           "- 8 Minutes \n"
                                           "- Items Off \n"
                                           "- Team Attack ON \n"
                                           "- Dream Land only"));
    AddWidget(path, "Neutral Spawns on Dreamland", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::NeutralSpawnsCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Forces balanced, opposite-side starting positions for 1v1 and Team Battles."));

    // --- Z-Cancel options ---
    AddWidget(path, "Z-Cancel Options", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Auto Z-Cancel", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::AutoZCancelCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Automatically cancels the landing lag from aerial attacks."));
    AddWidget(path, "Flash on Failed Z-Cancel", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::FailedZCancelCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Makes your character temporarily flash in flames on a failed Z-Cancel."));

    // --- Quality-of-Life (still in Gameplay sidebar) ---
    AddWidget(path, "Quality-of-Life", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Boot to VS CSS", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::BootToVSCSSCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Skips the intro sequences and boots directly to the VS Mode Character Select Screen."));
    AddWidget(path, "Classic Co-op (Needs reload)", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::ClassicCoopCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Classic (1P) mode uses the VS Mode Character Select Screen so a "
                              "second player can join the whole run as a co-op partner. If only "
                              "one player picks a character, the run plays as normal solo "
                              "Classic. Restart required for the toggle to take effect.")
                     .DefaultValue(true));
    AddWidget(path, "Co-op Friendly Fire", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::ClassicCoopFriendlyFireCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("Lets the two Classic Co-op players hit each other, like the "
                              "original 1P mode's CPU ally could. Off by default; takes effect "
                              "from the next stage.")
                     .DefaultValue(false));
    AddWidget(path, "Skip Results Screen", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::SkipResultsScreenCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Skips the victory podium and returns directly to the Character Select Screen."));
    AddWidget(path, "Force CPU Level 9", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::CpuLevel9CVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Automatically forces all CPU players to Level 9."));
    AddWidget(path, "Shuffle Music", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::ShuffleMusicCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Randomizes the BGM after a stage is chosen. Useful when enabling the competitive ruleset."));
    AddWidget(path, "Add Music Selection Screen", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::MusicSelectionCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Allows the player to pick a custom BGM track after the stage selection screen (music shuffling is ignored if this is turned on)."));

    // --- Other ---
    AddWidget(path, "Other", WIDGET_SEPARATOR_TEXT);
        AddWidget(path, "Disable HUD", WIDGET_CVAR_CHECKBOX)
        .CVar(ssb64::enhancements::DisableHUDCVarName())
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Disables the in-game HUD. Note that some icons will still be visible (e.g. CPU icons in 1P Mode on the top-left.)"));

    // --- Input customization ---
    path.sidebarName = "Input Mappings";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Input Mappings", 1);

    AddWidget(path, "Controller", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Controller Configuration", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_CONTROLLER_CONFIGURATION_WINDOW_OPEN)
        .RaceDisable(false)
        .WindowName("Input Editor")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the controller configuration window."));

    // --- Controls sidebar: per-player input remapping ---
    path.sidebarName = "Control Enhancements";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Control Enhancements", 1);

    for (int p = 0; p < PORT_ENHANCEMENT_MAX_PLAYERS; ++p) {
        const std::string playerLabel = fmt::format("Player {}", p + 1);
        AddWidget(path, playerLabel, WIDGET_SEPARATOR_TEXT);

        AddWidget(path, fmt::format("Disable Tap Jump (P{})", p + 1), WIDGET_CVAR_CHECKBOX)
            .CVar(enhancements::TapJumpCVarName(p))
            .RaceDisable(false)
            .Options(CheckboxOptions().Tooltip("Disables jumping by pushing up on the analog stick."));

        AddWidget(path, fmt::format("C-Stick Smash (P{})", p + 1), WIDGET_CVAR_CHECKBOX)
            .CVar(enhancements::CStickSmashCVarName(p))
            .RaceDisable(false)
            .Options(CheckboxOptions().Tooltip("Replaces C-Button inputs with instant smash attacks."));

        AddWidget(path, fmt::format("D-Pad to Jump (P{})", p + 1), WIDGET_CVAR_CHECKBOX)
            .CVar(enhancements::DPadJumpCVarName(p))
            .RaceDisable(false)
            .Options(CheckboxOptions().Tooltip("Translates N64 D-Pad inputs into C-Up (Jump)."));

        AddWidget(path, fmt::format("NRage Analog Stick Remap (P{})", p + 1), WIDGET_CVAR_CHECKBOX)
            .CVar(enhancements::AnalogRemapCVarName(p))
            .RaceDisable(false)
            .Options(CheckboxOptions().Tooltip(
                "When ON: replaces libultraship's stick processing with the NRage per-axis "
                "deadzone+range formula (no octagonal gate, no radial deadzone, no notch snap). "
                "When OFF: libultraship's stock pipeline runs unchanged. Both schemes coexist; "
                "this toggle picks which one runs for this player."));

        const char* deadzoneCVar = enhancements::AnalogRemapDeadzoneCVarName(p);
        const char* enableCVar   = enhancements::AnalogRemapCVarName(p);
        AddWidget(path, fmt::format("Remap Deadzone (P{})", p + 1), WIDGET_CVAR_SLIDER_FLOAT)
            .CVar(deadzoneCVar)
            .RaceDisable(false)
            .PreFunc([enableCVar](WidgetInfo& info) {
                if (!CVarGetInteger(enableCVar, 0)) {
                    info.options->disabled = true;
                    info.options->disabledTooltip = "Enable NRage Analog Stick Remap to adjust this.";
                }
            })
            .Options(FloatSliderOptions()
                         .Min(0.0f).Max(0.99f).DefaultValue(0.10f)
                         .IsPercentage()
                         .Tooltip("Per-axis deadzone (0–99%). Inputs inside this fraction of "
                                  "max deflection on each axis read as 0."));

        const char* rangeCVar = enhancements::AnalogRemapRangeCVarName(p);
        AddWidget(path, fmt::format("Remap Range (P{})", p + 1), WIDGET_CVAR_SLIDER_FLOAT)
            .CVar(rangeCVar)
            .RaceDisable(false)
            .PreFunc([enableCVar](WidgetInfo& info) {
                if (!CVarGetInteger(enableCVar, 0)) {
                    info.options->disabled = true;
                    info.options->disabledTooltip = "Enable NRage Analog Stick Remap to adjust this.";
                }
            })
            .Options(FloatSliderOptions()
                         .Min(0.5f).Max(1.5f).DefaultValue(1.0f)
                         .IsPercentage()
                         .Tooltip("Output scale (50–150%). 100% caps at the N64's natural max."));
    }

    // --- Audio ---
    path.sidebarName = "Audio";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Audio", 1);

    AddWidget(path, "Volume Levels", WIDGET_SEPARATOR_TEXT);


    // Master
    AddWidget(path, "Master Volume", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar("gSettings.Audio.MasterVolume")
        .RaceDisable(false)
        .Options(FloatSliderOptions()
        .Tooltip("Adjust the overall volume of the entire game.")
        .DefaultValue(1.0f)
        .Min(0.0f)
        .Max(1.0f)
        .IsPercentage());

    // Music Volume Slider
    AddWidget(path, "Music Volume", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar("gSettings.Audio.MusicVolume")
        .RaceDisable(false)
        .Options(FloatSliderOptions()
        .Tooltip("Adjust the volume of the background music.")
        .DefaultValue(1.0f)
        .Min(0.0f)
        .Max(1.0f)
        .IsPercentage());

    // Sound Effects Volume Slider
    AddWidget(path, "Sound Volume", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar("gSettings.Audio.SFXVolume") // Standard LUS native CVar
        .RaceDisable(false)
        .Options(FloatSliderOptions()
        .Tooltip("Adjust the volume of sound effects and voices.")
        .DefaultValue(1.0f)
        .Min(0.0f)
        .Max(1.0f)
        .IsPercentage());

    // voice
    AddWidget(path, "Voice Volume", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar("gSettings.Audio.VoiceVolume")
        .RaceDisable(false)
        .Options(FloatSliderOptions()
        .Tooltip("Adjust the volume of character voices and the announcer.")
        .DefaultValue(1.0f)
        .Min(0.0f)
        .Max(1.0f)
        .IsPercentage());

    AddWidget(path, "Focus Behavior", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Mute on Focus Loss", WIDGET_CVAR_CHECKBOX)
        .CVar("gSettings.FocusControl.MuteOnFocusLoss")
        .RaceDisable(false)
        .Options(CheckboxOptions()
            .Tooltip("Silences game audio while the window is unfocused. Restores volume on refocus.")
            .DefaultValue(false));

    // --- Cheats ---
    path.sidebarName = "Cheats";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Cheats", 1);

    AddWidget(path, "Master Unlocks", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Unlock Everything", WIDGET_CVAR_CHECKBOX)
        .CVar("gCheats.UnlockAll")
        .RaceDisable(false);

    AddWidget(path, "Individual Characters", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Unlock Captain Falcon", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockCaptain").RaceDisable(false);
    AddWidget(path, "Unlock Jigglypuff", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockPurin").RaceDisable(false);
    AddWidget(path, "Unlock Luigi", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockLuigi").RaceDisable(false);
    AddWidget(path, "Unlock Ness", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockNess").RaceDisable(false);

    AddWidget(path, "Individual Features", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Unlock Mushroom Kingdom", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockInishie").RaceDisable(false);
    AddWidget(path, "Unlock Item Switch Menu", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockItemSwitch").RaceDisable(false);
    AddWidget(path, "Unlock Sound Test Menu", WIDGET_CVAR_CHECKBOX).CVar("gCheats.UnlockSoundTest").RaceDisable(false);

    // --- Tools ---
    path.sidebarName = "Tools";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Settings", "Tools", 1);

    AddWidget(path, "Debug Windows", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Stats", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_STATS_WINDOW_OPEN)
        .RaceDisable(false)
        .WindowName("Stats")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Stats window."));
    AddWidget(path, "Console", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_CONSOLE_WINDOW_OPEN)
        .RaceDisable(false)
        .WindowName("Console")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Console window."));
    AddWidget(path, "Gfx Debugger", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_GFX_DEBUGGER_WINDOW_OPEN)
        .RaceDisable(false)
        .WindowName("GfxDebuggerWindow")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the graphics debugger window."));
}

void PortMenu::AddMenuAssets() {
    AddMenuEntry("Assets", CVAR_SETTING("Menu.AssetsSidebarSection"));

    WidgetPath path = { "Assets", "Extraction", SECTION_COLUMN_1 };
    AddSidebarEntry("Assets", "Extraction", 1);
    AddWidget(path, "Extraction", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, SSB64_O2R_NAME " is generated from your ROM and loaded on launch.", WIDGET_TEXT);
    AddWidget(path, "Schedule Re-extract", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([this](WidgetInfo&) {
            const auto path = Ship::Context::GetPathRelativeToAppDirectory(SSB64_O2R_NAME);
            std::error_code ec;
            fs::remove(path, ec);
            mShowReextractMessage = true;
        })
        .Options(ButtonOptions().Tooltip("Deletes " SSB64_O2R_NAME " so it is regenerated from the ROM on next launch.")
                     .Size(Sizes::Inline));

    path.sidebarName = "Paths";
    AddSidebarEntry("Assets", "Paths", 1);
    AddWidget(path, "Runtime Paths", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, fmt::format("App directory: {}", Ship::Context::GetPathRelativeToAppDirectory("")), WIDGET_TEXT);
    AddWidget(path, fmt::format("Main archive: {}", Ship::Context::GetPathRelativeToAppDirectory(SSB64_O2R_NAME)),
              WIDGET_TEXT);

#ifdef PORT_HIRES_ENABLED
    path.sidebarName = "Mods";
    path.column = SECTION_COLUMN_1;
    AddSidebarEntry("Assets", "Mods", 1);

    AddWidget(path, "Hi-Res Texture Pack", WIDGET_SEPARATOR_TEXT);
    AddWidget(path,
              "Drop a PNG pack into the mods/ folder. "
              "Files are matched by decoded-RGBA8 CRC32 — no per-PNG configuration needed. "
              "Toggle takes effect on the next texture cache miss.",
              WIDGET_TEXT);
    AddWidget(path, "Enable Hi-Res Texture Pack", WIDGET_CVAR_CHECKBOX)
        .CVar("gHiResTextures.Enabled")
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("When on, every cache-miss texture upload hashes the decoded RGBA8 image "
                                            "and substitutes a matching PNG from the mods/ index at the pack's higher "
                                            "resolution.")
                     .DefaultValue(ssb64::hires::kHiResEnabledDefault != 0));
    AddWidget(path, "Preload Pack at Boot", WIDGET_CVAR_CHECKBOX)
        .CVar("gHiResTextures.Preload")
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("Decodes the pack into RAM on background threads at startup so first use "
                                            "of a texture doesn't stutter on a synchronous PNG decode. Warms up to "
                                            "the cache budget below. Takes effect on next launch.")
                     .DefaultValue(ssb64::hires::kHiResPreloadDefault != 0));
    AddWidget(path, "Decoded Cache Budget (MB)", WIDGET_CVAR_SLIDER_INT)
        .CVar("gHiResTextures.CacheBudgetMB")
        .RaceDisable(false)
        .Options(IntSliderOptions()
                     .Tooltip("RAM budget for decoded pack textures. A large HD pack whose working set "
                              "exceeds this re-decodes evicted textures on the render thread (stutter) — "
                              "raise it if you have RAM to spare. Takes effect on next launch.")
                     .Min(ssb64::hires::kMinLruBudgetMB)
                     .Max(4096)
                     .DefaultValue(ssb64::hires::kDefaultLruBudgetMB));
#if defined(__ANDROID__)
    AddWidget(path,
              "Mobile note: decoded textures are uncompressed in RAM and on the GPU. "
              "The pack is capped to a 128 MB cache (gHiResTextures.CacheBudgetMB) and "
              "per-texture upscale limit, but large packs can still pressure memory on "
              "low-RAM devices. Off by default — enable only if your device has headroom.",
              WIDGET_TEXT);
#endif
    AddWidget(path, "Open Mods Folder", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            std::string modsPath = Ship::Context::GetPathRelativeToAppDirectory("mods");
            std::error_code ec;
            fs::create_directories(modsPath, ec); // make it if it isn't there yet
            SDL_OpenURL(std::string("file:///" + fs::absolute(modsPath).string()).c_str());
        })
        .Options(ButtonOptions().Tooltip("Opens the mods/ folder in your file browser. Drop pack subfolders here."));
    AddWidget(path,
              fmt::format("Indexed PNGs: {}",
                           ssb64::hires::HiResPack::Get().Stats().indexedTextures),
              WIDGET_TEXT);

    // Pack-authoring dump tooling is desktop-only — it writes hundreds of
    // files into <app>/ and is driven from a separate offline conversion
    // pipeline, neither of which makes sense on a touch device.
#if !defined(__ANDROID__)
    AddWidget(path, "Pack Authoring", WIDGET_SEPARATOR_TEXT);
    AddWidget(path,
              "Dump Source Textures writes one .bin per unique texture into "
              "<app>/hires_dump/. Used by the offline conversion tool to "
              "re-name a third-party Rice-CRC pack into our hash. Leave off "
              "in normal play.",
              WIDGET_TEXT);
    AddWidget(path, "Dump Source Textures", WIDGET_CVAR_CHECKBOX)
        .CVar("gHiResTextures.DumpSource")
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("On: every cache-miss with no pack hit dumps the source bytes + dimensions "
                                            "to <app>/hires_dump/. One file per unique texture per session.")
                     .DefaultValue(false));
    AddWidget(path, "Open Dump Folder", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            std::string dumpPath = Ship::Context::GetPathRelativeToAppDirectory("hires_dump");
            std::error_code ec;
            fs::create_directories(dumpPath, ec);
            SDL_OpenURL(std::string("file:///" + fs::absolute(dumpPath).string()).c_str());
        })
        .Options(ButtonOptions().Tooltip("Opens the hires_dump/ folder where source-texture dumps land."));

    AddWidget(path, "Miss Dump (native key)", WIDGET_SEPARATOR_TEXT);
    AddWidget(path,
              "On: every cache-miss with no pack hit writes a .bmp + .rgba into "
              "<app>/hires_miss_dump/, named with the port's native lookup key "
              "(miss#<hash>#<fmt>#<siz>_<w>x<h>). Rename the matching pack PNG to "
              "\"<anything>#<hash>#<fmt>#<siz>_all.png\" and drop it in mods/ to "
              "bind textures whose Rice CRC no dumper can reach (VS-records "
              "digits, tiny UI glyphs). Captures a full playthrough by default.",
              WIDGET_TEXT);
    AddWidget(path, "Dump Unmatched (native key)", WIDGET_CVAR_CHECKBOX)
        .CVar("gHiResTextures.DumpMissRgba")
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip("On: each unique unmatched texture is written as a viewable .bmp plus the "
                                            "raw .rgba the hash was computed over. The filename is the exact "
                                            "<hash>#<fmt>#<siz> the pack scanner looks up.")
                     .DefaultValue(false));
    AddWidget(path, "Open Miss-Dump Folder", WIDGET_BUTTON)
        .RaceDisable(false)
        .Callback([](WidgetInfo&) {
            std::string missPath = Ship::Context::GetPathRelativeToAppDirectory("hires_miss_dump");
            std::error_code ec;
            fs::create_directories(missPath, ec);
            SDL_OpenURL(std::string("file:///" + fs::absolute(missPath).string()).c_str());
        })
        .Options(ButtonOptions().Tooltip("Opens the hires_miss_dump/ folder where native-key dumps land."));
#endif // !__ANDROID__ (pack-authoring dump tooling)
#endif // PORT_HIRES_ENABLED
}

#ifndef DISABLE_SCRIPTING
// Cached, read-only mod list for the Mods panel. The set of installed mods
// only changes on a reload or restart, so we snapshot on demand (first view,
// Rescan, and after a Hot Reload) rather than re-scanning every frame.
static std::vector<ssb64::mods::ModInfo> s_modList;
static bool s_modListLoaded = false;

static void RefreshModList() {
    // Sync the mounted set to what's actually on disk so the list reflects mods
    // added or deleted at runtime: drop archives whose folder is gone, pick up
    // newly dropped ones. New mods appear as "not loaded" until a Hot Reload
    // compiles them; this only refreshes the inventory, it does not recompile.
    ssb64::UnmountMissingMods();
    ssb64::MountModsDir();
    s_modList = ssb64::mods::ModRegistry::Snapshot();
    s_modListLoaded = true;
}

static void DoHotReload() {
    auto scripting = Ship::Context::GetInstance()->GetScriptLoader();
    if (!scripting) {
        return;
    }
    try {
        /* Unload: ModExit fires, then HookManager uninstalls every
         * hook owned by that mod (postExit callback runs BEFORE
         * FreeLibrary so trampolines into the about-to-be-freed mod
         * code get cleanly removed). Then recompile + reload with the
         * SetCurrentOwner wrapper so new Install calls re-tag with
         * the right owner. */
        scripting->UnloadAll(
            /*preExit=*/std::nullopt,
            /*postExit=*/[](const std::string& mod) {
                ssb64::mods::HookManager::UninstallHooksForOwner(mod.c_str());
            });
        /* Now that every script is unloaded, drop archives whose mod folder
         * was deleted at runtime — otherwise it stays mounted and gets
         * recompiled below as if it were still installed. Then pick up any
         * new folders/archives that landed in mods/ since the last load
         * (idempotent: already-mounted archives are skipped). */
        ssb64::UnmountMissingMods();
        ssb64::MountModsDir();
        scripting->CompileAll();
        scripting->LoadAll(
            /*preInit=*/[](const std::string& mod) {
                ssb64::mods::HookManager::SetCurrentOwner(mod.c_str());
            },
            /*postInit=*/[](const std::string&) {
                ssb64::mods::HookManager::ClearCurrentOwner();
            });
        RefreshModList();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Mod reload failed: {}", e.what());
    }
}

void PortMenu::AddMenuMods() {
    WidgetPath path = { "Assets", "Script Mods", SECTION_COLUMN_1 };
    AddSidebarEntry("Assets", "Script Mods", 1);
    AddWidget(path, "mods_panel", WIDGET_CUSTOM)
        .CustomFunction([](WidgetInfo&) {
            // --- Action bar -------------------------------------------------
            if (ImGui::Button("Hot Reload")) {
                DoHotReload(); // re-snapshots s_modList when it finishes
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Unload + recompile + re-init all TCC mods.\n"
                                  "Adding or removing mod folders still needs an engine restart.");
            }
            ImGui::SameLine();
            if (ImGui::Button("Rescan")) {
                RefreshModList();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Re-read the mods/ folder without reloading the mods.");
            }
            ImGui::SameLine();
            if (ImGui::Button("Open Mods Folder")) {
                std::string modsPath = Ship::Context::GetPathRelativeToAppDirectory("mods");
                std::error_code ec;
                fs::create_directories(modsPath, ec);
                SDL_OpenURL(std::string("file:///" + fs::absolute(modsPath).string()).c_str());
            }

            // Build the list once on first view; thereafter only on demand.
            if (!s_modListLoaded) {
                RefreshModList();
            }

            // --- Filter -----------------------------------------------------
            static char filter[64] = "";
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##modfilter", "Filter by name or author", filter,
                                     sizeof(filter));

            ImGui::Separator();

            if (s_modList.empty()) {
                ImGui::TextWrapped("No mods found. Drop a mod folder or .o2r into the mods/ "
                                   "folder next to the executable, then press Rescan (or "
                                   "restart).");
                return;
            }

            auto toLower = [](std::string s) {
                std::transform(s.begin(), s.end(), s.begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
                return s;
            };
            const std::string needle = toLower(filter);
            auto matches = [&](const ssb64::mods::ModInfo& mod) {
                return needle.empty() ||
                       toLower(mod.name).find(needle) != std::string::npos ||
                       toLower(mod.author).find(needle) != std::string::npos;
            };

            // --- Mod cards --------------------------------------------------
            int loadedCount = 0;
            int issueCount = 0;
            int shown = 0;
            for (const auto& mod : s_modList) {
                using ssb64::mods::ModState;
                if (mod.state == ModState::Loaded) {
                    loadedCount++;
                }
                if (mod.state == ModState::InvalidManifest) {
                    issueCount++;
                }
                if (!matches(mod)) {
                    continue;
                }
                shown++;

                ImGui::PushID(mod.archivePath.c_str()); // unique id per card

                switch (mod.state) {
                    case ModState::Loaded:
                        ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.40f, 1.0f), "LOADED");
                        break;
                    case ModState::NotLoaded:
                        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), "not loaded");
                        break;
                    case ModState::InvalidManifest:
                        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.20f, 1.0f), "manifest issue");
                        break;
                }

                ImGui::SameLine();
                if (!mod.version.empty()) {
                    ImGui::Text("%s  v%s", mod.name.c_str(), mod.version.c_str());
                } else {
                    ImGui::TextUnformatted(mod.name.c_str());
                }
                if (ImGui::IsItemHovered() && !mod.archivePath.empty()) {
                    ImGui::SetTooltip("%s", mod.archivePath.c_str());
                }

                if (!mod.author.empty()) {
                    ImGui::TextDisabled("by %s", mod.author.c_str());
                }
                if (!mod.description.empty()) {
                    ImGui::TextWrapped("%s", mod.description.c_str());
                }
                if (!mod.note.empty()) {
                    ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.20f, 1.0f), "%s", mod.note.c_str());
                }

                ImGui::PopID();
                ImGui::Separator();
            }

            if (shown == 0) {
                ImGui::TextDisabled("No mods match the filter.");
            }

            ImGui::Spacing();
            std::string footer = std::to_string(static_cast<int>(s_modList.size())) +
                                 " installed  |  " + std::to_string(loadedCount) + " loaded";
            if (issueCount > 0) {
                footer += "  |  " + std::to_string(issueCount) + " issue(s)";
            }
            ImGui::TextUnformatted(footer.c_str());
        });
}
#endif

void PortMenu::AddMenuAbout() {
    AddMenuEntry("About", CVAR_SETTING("Menu.AboutSidebarSection"));

    // revamped credits menu
    WidgetPath path = { "About", "Build", SECTION_COLUMN_1 };
    AddSidebarEntry("About", "Build", 1);
    AddWidget(path, "About", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "BattleShip", WIDGET_TEXT);
    AddWidget(path, "Native Smash 64 PC port powered by libultraship", WIDGET_TEXT);
    AddWidget(path, "Built from the ssb-decomp-re decompilation", WIDGET_TEXT);

    AddWidget(path, "Credits", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "JRickey: Lead Developer", WIDGET_TEXT);
    AddWidget(path, "Jameriquiah: Developer", WIDGET_TEXT);
    AddWidget(path, "the-outcaster: Developer", WIDGET_TEXT);
    AddWidget(path, "TechnicallyComputers and NyxTheShield: Netcode Developers", WIDGET_TEXT);
    AddWidget(path, "VetriTheRetri: Former Decompilation Lead", WIDGET_TEXT);
    AddWidget(path, "MarioReincarnate: Decompilation Lead", WIDGET_TEXT);
    AddWidget(path, "Krix08: Playtesting", WIDGET_TEXT);
    AddWidget(path, "Bleee: Playtesting", WIDGET_TEXT);
    AddWidget(path, "Fray: Nrage Control Advising", WIDGET_TEXT);
    AddWidget(path, "ElBateSoli: Raphnet Playtesting", WIDGET_TEXT);
    AddWidget(path, "MickleMoose: C Modding Toolkit Developer", WIDGET_TEXT);
    AddWidget(path, "Zorkats: C Modding Documentation", WIDGET_TEXT);


#if !defined(__ANDROID__)
    // BUILT-IN UPDATER — hidden on Android. App updates come through the
    // Play Store on mobile; a curl-driven GitHub-releases updater can't
    // replace a system-managed installation, and the curl shell-out
    // (Updater.cpp) isn't built on Android in the first place.
    // The background check still fires when the menu loads
    ssb64::enhancements::CheckForUpdatesAsync(false);

    AddWidget(path, "Updates", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Version: " + std::string(BATTLESHIP_CURRENT_VERSION), WIDGET_TEXT);

    // 1. "Checking for updates..." Text
    AddWidget(path, "Checking for updates...", WIDGET_TEXT)
    .PreFunc([](WidgetInfo& info) {
        info.isHidden = !ssb64::enhancements::IsCheckingForUpdates();
    });

    // 2. "Download Update" Button
    AddWidget(path, "Download Update", WIDGET_BUTTON)
    .RaceDisable(false)
    .PreFunc([](WidgetInfo& info) {
        info.isHidden = ssb64::enhancements::IsCheckingForUpdates() ||
        ssb64::enhancements::IsDownloading() ||
        ssb64::enhancements::IsDownloadComplete() ||
        !ssb64::enhancements::IsUpdateAvailable();

        if (!info.isHidden) {
            info.name = "Download Update: " + ssb64::enhancements::GetLatestVersion();
        }
    })
    .Callback([](WidgetInfo&) {
        ssb64::enhancements::StartGameUpdate();
    });

    // 3. "Downloading... XX%" Text (Also handles the "Complete" text)
    AddWidget(path, "Download Status", WIDGET_TEXT)
    .PreFunc([](WidgetInfo& info) {
        info.isHidden = !ssb64::enhancements::IsDownloading() && !ssb64::enhancements::IsDownloadComplete();
        if (!info.isHidden) {
            // Pipe the curl progress bar or completion string straight to the UI
            info.name = ssb64::enhancements::GetDownloadStatus();
        }
    });

    // 4. Update result text
    AddWidget(path, "Update Status", WIDGET_TEXT)
    .PreFunc([](WidgetInfo& info) {
        const std::string status = ssb64::enhancements::GetUpdateStatus();
        info.isHidden = ssb64::enhancements::IsCheckingForUpdates() ||
                        ssb64::enhancements::IsUpdateAvailable() ||
                        status.empty();
        if (!info.isHidden) {
            info.name = status;
        }
    });

    // 5. "Check for Updates" Manual Button
    AddWidget(path, "Check for Updates", WIDGET_BUTTON)
    .RaceDisable(false)
    .PreFunc([](WidgetInfo& info) {
        // Hide this button while checking or downloading so they can't spam it
        info.isHidden = ssb64::enhancements::IsCheckingForUpdates() ||
        ssb64::enhancements::IsDownloading() ||
        ssb64::enhancements::IsDownloadComplete();
    })
    .Callback([](WidgetInfo&) {
        // Pass true to bypass the single-session lock
        ssb64::enhancements::CheckForUpdatesAsync(true);
    });
#endif // !__ANDROID__ (Updater)
}

void PortMenu::AddMenuElements() {
    AddMenuSettings();
    AddMenuAssets();
#ifndef DISABLE_SCRIPTING
    AddMenuMods();
#endif
    AddMenuAbout();

    if (CVarGetInteger(CVAR_SETTING("Menu.SidebarSearch"), 0)) {
        InsertSidebarSearch();
    }

    for (auto& initFunc : MenuInit::GetInitFuncs()) {
        initFunc();
    }

    mMenuElementsInitialized = true;
}

void PortMenu::InitElement() {
    if (!mMenuElementsInitialized) {
        AddMenuElements();
    }

    Ship::Menu::InitElement();
    disabledMap = {
        { DISABLE_FOR_NO_VSYNC,
          { [](disabledInfo&) -> bool { return !Ship::Context::GetInstance()->GetWindow()->CanDisableVerticalSync(); },
            "Disabling VSync not supported" } },
        { DISABLE_FOR_NO_WINDOWED_FULLSCREEN,
          { [](disabledInfo&) -> bool {
               return !Ship::Context::GetInstance()->GetWindow()->SupportsWindowedFullscreen();
           },
            "Windowed Fullscreen not supported" } },
        { DISABLE_FOR_NO_MULTI_VIEWPORT,
          { [](disabledInfo&) -> bool {
               return !Ship::Context::GetInstance()->GetWindow()->GetGui()->SupportsViewports();
           },
            "Multi-viewports not supported" } },
        { DISABLE_FOR_NOT_DIRECTX,
          { [](disabledInfo&) -> bool {
               return Ship::Context::GetInstance()->GetWindow()->GetWindowBackend() !=
                      Ship::WindowBackend::FAST3D_DXGI_DX11;
           },
            "Available only on DirectX" } },
        { DISABLE_FOR_DIRECTX,
          { [](disabledInfo&) -> bool {
               return Ship::Context::GetInstance()->GetWindow()->GetWindowBackend() ==
                      Ship::WindowBackend::FAST3D_DXGI_DX11;
           },
            "Not available on DirectX" } },
        { DISABLE_FOR_MATCH_REFRESH_RATE_ON,
          { [](disabledInfo&) -> bool { return CVarGetInteger(CVAR_SETTING("MatchRefreshRate"), 0); },
            "Match Refresh Rate is enabled" } },
        { DISABLE_FOR_ADVANCED_RESOLUTION_ON,
          { [](disabledInfo&) -> bool { return CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0); },
            "Advanced Resolution enabled" } },
        { DISABLE_FOR_VERTICAL_RES_TOGGLE_ON,
          { [](disabledInfo&) -> bool {
               return CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
           },
            "Vertical Resolution Toggle enabled" } },
        { DISABLE_FOR_LOW_RES_MODE_ON,
          { [](disabledInfo&) -> bool { return CVarGetInteger(CVAR_LOW_RES_MODE, 0); }, "N64 mode enabled" } },
        { DISABLE_FOR_ADVANCED_RESOLUTION_OFF,
          { [](disabledInfo&) -> bool { return !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0); },
            "Advanced Resolution is disabled" } },
        { DISABLE_FOR_VERTICAL_RESOLUTION_OFF,
          { [](disabledInfo&) -> bool {
               return !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
           },
            "Vertical Resolution Toggle is off" } },
    };
}

void PortMenu::UpdateElement() {
    Ship::Menu::UpdateElement();
}

void PortMenu::Draw() {
    Ship::Menu::Draw();
}

void PortMenu::DrawElement() {
    if (mMenuElementsInitialized) {
        Ship::Menu::DrawElement();
    }

    // Re-extract confirmation: render as an in-context ImGui modal rather
    // than calling SDL_ShowSimpleMessageBox, which would nest a blocking
    // native modal (NSAlert / Win32 MessageBox) inside the in-progress
    // ImGui frame and the GPU command encode for that frame. ImGui's own
    // popup runs in the same context with no thread/native-modal nesting.
    if (mShowReextractMessage) {
        ImGui::OpenPopup("Re-extract scheduled");
        mShowReextractMessage = false;
    }
    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Re-extract scheduled", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped(
            SSB64_O2R_NAME " will be regenerated from your ROM the next time the game launches.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    RenderPostProcessLowResWarnModal();
#if !defined(__ANDROID__)
    RenderShaderPackModal();
#endif
}

} // namespace ssb64

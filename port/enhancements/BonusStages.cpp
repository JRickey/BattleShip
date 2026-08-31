#include "enhancements.h"

#include <libultraship/bridge/consolevariablebridge.h>

// Issue #267 — the three port-added stages (Final Destination, Metal Cavern,
// Battlefield) live on their own stage-select page. This toggle hides that
// page for a vanilla stage roster. Default ON: the bonus stages are a
// headline port feature; hiding them is the opt-in.
//
// Consumed by mnMapsCheckLocked (decomp/src/mn/mnmaps/mnmaps.c): when the
// toggle is off the three gkinds report as locked, which in one stroke skips
// their icons, refuses page-jump landings, excludes them from random stage
// selection, and reroutes a restored cursor back to page 0.
constexpr const char* kBonusStagesCVar = "gEnhancements.BonusStagesEnabled";

extern "C" int port_enhancement_bonus_stages_enabled(void) {
    return CVarGetInteger(kBonusStagesCVar, 1) != 0;
}

namespace ssb64 {
namespace enhancements {

const char* BonusStagesCVarName() {
    return kBonusStagesCVar;
}

} // namespace enhancements
} // namespace ssb64

#include "enhancements.h"
#include <libultraship/bridge/consolevariablebridge.h>

namespace ssb64 {
    namespace enhancements {

        const char* BootToVSCSSCVarName() {
            return "gEnhancements.BootToVSCSS";
        }

    } // namespace enhancements
} // namespace ssb64

extern "C" int port_enhancement_boot_to_vs_css(void) {
    return CVarGetInteger(ssb64::enhancements::BootToVSCSSCVarName(), 0) != 0;
}

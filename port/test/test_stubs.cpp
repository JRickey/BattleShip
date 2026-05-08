/**
 * test_stubs.cpp — Linker stubs for game-side externs the bridge references
 * but the harness's load path never meaningfully exercises.
 *
 * gmColScriptsLinkRelocTargets and port_capture_release_all are called by
 * lbRelocInitSetup. The harness invokes lbRelocInitSetup at the start of
 * each load to wire up status buffers + an extern heap, so these symbols
 * must resolve. They no-op safely:
 *   - gmColScriptsLinkRelocTargets: re-binds GC col-script reloc targets.
 *     Test loads don't read those targets back, so no-op is correct.
 *   - port_capture_release_all: drops FB-mirror registrations. Tests don't
 *     register any, so no-op is correct.
 *
 * syDebugPrintf and scManagerRunPrintGObjStatus are entered ONLY when the
 * status buffer overflows (an infinite-spin path). If reached, the test is
 * exceeding the buffer cap the harness allocated — a real bug, so abort.
 */

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern "C" {

void gmColScriptsLinkRelocTargets(void)
{
    /* No-op for tests. */
}

void port_capture_release_all(void)
{
    /* No-op for tests. */
}

void syDebugPrintf(const char *fmt, ...)
{
    std::fprintf(stderr, "test_stubs: syDebugPrintf — fmt='%s'\n  ", fmt ? fmt : "(null)");
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::abort();
}

void scManagerRunPrintGObjStatus(void)
{
    std::fprintf(stderr, "test_stubs: scManagerRunPrintGObjStatus reached — status buffer overflow path entered\n");
    std::abort();
}

} // extern "C"

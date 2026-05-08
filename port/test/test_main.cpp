/**
 * test_main.cpp — GTest entry point with controlled Ship::Context teardown.
 *
 * Mirrors the rationale in port.cpp's PortShutdown: Ship::Context owns
 * shared_ptrs to objects that destruct via spdlog::* logging calls. If
 * Ship::Context is held by a function-local static (the natural way to make
 * a singleton in C++), its destructor runs at program-exit static teardown
 * AFTER spdlog has shut down, raising "mutex lock failed: Invalid argument"
 * inside the Fast3dWindow destructor's SPDLOG_DEBUG and SIGABRTing the
 * process — the test's own assertions all pass, but ctest sees the SEGFAULT
 * exit and reports the test as failed.
 *
 * Workaround: the GoogleTest Environment registered here calls
 * ArchiveSession::Shutdown() in TearDown(), running BEFORE main returns
 * and BEFORE static destructors fire.
 */

#include "reloc_harness.h"

#include <gtest/gtest.h>

#include <execinfo.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>

namespace {

class HarnessEnvironment : public ::testing::Environment {
public:
    void SetUp() override {}
    void TearDown() override {
        harness::ArchiveSession::Shutdown();
    }
};

} // namespace

extern "C" void crash_handler(int sig)
{
    std::fprintf(stderr, "\n[harness] caught signal %d (SIGSEGV=%d) — backtrace:\n", sig, SIGSEGV); std::fflush(stderr);
    void *frames[64];
    int n = backtrace(frames, 64);
    backtrace_symbols_fd(frames, n, 2 /*stderr*/);
    std::fflush(stderr);
    _exit(128 + sig);
}

int main(int argc, char **argv)
{
    struct sigaction sa{};
    sa.sa_handler = &crash_handler;
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new HarnessEnvironment());
    return RUN_ALL_TESTS();
}

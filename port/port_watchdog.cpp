/**
 * port_watchdog.cpp — see port_watchdog.h for architecture notes.
 */

#include "port_watchdog.h"
#include "port_log.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#if !defined(_WIN32) && !defined(__SWITCH__)
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <unistd.h>
#endif

extern "C" {
unsigned char port_diag_get_scene_curr(void);
unsigned char port_diag_get_scene_prev(void);
const char *port_diag_get_scene_name(unsigned char id);
int port_log_get_fd(void);
}

/* GFX stale-DL diag dump (libultraship public API — fast/interpreter.h).
 * Walks the recent-DL-pushes + recent-segment-writes ring buffers and
 * writes them to spdlog. Called below from the SIGSEGV handler. */
#include "fast/interpreter.h"

namespace {

std::atomic<uint64_t> sYieldCount{0};
std::atomic<uint64_t> sFrameCount{0};
std::atomic<int>      sResumeActiveThreadId{-1};
std::atomic<uint64_t> sResumeStartMs{0};
std::atomic<bool>     sShutdown{false};
std::atomic<bool>     sStarted{false};
std::thread           sWatchdogThread;

#if !defined(_WIN32) && !defined(__SWITCH__)
pthread_t             sMainThread;
std::atomic<bool>     sBacktraceRequested{false};
std::atomic<bool>     sBacktraceDone{false};

/* Forward declaration — definition below, shared between the crash-
 * context dumper and the legacy signal-info formatter. */
size_t FormatHex(char *buf, uintptr_t value);

/* Write a byte string to both stderr and the log file fd (if open).
 * Async-signal-safe: write(2) is in the POSIX signal-safe list. */
void WriteBoth(const char *buf, size_t n) {
    write(STDERR_FILENO, buf, n);
    int log_fd = port_log_get_fd();
    if (log_fd >= 0) write(log_fd, buf, n);
}

/* Dump a backtrace to stderr and the log file. Async-signal-safe:
 * backtrace() and backtrace_symbols_fd() are listed as signal-safe on
 * glibc and Darwin; the former uses no heap, the latter writes directly. */
void DumpBacktraceBoth() {
    constexpr int kMaxFrames = 64;
    void *frames[kMaxFrames];
    int n = backtrace(frames, kMaxFrames);
    const char msg[] = "SSB64: ---- main-thread backtrace ----\n";
    WriteBoth(msg, sizeof(msg) - 1);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    int log_fd = port_log_get_fd();
    if (log_fd >= 0) backtrace_symbols_fd(frames, n, log_fd);
    const char end[] = "SSB64: ---- end backtrace ----\n";
    WriteBoth(end, sizeof(end) - 1);
}

/* Registers captured from the signal's ucontext at the fault site.
 * libc's backtrace() unwinds via libunwind/DWARF CFI, which can't cross
 * the kernel-inserted signal frame on Darwin — the first user frame it
 * can recover is the signal handler's caller (i.e. _sigtramp's caller,
 * not the faulting code). Walking the FP chain from the ucontext
 * directly sidesteps that and gets the real fault frame + its callers. */
struct CrashRegs {
    bool valid;
    uintptr_t pc;
    uintptr_t lr;
    uintptr_t sp;
    uintptr_t fp;
    uintptr_t x[6];  /* x0..x5 — first 6 args on AArch64 and x86_64-SysV */
};

CrashRegs ExtractCrashRegs(void *uc_v) {
    CrashRegs r = {};
    if (uc_v == nullptr) return r;
#if defined(__APPLE__) && defined(__aarch64__)
    auto *uc = static_cast<ucontext_t *>(uc_v);
    if (uc->uc_mcontext == nullptr) return r;
    r.pc = (uintptr_t)uc->uc_mcontext->__ss.__pc;
    r.lr = (uintptr_t)uc->uc_mcontext->__ss.__lr;
    r.sp = (uintptr_t)uc->uc_mcontext->__ss.__sp;
    r.fp = (uintptr_t)uc->uc_mcontext->__ss.__fp;
    for (int i = 0; i < 6; i++) r.x[i] = (uintptr_t)uc->uc_mcontext->__ss.__x[i];
    r.valid = true;
#elif defined(__linux__) && defined(__aarch64__)
    auto *uc = static_cast<ucontext_t *>(uc_v);
    r.pc = (uintptr_t)uc->uc_mcontext.pc;
    r.lr = (uintptr_t)uc->uc_mcontext.regs[30];
    r.sp = (uintptr_t)uc->uc_mcontext.sp;
    r.fp = (uintptr_t)uc->uc_mcontext.regs[29];
    for (int i = 0; i < 6; i++) r.x[i] = (uintptr_t)uc->uc_mcontext.regs[i];
    r.valid = true;
#elif defined(__APPLE__) && defined(__x86_64__)
    auto *uc = static_cast<ucontext_t *>(uc_v);
    if (uc->uc_mcontext == nullptr) return r;
    r.pc = (uintptr_t)uc->uc_mcontext->__ss.__rip;
    r.lr = 0;
    r.sp = (uintptr_t)uc->uc_mcontext->__ss.__rsp;
    r.fp = (uintptr_t)uc->uc_mcontext->__ss.__rbp;
    r.x[0] = (uintptr_t)uc->uc_mcontext->__ss.__rdi;
    r.x[1] = (uintptr_t)uc->uc_mcontext->__ss.__rsi;
    r.x[2] = (uintptr_t)uc->uc_mcontext->__ss.__rdx;
    r.x[3] = (uintptr_t)uc->uc_mcontext->__ss.__rcx;
    r.x[4] = (uintptr_t)uc->uc_mcontext->__ss.__r8;
    r.x[5] = (uintptr_t)uc->uc_mcontext->__ss.__r9;
    r.valid = true;
#elif defined(__linux__) && defined(__x86_64__)
    auto *uc = static_cast<ucontext_t *>(uc_v);
    r.pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
    r.lr = 0;
    r.sp = (uintptr_t)uc->uc_mcontext.gregs[REG_RSP];
    r.fp = (uintptr_t)uc->uc_mcontext.gregs[REG_RBP];
    r.x[0] = (uintptr_t)uc->uc_mcontext.gregs[REG_RDI];
    r.x[1] = (uintptr_t)uc->uc_mcontext.gregs[REG_RSI];
    r.x[2] = (uintptr_t)uc->uc_mcontext.gregs[REG_RDX];
    r.x[3] = (uintptr_t)uc->uc_mcontext.gregs[REG_RCX];
    r.x[4] = (uintptr_t)uc->uc_mcontext.gregs[REG_R8];
    r.x[5] = (uintptr_t)uc->uc_mcontext.gregs[REG_R9];
    r.valid = true;
#else
    (void)uc_v;
#endif
    return r;
}

size_t AppendLabel(char *buf, size_t pos, const char *s) {
    while (*s) buf[pos++] = *s++;
    return pos;
}

void WriteCrashRegs(const CrashRegs &r) {
    if (!r.valid) return;
    char line[256];

    const char hdr[] = "SSB64: ---- registers ----\n";
    WriteBoth(hdr, sizeof(hdr) - 1);

    size_t pos = 0;
    pos = AppendLabel(line, pos, "SSB64: pc=");
    pos += FormatHex(line + pos, r.pc);
    pos = AppendLabel(line, pos, " lr=");
    pos += FormatHex(line + pos, r.lr);
    pos = AppendLabel(line, pos, " sp=");
    pos += FormatHex(line + pos, r.sp);
    pos = AppendLabel(line, pos, " fp=");
    pos += FormatHex(line + pos, r.fp);
    line[pos++] = '\n';
    WriteBoth(line, pos);

    pos = 0;
    pos = AppendLabel(line, pos, "SSB64:");
    for (int i = 0; i < 6; i++) {
        char label[6] = {' ', 'x', (char)('0' + i), '=', 0};
        pos = AppendLabel(line, pos, label);
        pos += FormatHex(line + pos, r.x[i]);
    }
    line[pos++] = '\n';
    WriteBoth(line, pos);
}

/* Walk the AArch64/x86_64 frame-pointer chain starting from the fault
 * context. Seed the trace with PC (the actual faulting instruction),
 * then LR on AArch64 (caller's return addr), then walk saved FPs.
 *
 * Frame layout on both AArch64 and x86_64 with FP frames enabled:
 *   [fp + 0]  = saved FP of caller
 *   [fp + 8]  = saved return addr (LR/x30 on AArch64, pushed return on x86_64)
 *
 * Sanity checks bail out if the chain looks corrupt — no heap, no libc
 * beyond what backtrace_symbols_fd uses. */
int WalkFPChain(const CrashRegs &r, void **frames, int max_frames) {
    if (!r.valid || max_frames <= 0) return 0;
    int n = 0;
    if (r.pc != 0) frames[n++] = (void *)r.pc;
#if defined(__aarch64__)
    if (r.lr != 0 && n < max_frames) frames[n++] = (void *)r.lr;
#endif
    uintptr_t fp = r.fp;
    while (n < max_frames && fp != 0) {
        /* Basic sanity: FP must be aligned and not absurdly small. */
        if (fp < 0x1000 || (fp & 0x7) != 0) break;
        auto *fp_ptr = reinterpret_cast<uintptr_t *>(fp);
        uintptr_t saved_fp = fp_ptr[0];
        uintptr_t saved_ret = fp_ptr[1];
        if (saved_ret == 0) break;
        frames[n++] = (void *)saved_ret;
        /* Stack grows down; next FP must be higher than current and
         * within a sane stride (coroutine stacks are a few MB). */
        if (saved_fp <= fp) break;
        if (saved_fp > fp + 0x01000000) break;
        fp = saved_fp;
    }
    return n;
}

void DumpBacktraceFromContext(void *uc_v) {
    CrashRegs r = ExtractCrashRegs(uc_v);
    WriteCrashRegs(r);

    const char hdr[] = "SSB64: ---- main-thread backtrace (fault context) ----\n";
    WriteBoth(hdr, sizeof(hdr) - 1);

    constexpr int kMaxFrames = 64;
    void *frames[kMaxFrames];
    int n = 0;
    if (r.valid) n = WalkFPChain(r, frames, kMaxFrames);
    if (n == 0) n = backtrace(frames, kMaxFrames);

    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    int log_fd = port_log_get_fd();
    if (log_fd >= 0) backtrace_symbols_fd(frames, n, log_fd);

    const char end[] = "SSB64: ---- end backtrace ----\n";
    WriteBoth(end, sizeof(end) - 1);
}

/* Signal handler: dumps main-thread backtrace (used for SIGUSR1). */
void BacktraceSignalHandler(int /*sig*/) {
    DumpBacktraceBoth();
    sBacktraceDone.store(true, std::memory_order_release);
}

/* Format an unsigned value as hex into buf[]. Returns number of bytes
 * written. buf must have room for 2 + 16 + 1 = at least 19 bytes for a
 * 64-bit value. Async-signal-safe: no libc calls. */
size_t FormatHex(char *buf, uintptr_t value) {
    static const char kHex[] = "0123456789abcdef";
    char tmp[32];
    int len = 0;
    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value != 0) {
            tmp[len++] = kHex[value & 0xF];
            value >>= 4;
        }
    }
    size_t out = 0;
    buf[out++] = '0';
    buf[out++] = 'x';
    for (int i = len - 1; i >= 0; i--) buf[out++] = tmp[i];
    return out;
}

/* Fatal-signal handler: prints signal name + fault address, dumps a
 * backtrace, then restores the default handler and re-raises so the
 * process exits with the original cause (core dump / proper exit code).
 *
 * Only async-signal-safe operations are used: write(2), backtrace(3),
 * backtrace_symbols_fd(3), signal(2), raise(3). */
void CrashSignalHandler(int sig, siginfo_t *info, void *ucontext) {
    const char *name = "SIGUNKNOWN";
    switch (sig) {
    case SIGSEGV: name = "SIGSEGV"; break;
    case SIGBUS:  name = "SIGBUS";  break;
    case SIGILL:  name = "SIGILL";  break;
    case SIGFPE:  name = "SIGFPE";  break;
    case SIGABRT: name = "SIGABRT"; break;
    }

    char line[128];
    size_t pos = 0;
    const char prefix[] = "SSB64: !!!! CRASH ";
    for (size_t i = 0; i < sizeof(prefix) - 1; i++) line[pos++] = prefix[i];
    for (const char *p = name; *p; p++) line[pos++] = *p;
    const char addr_s[] = " fault_addr=";
    for (size_t i = 0; i < sizeof(addr_s) - 1; i++) line[pos++] = addr_s[i];
    uintptr_t fa = info ? (uintptr_t)info->si_addr : 0;
    pos += FormatHex(line + pos, fa);
    line[pos++] = '\n';
    WriteBoth(line, pos);

    DumpBacktraceFromContext(ucontext);

    /* Dump the GFX stale-DL diag ring buffer (recent DL pushes + segment
     * writes, all classified by source memory range via the registered
     * Fast::AddressClassifier — see port_dl_ranges.cpp). This identifies
     * the upstream holder behind variant-5-style "walker ran past
     * registered DL range" crashes in gfx_step. NOT async-signal-safe
     * (uses spdlog) — accepts the deadlock risk because we're already
     * crashing and the diag is more valuable than perfect signal safety. */
    Fast::DumpDLDiag(info ? info->si_addr : nullptr,
                     "port_watchdog::CrashSignalHandler");

    /* Restore default handler and re-raise so the OS still produces the
     * normal termination behavior (core file, exit status). */
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, nullptr);
    raise(sig);
}
#endif

constexpr uint64_t kHangThresholdMs = 3000;
constexpr uint64_t kRepeatLogMs     = 2000;

uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

const char *ThreadLabel(int id) {
    /* Thread IDs are set by osCreateThread throughout sys/main.c and friends.
     * Values match dSYMainIdleStackArg, scheduler init, etc. */
    switch (id) {
    case -1: return "none";
    case 1:  return "idle";
    case 3:  return "scheduler";
    case 4:  return "audio";
    case 5:  return "game";
    case 6:  return "controller";
    default: return "service";
    }
}

void WatchdogLoop() {
    uint64_t last_yield = sYieldCount.load();
    uint64_t last_frame = sFrameCount.load();
    uint64_t last_yield_change_ms = NowMs();
    uint64_t last_frame_change_ms = NowMs();
    uint64_t last_log_ms = 0;

    while (!sShutdown.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        uint64_t now = NowMs();
        uint64_t yc  = sYieldCount.load(std::memory_order_relaxed);
        uint64_t fc  = sFrameCount.load(std::memory_order_relaxed);

        if (yc != last_yield) { last_yield = yc; last_yield_change_ms = now; }
        if (fc != last_frame) { last_frame = fc; last_frame_change_ms = now; }

        /* Grace period — don't fire alarms during early boot before the
         * first frame has been pumped. */
        if (fc == 0) continue;

        uint64_t since_yield_ms = now - last_yield_change_ms;
        uint64_t since_frame_ms = now - last_frame_change_ms;

        /* Require both liveness counters to stall. A running coroutine
         * could yield tens of thousands of times per frame; a frame that
         * takes >3s without any yield is a genuine hang. */
        if (since_frame_ms > kHangThresholdMs && since_yield_ms > kHangThresholdMs) {
            if (now - last_log_ms < kRepeatLogMs) continue;
            last_log_ms = now;

            int active_id = sResumeActiveThreadId.load(std::memory_order_relaxed);
            uint64_t resume_start = sResumeStartMs.load(std::memory_order_relaxed);
            uint64_t resume_elapsed_ms =
                (active_id >= 0 && resume_start > 0) ? (now - resume_start) : 0;

            unsigned char scene      = port_diag_get_scene_curr();
            unsigned char scene_prev = port_diag_get_scene_prev();

            /* Write via both port_log (persistent) and stderr (terminal). */
            const char *fmt =
                "SSB64: WATCHDOG HANG since_frame=%llums since_yield=%llums "
                "active_tid=%d(%s) active_elapsed=%llums "
                "scene=%u(%s) prev=%u(%s) frame=%llu yield_count=%llu\n";

            port_log(fmt,
                     (unsigned long long)since_frame_ms,
                     (unsigned long long)since_yield_ms,
                     active_id, ThreadLabel(active_id),
                     (unsigned long long)resume_elapsed_ms,
                     (unsigned)scene, port_diag_get_scene_name(scene),
                     (unsigned)scene_prev, port_diag_get_scene_name(scene_prev),
                     (unsigned long long)fc,
                     (unsigned long long)yc);

            std::fprintf(stderr, fmt,
                         (unsigned long long)since_frame_ms,
                         (unsigned long long)since_yield_ms,
                         active_id, ThreadLabel(active_id),
                         (unsigned long long)resume_elapsed_ms,
                         (unsigned)scene, port_diag_get_scene_name(scene),
                         (unsigned)scene_prev, port_diag_get_scene_name(scene_prev),
                         (unsigned long long)fc,
                         (unsigned long long)yc);
            std::fflush(stderr);

#if !defined(_WIN32) && !defined(__SWITCH__)
            /* Ask the main thread to dump its own backtrace via SIGUSR1. One
             * dump per hang is enough — clearing sBacktraceDone lets the next
             * distinct hang dump again. */
            bool done_expected = true;
            bool already_requested = false;
            sBacktraceDone.compare_exchange_strong(done_expected, false);
            if (!sBacktraceRequested.compare_exchange_strong(already_requested, true)) {
                /* A request is in flight; don't pile on. */
            } else {
                pthread_kill(sMainThread, SIGUSR1);
                /* Allow the signal handler up to 500ms to complete. */
                for (int i = 0; i < 10 && !sBacktraceDone.load(); i++) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                sBacktraceRequested.store(false);
            }
#endif
        }
    }
}

} // namespace

extern "C" void port_watchdog_init(void) {
    bool expected = false;
    if (!sStarted.compare_exchange_strong(expected, true)) return;
#if !defined(_WIN32) && !defined(__SWITCH__)
    sMainThread = pthread_self();

    /* Alternate signal stack so a stack-overflow SIGSEGV still has room to
     * run the crash handler. SIGSTKSZ is small on some platforms — bump it. */
    static uint8_t s_altstack[64 * 1024];
    stack_t ss;
    ss.ss_sp = s_altstack;
    ss.ss_size = sizeof(s_altstack);
    ss.ss_flags = 0;
    sigaltstack(&ss, nullptr);

    struct sigaction sa;
    sa.sa_handler = BacktraceSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);

    /* Fatal-signal handlers: dump backtrace before the process dies.
     *
     * When AddressSanitizer is compiled in, skip SIGSEGV/SIGBUS so ASan's
     * own handler (installed in __asan_init via preinit) catches the fault
     * and produces its full report — alloc/free stacks, shadow-byte map,
     * the works. Our handler only prints registers + a backtrace, which
     * loses the diagnostic value that's the entire reason ASan is in the
     * build. SIGILL/SIGFPE/SIGABRT remain ours: ASan doesn't intercept
     * them, and SIGABRT is what ASan itself raises after printing its
     * report (so chaining is fine — by then the report is already out). */
    struct sigaction csa;
    sigemptyset(&csa.sa_mask);
    csa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    csa.sa_sigaction = CrashSignalHandler;
#if !defined(__SANITIZE_ADDRESS__) && !(defined(__has_feature) && __has_feature(address_sanitizer))
    sigaction(SIGSEGV, &csa, nullptr);
    sigaction(SIGBUS,  &csa, nullptr);
#endif
    sigaction(SIGILL,  &csa, nullptr);
    sigaction(SIGFPE,  &csa, nullptr);
    sigaction(SIGABRT, &csa, nullptr);
#endif
    sWatchdogThread = std::thread(WatchdogLoop);
    port_log("SSB64: watchdog started (hang threshold=%llums)\n",
             (unsigned long long)kHangThresholdMs);
}

extern "C" void port_watchdog_shutdown(void) {
    if (!sStarted.load()) return;
    sShutdown.store(true, std::memory_order_release);
    if (sWatchdogThread.joinable()) {
        sWatchdogThread.join();
    }
}

extern "C" void port_watchdog_note_yield(void) {
    sYieldCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void port_watchdog_note_resume_start(int thread_id) {
    sResumeStartMs.store(NowMs(), std::memory_order_relaxed);
    sResumeActiveThreadId.store(thread_id, std::memory_order_relaxed);
}

extern "C" void port_watchdog_note_resume_end(int /*thread_id*/) {
    sResumeActiveThreadId.store(-1, std::memory_order_relaxed);
    sResumeStartMs.store(0, std::memory_order_relaxed);
}

extern "C" void port_watchdog_note_frame_end(void) {
    sFrameCount.fetch_add(1, std::memory_order_relaxed);
}

#if !defined(_WIN32) && !defined(__SWITCH__)
extern "C" void port_dump_backtrace(void) {
    DumpBacktraceBoth();
}
#else
extern "C" void port_dump_backtrace(void) { /* stub on Windows */ }
#endif

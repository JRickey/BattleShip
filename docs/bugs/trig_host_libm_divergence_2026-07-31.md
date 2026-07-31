# Game trig routed to host libm instead of libultra's Cody-Waite polynomials

**Date:** 2026-07-31
**Status:** RESOLVED (branch `agent/cody-waite`)
**Class:** silent behavioral divergence from hardware (physics/animation trig), not a crash

## Symptom

No visible crash — the port silently computed all game trig with the host's
`libm` instead of the N64's own implementation. Every host (glibc, musl, macOS
libSystem, MSVC UCRT) rounds differently from SGI's polynomial, so:

- Physics/trajectory math (knockback vectors in `ftcommondamage.c`, fighter
  specials, camera, particles — ~185 call sites) diverged from console by a
  few ulps per call, compounding over frames. Replays/TAS-style determinism
  across platforms was impossible: Linux, macOS, and Windows each produced
  slightly different trajectories.
- N64 edge behaviors were absent: libultra's `sinf`/`cosf` return `0.0f`
  outright for `|x| >= 2^28` (the "give up" branch) where hosts return a
  properly reduced value, and `sinf` returns `x` verbatim below `2^-12`.
  Game code written against console behavior saw different results.

## Root cause

Three stacked causes:

1. **The real sources were never compiled.** The decomp carries the original
   SGI `decomp/src/libultra/gu/sinf.c` / `cosf.c` — double-precision
   polynomial evaluation with Cody-Waite argument reduction (the same files
   the SM64 PC port revived). The CMake libultra allowlist
   (`SSB64_LIBULTRA_PORT`) only picked the four `mtx*`/`normalize` files;
   `port/stubs/libc_compat.c` forwarded `__sinf`/`__cosf` to host
   `sinf`/`cosf` instead.
2. **Endianness bug lay in wait.** The sources build their double constants
   from `{hi, lo}` 32-bit word pairs via the `du` union in
   `decomp/include/PR/guint.h` — a layout that assumes big-endian MIPS. On
   little-endian hosts every coefficient reassembled with its words swapped
   (garbage / inf results), which is very likely *why* the stub forwarding
   existed in the first place. Positional initializers fill ascending
   addresses regardless of member names, so swapping the union members alone
   does not fix it — the initializer itself must flip.
3. **`__libm_qnan_f` only existed as MIPS assembly** (`gu/libm_vals.s`), so
   the NaN branch of both functions couldn't link on the port.

## Fix

All on branch `agent/cody-waite`; decomp changes on the fork's
`port-patches` branch.

- `decomp/include/PR/guint.h`: `du` union members swap on little-endian
  hosts, and a new `DU_INIT(hi, lo)` macro emits `{lo, hi}` there. On
  big-endian / IDO the `#else` branches preprocess to the original tokens
  (verified with `gcc -E -U__BYTE_ORDER__`), so the matching N64 build is
  byte-identical.
- `decomp/src/libultra/gu/sinf.c`, `cosf.c`: constant tables wrapped in
  `DU_INIT(...)` — pure token substitution, no algorithm change.
- `CMakeLists.txt`: both files added to `SSB64_LIBULTRA_PORT`, compiled with
  `-ffp-contract=off` (AArch64 would otherwise contract `a*b+c` into `fmadd`
  and perturb the last ulp vs. N64/x86) and `-fno-strict-aliasing` (the
  sources type-pun floats through `int*`). `ssb64_game` additionally gets
  `-fno-builtin-sinf -fno-builtin-cosf` so GCC/Clang can't constant-fold or
  sincos-fuse the bare call sites past our definitions.
- `port/stubs/libc_compat.c`: host-libm wrappers deleted; defines
  `f32 __libm_qnan_f = NAN;`. Deliberate deviation: the N64 bit pattern
  `0x7F810000` is a quiet NaN under MIPS's inverted quiet-bit convention but
  a *signaling* NaN on x86/ARM; callers only propagate or `x != x` test it,
  so the payload is unobservable and a host qNaN is safer.

## Linkage notes (read before touching)

- The SGI sources carry `#pragma weak sinf = __sinf` / `cosf = __cosf`. On
  GCC/Clang these weak aliases capture the ~16 bare `sinf()`/`cosf()` decomp
  call sites (e.g. `sys/matrix.c` `guPerspectiveF`, `ftphysics.c`) — exactly
  how libultra linked on hardware. Because the Linux link uses
  `-Wl,-export-dynamic` (TCC mod support), the aliases also interpose host
  libm for shared libraries loaded into the process and for mods; the
  polynomial is accurate to ~3.6e-8 absolute so this is benign, and mods
  doing game math arguably *want* game semantics. If it ever bites, hide the
  aliases with a version script rather than dropping the pragma.
- **MSVC ignores `#pragma weak`**: on Windows the ~169 `__sinf`/`__cosf`
  sites get the polynomial, but the 16 bare `sinf`/`cosf` sites keep UCRT.
  Windows is currently untested anyway; if it matters later, remap them via
  `#define` under `#ifdef PORT` or a linker `/ALTERNATENAME` shim.

## Verification

- Standalone harness (scratchpad `trig_harness.c`) compiling the two decomp
  sources with the build's exact flags, referenced against double-precision
  `sin()`/`cos()` (deliberately *not* `sinf`, which the weak alias captures
  inside the harness too): max abs error 3.6e-8 across `[-4,4]`, `[-360,360]`,
  `[-4e4,4e4]`, `[-2.6e8,2.6e8]`; give-up-to-`0.0f` at/above `2^28`;
  `sinf(1e-5f)==1e-5f` identity; `cosf(0)==1.0f`; `sinf(pi/2)==1.0f`; NaN in
  → NaN out. All pass.
- `nm` on the built binary: `sinf`/`cosf` present as weak aliases at the
  same address as `__sinf`/`__cosf` from `sinf.c.o`/`cosf.c.o`;
  `libc_compat.c.o` no longer defines them.
- Full Debug build + boot smoke test on Linux.

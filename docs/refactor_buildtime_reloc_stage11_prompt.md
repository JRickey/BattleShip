# Stage 11 next-agent prompt

Drop this into a fresh `claude` session opened in
`/Users/jackrickey/Dev/ssb64-port/.claude/worktrees/buildtime-reloc`.
The agent should be able to pick up Stage 11 with this prompt alone.

---

```
Pick up the build-time reloc refactor at Stage 11.

Worktree: .claude/worktrees/buildtime-reloc (branch agent/buildtime-reloc)
Plan:     docs/refactor_buildtime_reloc_plan.md
Handoff:  docs/refactor_buildtime_reloc_handoff.md  ← read this first
Backlog:  docs/refactor_buildtime_reloc_followups.md  ← Stage 10 leftovers

Stages 1–10 plus follow-ups A (pixel-buffer enumeration), B (drift-gate
sub-resource validator), and C (mods scanner determinism) are landed.
Stage 11 trims the runtime byteswap/halfswap helpers that v3 archives
no longer exercise.

Sanity check before any changes (must be green):

    ./build/port/test/port_reloc_regen --all --check --check-subres --quiet
    # expected: unchanged=2132, mismatch=0, validated=4048, failed=0

Two ADRs own carve-outs that Stage 11 must NOT delete — read them
before any rm:

  - docs/decision_aobjevent32_runtime_walker_2026-05-09.md
    (Stage 7: AObjEvent32 unhalfswap walker stays runtime-side; bit 9
    of processing_flags reserved for a future torch capture.)
  - docs/decision_4c_sprite_codec_runtime_2026-05-09.md
    (Stage 8: portDeswizzleDecodedSprite4c stays runtime-side.)

Scope for Stage 11 (per handoff):

  - port/bridge/lbreloc_byteswap.cpp shrinks from ~1500 LOC to ~150 LOC.
    DELETE the byteswap/struct/halfswap helpers (every flag they consult
    is set on v3 input). KEEP the heap-absolute trackers
    (sStructU16Fixups, portTextureCacheDeleteRange callers,
    portEvictStructFixupsInRange, etc.) — those are cache invalidation,
    not byteswap. Also keep portDeswizzleDecodedSprite4c (Stage 8 ADR).
    Don't `rm` the file; split the trackers into a thin header + delete
    the byteswap guts.
  - port/port_aobj_fixup.{h,cpp}: STAYS (Stage 7 ADR). Skip.
  - port/bridge/lbreloc_bridge.cpp::portRelocByteSwapBlob: every flag
    it consults is set, so the call is a no-op. Either delete the call
    or convert to a defensive assert(all flags set).

V0/V1/V2 reader decision (Stage 10 follow-up E — couples to Stage 11):

  Once the byteswap helpers are gone, V0/V1/V2 archives lose their
  fallback. Recommended path: DROP V0/V1/V2 entirely. Asset/torch
  coupling already forces re-extract on every torch SHA bump, so older
  archives are de-facto unsupported. Implementation: remove the
  V0/V1/V2 RegisterResourceFactory calls in port/port.cpp +
  port/test/reloc_harness.cpp, delete the matching factory classes in
  port/resource/RelocFileFactory.cpp. Pick this up while doing the
  byteswap trim, not separately.

Phased execution (CLAUDE.md Rule #2 — max 5 files per phase, gate
green between phases):

  1. Pass1/Pass2 helpers + call sites.
  2. Struct fixup helpers (one family per commit if you want
     bisectable history: SPRITE / BITMAP / MOBJSUB / STRUCT_U16 /
     STRUCT_U32 / FTATTRIBUTES).
  3. Halfswap helpers.
  4. portRelocByteSwapBlob delete-or-assert.
  5. V0/V1/V2 factory drop (follow-up E) — last, since the helpers
     they relied on are already gone.

After each phase:
  - cmake --build build --target ssb64 -j 4
  - cmake --build build --target port_reloc_regen -j 4
  - ./build/port/test/port_reloc_regen --all --check --check-subres --quiet
  - Optionally boot the binary and run the attract chain to catch
    anything the gate doesn't cover.

Don't skip the sanity check between phases. The drift gate has been
the load-bearing guard across the whole refactor — every Stage-N
change has left it green, and Stage 11 is the largest deletion yet.

Submodule pointers (push when ready for review):
    git -C torch        push origin ssb64-buildtime-reloc
    git -C libultraship push origin ssb64-buildtime-reloc

If Stage 11 lands cleanly, the next pivot is Stage 12 (productize
the mods/ scanner: menu UX + sub-resource discovery) or any of the
remaining D, F, G, H, I, J, K, L follow-ups in the backlog doc.
```

# Stage 12 next-agent prompt

Drop this into a fresh `claude` session opened in
`/Users/jackrickey/Dev/ssb64-port/.claude/worktrees/buildtime-reloc`.
The agent should be able to pick up Stage 12 with this prompt alone.

---

```
Pick up the build-time reloc refactor at Stage 12.

Worktree: .claude/worktrees/buildtime-reloc (branch agent/buildtime-reloc)
Plan:     docs/refactor_buildtime_reloc_plan.md
Handoff:  docs/refactor_buildtime_reloc_handoff.md  ← read this first
Backlog:  docs/refactor_buildtime_reloc_followups.md  ← D, F-L still open

Stages 1–11 plus follow-ups A (pixel-buffer enumeration), B (drift-gate
sub-resource validator), C (mods-scanner determinism), and E (V0/V1/V2
reader drop) are landed. Stage 11 Phase 2 (struct-fixup byte-transform
deletion) was reverted (`b28352b`) — see the plan + handoff for why
(short version: catalog only covers 125 of 2,132 files, deleting the
runtime fallback broke any sprite/bitmap in an uncovered file). The
runtime helpers are the source of truth; the catalog is a perf
optimization. Don't try to re-delete them without first landing
follow-up M (static-analysis catalog).

Stage 12 productizes the mods/ scanner that ships in port.cpp today
as a one-shot startup scan with no UI.

Sanity check before any changes (must be green):

    ./build/port/test/port_reloc_regen --all --check --check-subres --quiet
    # expected: unchanged=2132, mismatch=0, validated=4048, failed=0

What ships today (Stage 10 baseline):

  - port/port.cpp:600-650 scans <bin_dir>/mods/*.{o2r,otr} once at
    startup. Sorted alphabetically by full path (follow-up C). Each
    candidate is added via ArchiveManager::AddArchive AFTER
    BattleShip.o2r, so LUS's last-archive-wins resolution lets the mod
    claim any matching CRC64 hashes. portRelocSetOverridesActive(1) is
    flipped on if any mod loaded; the bridge's overlay path is
    short-circuited otherwise.
  - tools/example_mod/build_example_mod.py builds a minimal mod that
    XORs one byte of Mario's FTAttributes (file_id 203, byte 1064 ^=
    0xAA). End-to-end verified — drop in mods/, boot, see the mutation.
  - No menu surface. No way to know which mods are loaded without
    grepping ssb64.log for "mod archive registered". No way to know
    which sub-resource hashes a given mod overrides.

Stage 12 scope (per handoff):

  1. Discoverability UI in port/gui/PortMenu.{h,cpp} — list active
     mods. Existing sidebar sections are "Settings", "Windows",
     "Assets", "About". A new "Mods" sidebar (or "Assets > Mods")
     fits naturally; copy AddSidebarEntry pattern from existing
     sections (see PortMenu.cpp:139, :216, :456).
  2. Per-mod sub-resource discovery — cross-reference each loaded
     mod archive's entries against the container's
     RelocFile::SubResourceHashes table. For every mod entry whose
     CRC64 matches a sub-resource hash, surface "mod overrides
     <container_path>/<kind>/<offset>". Helps end users debug
     "which container is this mod hitting".
  3. Optional: per-mod enable toggles. The override path can
     re-engage at runtime simply by toggling
     portRelocSetOverridesActive and re-loading the affected reloc
     files; LUS lacks an "unload archive" API, so a true unload
     requires restart. Recommend v1 ships restart-required toggles
     (write enable state to the config file, restart applies).
  4. Optional: "Generate Example Mod" button that shells out to
     tools/example_mod/build_example_mod.py and writes the result
     into mods/.

Files likely touched (≤5 per phase per CLAUDE.md):
  - port/gui/PortMenu.h / .cpp
  - port/port.cpp (lift the mods scanner into a reusable function
    that returns the list of {path, archive_handle}; the menu reads
    it instead of re-walking the filesystem)
  - port/resource/RelocFile.h (the SubResourceHashes table is already
    exposed; menu code reads it via ResourceManager::LoadResource for
    each container)
  - tools/example_mod/README.md (touch as part of follow-up K — the
    mod-author guide)

What NOT to touch:

  - Anything that affected Stage 11's drift gate. The bridge's
    overlay path, sub-resource emission in torch, container
    schema — all stable. Menu work is pure read-side UX.
  - Stage 7 / Stage 8 ADRs:
    - docs/decision_aobjevent32_runtime_walker_2026-05-09.md
    - docs/decision_4c_sprite_codec_runtime_2026-05-09.md
  - port/bridge/lbreloc_byteswap.cpp's runtime fixup paths
    (chain_fixup_*, portRelocFixupVertex/TextureAtRuntime,
    portFixupSpriteBitmapData) — those still do real work on v3
    bitmap pixel data. Leave them alone.

Phased execution (CLAUDE.md Rule #2 — max 5 files per phase, gate
green between phases):

  1. Refactor port.cpp's mods scanner into a reusable surface that
     records {path, archive_handle, sub-resource hash list it
     overrides} into a global the menu can read. Existing scan
     behavior must be byte-identical to today; commit before any
     UI work.
  2. Add a "Mods" sidebar to PortMenu listing each loaded mod's path
     and override count. No toggles yet.
  3. Add per-mod sub-resource expansion (collapsible: show which
     containers + kinds the mod hits).
  4. (Optional) Add per-mod enable toggle wired to a restart-required
     config knob.
  5. (Optional, follow-up K) Mod-author guide in
     tools/example_mod/README.md and/or docs/.

After each phase:
  - cmake --build build --target ssb64 -j 4
  - cmake --build build --target port_reloc_regen -j 4
  - ./build/port/test/port_reloc_regen --all --check --check-subres --quiet
  - Boot the binary and Esc-menu sanity-check the new UI. Drop the
    example mod into mods/ and confirm the listing reflects it.

Submodule pointers (push when ready for review — none should change
in Stage 12; this is pure port-side UX work):

    git -C torch        push origin ssb64-buildtime-reloc
    git -C libultraship push origin ssb64-buildtime-reloc

Other open follow-ups in docs/refactor_buildtime_reloc_followups.md
(can be picked up alongside Stage 12 if a session has spare cycles):

  D — Override identity-check optimization (defer until profiled)
  F — OTR path stability vs. yaml refactors (doc-only first)
  G — __sub suffix collision audit (~10 min)
  H — Pixel-format disambiguation in tex paths (audit first)
  I — Catalog-drift CI gate (~15 min)
  J — Per-file census + collision observability (tooling)
  K — Mod-author guide (Stage-12 deliverable, see Phase 5 above)
  L — Full-container override pathway docs (~10 min)
  M — Static-analysis struct fixup catalog (Phase 2 retry prereq, ~3 sessions)
```

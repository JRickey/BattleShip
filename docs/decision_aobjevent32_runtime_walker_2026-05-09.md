# Decision: keep AObjEvent32 unhalfswap walker on the runtime side

**Date:** 2026-05-09
**Status:** Accepted
**Context:** Build-time reloc refactor, Stage 7
**Related:** `docs/refactor_buildtime_reloc_plan.md`,
`docs/refactor_buildtime_reloc_handoff.md`,
`port/port_aobj_fixup.{h,cpp}` (the walker that stays),
`MEMORY.md` → `project_aobjevent32_halfswap.md`

## TL;DR

Stage 7 of the build-time reloc refactor was originally specified as
"halfswap + AObjEvent32 unhalfswap → torch." Halfswap shipped as
designed (commit `6ac8a09`, torch `bfa2034`, `PROC_HALFSWAP_DONE` =
bit 8). The AObjEvent32 unhalfswap migration is **explicitly skipped**.
The lazy walker in `port/port_aobj_fixup.{h,cpp}` stays on the runtime
side permanently. `processing_flags` bit 9
(`PROC_AOBJEVENT32_UNHALFSWAP_DONE`) is **reserved-unused** with no
plans to consume it.

## What was on the table

The runtime walker fires lazily from `gcParseDObjAnimJoint` /
`gcParseMObjMatAnimJoint` (`src/sys/objanim.c`) the first time an
EVENT32 reader touches a stream. It rotates16 each command/data u32 in
place to undo the load-time blanket halfswap, then caches the head
pointer in `sUnswappedHeads` so subsequent reads are no-ops. The
walker is a 354-LOC two-phase opcode-driven state machine that mirrors
the decomp parser's switch (opcodes 0..22).

The migration target was: have torch walk every fighter animation
table at extraction, follow each AObjEvent32 head, simulate the
walker, and emit already-unhalfswapped bytes. Set bit 9 on
`processing_flags`; runtime walker becomes a no-op when the bit is
present. (Per the plan's Stage 7 status note, the runtime walker
would still need to stay live for streams reachable only via
cross-file references — so even the "successful migration" outcome
leaves the walker in place.)

## Why we are skipping it

### 1. The runtime walker has to stay anyway

Stage 7 of the original plan acknowledged this directly:

> Edge case: streams reachable only through cross-file references.
> Audit these via the Stage 2 fixtures — if any animation stream is
> only reachable through external chain, document and keep its lazy
> fixup. (Likely 0 cases, but verify.)

Whether or not "0 cases" turns out to be true, the runtime walker
exists today and is correct. Adding a torch implementation creates a
**second** implementation of the same logic (opcode table + scan loop
+ apply phase) that must be kept in sync forever. Two implementations
of one walker is strictly worse than one.

### 2. The benefit is marginal

- **Modders don't author AObjEvent32 streams.** Stage 10 (sub-asset
  emission for `mods/*.o2r` overrides) targets vertices, textures,
  TLUTs, sprites, structs — assets that exist in modder-friendly
  formats (Blender + fast64, Photoshop + texture conversion). Animation
  streams are byte-exact opcode arrays the decomp parsers consume;
  authoring them outside the original build flow is impractical.
- **No load-time perf win.** The lazy walker is one-shot per stream,
  cached. Total cost across a session is well under any frame budget.
- **No code-size win.** Bit 9's gate would be a 2-line `#ifdef PORT`
  early-return in `gcParseDObjAnimJoint`. The walker file stays for
  the cross-file-ref case anyway. Nothing is deleted.

### 3. The cost is high and concentrated on a load-bearing path

Doing this migration correctly requires three things:

- **Stream-head enumerator.** A torch+harness-shared C++ module that
  walks fighter `ftData` → motion entries → DObj/MObj joint trees →
  AObjEvent32 head pointers. This logic exists today only inside the
  decomp parsers (`gcParseDObjAnimJoint` etc.) and uses runtime
  pointer-token resolution. Replicating it standalone is the
  load-bearing complexity.
- **Walker port to torch C++.** Mirror the two-phase scan/apply logic
  including all opcode validation. ~300 LOC of duplication.
- **Lazy-fixup-gap solution in the harness.** Currently fixtures
  capture pre-walker (halfswapped) bytes; the runtime walker's effects
  never enter the snapshot. Migration without closing this gap
  produces a false-green or false-red drift gate. Closing it
  *correctly* (option a from the handoff: harness drives the synthetic
  walk via the production walker on enumerated heads) requires the
  enumerator above.

That's 1–2 sessions of work touching a system flagged in `MEMORY.md`
as historically regression-prone (Master Hand class). The risk is
disproportionate to any benefit listed above.

### 4. The original goal was correctness, not maximalism

The refactor's goal (per `refactor_buildtime_reloc_plan.md`) is
sub-asset moddability and replacing build-time work the runtime should
not be doing. AObjEvent32 unhalfswap is not in either category:

- It is not a candidate for sub-asset emission (animation streams are
  byte-exact, not authorable).
- It is genuinely runtime work — it depends on opcode interpretation,
  not blind byte transformation. The "deterministic byte transform"
  framing the plan uses for pass1/pass2/struct fixups doesn't apply
  cleanly to a walker that decides what to do based on parsed opcodes.

Skipping it does not leave the refactor "incomplete." It draws a
deliberate boundary that matches the actual goals.

## What this changes

- **`processing_flags` bit 9** is reserved-unused. The plan
  (`docs/refactor_buildtime_reloc_plan.md`) is updated to reflect this.
  Future bits start at 10 with `PROC_CHAIN_FLATTENED` (Stage 9 — was
  the original assignment) — i.e. no renumbering, just a documented
  hole at bit 9.
- **`port/port_aobj_fixup.{h,cpp}`** stays in the codebase. Stage 11's
  "runtime simplification" no longer claims `port_aobj_fixup family
  becomes empty"; that statement is corrected in the plan.
- **No new flag bit, no new code paths, no fixture regen needed.**
  Stage 7 is closed at the halfswap boundary.

## What would change this decision

If a future modding feature needs **per-animation-stream override**
(e.g. swap one fighter's idle animation), the migration becomes
worth reconsidering. At that point the stream-head enumerator
becomes load-bearing for an actual user feature, and the lazy-walker
duplication is justified by what it unlocks. Until then, the cost
exceeds the benefit.

If the runtime walker ever surfaces a bug class that's hard to
diagnose because of its laziness (e.g. stale state across scene
transitions that doesn't reproduce in fixture testing), eager-at-
build-time becomes a debugging aid worth paying for. Today's signal
is the opposite: the walker has been stable since the
`project_aobjevent32_halfswap` work landed and the
"belt-and-suspenders" `default:` case in `objanim.c` covers known
edge cases.

# Deblob System — Typed Asset Extraction + Load-Time Reconstruction

**Status (2026-08-31): LIVE on branch `deblob-asset-extraction`.**
All phases through the `archive: false` flip are landed: fighter parent
blobs no longer ship, every fighter bundle is synthesized from its typed
slices (fast path 106/106 byte-exact via `synth_verify`; relayout
smoke-tested with grown-DL and broken-slice mod artifacts), and the
recipe hash auto-re-extracts existing users' archives on upgrade.

**Remaining follow-ups** (tracked, not blocking):
- Sprite-region textures stay BLOB (typing needs a Sprite-struct walker —
  see Classification below). Blob-granular replacement still works.
- JP runtime validation: JP artifacts are generated and extraction is
  verified, but no JP build has run `synth_verify` or booted yet.
- Hires pack runtime spot-check with SSB Reloaded (covered in principle
  by byte-exactness — identical bundle bytes ⇒ identical rgba8Crc keys —
  but a played before/after `DumpMissRgba` diff hasn't been done).
- Visual confirmation by a human (all gates here are structural).
- Torch OTR encodings lose G_DL branch bits and unresolved-MOVEMEM
  pointers (upstream macro design); fighter data doesn't use either
  (I5-verified), but mod DLs relying on G_DL_NOPUSH will mis-round-trip.
- CharacterEngine-style mods that LoadResource a parent path directly now
  miss; they get `portRelocLoadFileFromBytesPrivateEx` +
  `port_synth_reloc_get_intern_pairs` instead — needs a doc pass in
  docs/modding.md when the first mod hits it.

## What this is

Fighter reloc bundles (`reloc_fighters_main`, 106 files/region) ship as
typed per-asset o2r entries (`Fast::Vertex/DisplayList/Texture` +
`Ship::Blob`) instead of opaque whole-file blobs. At load, the port
re-synthesizes the big-endian bundle from the slices and feeds it through
the unchanged reloc/token/byteswap machinery. Purpose: **custom character
models as `.o2r` drop-ins in `mods/`** — libultraship's archive VFS
(last-mounted-wins over CRC64(path)) overrides individual slices.

Mod replacements need NOT match vanilla sizes: when any loaded slice's size
differs from spec, the builder re-layouts the bundle and remaps every
intra-bundle pointer. Vanilla input always round-trips byte-exact — that is
the load-bearing validation gate.

Animations/submotions are deliberately NOT sliced: figatrees are pure u16
keyframe streams with no typed content, already individually addressable
o2r entries (`reloc_animations/FT...`) — whole-file mod override already
works for them.

## The abstraction tower

Every level has a canonical artifact, an inspector, and a cross-level gate.
Debugging = "which level diverged?" — diff adjacent artifacts, never
re-derive structure from raw bytes.

```
L0  ROM bytes                inspector: debug_tools/reloc_extract
L1  reloc parse              table entry + decompressed bundle + chains
      tools/ssb64_reloc_lib.py == torch RelocFactory semantics (pinned by I4)
L2  MANIFEST (canonical IR)  yamls/<v>/reloc_fighters_main/manifests/<Symbol>.json
L3  derived views (same tool, same parse — cannot drift; I10):
      child yaml <Symbol>.yml (torch input)
      port/bridge/ssb64_reloc_rebuild.<v>.cpp (runtime spec)
L4  o2r archive              inspector: unzip -l build/extracted/BattleShip.o2r
L5  runtime synthesis        inspector: SSB64_SYNTH_INSPECT=<id> → JSON
L6  game heap                existing token/byteswap machinery, unchanged
```

Cross-level gates: **I4** pins L1↔L2; **synth_verify** pins L0↔L5;
**manifest↔inspect diff** pins L2↔L5.

## Glossary

- **bundle** — one reloc file's decompressed big-endian contents.
- **slice** — one contiguous typed span of a bundle; becomes one o2r entry.
  Slice paths (`reloc_fighters_main/<Symbol>/<sliceName>`) are mod-facing
  API — frozen.
- **slot** — a 4-byte relocation descriptor word inside the bundle
  (`[next:16 | target:16]`, word offsets). Intern slots point within the
  bundle; extern slots consume one extern file-id each, in chain order.
- **relocated view** — the bundle with every INTERN slot rewritten to
  `target_word * 4` (a plain byte offset). This is what torch slices from,
  what synthesis reproduces, and what synth_verify compares against.
  Extern slots keep descriptor form.
- **incoming extern target** — an offset in THIS bundle referenced by
  another file's extern chain (e.g. LinkMain → LinkModel). Seeds
  classification and splits blob tiling.
- **vanilla layout** — the spec's offsets/sizes: lookup keys and fast-path
  values, never invariants for mod-replaced slices.
- **relayout** — the variable-size rebuild path: reassign offsets, patch DL
  refs, remap slot positions and values.
- **manifest** — the canonical JSON IR per bundle (L2): slices, slots,
  classifications, reports, provenance, content hash.
- **synth cache** — the runtime's `{file_id → RelocFile + LayoutMap}` map;
  mandatory (I8), evicted only on mod mount/hot-reload.

## Invariants

Every generator hard-failure, runtime fatal, and selftest verdict cites one
of these by ID, in the format
`[deblob] I<N> violated: <what> (<where>) — diagnose: <command>`.

- **I1 tiling** — slices sorted, non-overlapping, exhaustively tile
  `[0, vanilla_data_size)`.
- **I2 slot containment** — every intern/extern slot wholly inside exactly
  one slice.
- **I3 target containment** — every relocation target inside exactly one
  slice.
- **I4 inverse identity** — restoring the manifest's chain descriptors
  byte-reproduces the ROM bundle.
- **I5 vanilla round-trip** — fast-path synthesis equals the relocated-view
  reference byte-for-byte.
- **I6 builder equivalence** — the relayout machinery must not diverge
  from the fast path. Realized structurally (both paths share the
  patch-list / layout-table code; the fast path IS the layout table with
  vanilla offsets) plus the grown-DL mod smoke test, rather than as a
  forced byte-compare — vanilla padding makes a from-scratch repack
  legitimately differ in offsets.
- **I7 resolution totality** — every DL reference resolves (sibling hash /
  injected-pair literal) or fails fatally; never silent passthrough.
- **I8 layout coherence** — size query and load observe one identical
  synthesized bundle.
- **I9 structural soundness** — relayout output: all remapped slots/values
  in-bundle and within target extents; all patch sites patched; DL graph
  terminates in-bundle.
- **I10 artifact coherence** — manifest, yaml, and spec come from one parse
  and carry the same content hash.

## Commands

| Intent | Command |
|---|---|
| Regenerate manifests + child yamls + specs | `python tools/generate_fighter_slices.py --version {us,jp}` |
| Validate one bundle without writing | `python tools/generate_fighter_slices.py --only <Symbol> --check` |
| Extract a bundle's raw bytes from ROM | `debug_tools/reloc_extract/reloc_extract.py extract <rom> <file_id> out.bin` |
| Full vanilla byte-exactness gate | `cmake --build build --target synth_verify` (ROM-independent: spec CRCs are the ROM-anchored truth via generator I4; results in `debug_traces/synth_verify_results.json`) |
| Dump a synthesized bundle's bytes | `SSB64_DUMP_SYNTH_RELOC=1` (all) or `SSB64_DUMP_SYNTH_RELOC_FILE_ID=<id>` |
| Dump synthesis decisions as JSON | `SSB64_SYNTH_INSPECT=<id>` → `debug_traces/synth_inspect_<id>.json` |

| Regenerate everything, both regions | `cmake --build build --target DeblobRegen` |
| Build a mod test artifact | `python debug_tools/deblob_modkit.py grow-dl LinkModel dLinkModel_Gfx_0x1D88` |

Deblobbing another category later: add it to
`tools/deblob_categories.py::DEBLOBBED_CATEGORIES`, regenerate, done — no
runtime, spec-schema, or CMake edits.

## Replacing a character asset (mod quickstart)

1. `unzip -l BattleShip.o2r | grep <Symbol>/` lists the slices; the
   manifest (`yamls/us/reloc_fighters_main/manifests/<Symbol>.json`)
   describes each one (kind, vanilla offset/size, what references it).
2. Author a replacement resource in the same OTR format (a DL slice is a
   torch-exported binary DisplayList, a texture is raw N64 texels behind
   a Texture header, etc.) — sizes may differ from vanilla; the port
   re-layouts the bundle and remaps every reference.
3. Zip it at the identical path into a `.o2r`, drop it in `mods/`.
   Last-mounted archive wins; a mods-menu rescan applies it without a
   restart (next scene load).
4. Constraints: references landing mid-slice are only valid for
   prefix-preserving edits (the loader warns); a malformed slice fails
   the whole bundle loudly rather than rendering garbage. Replaced
   textures re-key for hires packs by design (their rgba8Crc changes).

## Classification (generator)

Chain-seeded, conservative: only intern-chain targets + incoming extern
targets (and DL references discovered from them, to fixpoint) become typed
slices. Everything unproven stays BLOB — always memcpy-safe for the
round-trip. Key rules learned from calibration against the contributor's
LinkMain/LinkModel template (multiset-validated):

- Joint DLs are count-delimited, packed back-to-back with no `G_ENDDL`; a
  candidate ends at ENDDL or exactly at the next boundary.
- A real DL draws, loads a texture, or calls a sub-DL; framebuffer ops
  (0xE4/0xE5/0xF6/0xFE/0xFF) never appear; `G_NOOP` must be a true
  `w0 == 0`; a G_VTX must reference in-file data or an extern slot; a
  relocation slot can only occupy a w1 (pointer) position.
- CI textures LOADBLOCK in 16-bit units: format comes from the RENDER
  `G_SETTILE`, dimensions from `G_SETTILESIZE` or SETTILE's `line` field.
- Extern slots may live inside GFX slices (torch passes their descriptor
  words through as unresolvable literals — byte-preserved); never inside
  vtx/tex/lut data slices.

**Known follow-up**: sprite-region textures (batch-loaded via Sprite
structs, e.g. LinkModel 0xB1B0–0xC6xx shield/sword art) stay BLOB — typing
them needs a Sprite-struct walker (layout documented in
`port/bridge/lbreloc_byteswap.cpp` `portFixupSprite`). Recorded per-file in
the manifest's `demotions` report.

## Failure playbook

| Symptom | First move |
|---|---|
| Generator fails `I1..I4` | Run the `--only <Symbol> --check` command from the message; inspect the manifest reports for that bundle. |
| Fighter renders garbage (vanilla) | `synth_verify`; if it passes, the bug is downstream of synthesis (L6) — see docs/bugs/. If it fails, diff `SSB64_DUMP_SYNTH_RELOC` output against `reloc_extract.py extract` + relocated-view transform; the failure names the owning slice. |
| Fighter renders garbage (modded) | `SSB64_SYNTH_INSPECT=<id>` → diff the layout/patch decisions against the manifest; check the mid-slice-target policy warnings. |
| Mod .o2r has no effect | Archive mount order (last wins); resource + synth cache eviction on late mounts; re-enter the scene. |
| Hires pack textures stopped matching | Only possible if vanilla bytes changed: run synth_verify. Mod-replaced textures re-key by design (rgba8Crc changes). |
| Extraction stale after yaml edit | Recipe hash + o2r DEPENDS (Phase 8); check `<o2r>.recipe` sidecar. |

## Spec schema (generator ↔ runtime contract)

Defined in `port/bridge/ssb64_reloc_rebuild.h`; data in generated
`ssb64_reloc_rebuild.<v>.cpp` (file_id-sorted, binary-searched). All
offsets slice-relative where possible, self-validating at generation:

- `SSB64SyntheticSlice { path, vanilla_offset, vanilla_size, kind }`
- `SSB64RelocInternSlot { slot_slice, target_slice, slot_offset_in_slice, target_offset_in_slice }` (chain order)
- `SSB64RelocExternSlot { slot_slice, dep_file_id, slot_offset_in_slice, dep_word_offset }` (chain order — the runtime rebuilds extern relocation from this list; chain-next links die under relayout)
- `SSB64SyntheticRelocSpec { file_id, parent_path, vanilla_data_size, relocated_view_crc32, manifest_hash, slices[], intern_slots[], extern_slots[] }`

No positional DL-hash metadata: DL reference resolution is semantic (OTR
opcode + CRC64 sibling map + injected-pair sentinel) because that is the
format mod DLs author against; unresolvable references are fatal (I7).

## Design rationale pointers

- Full migration plan: `~/.claude/plans/shimmying-sniffing-sunset.md`
  (session-local; the durable copy is this doc + the PR description).
- Why round-trip instead of native typed rendering: the bundle's pointer
  graph is offset-indexed by game code (`docs/plan-token-pointer-system.md`);
  re-synthesis keeps all of that untouched.
- Why the synthesized bundle differs from raw ROM at chain slots: torch
  pre-relocates intern slots at extraction (the chain `next` field is
  unknowable from a sliced DL), so the runtime uses explicit slot lists and
  every comparison uses the **relocated view**.
- Hires packs hash decoded RGBA8 + fmt/siz at upload (`port/hires/`), not
  paths — vanilla byte-exactness ⇒ existing packs keep binding.

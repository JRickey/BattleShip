# example_mod — Stage 10 Phase C demo

A canned mod-archive builder demonstrating the build-time reloc refactor's
sub-resource override pathway (`docs/refactor_buildtime_reloc_plan.md`
Stage 10).

## What it does

- Reads `build/BattleShip.o2r`, picks one sub-resource (default: Mario's
  `FTAttributes` struct at offset 1064 in `reloc_fighters_main/MarioMain`).
- XORs one byte of the original payload to produce a deviation that
  isn't gameplay-breaking but is observable in `SSB64_DUMP_FILE_ID` dumps.
- Writes a one-entry mod `.o2r` at `build/mods/example_mod.o2r` whose
  single Blob resource shares the OTR path of the targeted sub-resource.

## Workflow

```bash
# 1) Build the mod (uses the build/BattleShip.o2r in this worktree).
./tools/example_mod/build_example_mod.py

# 2) Boot the game. The minimal mods/ scanner in port/port.cpp
#    auto-registers everything in build/mods/*.o2r and flips the
#    overlay-active flag.
./build/BattleShip

# 3) Verify in the log (~/Library/Application Support/BattleShip/ssb64.log
#    on macOS, %APPDATA%/BattleShip/ssb64.log on Windows):
#       SSB64: mod archive registered -> mods/example_mod.o2r
#       SSB64: 1 mod archive(s) loaded — sub-resource overlay active

# 4) Optionally dump the targeted file pre-/post-overlay to confirm bytes:
SSB64_DUMP_FILE_ID=203 ./build/BattleShip   # writes debug_traces/port_file_203.bin
# Compare: byte at offset 1064 differs by exactly 0xAA from the no-mod dump.
```

## Inspect available targets

```bash
./tools/example_mod/build_example_mod.py --list                    # all sub-resources
./tools/example_mod/build_example_mod.py --list --list-kinds sprite,bitmap
```

## Custom override

```bash
./tools/example_mod/build_example_mod.py \
    --target reloc_menus/MNCommon__sub/sprite/488 \
    --mutate-byte-idx 12 --mutate-byte-xor 0x80 \
    --out build/mods/my_mod.o2r
```

## How the override pathway works

1. Torch (`torch/src/factories/ssb64/RelocFactory.cpp`) emits each
   sub-resource as a `Blob` at a deterministic OTR path of the form
   `<container_path>__sub/<kind>/<dec_offset>` and records
   `(CRC64(path), byte_offset, size, kind)` in the container's v3 header.
2. The runtime (`port/bridge/lbreloc_bridge.cpp::portRelocBuildOverlay`)
   asks `ResourceManager::LoadResource(hash)` for each recorded entry.
3. LUS's last-archive-wins resolution
   (`libultraship/src/ship/resource/archive/ArchiveManager.cpp`) returns
   bytes from the most-recently-added archive that contains the hash —
   the mod, if it ships an entry at that path; otherwise the base.
4. If the returned bytes differ from the container's slice at
   `byte_offset`, the bridge clones `Data` once and overlays the mod
   bytes at the recorded offset. The chain walker that follows operates
   on the overlaid buffer.

Stage 12 will replace the bare-bones `mods/` scanner with a UI; the
underlying override-path mechanics are the Stage 10 deliverable.

# Asset recipe rewrites and stale Torch builds

The contributor reported mass YAML diffs after building and changes to Torch
being ignored until its entire build directory was rebuilt.

## Causes

- `RegenerateRelocYamls` is an always-run custom target. Its `BYPRODUCTS`
  included tracked source YAML files. Because extraction depends on those
  files, Ninja connected normal builds to regeneration. This also made
  source YAMLs eligible for deletion by a build-system clean. The copies
  beside the executable were not being copied back into the repository;
  the generator itself was rewriting the source recipes.
- Torch was an `ExternalProject` with no download/update step and no
  `BUILD_ALWAYS`. Once its completion stamp existed, the outer build did
  not consult Torch's dependency graph after edits. Its source globs also
  lacked `CONFIGURE_DEPENDS`, so adding files could require reconfiguration.
- Extraction depended on the Torch target, but not its executable as a
  file. The recipe fingerprint used Torch's commit ID, missing uncommitted
  source edits. Recipe edits did not reliably rerun fingerprint generation.
- Runtime copies were attached to the game's link step. An asset-only
  build could therefore leave the runnable directory stale.
- The reloc-table and credits generators rewrote unchanged outputs,
  causing needless game compilation and relinking on no-op builds.

## Changes

Keep recipe regeneration explicit, stage read-only source inputs under the
build directory, and extract using that staged source directory. Staging
copies only changed bytes and prunes obsolete recipes in the selected
region. It runs independently of linking and copies the matching archive
and extraction fingerprint together.

Run Torch's nested incremental build every time and declare the executable
as a build byproduct and extraction dependency. Watch source additions and
hash source/recipe contents and paths for extraction invalidation. Define
the fingerprint only for `first_run.cpp`, which consumes it, so changing
one recipe does not recompile the entire game. Preserve unchanged generated
reloc-table and credits output timestamps.

libultraship's existing source files already participate in the main build
graph; its source registration does not require the Torch workaround.

## Validation

Passed on the real Linux/Ninja desktop build with a locally supplied US
baserom: Torch source addition/edit/removal; recipe edits and restoration;
a libultraship source timestamp change; and equality of staged/extracted
archives, fingerprints, and Torch binaries. The final no-op build took
0.61 seconds with zero compilation, linking, or extraction. All 246 source
YAMLs retained their contents and timestamps during the initial no-op check.
Source probes were removed and recipe bytes restored after testing.

A small staging fixture passed nested `.yaml` files, paths with spaces,
unchanged timestamps, stale recipe removal, and US/JP directory isolation.
The rebuilt archive passed the vanilla synthesis gate: 106 bundles passed,
zero failed.

Windows/macOS compilation and a JP runtime boot require their respective
platform/region validation; Linux success alone does not establish those.

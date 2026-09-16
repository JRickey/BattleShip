"""deblob_categories — single registry of reloc yaml categories that are deblobbed.

A "deblobbed" category ships its reloc bundles as typed per-asset o2r slices
(see docs/deblob.md) instead of whole-file blobs. This set is the ONLY place
that knowledge lives; it is imported by:

  - tools/generate_fighter_slices.py  (which bundles to slice)
  - tools/generate_yamls.py           (which parent entries get `archive: false`
                                       once the runtime flip lands — Phase 9)

Adding a category here + running the DeblobRegen target is the complete
recipe for deblobbing another category. No runtime, spec, or CMake edits.
"""

# Categories whose bundles get typed slice extraction (child yamls + manifests
# + rebuild specs). Keys match the reloc_<category>.yml basename.
DEBLOBBED_CATEGORIES = {
    "fighters_main",
}

# Flipped 2026-08-31 (Phase 9) after the synthesis path passed synth_verify
# 106/106: generate_yamls.py emits `archive: false` for parent entries in
# DEBLOBBED_CATEGORIES, so their whole-file blobs stop shipping and the port
# synthesizes them from their typed slices (docs/deblob.md).
EMIT_ARCHIVE_FALSE = True

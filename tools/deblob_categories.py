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

# Once the load-time synthesis path is proven (docs/deblob.md, invariant I5),
# Phase 9 flips this on so generate_yamls.py emits `archive: false` for parent
# entries in DEBLOBBED_CATEGORIES, dropping their whole-file blobs from the
# o2r. Until then, slices and parent blobs coexist and the runtime ignores
# the slices.
EMIT_ARCHIVE_FALSE = False

#pragma once

// Deblob synthesis engine (docs/deblob.md, L5): rebuilds a reloc bundle's
// big-endian byte image from its typed o2r slices, per the generated spec
// (port/bridge/ssb64_reloc_rebuild.h). Fast path reproduces the vanilla
// relocated view byte-for-byte (invariant I5); when a mods-folder override
// changes any slice's size, the relayout path reassigns offsets and remaps
// every intra-bundle pointer (I9).

#include <cstdint>
#include <memory>

class RelocFile;

// Cached build (I8: size queries and loads must observe one identical
// bundle). Returns nullptr when file_id has no spec or a slice failed —
// callers fall back to the archived parent while one still exists.
std::shared_ptr<RelocFile> portBuildSyntheticRelocResource(uint32_t file_id);

// Drop every cached synthesized bundle (mod mount / hot-reload — the only
// legitimate mutation points for slice content).
void portSyntheticRelocEvictAll();

extern "C" {
// Mod-facing: the explicit intern (slot,target) byte-offset pair list for a
// deblobbed file, in the layout of the CURRENT cached synthesis — the input
// portRelocLoadFileFromBytesPrivateEx expects for these bundles. Returns 0
// and a valid pointer/count on success, nonzero when file_id is not
// deblobbed (mods should then use the ordinary chain-walk path).
int port_synth_reloc_get_intern_pairs(uint32_t file_id,
                                      const uint32_t **pairs_out,
                                      uint32_t *pair_count_out);
}

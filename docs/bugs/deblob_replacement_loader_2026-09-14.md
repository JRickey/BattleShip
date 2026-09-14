# Replacement display-list reconstruction

The contributor's Link replacement decoded to 0x98 bytes, while the original
display list was 0x1B8 bytes. Reconstruction still applied the original
display list's relocation slots. This could either reject a shorter list
or overwrite unrelated commands in a longer or reordered one. The original
grow-by-no-ops test did not detect misplaced pointer fixups.

A second bug applied the original CRC to modified assets whenever their
sizes happened to match. A same-size vertex-color edit was therefore
rejected. The loader also assumed every hash referenced an existing slice,
and did not decode Fast64 XML's host-path references.

The loader now records the extracted archive's identity before mounting
mods, applies the vanilla CRC only to original resources, and derives the
pointer slots of replaced display lists from their actual commands. XML
path commands and Triangle1 commands are converted to N64 commands. New
named display lists, vertices, textures and blobs are included in the
bundle with their dependencies. The loader validates vertex ranges and
types, terminators, pointer widths and DL cycles. Broken cross-bundle
dependencies fail explicitly instead of falling back to stale offsets.

`debug_tools/test_deblob_mods.py` tests real isolated game instances. The
base game passes all 106 bundle CRCs. Cases cover shorter, same-size and
larger reordered display lists, same-size vertex edits, new XML mesh and
vertex paths, invalid offsets and ranges, empty vertices, wrong reference
types, missing assets, cyclic lists and truncated binaries. The test also
checks the actual relocation slot positions, not just the process exit.

Initial Linux boot checks completed 110 frames of Link's opening scene
with the original model, the shorter replacement and the new XML mesh.
The opening scene waits until global tick 1695 before drawing. Longer
1820-frame runs produced all three requested screenshots for each case.
Inspection confirmed intact vanilla geometry and the expected missing
waist geometry in the deliberately reduced replacement. No crashes occurred.

## Contributor PR review

[PR #271](https://github.com/JRickey/BattleShip/pull/271), by Jameriquiah,
targets `deblob-asset-extraction`, not the Torch repository. Its core
replacement-slot, XML-path, dependency-append and CRC fixes overlap the
reviewed implementation here. The merge retains its useful recovery of
unchanged external-chain descriptors after command movement, while retaining
the stronger range/type/cycle/termination checks and archive-object identity.
Descriptor matches are checked before treating a word as a segmented pointer;
the high byte of a chain-next field can also look like a segment number.
The added regression moves two external texture references by one command
and checks both new slot locations and their original dependency identities.

Jameriquiah's Torch `asset-extraction` commit `f174e66` has the identical
stable patch ID (`6a7515a8995f2878765412d7c5985d00d32d1f1e`) as our
already-integrated `ab9c70a`. Our subsequent changes restore JP layouts,
preserve companion files for `archive: false`, keep unresolved references
byte-exact, report archive-write failures and fix incremental source discovery.
There is no additional Torch patch to reapply from that branch.

This work does not yet establish a complete custom-character workflow.
Fast64's SSB64JointTree needs a game-specific adapter; arbitrary rigs must
also satisfy fighter joint/attachment requirements. Animation export and
moveset integration remain separate validation work. The fork inspected
was Jameriquiah/fast64, ssb64-model at 80f22486213d31bb26636850255b41345937eed4.

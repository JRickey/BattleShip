#!/usr/bin/env python3
"""generate_fighter_slices — deblob slice-map generator for SSB64 reloc bundles.

For every bundle in the deblobbed categories (tools/deblob_categories.py),
this tool decompresses the bundle from ROM, walks its relocation chains,
classifies its contents (chain-seeded: display lists, vertex runs, textures,
opaque blobs), and emits THREE artifacts from that single parse (invariant
I10 — they cannot drift):

  yamls/<v>/<cat>/manifests/<Symbol>.json   canonical IR (L2) — slices, slots,
                                            classifications, reports
  yamls/<v>/<cat>/<Symbol>.yml              torch extraction input (L3)
  port/bridge/ssb64_reloc_rebuild.<v>.cpp   runtime rebuild/relayout spec (L3)

See docs/deblob.md for the abstraction tower, the invariant list (I1-I4 are
enforced here as hard failures), the slice naming scheme (frozen — slice
paths are mod-facing API), and the failure playbook.

Usage:
    python tools/generate_fighter_slices.py [--version {us,jp}] [--rom PATH]
                                            [--only SYMBOL] [--check]

Classification is CHAIN-SEEDED: only intern-chain targets (and display-list
references discovered from them) become typed slices; everything else stays
BLOB, which is always memcpy-safe for the round-trip. A flat scan mirroring
the runtime's scan_display_lists (port/bridge/lbreloc_byteswap.cpp) runs as
a cross-check and reports typed-looking regions the chain could not reach.
"""

import argparse
import json
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ssb64_reloc_lib as rl
from deblob_categories import DEBLOBBED_CATEGORIES

TOOL_VERSION = "1.0"

# F3DEX2 opcodes (match port/bridge/lbreloc_byteswap.cpp + fast3d interpreter)
G_VTX = 0x01
G_DL = 0xDE
G_ENDDL = 0xDF
G_MTX = 0xDA
G_MOVEMEM = 0xDC
G_SETTIMG = 0xFD
G_SETTILE = 0xF5
G_SETTILESIZE = 0xF2
G_LOADBLOCK = 0xF3
G_LOADTLUT = 0xF0

# The interpreter treats opcodes 0x10..0xD6 as invalid (interpreter.cpp
# portNormalizeDisplayListPointer); a strict DL parse uses the same rule.
def is_valid_opcode(op):
    return op <= 0x0F or op >= 0xD7

# A candidate region only counts as a DL if it contains at least one of
# these geometry/state opcodes — struct and padding regions full of zero
# words otherwise parse as endless G_NOOP runs and misclassify as GFX.
CORE_OPS = {0x01, 0x05, 0x06, 0x07, G_DL, G_SETTIMG, 0xFC, G_MTX, 0xD9, G_SETTILE}

# Framebuffer ops never appear in fighter model DLs; a "command" with one of
# these opcodes is struct/vertex data whose first byte happens to collide
# (e.g. negative vertex coordinates read as G_SETCIMG 0xFF).
FORBIDDEN_OPS = {0xE4, 0xE5, 0xF6, 0xFE, 0xFF}

FILE_SEGMENT_ID = 0x0E

# fmt (bits 23-21 of SETTIMG w0) x siz (bits 20-19) -> torch TEXTURE format
TEX_FORMATS = {
    (0, 2): "RGBA16", (0, 3): "RGBA32",
    (2, 0): "CI4", (2, 1): "CI8",
    (3, 0): "IA4", (3, 1): "IA8", (3, 2): "IA16",
    (4, 0): "I4", (4, 1): "I8",
}
BPP = {0: 4, 1: 8, 2: 16, 3: 32}


def be32(data, off):
    return struct.unpack(">I", data[off:off + 4])[0]


# ---------------------------------------------------------------------------
# Reference model: an in-file pointer operand inside a display list
# ---------------------------------------------------------------------------

class DlRef:
    """One in-file reference found while parsing a DL on the relocated view.

    encoding:
      'slot'  — the w1 word is an intern chain slot (relocated to a plain
                byte offset; vanilla bytes held the chain descriptor)
      'seg'   — raw segment-0x0E word (vanilla bytes keep the 0x0E prefix)
    """

    def __init__(self, cmd_off, opcode, encoding, target):
        self.cmd_off = cmd_off      # byte offset of the 8-byte command
        self.opcode = opcode
        self.encoding = encoding
        self.target = target        # in-file byte offset the ref points at


def classify_ref(w1, w1_off, slot_offsets, file_size):
    """Decide whether a DL operand word is an in-file reference."""
    if w1_off in slot_offsets:
        if w1 < file_size:
            return "slot", w1
        return None, None
    if (w1 >> 24) == FILE_SEGMENT_ID and (w1 & 0x00FFFFFF) < file_size:
        return "seg", w1 & 0x00FFFFFF
    return None, None


# ---------------------------------------------------------------------------
# Display-list discovery (on the relocated view)
# ---------------------------------------------------------------------------

def parse_dl(view, start, boundaries, slot_offsets, extern_slot_offsets):
    """Strictly parse one DL at `start` on the relocated view.

    Ends after G_ENDDL, or (SSB64 joint DLs are count-delimited and packed
    back-to-back with no terminator) at the next known boundary. Returns
    (end_offset, refs, tex_events) or None if this is not a DL.

    tex_events: [(kind, target, encoding, meta)] for texture sizing. CI
    textures LOADBLOCK in 16-bit units, so the real format comes from the
    RENDER G_SETTILE (tile != 7) after the load, and width/height from the
    following G_SETTILESIZE — the load SETTIMG only sizes the byte run.
    """
    file_size = len(view)
    refs = []
    tex_events = []
    off = start
    pending_img = None  # (target, encoding, cmd_off) from SETTIMG
    open_tex = None     # meta dict of the last LOADBLOCK awaiting SETTILE/SIZE

    # next boundary strictly after start
    next_boundary = file_size
    for b in boundaries:
        if b > start:
            next_boundary = b
            break

    saw_core = False
    saw_draw = False
    saw_enddl = False
    while off + 8 <= min(next_boundary, file_size):
        w0 = be32(view, off)
        w1 = be32(view, off + 4)
        op = (w0 >> 24) & 0xFF
        if not is_valid_opcode(op) or op in FORBIDDEN_OPS:
            return None
        # A relocation slot can only occupy a command's w1 (pointer operand);
        # a slot at a w0 position means this is chain/struct data, not a DL
        # (e.g. LinkMain's packed extern-pointer tables mimic G_VTX runs).
        if off in slot_offsets or off in extern_slot_offsets:
            return None
        # G_NOOP is exactly 0x00000000 in w0; anything else with a zero
        # opcode byte is struct data, not a DL command.
        if op == 0x00 and w0 != 0:
            return None
        if op in CORE_OPS:
            saw_core = True
        if op in (0x01, 0x05, 0x06, 0x07):
            saw_draw = True

        if op == G_VTX:
            n = (w0 >> 12) & 0xFF
            enc, tgt = classify_ref(w1, off + 4, slot_offsets, file_size)
            if enc and n > 0 and tgt + n * 16 <= file_size:
                refs.append(DlRef(off, op, enc, tgt))
                refs[-1].vtx_count = n
            elif (off + 4) not in extern_slot_offsets:
                # a fighter-DL G_VTX always references in-file vertex data
                # or an extern dependency; anything else is u16-table data
                # whose high byte happens to read as opcode 0x01
                return None
        elif op == G_DL:
            enc, tgt = classify_ref(w1, off + 4, slot_offsets, file_size)
            if enc:
                refs.append(DlRef(off, op, enc, tgt))
        elif op == G_SETTIMG:
            open_tex = None
            enc, tgt = classify_ref(w1, off + 4, slot_offsets, file_size)
            if enc:
                refs.append(DlRef(off, op, enc, tgt))
                pending_img = (tgt, enc, off)
            else:
                pending_img = None
        elif op == G_LOADBLOCK:
            if pending_img is not None:
                tgt, enc, cmd_off = pending_img
                meta = {"load_siz": (0, 0), "texels": ((w1 >> 12) & 0xFFF) + 1,
                        "cmd_off": cmd_off}
                # load-time siz comes from the preceding SETTIMG's siz field;
                # re-read it from the recorded command
                simg_w0 = be32(view, cmd_off)
                meta["load_siz"] = (simg_w0 >> 19) & 0x03
                tex_events.append(("block", tgt, enc, meta))
                open_tex = meta
                pending_img = None
        elif op == G_LOADTLUT:
            if pending_img is not None:
                tgt, enc, cmd_off = pending_img
                count = ((w1 >> 14) & 0x3FF) + 1
                tex_events.append(("tlut", tgt, enc,
                                   {"colors": count, "cmd_off": cmd_off}))
                pending_img = None
                open_tex = None
        elif op == G_SETTILE:
            tile = (w1 >> 24) & 0x07
            if open_tex is not None and tile != 7 and "fmt" not in open_tex:
                open_tex["fmt"] = (w0 >> 21) & 0x07
                open_tex["siz"] = (w0 >> 19) & 0x03
                open_tex["line"] = (w0 >> 9) & 0x1FF
        elif op == G_SETTILESIZE:
            if open_tex is not None and "width" not in open_tex:
                open_tex["width"] = ((w1 >> 12) & 0xFFF) // 4 + 1
                open_tex["height"] = (w1 & 0xFFF) // 4 + 1
                open_tex = None
        elif op == G_MOVEMEM or op == G_MTX:
            enc, tgt = classify_ref(w1, off + 4, slot_offsets, file_size)
            if enc:
                refs.append(DlRef(off, op, enc, tgt))

        off += 8
        if op == G_ENDDL:
            saw_enddl = True
            break

    # Accept only DLs with real content. Every real fighter DL draws,
    # loads a texture, or calls a sub-DL — struct data with a lucky state
    # opcode (or a stray 0xDF word) otherwise slips through as a tiny
    # false "DL". Count-delimited joint DLs (no terminator, packed
    # back-to-back) must additionally run exactly to the next boundary.
    if off == start or not saw_core:
        return None
    has_substance = (saw_draw or tex_events or
                     any(r.opcode == G_DL for r in refs))
    if not has_substance:
        return None
    if not saw_enddl and off != next_boundary:
        return None
    return off, refs, tex_events


def discover_dls(view, seeds, boundaries, slot_offsets, extern_slot_offsets):
    """Fixpoint DL discovery: parse candidate DLs at chain-target seeds, then
    at every G_DL target found, until closed. Returns {start: parse_result}."""
    dls = {}
    queue = sorted(seeds)
    pending = set(queue)
    while queue:
        start = queue.pop(0)
        pending.discard(start)
        if start in dls or start % 8 != 0:
            continue
        result = parse_dl(view, start, boundaries, slot_offsets,
                          extern_slot_offsets)
        if result is None:
            continue
        dls[start] = result
        for ref in result[1]:
            if ref.opcode == G_DL and ref.target not in dls and ref.target not in pending:
                queue.append(ref.target)
                pending.add(ref.target)
    return dls


# ---------------------------------------------------------------------------
# Flat cross-check scan — Python port of scan_display_lists
# (port/bridge/lbreloc_byteswap.cpp:709), run on PRE-relocation bytes.
# ---------------------------------------------------------------------------

def flat_scan(data):
    regions = []
    pending = None  # (offset, siz)
    for off in range(0, len(data) - 7, 8):
        w0 = be32(data, off)
        w1 = be32(data, off + 4)
        op = (w0 >> 24) & 0xFF
        if op == G_VTX:
            n = (w0 >> 12) & 0xFF
            seg = (w1 >> 24) & 0xFF
            tgt = w1 & 0x00FFFFFF
            if seg == FILE_SEGMENT_ID and n > 0 and tgt + n * 16 <= len(data):
                regions.append(("vtx", tgt, n * 16))
        elif op == G_SETTIMG:
            seg = (w1 >> 24) & 0xFF
            if seg == FILE_SEGMENT_ID:
                pending = (w1 & 0x00FFFFFF, (w0 >> 19) & 0x03)
            else:
                pending = None
        elif op == G_LOADBLOCK and pending is not None:
            tgt, siz = pending
            texels = ((w1 >> 12) & 0xFFF) + 1
            nbytes = ((texels * BPP[siz] + 7) // 8 + 3) & ~3
            if tgt < len(data):
                regions.append(("tex", tgt, min(nbytes, len(data) - tgt)))
            pending = None
        elif op == G_LOADTLUT and pending is not None:
            tgt, _ = pending
            nbytes = ((((w1 >> 14) & 0x3FF) + 1) * 2 + 3) & ~3
            if tgt < len(data):
                regions.append(("tlut", tgt, min(nbytes, len(data) - tgt)))
            pending = None
    return regions


# ---------------------------------------------------------------------------
# Slice assembly
# ---------------------------------------------------------------------------

class Slice:
    def __init__(self, offset, size, kind, extra=None):
        self.offset = offset
        self.size = size
        self.kind = kind          # gfx | vtx | tex | lut | blob | gap | pad
        self.extra = extra or {}
        self.name = None

    @property
    def end(self):
        return self.offset + self.size


def build_slices(symbol, view, intern_chain, extern_chain, incoming_targets,
                 file_size, report):
    """Run classification and return the final exhaustive slice list.

    incoming_targets: byte offsets in THIS file referenced by OTHER files'
    extern chains (e.g. LinkMain's extern refs into LinkModel). They are
    addressable object starts, so they seed classification and split blob
    tiling exactly like intern-chain targets.
    """
    slot_offsets = {e["slot_off"] for e in intern_chain}
    extern_slot_offsets = {e["slot_off"] for e in extern_chain}
    targets = sorted({e["target_words"] * 4 for e in intern_chain} |
                     set(incoming_targets))
    boundaries = sorted(set(targets) | {file_size})

    dls = discover_dls(view, targets, boundaries, slot_offsets,
                       extern_slot_offsets)

    typed = []          # candidate typed slices
    vtx_runs = []       # (start, end, [ref targets])
    tex_slices = {}     # offset -> Slice

    for start in sorted(dls):
        end, refs, tex_events = dls[start]
        typed.append(Slice(start, end - start, "gfx",
                           {"count": (end - start) // 8}))
        for ref in refs:
            if ref.opcode == G_VTX:
                vtx_runs.append((ref.target, ref.target + ref.vtx_count * 16))
        for kind, tgt, enc, meta in tex_events:
            if tgt in tex_slices:
                continue
            if kind == "tlut":
                size = ((meta["colors"] * 2) + 3) & ~3
                tex_slices[tgt] = Slice(tgt, size, "lut",
                                        {"colors": meta["colors"]})
            else:
                # byte run is sized by the LOAD (SETTIMG siz + LOADBLOCK
                # texels); the real format/dimensions come from the RENDER
                # SETTILE/SETTILESIZE (CI4 loads as 16-bit units).
                load_siz = meta["load_siz"]
                load_bytes = (meta["texels"] * BPP[load_siz] + 7) // 8
                size = (load_bytes + 3) & ~3
                fmt, siz = meta.get("fmt"), meta.get("siz")
                fmt_name = TEX_FORMATS.get((fmt, siz)) if fmt is not None else None
                w, h = meta.get("width"), meta.get("height")
                # sprite-style loads have no render SETTILESIZE; recover the
                # geometry from SETTILE's line field (64-bit words per row)
                if fmt_name and not w and meta.get("line"):
                    w = meta["line"] * 64 // BPP[siz]
                    if w and (load_bytes * 8) % (w * BPP[siz]) == 0:
                        h = load_bytes * 8 // (w * BPP[siz])
                ok = (fmt_name is not None and w and h and
                      (w * h * BPP[siz] + 7) // 8 == load_bytes)
                if ok:
                    tex_slices[tgt] = Slice(tgt, size, "tex",
                                            {"format": fmt_name, "width": w, "height": h})
                else:
                    report["demotions"].append({
                        "offset": tgt, "size": size, "reason": "ambiguous-texture",
                        "fmt": fmt, "siz": siz, "load_siz": load_siz,
                        "width": w, "height": h, "texels": meta["texels"]})

    # merge vtx runs only on true overlap — adjacent runs stay separate
    # slices (finer mod granularity, matches the reference template)
    vtx_runs.sort()
    merged = []
    for start, end in vtx_runs:
        if merged and start < merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])
    for start, end in merged:
        if (end - start) % 16 != 0:
            report["demotions"].append({
                "offset": start, "size": end - start,
                "reason": "vtx-run-not-16-aligned"})
            continue
        typed.append(Slice(start, end - start, "vtx",
                           {"count": (end - start) // 16}))

    typed.extend(tex_slices.values())
    typed.sort(key=lambda s: (s.offset, -s.size))

    # conflict resolution: overlapping typed slices, or typed slices holding
    # extern chain slots (their descriptor words are live chain state) or —
    # for vtx/tex — intern slots, all demote to blob (I2/I3 safety).
    accepted = []
    for s in typed:
        if accepted and s.offset < accepted[-1].end:
            a = accepted[-1]
            # identical duplicate is fine to drop silently
            if not (s.offset == a.offset and s.size == a.size and s.kind == a.kind):
                report["demotions"].append({
                    "offset": s.offset, "size": s.size, "kind": s.kind,
                    "reason": "overlap", "with": {
                        "offset": a.offset, "size": a.size, "kind": a.kind}})
            continue
        # Data slices must not contain relocation slots (a pointer word
        # inside texel/vertex bytes means this is not what it looks like).
        # GFX slices MAY contain intern slots (their own pointer operands)
        # and extern slots (torch passes the descriptor through as an
        # unresolvable literal, preserving the bytes).
        bad_slots = []
        if s.kind in ("vtx", "tex", "lut"):
            bad_slots = [off for off in extern_slot_offsets
                         if s.offset <= off < s.end]
            bad_slots += [off for off in (e["slot_off"] for e in intern_chain)
                          if s.offset <= off < s.end]
        if bad_slots:
            report["demotions"].append({
                "offset": s.offset, "size": s.size, "kind": s.kind,
                "reason": "contains-chain-slot", "slots": bad_slots})
            continue
        accepted.append(s)

    # blob tiling of the residue, split at chain-target boundaries
    slices = []
    cursor = 0
    target_set = set(targets)

    def emit_blob(start, end):
        cuts = [start] + [t for t in targets if start < t < end] + [end]
        for a, b in zip(cuts, cuts[1:]):
            if b > a:
                kind = "blob" if a in target_set else ("pad" if b - a <= 8 else "gap")
                slices.append(Slice(a, b - a, kind))

    for s in accepted:
        if s.offset > cursor:
            emit_blob(cursor, s.offset)
        slices.append(s)
        cursor = s.end
    if cursor < file_size:
        emit_blob(cursor, file_size)

    # frozen naming scheme (mod-facing API): d<Symbol>_<Kind>_0x%04X
    kind_label = {"gfx": "Gfx", "vtx": "Vtx", "tex": "Tex", "lut": "Lut",
                  "blob": "blob", "gap": "gap", "pad": "pad"}
    for s in slices:
        s.name = f"d{symbol}_{kind_label[s.kind]}_0x{s.offset:04X}"

    return slices, dls


# ---------------------------------------------------------------------------
# Invariant checks (I1-I4) — hard failures
# ---------------------------------------------------------------------------

class InvariantError(RuntimeError):
    pass


def fail(inv, what, symbol, diagnose):
    raise InvariantError(
        f"[deblob] {inv} violated: {what} ({symbol}) — diagnose: {diagnose}")


def slice_containing(slices, off):
    lo, hi = 0, len(slices) - 1
    while lo <= hi:
        mid = (lo + hi) // 2
        s = slices[mid]
        if off < s.offset:
            hi = mid - 1
        elif off >= s.end:
            lo = mid + 1
        else:
            return mid
    return None


def check_invariants(symbol, slices, intern_chain, extern_chain, file_size,
                     view, original, report):
    diag = f"python tools/generate_fighter_slices.py --only {symbol} --check"

    # I1: exhaustive, sorted, non-overlapping tiling
    cursor = 0
    for s in slices:
        if s.offset != cursor:
            fail("I1", f"tiling hole/overlap at 0x{cursor:X} (next slice 0x{s.offset:X})",
                 symbol, diag)
        cursor = s.end
    if cursor != file_size:
        fail("I1", f"tiling ends at 0x{cursor:X}, file size 0x{file_size:X}",
             symbol, diag)

    # I2: every intern/extern slot wholly inside exactly one slice
    for e in intern_chain + extern_chain:
        idx = slice_containing(slices, e["slot_off"])
        if idx is None or e["slot_off"] + 4 > slices[idx].end:
            fail("I2", f"chain slot at 0x{e['slot_off']:X} not inside one slice",
                 symbol, diag)

    # I3: every intern target inside exactly one slice
    for e in intern_chain:
        tgt = e["target_words"] * 4
        if slice_containing(slices, tgt) is None:
            fail("I3", f"chain target 0x{tgt:X} not inside any slice", symbol, diag)

    # I4: inverse-relocation identity
    if rl.restore_chain_form(view, intern_chain) != original:
        fail("I4", "restoring chain descriptors does not reproduce ROM bytes",
             symbol, diag)


# ---------------------------------------------------------------------------
# Reports
# ---------------------------------------------------------------------------

def build_reports(slices, dls, intern_chain, original, report):
    # mid-slice targets: refs/targets landing at delta > 0 inside a slice
    mids = []
    for e in intern_chain:
        tgt = e["target_words"] * 4
        idx = slice_containing(slices, tgt)
        s = slices[idx]
        if tgt != s.offset:
            mids.append({"target": tgt, "slice": s.name,
                         "delta": tgt - s.offset, "via": "chain"})
    for start in dls:
        for ref in dls[start][1]:
            idx = slice_containing(slices, ref.target)
            s = slices[idx]
            if ref.target != s.offset:
                mids.append({"target": ref.target, "slice": s.name,
                             "delta": ref.target - s.offset,
                             "via": f"dl@0x{start:X}+0x{ref.cmd_off - start:X}"})
    report["mid_slice_targets"] = sorted(mids, key=lambda m: (m["target"], m["via"]))

    # flat-scan cross-check: typed-looking regions that stayed blob
    missed = []
    for kind, tgt, size in flat_scan(original):
        idx = slice_containing(slices, tgt)
        if idx is not None and slices[idx].kind in ("blob", "gap", "pad"):
            missed.append({"kind": kind, "offset": tgt, "size": size,
                           "slice": slices[idx].name})
    report["flat_scan_unclaimed"] = missed


# ---------------------------------------------------------------------------
# Emission
# ---------------------------------------------------------------------------

YAML_TYPE = {"gfx": "GFX", "vtx": "VTX", "tex": "TEXTURE", "lut": "TEXTURE",
             "blob": "BLOB", "gap": "BLOB", "pad": "BLOB"}
SPEC_KIND = {"gfx": "DisplayList", "vtx": "Vertex", "tex": "Texture",
             "lut": "Texture", "blob": "Blob", "gap": "Blob", "pad": "Blob"}


def provenance(args, layout, romsha):
    return {
        "generator": f"tools/generate_fighter_slices.py v{TOOL_VERSION}",
        "regen": f"python tools/generate_fighter_slices.py --version {layout.version}",
        "rom_sha1": romsha,
        "version": layout.version,
    }


def emit_manifest(path, prov, symbol, category, info, extern_ids, slices,
                  intern_chain, extern_chain, view_crc, report):
    slice_index = {s.offset: i for i, s in enumerate(slices)}

    def slot_ref(e):
        idx = slice_containing(slices, e["slot_off"])
        return idx, e["slot_off"] - slices[idx].offset

    intern = []
    for e in intern_chain:
        s_idx, s_delta = slot_ref(e)
        tgt = e["target_words"] * 4
        t_idx = slice_containing(slices, tgt)
        intern.append({
            "slot_slice": s_idx, "slot_offset_in_slice": s_delta,
            "target_slice": t_idx,
            "target_offset_in_slice": tgt - slices[t_idx].offset,
        })
    extern = []
    for i, e in enumerate(extern_chain):
        s_idx, s_delta = slot_ref(e)
        extern.append({
            "slot_slice": s_idx, "slot_offset_in_slice": s_delta,
            "dep_file_id": extern_ids[i] if i < len(extern_ids) else None,
            "dep_word_offset": e["target_words"],
        })

    manifest = {
        "provenance": prov,
        "symbol": symbol,
        "category": category,
        "file_id": info["file_id"],
        "parent_path": f"{category_dir(category)}/{symbol}",
        "vanilla_data_size": info["decompressed_bytes"],
        "relocated_view_crc32": view_crc,
        "reloc_intern_offset": info["reloc_intern_offset"],
        "reloc_extern_offset": info["reloc_extern_offset"],
        "extern_file_ids": extern_ids,
        "slices": [
            {"name": s.name, "offset": s.offset, "size": s.size,
             "kind": s.kind, **s.extra}
            for s in slices
        ],
        "intern_slots": intern,
        "extern_slots": extern,
        "reports": report,
    }
    body = json.dumps(manifest, indent=1, sort_keys=True)
    manifest["content_hash"] = hashlib_sha1(body)
    path.write_text(json.dumps(manifest, indent=1, sort_keys=True) + "\n")
    return manifest


def hashlib_sha1(text):
    import hashlib
    return hashlib.sha1(text.encode()).hexdigest()


def category_dir(category):
    return f"reloc_{category}"


def emit_child_yaml(path, prov, symbol, category, info, slices):
    has_gfx = any(s.kind == "gfx" for s in slices)
    out = [
        f"# {symbol} slice map — typed extraction of the decompressed reloc bundle",
        f"# Auto-generated by {prov['generator']} — DO NOT EDIT",
        f"# Regenerate: {prov['regen']}",
        f"# ROM sha1: {prov['rom_sha1']}   file_id: {info['file_id']}",
        "",
        ":config:",
        "  force: true",
        "  external_files:",
        f"    - yamls/{prov['version']}/{category_dir(category)}.yml",
    ]
    if has_gfx:
        # segment 0x07 is a torch-indexing convention: node offsets are
        # rewritten to (seg<<24)|offset in the first pass and DL pointer
        # operands (plain byte offsets on the relocated view) are matched
        # back through GetNodeByAddr's segmented fallback.
        out += ["  segments:", "    - [0x07, 0x0]"]
    out.append(f"  ssb64_reloc_parent: {symbol}")
    out.append("")

    for s in slices:
        out.append(f"{s.name}:")
        out.append(f"  type: {YAML_TYPE[s.kind]}")
        out.append(f"  symbol: {s.name}")
        out.append(f"  offset: 0x{s.offset:X}")
        out.append(f"  size: 0x{s.size:X}")
        if s.kind in ("gfx", "vtx"):
            out.append(f"  count: {s.extra['count']}")
        elif s.kind == "tex":
            out.append(f"  format: {s.extra['format']}")
            out.append(f"  width: {s.extra['width']}")
            out.append(f"  height: {s.extra['height']}")
        elif s.kind == "lut":
            out.append("  format: TLUT")
            out.append(f"  colors: {s.extra['colors']}")
            out.append("  ctype: u16")
        out.append("")

    path.write_text("\n".join(out))


def emit_spec_cpp(path, prov, manifests):
    """One generated TU per region: slice/slot arrays + a file_id-sorted spec
    table consumed via binary search (port/bridge/ssb64_reloc_rebuild.h)."""
    L = [
        "// SSB64 synthetic reloc rebuild specs — vanilla layout of every",
        "// deblobbed bundle (see docs/deblob.md). All offsets/sizes describe",
        "// the VANILLA layout: the runtime treats them as lookup keys and",
        "// fast-path values, never as invariants for mod-replaced slices.",
        f"// Auto-generated by {prov['generator']} — DO NOT EDIT",
        f"// Regenerate: {prov['regen']}",
        f"// ROM sha1: {prov['rom_sha1']}",
        "",
        '#include "ssb64_reloc_rebuild.h"',
        "",
        "namespace {",
        "",
    ]
    ordered = sorted(manifests, key=lambda m: m["file_id"])
    for m in ordered:
        sym = m["symbol"]
        L.append(f"// --- {sym} (file_id {m['file_id']}) ---")
        L.append(f"const SSB64SyntheticSlice k{sym}Slices[] = {{")
        for s in m["slices"]:
            L.append(
                f'    {{ "{m["parent_path"]}/{s["name"]}", '
                f'0x{s["offset"]:X}, 0x{s["size"]:X}, '
                f'SSB64SyntheticSliceKind::{SPEC_KIND[s["kind"]]} }},')
        L.append("};")
        if m["intern_slots"]:
            L.append(f"const SSB64RelocInternSlot k{sym}InternSlots[] = {{")
            for e in m["intern_slots"]:
                L.append(f'    {{ {e["slot_slice"]}, {e["target_slice"]}, '
                         f'0x{e["slot_offset_in_slice"]:X}, '
                         f'0x{e["target_offset_in_slice"]:X} }},')
            L.append("};")
        if m["extern_slots"]:
            L.append(f"const SSB64RelocExternSlot k{sym}ExternSlots[] = {{")
            for e in m["extern_slots"]:
                L.append(f'    {{ {e["slot_slice"]}, {e["dep_file_id"]}, '
                         f'0x{e["slot_offset_in_slice"]:X}, '
                         f'0x{e["dep_word_offset"]:X} }},')
            L.append("};")
        L.append("")

    L.append("} // namespace")
    L.append("")
    L.append("const SSB64SyntheticRelocSpec gSyntheticRelocSpecs[] = {")
    for m in ordered:
        sym = m["symbol"]
        intern = (f"k{sym}InternSlots, {len(m['intern_slots'])}"
                  if m["intern_slots"] else "nullptr, 0")
        extern = (f"k{sym}ExternSlots, {len(m['extern_slots'])}"
                  if m["extern_slots"] else "nullptr, 0")
        L.append(
            f'    {{ {m["file_id"]}, "{m["parent_path"]}", '
            f'0x{m["vanilla_data_size"]:X}, 0x{m["relocated_view_crc32"]:08X}u, '
            f'"{m["content_hash"]}", '
            f'k{sym}Slices, {len(m["slices"])}, {intern}, {extern} }},')
    L.append("};")
    L.append("const uint32_t gSyntheticRelocSpecCount = "
             f"{len(ordered)};")
    L.append("")
    path.write_text("\n".join(L))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def collect_incoming_extern_targets(rom, layout, deblobbed_ids):
    """One pass over every reloc file's extern chain, bucketing the targets
    that point INTO a deblobbed bundle: {dep_file_id: [(src_file_id, off)]}.
    Only files whose extern-id list intersects the deblobbed set need their
    chain walked (and thus decompressed)."""
    incoming = {fid: [] for fid in deblobbed_ids}
    for fid in range(layout.file_count):
        info = rl.read_table_entry(rom, layout, fid)
        if info["reloc_extern_offset"] == 0xFFFF:
            continue
        ids = rl.read_extern_ids(rom, layout, info)
        if not set(ids) & deblobbed_ids:
            continue
        _, data = rl.extract_file(rom, layout, fid)
        chain = rl.walk_chain(data, info["reloc_extern_offset"])
        for i, e in enumerate(chain):
            if i < len(ids) and ids[i] in deblobbed_ids:
                incoming[ids[i]].append((fid, e["target_words"] * 4))
    return incoming


def process_file(symbol, category, rom, layout, prov, out_dir, check_only,
                 incoming):
    info, original = rl.extract_file(rom, layout, symbol_file_ids[symbol])
    extern_ids = rl.read_extern_ids(rom, layout, info)

    intern_chain = (rl.walk_chain(original, info["reloc_intern_offset"])
                    if info["reloc_intern_offset"] != 0xFFFF else [])
    extern_chain = (rl.walk_chain(original, info["reloc_extern_offset"])
                    if info["reloc_extern_offset"] != 0xFFFF else [])
    if len(extern_chain) != len(extern_ids):
        raise InvariantError(
            f"[deblob] extern chain length {len(extern_chain)} != extern id "
            f"count {len(extern_ids)} ({symbol})")

    view = rl.make_relocated_view(original, intern_chain)

    incoming_refs = incoming.get(info["file_id"], [])
    incoming_targets = sorted({off for _, off in incoming_refs
                               if off < len(original)})

    report = {"demotions": [],
              "incoming_extern_refs": [
                  {"from_file_id": src, "target": off}
                  for src, off in sorted(set(incoming_refs))]}
    slices, dls = build_slices(symbol, view, intern_chain, extern_chain,
                               incoming_targets, len(original), report)
    check_invariants(symbol, slices, intern_chain, extern_chain,
                     len(original), view, original, report)
    build_reports(slices, dls, intern_chain, original, report)

    if check_only:
        return None

    manifests_dir = out_dir / "manifests"
    manifests_dir.mkdir(parents=True, exist_ok=True)
    manifest = emit_manifest(manifests_dir / f"{symbol}.json", prov, symbol,
                             category, info, extern_ids, slices, intern_chain,
                             extern_chain, zlib.crc32(view), report)
    emit_child_yaml(out_dir / f"{symbol}.yml", prov, symbol, category, info,
                    slices)
    return manifest


symbol_file_ids = {}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--version", choices=sorted(rl.VERSIONS), default="us")
    ap.add_argument("--rom", default=None)
    ap.add_argument("--only", default=None, metavar="SYMBOL")
    ap.add_argument("--check", action="store_true",
                    help="validate only, write nothing")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parent.parent
    layout = rl.RelocLayout(args.version)
    rom_path = args.rom or repo / f"baserom.{args.version}.z64"
    rom = rl.load_rom(rom_path)
    prov = provenance(args, layout, rl.rom_sha1(rom))

    # incoming extern targets need the full deblobbed id set up front
    deblobbed_ids = set()
    category_entries = {}
    for category in sorted(DEBLOBBED_CATEGORIES):
        parent_yaml = repo / "yamls" / args.version / f"{category_dir(category)}.yml"
        entries = rl.read_parent_yaml_entries(parent_yaml)
        category_entries[category] = entries
        deblobbed_ids |= {fid for _, fid in entries}
    incoming = collect_incoming_extern_targets(rom, layout, deblobbed_ids)

    all_manifests = []
    failures = []
    for category in sorted(DEBLOBBED_CATEGORIES):
        out_dir = repo / "yamls" / args.version / category_dir(category)
        for symbol, file_id in category_entries[category]:
            if args.only and symbol != args.only:
                continue
            symbol_file_ids[symbol] = file_id
            try:
                m = process_file(symbol, category, rom, layout, prov, out_dir,
                                 args.check, incoming)
                if m:
                    all_manifests.append(m)
                stats = ""
                if m:
                    kinds = {}
                    for s in m["slices"]:
                        kinds[s["kind"]] = kinds.get(s["kind"], 0) + 1
                    stats = "  " + " ".join(f"{k}:{v}" for k, v in sorted(kinds.items()))
                print(f"  ok   {symbol} (file_id {file_id}){stats}")
            except InvariantError as e:
                failures.append(str(e))
                print(f"  FAIL {symbol}: {e}", file=sys.stderr)

    if failures:
        print(f"\n{len(failures)} file(s) failed invariants", file=sys.stderr)
        return 1

    if not args.check and not args.only:
        spec_path = repo / "port" / "bridge" / f"ssb64_reloc_rebuild.{args.version}.cpp"
        emit_spec_cpp(spec_path, prov, all_manifests)
        print(f"\nWrote {spec_path} ({len(all_manifests)} specs)")
    elif not args.check and args.only:
        print("\n--only given: spec .cpp NOT regenerated (needs a full run)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

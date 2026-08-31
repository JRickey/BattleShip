#!/usr/bin/env python3
"""deblob_modkit — build repeatable mod-override test artifacts (docs/deblob.md).

Produces small mods-folder .o2r files that exercise the deblob relayout
path end-to-end against a real extracted archive:

  grow-dl   <Symbol> <SliceName> [n]   append n G_NOOPs to a DL slice
                                       (renders pixel-identical; proves
                                       relayout + reference patching)
  grow-blob <Symbol> <SliceName> [n]   append n zero bytes to a blob slice
  break     <Symbol> <SliceName>       truncate a slice mid-way — the port
                                       must fail loudly (I9/size checks),
                                       never render corruption

Usage:
    python debug_tools/deblob_modkit.py grow-dl LinkModel dLinkModel_Gfx_0x1D88 \
        [--archive build/extracted/BattleShip.o2r] [--out mods/deblob_test.o2r]

Drop the output in the game's mods/ directory (next to the binary) and
play the affected character. `SSB64_SYNTH_INSPECT=<file_id>` shows the
relayout decisions; a clean run logs prefix-preservation warnings only.
"""

import argparse
import sys
import zipfile
from pathlib import Path


def load_entry(archive, symbol, slice_name):
    path = None
    with zipfile.ZipFile(archive) as z:
        for name in z.namelist():
            if name.endswith(f"/{symbol}/{slice_name}"):
                path = name
                break
        if path is None:
            sys.exit(f"error: no entry */{symbol}/{slice_name} in {archive}")
        return path, bytearray(z.read(path))


def write_mod(out, path, data):
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr(path, bytes(data))
    print(f"wrote {out}: {path} ({len(data)} bytes)")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("mode", choices=["grow-dl", "grow-blob", "break"])
    ap.add_argument("symbol")
    ap.add_argument("slice_name")
    ap.add_argument("n", nargs="?", type=int, default=2)
    ap.add_argument("--archive", default="build/extracted/BattleShip.o2r")
    ap.add_argument("--out", default="deblob_test.o2r")
    args = ap.parse_args()

    path, data = load_entry(args.archive, args.symbol, args.slice_name)

    if args.mode == "grow-dl":
        # OTR DL binary stores LE words; torch appends a G_ENDDL
        # terminator (word 0xDF000000 -> bytes 00 00 00 df LE).
        if data[-8:-4] != b"\x00\x00\x00\xdf":
            sys.exit(f"error: {args.slice_name} does not end with G_ENDDL "
                     f"(tail {data[-8:].hex()}) — is it a DL slice?")
        grown = data[:-8] + b"\x00" * (8 * args.n) + data[-8:]
        write_mod(args.out, path, grown)
        print(f"appended {args.n} G_NOOP command(s) before the terminator")
    elif args.mode == "grow-blob":
        write_mod(args.out, path, data + b"\x00" * args.n)
        print(f"appended {args.n} zero byte(s) — Blob payload grows by that "
              f"amount (the LUS factory pad is part of the container)")
    else:  # break
        cut = max(len(data) // 2, 1)
        write_mod(args.out, path, data[:cut])
        print("truncated — the port must reject this bundle with a clear "
              "[deblob] error, never render it")


if __name__ == "__main__":
    main()

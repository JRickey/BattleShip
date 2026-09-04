#!/usr/bin/env python3
"""
render_nameplates.py — Render a stage-select nameplate PNG from a text string.

Usage:
    python3 tools/render_nameplates.py <TEXT> <output.png>

Renders <TEXT> as black-on-transparent 96x10 pixel-art text using the
embedded 5x7 pixel font below (pure 1-bit alpha by construction), matching
the ROM's IA4 nameplate sprite dimensions. This is PURE TEXT RENDERING — no ROM data is involved — so the
output is legal to bake into shipped binaries (see the CMake nameplate
custom commands, which pipe these PNGs through png_to_c_array.py into
headers used by port/css_icons/port_css_stage_assets.cpp as the release
fallback for port-introduced stages).

tools/derive_stage_assets.py imports render_name_png from here so the dev
PNG pipeline and the baked pipeline can never drift.
"""

import sys
from pathlib import Path

# Nameplate PNG dimensions — derived from ROM IA4 nameplate sprites in
# reloc file 30 (MNMaps), e.g. PeachsCastleText at offset 0x1f8:
#   width=96, height=10, bmfmt=4 (G_IM_FMT_IA), bmsiz=0 (G_IM_SIZ_4b=IA4)
# The SObj caller forces red=green=blue=0x00 (black), so only the alpha
# channel matters at runtime. The PNG is stored as RGBA with black opaque
# pixels on a transparent background.
NAME_W = 96
NAME_H = 10


# ---------------------------------------------------------------------------
# Embedded 5x7 pixel font (classic HD44780-style letterforms, public domain
# shapes). Rendering from a hand-defined bitmap keeps the output pure 1-bit
# by construction — the runtime converts the PNG to RGBA16 whose alpha is a
# single bit, and any anti-aliased rendering (which modern Pillow's
# load_default() produces) gets shredded by that threshold into broken
# glyphs (observed: METAL CAVERN's R rendering as "P."). A bitmap font is
# also deterministic across Pillow versions. 'I' is 3 wide so
# "FINAL DESTINATION" (the widest current string, 93 px) fits the 96 px
# canvas. Extend FONT when a new stage name needs more characters.
# ---------------------------------------------------------------------------
FONT = {
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "B": ["11110", "10001", "10001", "11110", "10001", "10001", "11110"],
    "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    "D": ["11100", "10010", "10001", "10001", "10001", "10010", "11100"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["111", "010", "010", "010", "010", "010", "111"],
    "J": ["00111", "00010", "00010", "00010", "00010", "10010", "01100"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "Q": ["01110", "10001", "10001", "10001", "10101", "10010", "01101"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    "W": ["10001", "10001", "10001", "10101", "10101", "10101", "01010"],
    "X": ["10001", "10001", "01010", "00100", "01010", "10001", "10001"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "Z": ["11111", "00001", "00010", "00100", "01000", "10000", "11111"],
    "'": ["01", "01", "10", "00", "00", "00", "00"],
    ".": ["00", "00", "00", "00", "00", "00", "11"],
    " ": ["000", "000", "000", "000", "000", "000", "000"],
}
GLYPH_H = 7
TRACKING = 1  # blank columns between glyphs


def measure_text(text: str) -> int:
    width = 0
    for i, ch in enumerate(text):
        if ch not in FONT:
            raise ValueError(f"render_nameplates: no glyph for {ch!r} — extend FONT")
        width += len(FONT[ch][0])
        if i != len(text) - 1:
            width += TRACKING
    return width


def render_name_png(text: str, output_path: Path) -> None:
    """
    Render text as black-on-transparent NAME_W x NAME_H pixel-art, using the
    embedded 5x7 font: pure 1-bit alpha, exactly centered in the canvas
    (the runtime anchors the sprite so the canvas center sits on the plate
    pill's center).
    """
    from PIL import Image

    text_w = measure_text(text)
    if text_w > NAME_W:
        raise ValueError(
            f"render_nameplates: {text!r} is {text_w}px wide — exceeds the "
            f"{NAME_W}px nameplate canvas; shorten it or narrow glyphs")

    x = (NAME_W - text_w) // 2
    y = (NAME_H - GLYPH_H) // 2

    img = Image.new("RGBA", (NAME_W, NAME_H), (0, 0, 0, 0))
    for ch in text:
        rows = FONT[ch]
        for ry, row in enumerate(rows):
            for rx, bit in enumerate(row):
                if bit == "1":
                    img.putpixel((x + rx, y + ry), (0, 0, 0, 255))
        x += len(rows[0]) + TRACKING

    output_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(output_path)

    filesize = output_path.stat().st_size
    print(
        f"[{text}] Saved nameplate: {output_path} ({NAME_W}x{NAME_H}, "
        f"ink {text_w}px, {filesize} bytes)",
        file=sys.stderr,
    )


def main(argv: list) -> int:
    if len(argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    render_name_png(argv[0], Path(argv[1]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

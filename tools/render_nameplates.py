#!/usr/bin/env python3
"""
render_nameplates.py — Render a stage-select nameplate PNG from a text string.

Usage:
    python3 tools/render_nameplates.py <TEXT> <output.png>

Renders <TEXT> as black-on-transparent 96x10 pixel-art text (PIL's built-in
bitmap font, pure 1-bit alpha), matching the ROM's IA4 nameplate sprite
dimensions. This is PURE TEXT RENDERING — no ROM data is involved — so the
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


def render_name_png(text: str, output_path: Path) -> None:
    """
    Render text as a black-on-transparent nameplate PNG at NAME_W x NAME_H.

    Font: PIL's built-in bitmap font (no external TTF dependency).  The
    natural size (8px tall glyphs) fits cleanly in a 10-row canvas with one
    row of padding at top and bottom.  Anti-aliasing is off by construction
    (bitmap font), so the output is pure 1-bit alpha — pixel-art style
    matching the ROM's IA4 nameplates.
    """
    from PIL import Image, ImageDraw, ImageFont

    font = ImageFont.load_default()

    # Measure on a scratch image (avoid drawing to destination before centering).
    probe = Image.new("RGBA", (NAME_W * 4, NAME_H * 2), (0, 0, 0, 0))
    probe_draw = ImageDraw.Draw(probe)
    bbox = probe_draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]

    # Center horizontally; place baseline so text is vertically centered.
    x = (NAME_W - text_w) // 2 - bbox[0]
    y = (NAME_H - text_h) // 2 - bbox[1]

    # Draw on the real canvas.
    img = Image.new("RGBA", (NAME_W, NAME_H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.text((x, y), text, font=font, fill=(0, 0, 0, 255))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(output_path)

    filesize = output_path.stat().st_size
    print(
        f"[{text}] Saved nameplate: {output_path} ({NAME_W}x{NAME_H}, {filesize} bytes)",
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

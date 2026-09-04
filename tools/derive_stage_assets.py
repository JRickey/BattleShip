#!/usr/bin/env python3
"""
derive_stage_assets.py — Extract CSS stage assets from baserom.us.z64.

For each stage in the STAGES manifest this script:
  1. Extracts the reloc file from baserom.us.z64 (using the VPK0/reloc logic
     shared with debug_tools/reloc_extract/reloc_extract.py).
  2. Parses the Sprite struct at the specified byte offset (big-endian ROM layout).
  3. Resolves each Bitmap's texel-buffer pointer using the SSB64 RELOC linked-list
     encoding (see "RELOC internal-pointer resolver" section below).
  4. Un-swizzles the N64 TMEM odd-row XOR4 pattern for 16bpp textures.
  5. Stitches all bitmap strips into one full RGBA image.
  6. Saves <output_dir>/<name>_background.png  (full resolution, e.g. 300x220).
  7. Saves <output_dir>/<name>_small.png       (48x36 LANCZOS downscale).
  8. If the stage entry has a "name_text" key: renders a black-on-transparent
     nameplate PNG at 96x10 (matching the ROM IA4 nameplate sprite dimensions)
     and saves it as <output_dir>/<name>_name.png.

Usage:
  python3 tools/derive_stage_assets.py <baserom.z64> <output_dir>

The output PNGs are loaded at runtime by port/css_icons/port_css_stage_assets.cpp
via stb_image. They are NOT committed to the repo — they are derived from the
user's own baserom at build time by the CMake pipeline.

Adding a new stage:
  Append an entry to the STAGES list below and re-run (CMake will re-run
  automatically because derive_stage_assets.py is listed as a DEPENDS).
"""

import struct
import sys
from pathlib import Path
from PIL import Image

# ---------------------------------------------------------------------------
# Import the VPK0 decoder and RELOC extractor from the existing debug tool.
# We add the debug_tools/reloc_extract directory to the path and import the
# relevant functions directly so we don't duplicate the logic.
# ---------------------------------------------------------------------------
_SCRIPT_DIR = Path(__file__).resolve().parent
_REPO_ROOT   = _SCRIPT_DIR.parent
sys.path.insert(0, str(_REPO_ROOT / "debug_tools" / "reloc_extract"))
from reloc_extract import extract_file  # type: ignore  # noqa: E402

# Nameplate rendering is shared with the ROM-free baked pipeline (the CMake
# nameplate custom commands call render_nameplates.py directly) so the dev
# PNG and the baked release fallback can never drift.
sys.path.insert(0, str(_SCRIPT_DIR))
from render_nameplates import NAME_W, NAME_H, render_name_png  # noqa: E402

# ---------------------------------------------------------------------------
# Stage manifest.
#
# To add a new port-introduced stage:
#   1. Append a dict with these keys:
#        name        — filename stem (e.g. "brinstar")
#        gkind       — GRKind enum value (for runtime lookup table)
#        reloc_file  — file index inside the ROM's RELOC table
#        sprite_off  — byte offset of the Sprite struct inside that file
#        name_text   — (optional) string to render as a synthetic nameplate PNG.
#                      Only needed when no ROM-shipped nameplate exists for this
#                      stage (e.g. nGRKindLast/Final Destination). Stages that
#                      ship a ROM nameplate should omit this key.
#   2. Re-run this script (or trigger a CMake rebuild — it lists this file
#      and baserom.us.z64 as DEPENDS so it re-runs automatically).
#   3. The new <name>_background.png and <name>_small.png appear in the
#      output directory and are picked up by portCSSGetStageBackgroundSprite()
#      and portCSSGetStageIconSprite() at runtime via the same gkind lookup.
#   4. If name_text is set, <name>_name.png also appears and is picked up
#      by portCSSGetStageNameSprite() at runtime.
# ---------------------------------------------------------------------------
STAGES = [
    {
        "name":       "final_destination",   # filename stem
        "gkind":      16,                    # nGRKindLast
        "reloc_file": 96,                    # file 96 = StageLastFile1
        "sprite_off": 0x26c88,              # dStageLastBackground_0x26c88
        "name_text":  "FINAL DESTINATION",  # synthetic nameplate (no ROM sprite)
        "emblem_src": {
            "reloc_file": 345,              # llMasterHandIconFileID = 0x159
            "sprite_off": 0x2b8,            # llMasterHandIconFTEmblemSprite = 0x2b8
        },
    },
    {
        # nGRKindMetal — Meta Crystal (Metal Mario boss stage). The stage data
        # file 0x62 ships a full 300x220 CSS wallpaper at the same 0x26c88
        # offset every Stage*FileID uses; the matching map file (0x10d) carries
        # collision data, not visuals.
        "name":       "metal_cavern",
        "gkind":      13,                    # nGRKindMetal
        "reloc_file": 0x62,                  # ll_98_FileID — Meta Crystal stage data
        "sprite_off": 0x26c88,
        "name_text":  "METAL CAVERN",        # no ROM nameplate exists; synthesize
        # No emblem_src — the FD emblem path is still stubbed off in
        # mnMapsMakeEmblem pending IA4 debug; new stages take the same skip.
    },
    {
        # nGRKindZako — Duel Zone (Polygon Fighters arena). Same wallpaper-in-
        # stage-data-file pattern as Metal Cavern; stage data file is 0x61.
        "name":       "battlefield",
        "gkind":      14,                    # nGRKindZako
        "reloc_file": 0x61,                  # ll_97_FileID — Duel Zone stage data
        "sprite_off": 0x26c88,
        "name_text":  "BATTLEFIELD",
    },
]

# ---------------------------------------------------------------------------
# Nameplate dimensions/rendering live in tools/render_nameplates.py (shared
# with the ROM-free baked pipeline); NAME_W/NAME_H/render_name_png imported.
# ---------------------------------------------------------------------------
# CSS thumbnail dimensions — same for all stage icons (matches N64 CSS format).
ICON_W = 48
ICON_H = 36

# FT emblem target dimensions — matches every llFTEmblemSprites* sprite in file 20
# (width=64, height=48, bmfmt=4/IA, bmsiz=0/IA4, verified empirically).
EMBLEM_W = 64
EMBLEM_H = 48

# ---------------------------------------------------------------------------
# Sprite struct / Bitmap struct sizes (bytes).
# ---------------------------------------------------------------------------
SPRITE_SIZE = 68
BITMAP_SIZE = 16

# ---------------------------------------------------------------------------
# Big-endian field readers.
# ---------------------------------------------------------------------------


def _be_s16(data: bytes, off: int) -> int:
    return struct.unpack_from(">h", data, off)[0]


def _be_u32(data: bytes, off: int) -> int:
    return struct.unpack_from(">I", data, off)[0]


def _be_u8(data: bytes, off: int) -> int:
    return data[off]


# ---------------------------------------------------------------------------
# Sprite struct parser (big-endian ROM layout, total 68 bytes).
#
# Field offsets (from port_css_fd_icon.cpp header comment):
#   0x00  s16 x, y
#   0x04  s16 width, height
#   0x08  f32 scalex
#   0x0C  f32 scaley
#   0x10  s16 expx, expy
#   0x14  u16 attr, s16 zdepth
#   0x18  u8  red, green, blue, alpha
#   0x1C  s16 startTLUT, nTLUT
#   0x20  u32 LUT  (reloc token)
#   0x24  s16 istart, istep
#   0x28  s16 nbitmaps, ndisplist
#   0x2C  s16 bmheight, bmHreal
#   0x30  u8  bmfmt, u8 bmsiz, pad, pad
#   0x34  u32 bitmap pointer (reloc token)
#   0x38  u32 rsp_dl
#   0x3C  u32 rsp_dl_next
#   0x40  s16 frac_s, frac_t
#
# Bitmap struct (16 bytes, big-endian ROM layout):
#   0x00  s16 width, width_img
#   0x04  s16 s, t
#   0x08  u32 buf  (reloc token)
#   0x0C  s16 actualHeight, LUToffset
# ---------------------------------------------------------------------------


def parse_sprite(file_data: bytes, sprite_off: int) -> dict:
    """
    Parse the Sprite struct at sprite_off inside file_data (big-endian).
    Returns a dict with the fields we care about.
    """
    d = file_data
    o = sprite_off
    sp = {
        "x":         _be_s16(d, o + 0x00),
        "y":         _be_s16(d, o + 0x02),
        "width":     _be_s16(d, o + 0x04),
        "height":    _be_s16(d, o + 0x06),
        "nbitmaps":  _be_s16(d, o + 0x28),
        "ndisplist": _be_s16(d, o + 0x2A),
        "bmheight":  _be_s16(d, o + 0x2C),
        "bmHreal":   _be_s16(d, o + 0x2E),
        "bmfmt":     _be_u8(d, o + 0x30),
        "bmsiz":     _be_u8(d, o + 0x31),
        "sprite_off": sprite_off,
    }
    return sp


def parse_bitmap(file_data: bytes, bm_off: int) -> dict:
    """Parse one Bitmap struct at bm_off (big-endian)."""
    d = file_data
    o = bm_off
    bm = {
        "width":        _be_s16(d, o + 0x00),
        "width_img":    _be_s16(d, o + 0x02),
        "s":            _be_s16(d, o + 0x04),
        "t":            _be_s16(d, o + 0x06),
        "buf_raw":      _be_u32(d, o + 0x08),
        "actualHeight": _be_s16(d, o + 0x0C),
        "LUToffset":    _be_s16(d, o + 0x0E),
        "off": o,
    }
    return bm


# ---------------------------------------------------------------------------
# RELOC internal-pointer resolver.
#
# After VPK0 decompression the file bytes contain pre-relocation data.
# The N64 OS would patch each internal-reloc slot by adding the KSEG0 load
# address; we see the raw pre-patch values from reloc_extract.py.
#
# Encoding of each u32 internal-reloc slot (big-endian):
#   high 16 bits — word index of next reloc entry in the linked list
#                  (0xFFFF = end of list)
#   low  16 bits — (file_relative_byte_offset / 4) for the pointed-to data
#
# To recover the file-relative byte offset of the pointed-to data:
#   file_off = (raw_u32 & 0xFFFF) * 4
#
# Empirically verified against file 96 (FD background, 44-bitmap RGBA16 sprite):
#   - Sprite.bitmap ptr at sprite+0x34: raw=0xFFFF9A72 -> 0x9A72*4=0x269C8
#     (which is the Bitmap array start in that file)
#   - Bitmap[0].buf at bm+0x08: raw=0x9A780002 -> 0x0002*4=0x8
#     (start of first texel strip, 8 bytes after the file start for alignment)
# ---------------------------------------------------------------------------


def resolve_reloc_ptr(file_data: bytes, field_off: int) -> int:
    """
    Read the 4-byte big-endian reloc pointer at field_off and return the
    file-relative byte offset it encodes: (raw_u32 & 0xFFFF) * 4.
    """
    raw = _be_u32(file_data, field_off)
    return (raw & 0xFFFF) * 4


def resolve_bitmap_offsets(file_data: bytes, sp: dict) -> list:
    """
    Locate the Bitmap array (via the sprite's bitmap ptr reloc field) and
    resolve each bitmap's buf field to a file-relative byte offset.
    Returns a list of dicts (one per bitmap) with an added 'buf_off' key.
    """
    sprite_off = sp["sprite_off"]
    nbitmaps   = sp["nbitmaps"]

    # Resolve Bitmap array location from sprite+0x34 (bitmap ptr reloc field).
    bm_array_start = resolve_reloc_ptr(file_data, sprite_off + 0x34)

    bitmaps = []
    for i in range(nbitmaps):
        bm_off = bm_array_start + i * BITMAP_SIZE
        bm = parse_bitmap(file_data, bm_off)
        # Resolve each bitmap's texel buf pointer.
        bm["buf_off"] = resolve_reloc_ptr(file_data, bm_off + 0x08)
        bitmaps.append(bm)

    return bitmaps


# ---------------------------------------------------------------------------
# TMEM odd-row un-swizzle for 16bpp (RGBA16 / IA16) textures.
#
# The N64 RDP pre-swizzles RGBA16/IA16 texture data stored in DRAM so that
# LOAD_BLOCK (dxt=0) can stream it into TMEM without stalls. For 16bpp, odd
# rows (strip-local row index within each bitmap) have the two 4-byte halves
# of each 8-byte qword swapped (XOR4 addressing). Fast3D reads texels linearly,
# so we must un-swap before writing pixels.
#
# Reference: docs/bugs/sprite_texel_tmem_swizzle_2026-04-10.md
# ---------------------------------------------------------------------------


def unswizzle_rgba16_strip(raw: bytes, width: int, height: int) -> bytes:
    """
    Un-swizzle one bitmap strip of RGBA16 data (width x height pixels,
    2 bytes per pixel, strip-local row indexing).

    For each odd row, within every 8-byte qword swap the two 4-byte halves.
    Returns the corrected bytes.
    """
    row_bytes = width * 2
    out = bytearray(raw)
    for row in range(height):
        if row % 2 == 0:
            continue  # even rows are fine
        row_start = row * row_bytes
        qword = 0
        while qword * 8 < row_bytes:
            q_off = row_start + qword * 8
            # Swap the two 4-byte halves within this 8-byte qword.
            a = out[q_off:q_off + 4]
            b = out[q_off + 4:q_off + 8]
            out[q_off:q_off + 4] = b
            out[q_off + 4:q_off + 8] = a
            qword += 1
    return bytes(out)


# ---------------------------------------------------------------------------
# RGBA16 -> RGBA8888 pixel decoder.
#
# N64 RGBA16 layout: R5 G5 B5 A1 (big-endian u16, MSB = R bit 4).
#   bits 15..11 = R5, bits 10..6 = G5, bits 5..1 = B5, bit 0 = A1
# ---------------------------------------------------------------------------


def rgba16_to_rgba8888(raw: bytes) -> bytes:
    """
    Convert a big-endian RGBA16 buffer to 8-bit RGBA.
    Returns bytes of length (len(raw) / 2) * 4.
    """
    npixels = len(raw) // 2
    out = bytearray(npixels * 4)
    for i in range(npixels):
        word = (raw[i * 2] << 8) | raw[i * 2 + 1]
        r5 = (word >> 11) & 0x1F
        g5 = (word >>  6) & 0x1F
        b5 = (word >>  1) & 0x1F
        a1 = word & 0x01
        out[i * 4 + 0] = (r5 << 3) | (r5 >> 2)  # expand 5 -> 8 bits
        out[i * 4 + 1] = (g5 << 3) | (g5 >> 2)
        out[i * 4 + 2] = (b5 << 3) | (b5 >> 2)
        out[i * 4 + 3] = 255 if a1 else 0
    return bytes(out)


# ---------------------------------------------------------------------------
# IA4 -> RGBA8888 pixel decoder.
#
# N64 IA4 layout: 4 bits per pixel, packed two-per-byte (big-endian nibble order):
#   high nibble of byte i -> pixel 2*i    (3-bit intensity, 1-bit alpha)
#   low  nibble of byte i -> pixel 2*i+1
#   bits 3..1 = I3, bit 0 = A1
#
# Alpha: 0 -> fully transparent, 1 -> fully opaque.
# Intensity: expanded 3-bit -> 8-bit: I8 = (I3 << 5) | (I3 << 2) | (I3 >> 1)
#   equivalent to round(I3 / 7.0 * 255).
#
# On the N64 the IA4 alpha channel equals the intensity when rendered with a
# combine mode that pipes intensity to alpha.  For CSS emblem use we emit
# (I, I, I, A) per pixel — the caller (mnmaps.c SObj path) overrides RGB
# channels to the franchise colour, leaving only alpha from the texture.
# ---------------------------------------------------------------------------


def ia4_to_rgba8888(raw: bytes, width: int, height: int) -> bytes:
    """
    Convert an N64 IA4 buffer to 8-bit RGBA.

    Each byte encodes two pixels (high nibble = left pixel, low nibble = right).
    The source stride in the ROM is padded to 32-bit alignment for LOAD_BLOCK
    (naturalWidth = round_up(width, 8) in nibbles, i.e. round_up(width, 8)//2
    bytes per row).  We read exactly (width+1)//2 * height bytes (the actual
    pixel data without alignment padding) unless the caller provides the padded
    stride — since parse_bitmap returns bitmap.width_img (the DMA-padded
    naturalWidth), we use that to compute the row stride.

    Args:
        raw:    Raw IA4 bytes (may include right-edge padding nibbles per row).
        width:  Rendered pixel width (the sprite's width field).
        height: Rendered pixel height.
    Returns:
        bytes of length width * height * 4 (RGBA8888).
    """
    # IA4 packs 2 pixels per byte.  Row stride in bytes uses width (already
    # padded to an even-nibble boundary by the sprite's width_img field which
    # resolve_bitmap_offsets reads as bm["width"]).  For safety, derive stride
    # from the actual rendered width (round up to next even number of pixels).
    row_stride = (width + 1) // 2  # bytes per row (2 pixels per byte)
    out = bytearray(width * height * 4)
    for row in range(height):
        for col in range(width):
            byte_idx = row * row_stride + col // 2
            nibble = (raw[byte_idx] >> 4) if (col % 2 == 0) else (raw[byte_idx] & 0x0F)
            i3 = (nibble >> 1) & 0x07
            a1 = nibble & 0x01
            # Expand 3-bit intensity to 8-bit: replicate MSBs into lower bits.
            i8 = (i3 << 5) | (i3 << 2) | (i3 >> 1)
            a8 = 255 if a1 else 0
            px = (row * width + col) * 4
            out[px + 0] = i8
            out[px + 1] = i8
            out[px + 2] = i8
            out[px + 3] = a8
    return bytes(out)


# ---------------------------------------------------------------------------
# Nameplate PNG renderer.
#
# Renders <text> as a black-on-transparent 96x10 PNG.  Uses PIL's built-in
# bitmap font (ImageFont.load_default()), which produces a pixel-art style
# glyph at a natural size of ~6 pixels tall (fitting comfortably in 10 rows).
#
# For "FINAL DESTINATION" the natural text width is 97px; centering clips the
# rightmost pixel column, which is transparent padding.  The result visually
# matches the ROM IA4 nameplates (same canvas dimensions, same 1-bit alpha
# style — the SObj caller forces all color channels to black at draw time).
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Emblem PNG extractor.
#
# Reads the source IA4 sprite from the reloc file specified in emblem_src,
# upscales it to EMBLEM_W x EMBLEM_H via LANCZOS, and saves an RGBA PNG.
#
# Target dimensions (EMBLEM_W=64, EMBLEM_H=48) are hard-coded because every
# llFTEmblemSprites* sprite in reloc file 20 is 64x48 IA4 (verified).
# ---------------------------------------------------------------------------


def extract_emblem(stage: dict, rom: bytes, output_dir: Path) -> None:
    """
    Extract and upscale the stage emblem sprite to EMBLEM_W x EMBLEM_H RGBA PNG.

    stage["emblem_src"] must contain:
        reloc_file: int  — reloc file index containing the source sprite
        sprite_off: int  — byte offset of the Sprite struct within that file
    """
    name = stage["name"]
    src  = stage["emblem_src"]
    reloc_file = src["reloc_file"]
    sprite_off = src["sprite_off"]

    print(
        f"[{name}] Extracting emblem from reloc file {reloc_file} "
        f"offset 0x{sprite_off:X}...",
        file=sys.stderr,
    )
    file_data = extract_file(rom, reloc_file)
    print(f"[{name}] Emblem file decompressed to {len(file_data)} bytes.", file=sys.stderr)

    sp = parse_sprite(file_data, sprite_off)
    src_w     = sp["width"]
    src_h     = sp["height"]
    nbitmaps  = sp["nbitmaps"]
    bmfmt     = sp["bmfmt"]
    bmsiz     = sp["bmsiz"]

    print(
        f"[{name}] Emblem source sprite: {src_w}x{src_h}, {nbitmaps} bitmap(s), "
        f"bmfmt={bmfmt} (4=IA), bmsiz={bmsiz} (0=IA4)",
        file=sys.stderr,
    )

    if bmfmt != 4 or bmsiz != 0:
        print(
            f"[{name}] WARNING: emblem sprite bmfmt={bmfmt}/bmsiz={bmsiz}, "
            f"expected IA4 (fmt=4, siz=0).  Decode may be wrong.",
            file=sys.stderr,
        )

    # Resolve bitmap array and get the single bitmap's buf.
    bitmaps = resolve_bitmap_offsets(file_data, sp)
    if not bitmaps:
        raise ValueError(f"[{name}] Emblem sprite has no bitmaps.")

    bm      = bitmaps[0]
    buf_off = bm["buf_off"]
    # IA4: (width_img) bytes per row (* height rows), width_img = round_up(width, 8)
    # bm["width"] is the DMA-padded row width (width_img field).
    row_stride = (bm["width"] + 1) // 2
    raw_bytes  = row_stride * src_h

    if buf_off < 0 or buf_off + raw_bytes > len(file_data):
        raise ValueError(
            f"[{name}] Emblem bitmap buf_off=0x{buf_off:X} out of range "
            f"(file_data={len(file_data):#X}, raw_bytes={raw_bytes})"
        )

    raw_strip = file_data[buf_off:buf_off + raw_bytes]

    # Decode IA4 -> RGBA8888.
    rgba8 = ia4_to_rgba8888(raw_strip, src_w, src_h)

    # Build PIL image and upscale to EMBLEM_W x EMBLEM_H via LANCZOS.
    src_img = Image.frombytes("RGBA", (src_w, src_h), rgba8)
    emblem_img = src_img.resize((EMBLEM_W, EMBLEM_H), Image.LANCZOS)

    # The runtime packs this emblem to IA4 using the alpha channel as a
    # silhouette mask, matching the ROM FT emblem format exactly. SObj-side
    # colour modulation (0x5C/0x22/0x00 franchise brown) does the tinting at
    # draw time, just like the 9 ROM stage emblems — so no per-channel bake
    # here.

    emblem_path = output_dir / f"{name}_emblem.png"
    output_dir.mkdir(parents=True, exist_ok=True)
    emblem_img.save(emblem_path)

    filesize = emblem_path.stat().st_size
    print(
        f"[{name}] Saved emblem:    {emblem_path} "
        f"({src_w}x{src_h} -> {EMBLEM_W}x{EMBLEM_H}, {filesize} bytes)",
        file=sys.stderr,
    )


# ---------------------------------------------------------------------------
# Main extraction logic for a single stage.
# ---------------------------------------------------------------------------


def extract_stage(stage: dict, rom: bytes, output_dir: Path) -> None:
    name       = stage["name"]
    reloc_file = stage["reloc_file"]
    sprite_off = stage["sprite_off"]

    print(f"[{name}] Extracting reloc file {reloc_file}...", file=sys.stderr)
    file_data = extract_file(rom, reloc_file)
    print(f"[{name}] Decompressed to {len(file_data)} bytes.", file=sys.stderr)

    # Parse the Sprite struct.
    sp = parse_sprite(file_data, sprite_off)
    w         = sp["width"]
    h         = sp["height"]
    nbitmaps  = sp["nbitmaps"]
    bmheight  = sp["bmheight"]
    bmsiz     = sp["bmsiz"]   # 2 -> G_IM_SIZ_16b (RGBA16)

    print(
        f"[{name}] Sprite: {w}x{h}, {nbitmaps} bitmaps, bmheight={bmheight}, "
        f"bmHreal={sp['bmHreal']}, bmfmt={sp['bmfmt']}, bmsiz={bmsiz}",
        file=sys.stderr,
    )

    if bmsiz != 2:
        print(
            f"[{name}] WARNING: bmsiz={bmsiz} (expected 2 for RGBA16). "
            f"Only RGBA16 is supported; output may be wrong.",
            file=sys.stderr,
        )

    # Resolve bitmap buffer offsets.
    bitmaps = resolve_bitmap_offsets(file_data, sp)

    # Decode each bitmap strip into RGBA rows.
    rows_rgba = []
    for i, bm in enumerate(bitmaps):
        strip_w     = bm["width"]
        strip_h     = bm["actualHeight"]
        buf_off     = bm["buf_off"]
        strip_bytes = strip_w * strip_h * 2  # 2 bytes per RGBA16 pixel

        if buf_off < 0 or buf_off + strip_bytes > len(file_data):
            raise ValueError(
                f"[{name}] bitmap[{i}].buf_off=0x{buf_off:X} out of range "
                f"(file_data length=0x{len(file_data):X}, strip_bytes={strip_bytes})"
            )

        raw_strip = file_data[buf_off:buf_off + strip_bytes]

        # Un-swizzle TMEM odd-row XOR4 pattern (16bpp, strip-local indexing).
        unswizzled = unswizzle_rgba16_strip(raw_strip, strip_w, strip_h)

        # Decode RGBA16 BE -> RGBA8888.
        rgba = rgba16_to_rgba8888(unswizzled)

        # Split into individual rows and accumulate (only bmheight rendered rows).
        row_bytes = strip_w * 4  # 4 bytes per pixel in RGBA8888
        rendered_rows = min(bmheight, strip_h)
        for row in range(rendered_rows):
            rows_rgba.append(rgba[row * row_bytes:(row + 1) * row_bytes])

    # Sanity check.
    if len(rows_rgba) != h:
        print(
            f"[{name}] WARNING: assembled {len(rows_rgba)} rows, expected {h}. "
            f"Output may be cropped or padded.",
            file=sys.stderr,
        )

    # Build a PIL Image from the decoded rows.
    img_data = b"".join(rows_rgba[:h])
    img = Image.frombytes("RGBA", (w, h), img_data)

    # Save full-resolution background PNG.
    bg_path = output_dir / f"{name}_background.png"
    output_dir.mkdir(parents=True, exist_ok=True)
    img.save(bg_path)
    bg_size = bg_path.stat().st_size
    print(
        f"[{name}] Saved background: {bg_path} ({w}x{h}, {bg_size} bytes)",
        file=sys.stderr,
    )

    # Downscale to 48x36 for the CSS icon thumbnail.
    # Use a center-crop if the aspect ratio doesn't match 4:3, then LANCZOS resize.
    target_w, target_h = ICON_W, ICON_H
    src_aspect = w / h
    tgt_aspect = target_w / target_h

    if abs(src_aspect - tgt_aspect) > 0.01:
        # Center-crop to match target aspect ratio.
        if src_aspect > tgt_aspect:
            # Image is wider -> crop width.
            new_w = int(h * tgt_aspect)
            left  = (w - new_w) // 2
            img_crop = img.crop((left, 0, left + new_w, h))
        else:
            # Image is taller -> crop height.
            new_h = int(w / tgt_aspect)
            top   = (h - new_h) // 2
            img_crop = img.crop((0, top, w, top + new_h))
    else:
        img_crop = img

    img_small = img_crop.resize((target_w, target_h), Image.LANCZOS)
    small_path = output_dir / f"{name}_small.png"
    img_small.save(small_path)
    small_size = small_path.stat().st_size
    print(
        f"[{name}] Saved icon:       {small_path} ({target_w}x{target_h}, {small_size} bytes)",
        file=sys.stderr,
    )

    # Render synthetic nameplate PNG if the stage has a name_text key.
    name_text = stage.get("name_text")
    if name_text:
        name_path = output_dir / f"{name}_name.png"
        render_name_png(name_text, name_path)

    # Extract and upscale emblem PNG if the stage has an emblem_src key.
    emblem_src = stage.get("emblem_src")
    if emblem_src:
        extract_emblem(stage, rom, output_dir)


# ---------------------------------------------------------------------------
# CLI entry point.
# ---------------------------------------------------------------------------


def main(argv: list) -> int:
    if len(argv) != 2:
        print(
            f"Usage: {Path(__file__).name} <baserom.z64> <output_dir>",
            file=sys.stderr,
        )
        return 2

    rom_path = Path(argv[0])
    out_dir  = Path(argv[1])

    if not rom_path.exists():
        print(f"ERROR: ROM not found: {rom_path}", file=sys.stderr)
        return 1

    rom = rom_path.read_bytes()
    if rom[:4] != bytes([0x80, 0x37, 0x12, 0x40]):
        print(
            f"WARNING: ROM header is not .z64 ({rom[:4].hex()}). "
            "Continuing anyway.",
            file=sys.stderr,
        )

    errors = 0
    for stage in STAGES:
        try:
            extract_stage(stage, rom, out_dir)
        except Exception as exc:
            print(f"ERROR [{stage['name']}]: {exc}", file=sys.stderr)
            errors += 1

    if errors:
        print(f"\n{errors} stage(s) failed.", file=sys.stderr)
        return 1

    print(f"\nAll {len(STAGES)} stage(s) extracted successfully.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

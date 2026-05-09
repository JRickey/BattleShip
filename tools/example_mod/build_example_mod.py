#!/usr/bin/env python3
"""build_example_mod.py — Stage 10 Phase C canned override demo.

Reads BattleShip.o2r, picks a target sub-resource (default: an FTAttributes
struct so the override is verifiable via in-game behavior), zeroes a
reasonable byte to demonstrate the override pathway, and emits a
self-contained mod .o2r at build/mods/example_mod.o2r.

The mod ships a single Blob resource at the SAME OTR path as the base
archive's sub-resource. LUS's last-archive-wins resolution claims the path
when the mod archive loads after BattleShip.o2r. The reloc-bridge overlay
step (port/bridge/lbreloc_bridge.cpp::portRelocBuildOverlay) then splices
the mod's bytes into the container at the recorded byte_offset before
memcpy.

Usage:
  ./build_example_mod.py                    # default target, writes to build/mods/
  ./build_example_mod.py --list             # list candidate sub-resources
  ./build_example_mod.py --target <path>    # custom OTR path
  ./build_example_mod.py --out <dir>        # custom output dir

Verify the mod loaded:
  Boot ssb64; tail build/logs/BattleShip.log for the line:
    "SSB64: N mod archive(s) loaded — sub-resource overlay active"
  ... and per-archive registration lines.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys
import zipfile
from pathlib import Path
from typing import Iterable


# libultraship's StrHash64 — matches CRC64(const char*) with NO final XOR
# (which IS what ArchiveManager hashes at runtime). The same polynomial as
# inspect_fixtures.py's crc64() but without the trailing inversion.
_CRC64_POLY = 0x42F0E1EBA9EA3693


def _build_crc64_table() -> list[int]:
    table: list[int] = []
    for n in range(256):
        c = n << 56
        for _ in range(8):
            c = ((c << 1) ^ _CRC64_POLY) if (c & 0x8000000000000000) else (c << 1)
            c &= 0xFFFFFFFFFFFFFFFF
        table.append(c)
    return table


_CRC64_TABLE = _build_crc64_table()


def crc64_str(s: str) -> int:
    """Matches libultraship/src/ship/utils/StrHash64.cpp::CRC64(const char*)."""
    crc = 0xFFFFFFFFFFFFFFFF
    for b in s.encode("utf-8"):
        crc = ((crc << 8) ^ _CRC64_TABLE[((crc >> 56) ^ b) & 0xFF]) & 0xFFFFFFFFFFFFFFFF
    return crc


# LUS resource header layout — mirrors torch/src/factories/BaseFactory.cpp.
# Total 0x40 bytes, padded with zeros after the populated fields.
_RES_TYPE_BLOB = 0x4F424C42  # "OBLB"


def write_lus_blob(data: bytes) -> bytes:
    """Serializes `data` as a LUS Blob binary resource:
    0x40-byte LUS header + u32 size + bytes.
    All multi-byte values little-endian (Native = Little on x86/ARM Apple)."""
    buf = bytearray(0x40)
    # byte 0: endianness — Little = 0
    buf[0] = 0
    # byte 1: is_asset_custom — 0
    # bytes 2..3: padding
    # bytes 4..7: resource type
    struct.pack_into("<I", buf, 4, _RES_TYPE_BLOB)
    # bytes 8..11: version — 0 for Blob
    struct.pack_into("<I", buf, 8, 0)
    # bytes 12..19: id sentinel
    struct.pack_into("<Q", buf, 12, 0xDEADBEEFDEADBEEF)
    # bytes 20..23: padding (already zero)
    # bytes 24..31: ROM CRC (zero)
    # bytes 32..35: ROM enum (zero)
    # bytes 36..63: pad with zero u32s (already zero)

    out = bytes(buf) + struct.pack("<I", len(data)) + data
    return out


def read_lus_blob(blob_bytes: bytes) -> bytes:
    """Inverse of write_lus_blob — strips LUS header, returns raw payload."""
    if len(blob_bytes) < 0x44:
        raise ValueError(f"blob too short ({len(blob_bytes)} < 0x44)")
    size = struct.unpack_from("<I", blob_bytes, 0x40)[0]
    if 0x44 + size > len(blob_bytes):
        raise ValueError(f"blob truncated: declared {size}, archive has {len(blob_bytes) - 0x44}")
    return blob_bytes[0x44 : 0x44 + size]


def list_candidate_subres(o2r_path: Path, kinds: Iterable[str] | None = None) -> list[str]:
    """Returns OTR paths of sub-resources in the archive matching `kinds`
    (e.g. ["sprite", "bitmap", "ftattributes"]). Returns all if kinds=None."""
    matches: list[str] = []
    with zipfile.ZipFile(o2r_path, "r") as z:
        for name in z.namelist():
            if "__sub/" not in name:
                continue
            kind = name.split("__sub/", 1)[1].split("/", 1)[0]
            if kinds is None or kind in kinds:
                matches.append(name)
    matches.sort()
    return matches


def build_mod(
    base_o2r: Path,
    target_path: str,
    out_path: Path,
    mutate_byte_idx: int = 0,
    mutate_byte_xor: int = 0xAA,
) -> tuple[int, int]:
    """Reads `target_path` from `base_o2r`, XORs one byte to produce a
    visible deviation, and writes a single-entry mod .o2r at `out_path`.

    Returns (target_hash, modified_size) for the caller to log.
    """
    with zipfile.ZipFile(base_o2r, "r") as z:
        if target_path not in z.namelist():
            raise SystemExit(
                f"target '{target_path}' not in {base_o2r}. "
                "Run with --list to inspect available sub-resources."
            )
        original_blob = z.read(target_path)

    payload = read_lus_blob(original_blob)
    if mutate_byte_idx >= len(payload):
        raise SystemExit(
            f"--mutate-byte-idx {mutate_byte_idx} out of range "
            f"(payload is {len(payload)} bytes)"
        )

    modified = bytearray(payload)
    modified[mutate_byte_idx] ^= mutate_byte_xor
    blob_bytes = write_lus_blob(bytes(modified))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as z:
        # OTR archive entry name == OTR resource path. ArchiveManager hashes
        # this name (CRC64 over the bytes of the string) and looks up
        # mFileToArchive[hash] when LoadResource is called with that hash.
        z.writestr(target_path, blob_bytes)

    return crc64_str(target_path), len(modified)


def main() -> int:
    here = Path(__file__).resolve().parent
    repo = here.parents[1]
    default_base = repo / "build" / "BattleShip.o2r"
    default_out = repo / "build" / "mods" / "example_mod.o2r"
    # FTAttributes is 0x348 bytes — large enough that an XOR-byte tweak
    # lands somewhere harmless (most fields are float/s32, so a single byte
    # flip in their LSB is a small numerical change). MarioMain at offset
    # 1064 is the first FTAttributes record, easy to spot in logs.
    default_target = "reloc_fighters_main/MarioMain__sub/ftattributes/1064"

    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--base-archive", type=Path, default=default_base,
                   help="Path to BattleShip.o2r (default: %(default)s)")
    p.add_argument("--target", type=str, default=default_target,
                   help="OTR sub-resource path to override (default: %(default)s)")
    p.add_argument("--out", type=Path, default=default_out,
                   help="Output mod .o2r path (default: %(default)s)")
    p.add_argument("--mutate-byte-idx", type=int, default=0,
                   help="Byte index in the payload to XOR (default: 0)")
    p.add_argument("--mutate-byte-xor", type=lambda s: int(s, 0), default=0xAA,
                   help="XOR mask for the mutation (default: 0xAA)")
    p.add_argument("--list", action="store_true",
                   help="List candidate sub-resources and exit")
    p.add_argument("--list-kinds", type=str, default=None,
                   help="Comma-separated kinds to filter --list "
                        "(e.g. 'ftattributes,sprite,bitmap')")
    args = p.parse_args()

    if not args.base_archive.exists():
        print(f"error: base archive not found: {args.base_archive}", file=sys.stderr)
        return 2

    if args.list:
        kinds = args.list_kinds.split(",") if args.list_kinds else None
        items = list_candidate_subres(args.base_archive, kinds)
        for item in items[:200]:
            print(item)
        if len(items) > 200:
            print(f"... ({len(items)} total)", file=sys.stderr)
        return 0

    target_hash, payload_size = build_mod(
        args.base_archive, args.target, args.out,
        mutate_byte_idx=args.mutate_byte_idx,
        mutate_byte_xor=args.mutate_byte_xor,
    )

    print(f"wrote {args.out}")
    print(f"  target path : {args.target}")
    print(f"  target hash : 0x{target_hash:016x}")
    print(f"  payload     : {payload_size} bytes "
          f"(byte[{args.mutate_byte_idx}] ^= 0x{args.mutate_byte_xor:02X})")
    print()
    print("To activate: drop the file in <build>/mods/ next to BattleShip.o2r.")
    print("Boot ssb64; ssb64.log should show:")
    print("  SSB64: mod archive registered -> .../mods/example_mod.o2r")
    print("  SSB64: 1 mod archive(s) loaded — sub-resource overlay active")
    return 0


if __name__ == "__main__":
    sys.exit(main())

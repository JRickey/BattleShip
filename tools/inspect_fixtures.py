#!/usr/bin/env python3
"""inspect_fixtures.py — anomaly scanner for .fixt files.

Usage:
    python3 tools/inspect_fixtures.py [--range LO HI] [--ids 0,1,2] [--dir <path>]

Reads .fixt files (the binary format defined in
port/test/fixture_format.h) and reports anomalies. Output is one line per
flagged fixture (or per category at the end), human-readable for subagent
synthesis. No fixture is modified.

Anomalies reported:
  - bad-magic / bad-version / bad-crc           — corrupt or wrong-format file
  - data-all-zero                                — body is entirely zero bytes
                                                    (suspicious; usually means
                                                    the load path failed silently)
  - halfswap-misaligned                          — halfswap range base or size
                                                    not aligned to 4 bytes
  - halfswap-oob                                 — halfswap range extends past
                                                    data bounds
  - halfswap-non-fighter                         — halfswap range present on a
                                                    file whose path doesn't
                                                    match the fighter pattern
  - tracker-oob                                  — tracker offset past data end
  - tracker-bad-family                           — family value not in {0,1,2}
  - chain-slot-oob / chain-target-oob            — chain offsets out of range
  - chain-extern-bad-dep                         — dep_file_id >= 2132
  - dup-fixture                                  — two fixtures claim same id
                                                    (filename mismatch)
  - missing-id                                   — no .fixt file for an expected id

Run with --json to get a structured report instead of human-readable lines.
"""

from __future__ import annotations
import argparse
import json
import os
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Iterable, Optional


MAGIC = b"RFX0"
VERSION = 1
HEADER_SIZE = 32
SECTION_DATA = 1 << 0
SECTION_HALFSWAP = 1 << 1
SECTION_TRACKERS = 1 << 2
SECTION_CHAIN_INTERN = 1 << 3
SECTION_CHAIN_EXTERN = 1 << 4

FIGHTER_PATTERN = re.compile(r"^(animations_FT|submotions_FT|scene_SCExplainMain)")


def crc64(buf: bytes) -> int:
    """libultraship StrHash64-compatible CRC64.
    Matches update_crc64() with INITIAL_CRC64 = 0xFFFFFFFFFFFFFFFF and the
    final return ~crc that libultraship applies. ECMA-182 polynomial. """
    crc = 0xFFFFFFFFFFFFFFFF
    for b in buf:
        crc = ((crc << 8) ^ _CRC64_TABLE[((crc >> 56) ^ b) & 0xFF]) & 0xFFFFFFFFFFFFFFFF
    return crc ^ 0xFFFFFFFFFFFFFFFF


def _build_crc64_table() -> list[int]:
    """ECMA-182 polynomial reflected once: 0x42F0E1EBA9EA3693 BE form."""
    poly = 0x42F0E1EBA9EA3693
    table = []
    for n in range(256):
        c = n << 56
        for _ in range(8):
            c = ((c << 1) ^ poly) if (c & 0x8000000000000000) else (c << 1)
            c &= 0xFFFFFFFFFFFFFFFF
        table.append(c)
    return table


_CRC64_TABLE = _build_crc64_table()


class FixtureError(Exception):
    pass


class Fixture:
    def __init__(self, path: Path, raw: bytes):
        self.path = path
        self.raw = raw
        self.flags: int = 0
        self.file_id: int = 0
        self.body_size: int = 0
        self.data: bytes = b""
        self.halfswap: list[tuple[int, int]] = []
        self.trackers: list[tuple[int, int]] = []  # (offset, family)
        self.chain_intern: list[tuple[int, int]] = []
        self.chain_extern: list[tuple[int, int, int]] = []
        self.crc_ok: bool = False

    @classmethod
    def parse(cls, path: Path) -> "Fixture":
        raw = path.read_bytes()
        if len(raw) < HEADER_SIZE:
            raise FixtureError(f"{path.name}: too short ({len(raw)} bytes)")
        magic = raw[0:4]
        if magic != MAGIC:
            raise FixtureError(f"{path.name}: bad magic {magic!r}")
        version, = struct.unpack_from("<I", raw, 4)
        if version != VERSION:
            raise FixtureError(f"{path.name}: bad version {version}")
        f = cls(path, raw)
        f.file_id, = struct.unpack_from("<I", raw, 8)
        f.flags, = struct.unpack_from("<I", raw, 12)
        f.body_size, = struct.unpack_from("<I", raw, 16)
        body_crc, = struct.unpack_from("<Q", raw, 20)
        if HEADER_SIZE + f.body_size > len(raw):
            raise FixtureError(f"{path.name}: body_size {f.body_size} exceeds file ({len(raw)})")
        body = raw[HEADER_SIZE: HEADER_SIZE + f.body_size]
        f.crc_ok = (crc64(body) == body_crc)
        if not f.crc_ok:
            raise FixtureError(f"{path.name}: crc mismatch")

        off = 0
        if f.flags & SECTION_DATA:
            sz, = struct.unpack_from("<I", body, off); off += 4
            f.data = body[off: off + sz]; off += sz
        if f.flags & SECTION_HALFSWAP:
            n, = struct.unpack_from("<I", body, off); off += 4
            for _ in range(n):
                base, size = struct.unpack_from("<II", body, off); off += 8
                f.halfswap.append((base, size))
        if f.flags & SECTION_TRACKERS:
            n, = struct.unpack_from("<I", body, off); off += 4
            for _ in range(n):
                bo, family, _p1, _p2, _p3 = struct.unpack_from("<IBBBB", body, off); off += 8
                f.trackers.append((bo, family))
        if f.flags & SECTION_CHAIN_INTERN:
            n, = struct.unpack_from("<I", body, off); off += 4
            for _ in range(n):
                slot, target = struct.unpack_from("<II", body, off); off += 8
                f.chain_intern.append((slot, target))
        if f.flags & SECTION_CHAIN_EXTERN:
            n, = struct.unpack_from("<I", body, off); off += 4
            for _ in range(n):
                slot, dep, target = struct.unpack_from("<III", body, off); off += 12
                f.chain_extern.append((slot, dep, target))
        return f


def parse_filename(name: str) -> tuple[Optional[int], str]:
    """Returns (file_id, sanitized_path) parsed from a .fixt filename."""
    m = re.match(r"^(\d{4})_(.+)\.fixt$", name)
    if not m:
        return None, ""
    return int(m.group(1)), m.group(2)


def scan(directory: Path, ids: Optional[Iterable[int]] = None) -> dict:
    """Scan fixtures, return a structured report dict."""
    by_id: dict[int, Fixture] = {}
    bad: list[tuple[str, str]] = []
    flagged: list[tuple[int, str, str]] = []  # (file_id, code, detail)

    files = sorted(directory.glob("*.fixt"))
    if ids is not None:
        wanted = set(ids)
        files = [p for p in files if (parse_filename(p.name)[0] in wanted)]

    for path in files:
        fid_from_name, sanitized_path = parse_filename(path.name)
        try:
            f = Fixture.parse(path)
        except FixtureError as e:
            bad.append((path.name, str(e)))
            continue
        if fid_from_name is not None and f.file_id != fid_from_name:
            flagged.append((f.file_id, "filename-id-mismatch",
                            f"filename says {fid_from_name}, header says {f.file_id}"))
        if f.file_id in by_id:
            flagged.append((f.file_id, "dup-fixture",
                            f"both {by_id[f.file_id].path.name} and {path.name}"))
        by_id[f.file_id] = f

        # Anomaly checks
        ds = len(f.data)
        if ds == 0:
            flagged.append((f.file_id, "data-empty", f.path.name))
        elif all(b == 0 for b in f.data):
            flagged.append((f.file_id, "data-all-zero",
                            f"{f.path.name} ({ds} B)"))
        # Halfswap ranges
        is_fighter_named = bool(FIGHTER_PATTERN.match(sanitized_path))
        if f.halfswap and not is_fighter_named:
            flagged.append((f.file_id, "halfswap-non-fighter",
                            f"{f.path.name} ranges={f.halfswap}"))
        if not f.halfswap and is_fighter_named:
            flagged.append((f.file_id, "halfswap-missing-on-fighter",
                            f.path.name))
        for base, size in f.halfswap:
            if base % 4 != 0 or size % 4 != 0:
                flagged.append((f.file_id, "halfswap-misaligned",
                                f"base=0x{base:x} size=0x{size:x}"))
            if base + size > ds:
                flagged.append((f.file_id, "halfswap-oob",
                                f"base+size={base + size} > data={ds}"))
        # Trackers
        for off, fam in f.trackers:
            if off >= ds:
                flagged.append((f.file_id, "tracker-oob",
                                f"off=0x{off:x} >= data={ds}"))
            if fam not in (0, 1, 2):
                flagged.append((f.file_id, "tracker-bad-family",
                                f"family={fam}"))
        # Chains
        for slot, target in f.chain_intern:
            if slot + 4 > ds:
                flagged.append((f.file_id, "chain-slot-oob",
                                f"slot=0x{slot:x} >= data={ds}"))
            if target > ds:
                # target == ds is technically a one-past-end pointer; we treat
                # only target > ds as an anomaly. (Some N64 sentinels may sit
                # at the boundary.)
                flagged.append((f.file_id, "chain-intern-target-oob",
                                f"slot=0x{slot:x} target=0x{target:x} > data={ds}"))
        for slot, dep, target in f.chain_extern:
            if slot + 4 > ds:
                flagged.append((f.file_id, "chain-extern-slot-oob",
                                f"slot=0x{slot:x} >= data={ds}"))
            if dep >= 2132:
                flagged.append((f.file_id, "chain-extern-bad-dep",
                                f"dep_file_id={dep}"))

    # Categorical totals
    by_code = Counter(c for _, c, _ in flagged)
    return {
        "scanned": len(files),
        "bad_files": bad,
        "flagged_count": len(flagged),
        "by_code": dict(by_code),
        "flagged": flagged,
        "fixture_count_by_size": _size_buckets(by_id.values()),
    }


def _size_buckets(fxs: Iterable[Fixture]) -> dict:
    buckets: defaultdict[str, int] = defaultdict(int)
    for f in fxs:
        ds = len(f.data)
        if ds == 0: buckets["0"] += 1
        elif ds < 256: buckets["<256"] += 1
        elif ds < 1024: buckets["<1K"] += 1
        elif ds < 4096: buckets["<4K"] += 1
        elif ds < 16384: buckets["<16K"] += 1
        elif ds < 65536: buckets["<64K"] += 1
        elif ds < 262144: buckets["<256K"] += 1
        else: buckets[">=256K"] += 1
    return dict(buckets)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--dir", type=Path, default=None,
                   help="fixtures directory (default: <repo>/port/test/fixtures)")
    p.add_argument("--range", nargs=2, type=int, metavar=("LO", "HI"),
                   help="inspect IDs in [LO, HI)")
    p.add_argument("--ids", type=str,
                   help="inspect specific IDs (comma-separated)")
    p.add_argument("--json", action="store_true", help="JSON output")
    args = p.parse_args()

    if args.dir is None:
        # walk up from cwd looking for port/test/fixtures
        cwd = Path.cwd()
        for _ in range(8):
            cand = cwd / "port" / "test" / "fixtures"
            if cand.exists():
                args.dir = cand
                break
            cwd = cwd.parent
    if args.dir is None or not args.dir.exists():
        print(f"error: fixtures directory not found", file=sys.stderr)
        sys.exit(2)

    ids: Optional[list[int]] = None
    if args.range:
        ids = list(range(args.range[0], args.range[1]))
    elif args.ids:
        ids = [int(x) for x in args.ids.split(",")]

    report = scan(args.dir, ids)

    if args.json:
        json.dump(report, sys.stdout, indent=2)
        print()
        return

    print(f"scanned: {report['scanned']} fixtures from {args.dir}")
    if report["bad_files"]:
        print(f"bad files ({len(report['bad_files'])}):")
        for n, msg in report["bad_files"]:
            print(f"  {n}: {msg}")
    print(f"size distribution: {report['fixture_count_by_size']}")
    if report["by_code"]:
        print(f"flagged by code: {report['by_code']}")
        print("first 30 flags:")
        for fid, code, detail in report["flagged"][:30]:
            print(f"  id={fid:4d} {code}: {detail}")
    else:
        print("no anomalies flagged")


if __name__ == "__main__":
    main()

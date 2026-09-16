#!/usr/bin/env python3
"""
reloc_extract — Standalone SSB64:RELOC file extractor and verifier.

Reads a US or JP baserom directly using the same logic as Torch's RelocFactory,
decompresses VPK0 if needed, and writes the raw decompressed bytes.

Use cases:
  - Extract a single file by file_id
  - Compare against the bytes loaded by the new port (after pass1 BSWAP32)
    to verify Torch's extraction is lossless

Usage:
  reloc_extract.py extract <baserom.z64> <file_id> <output.bin>
  reloc_extract.py diff <baserom.z64> <file_id> <port_dump.bin>
"""

import struct
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# ROM layout constants — must match torch/src/factories/ssb64/RelocFactory.h
# ---------------------------------------------------------------------------
RELOC_TABLE_ENTRY_SIZE  = 12
ROM_COUNTRY_CODE_OFF    = 0x3E
RELOC_LAYOUTS = {
    ord("E"): (0x001AC870, 2132),  # NALE, US v1.0
    ord("J"): (0x001ACAF0, 2107),  # NALJ, Japan
}


def get_reloc_layout(rom: bytes):
    """Return (table address, file count, data start) for this ROM."""
    if len(rom) <= ROM_COUNTRY_CODE_OFF:
        raise ValueError("ROM is too small to contain an N64 country code")
    country = rom[ROM_COUNTRY_CODE_OFF]
    if country not in RELOC_LAYOUTS:
        raise ValueError(f"unsupported ROM country code 0x{country:02X}")
    table_addr, file_count = RELOC_LAYOUTS[country]
    table_size = (file_count + 1) * RELOC_TABLE_ENTRY_SIZE
    return table_addr, file_count, table_addr + table_size


# ---------------------------------------------------------------------------
# VPK0 decoder — ported from torch/lib/libvpk0/vpk0.c
# ---------------------------------------------------------------------------
class BitStream:
    __slots__ = ("data", "size", "pos", "bits", "avail")

    def __init__(self, data: bytes):
        self.data = data
        self.size = len(data)
        self.pos = 0
        self.bits = 0
        self.avail = 0

    def refill(self, need: int) -> None:
        while self.avail < need:
            if self.pos >= self.size:
                raise ValueError("VPK0: bitstream underflow")
            self.bits = (self.bits << 8) | self.data[self.pos]
            self.pos += 1
            self.avail += 8

    def read(self, n: int) -> int:
        self.refill(n)
        self.avail -= n
        return (self.bits >> self.avail) & ((1 << n) - 1)


class HuffNode:
    __slots__ = ("left", "right", "value")

    def __init__(self):
        self.left = None
        self.right = None
        self.value = 0


def build_tree(bs: BitStream) -> HuffNode:
    """Read a Huffman tree from the bitstream in prefix notation."""
    stack = []
    while True:
        bit = bs.read(1)
        if bit != 0 and len(stack) < 2:
            break
        node = HuffNode()
        if bit != 0:
            node.left = stack[-2]
            node.right = stack[-1]
            stack[-2:] = [node]
        else:
            node.value = bs.read(8)
            stack.append(node)
    return stack[0]


def tree_decode(bs: BitStream, root: HuffNode) -> int:
    n = root
    while n.left is not None:
        n = n.right if bs.read(1) else n.left
    return n.value


def vpk0_decode(src: bytes) -> bytes:
    """Decompress a VPK0 buffer. Returns decompressed bytes."""
    if len(src) < 9 or src[:4] != b"vpk0":
        raise ValueError("VPK0: bad magic")

    # Decompressed size (BE u32 at offset 4)
    dec_size = struct.unpack(">I", src[4:8])[0]

    # The bitstream starts at byte 4 (consumes the 4-byte size + 1-byte sample method)
    bs = BitStream(src[4:])
    bs.read(16)  # high half of size
    bs.read(16)  # low half of size
    sample_method = bs.read(8)

    offsets_tree = build_tree(bs)
    lengths_tree = build_tree(bs)

    # Use bytearray for the output buffer so we can index into it as the LZ77
    # back-reference source.
    out = bytearray()
    while len(out) < dec_size:
        flag = bs.read(1)
        if flag == 0:
            out.append(bs.read(8))
        else:
            if sample_method != 0:
                # Two-sample offset mode
                sub_offset = 0
                extra_bits = tree_decode(bs, offsets_tree)
                value = bs.read(extra_bits) if extra_bits else 0
                if value <= 2:
                    sub_offset = value + 1
                    extra_bits2 = tree_decode(bs, offsets_tree)
                    value = bs.read(extra_bits2) if extra_bits2 else 0
                # copy_src = out - value * 4 - sub_offset + 8
                copy_src_idx = len(out) - value * 4 - sub_offset + 8
            else:
                extra_bits = tree_decode(bs, offsets_tree)
                value = bs.read(extra_bits) if extra_bits else 0
                copy_src_idx = len(out) - value

            len_bits = tree_decode(bs, lengths_tree)
            length = bs.read(len_bits) if len_bits else 0

            for _ in range(length):
                out.append(out[copy_src_idx])
                copy_src_idx += 1

    return bytes(out[:dec_size])


# ---------------------------------------------------------------------------
# RELOC extractor
# ---------------------------------------------------------------------------
def read_table_entry(rom: bytes, file_id: int):
    table_addr, file_count, data_start = get_reloc_layout(rom)
    if file_id < 0 or file_id >= file_count:
        raise ValueError(f"file_id {file_id} out of range (max {file_count - 1})")

    table_off = table_addr + file_id * RELOC_TABLE_ENTRY_SIZE
    if table_off + RELOC_TABLE_ENTRY_SIZE * 2 > len(rom):
        raise ValueError("ROM too small for table entry")

    te = rom[table_off:table_off + RELOC_TABLE_ENTRY_SIZE * 2]
    first_word         = struct.unpack(">I", te[0:4])[0]
    is_compressed      = (first_word >> 31) != 0
    data_offset        = first_word & 0x7FFFFFFF
    reloc_intern       = struct.unpack(">H", te[4:6])[0]
    compressed_words   = struct.unpack(">H", te[6:8])[0]
    reloc_extern       = struct.unpack(">H", te[8:10])[0]
    decompressed_words = struct.unpack(">H", te[10:12])[0]

    next_first_word    = struct.unpack(">I", te[12:16])[0]
    next_data_offset   = next_first_word & 0x7FFFFFFF

    return {
        "file_id": file_id,
        "is_compressed": is_compressed,
        "data_offset": data_offset,
        "reloc_intern": reloc_intern,
        "reloc_extern": reloc_extern,
        "compressed_bytes": compressed_words * 4,
        "decompressed_bytes": decompressed_words * 4,
        "next_data_offset": next_data_offset,
        "data_start": data_start,
    }


def extract_file(rom: bytes, file_id: int) -> bytes:
    info = read_table_entry(rom, file_id)
    print(
        f"file_id={info['file_id']}  compressed={info['is_compressed']}  "
        f"data_offset=0x{info['data_offset']:X}  "
        f"reloc_intern=0x{info['reloc_intern']:04X}  "
        f"reloc_extern=0x{info['reloc_extern']:04X}  "
        f"comp_bytes={info['compressed_bytes']}  "
        f"decomp_bytes={info['decompressed_bytes']}",
        file=sys.stderr,
    )

    data_rom_addr = info["data_start"] + info["data_offset"]
    print(
        f"data_rom_addr=0x{data_rom_addr:X} "
        f"(RELOC_DATA_START=0x{info['data_start']:X} + offset=0x{info['data_offset']:X})",
        file=sys.stderr,
    )

    if data_rom_addr + info["compressed_bytes"] > len(rom):
        raise ValueError("ROM too small for file data")

    file_data = rom[data_rom_addr:data_rom_addr + info["compressed_bytes"]]

    if info["is_compressed"]:
        decompressed = vpk0_decode(file_data)
        if len(decompressed) != info["decompressed_bytes"]:
            print(
                f"WARNING: vpk0 produced {len(decompressed)} bytes, "
                f"table says {info['decompressed_bytes']}",
                file=sys.stderr,
            )
    else:
        decompressed = file_data[:info["decompressed_bytes"]]

    return decompressed


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def cmd_extract(argv):
    if len(argv) != 3:
        print("usage: reloc_extract.py extract <baserom.z64> <file_id> <output.bin>",
              file=sys.stderr)
        return 2

    rom_path = Path(argv[0])
    file_id  = int(argv[1], 0)
    out_path = Path(argv[2])

    rom = rom_path.read_bytes()
    if rom[:4] != bytes([0x80, 0x37, 0x12, 0x40]):
        print(f"warning: ROM header is not z64 ({rom[:4].hex()})", file=sys.stderr)

    data = extract_file(rom, file_id)
    out_path.write_bytes(data)
    print(f"wrote {len(data)} bytes to {out_path}", file=sys.stderr)
    return 0


def cmd_diff(argv):
    if len(argv) != 3:
        print("usage: reloc_extract.py diff <baserom.z64> <file_id> <port_dump.bin>",
              file=sys.stderr)
        return 2

    rom_path  = Path(argv[0])
    file_id   = int(argv[1], 0)
    port_path = Path(argv[2])

    rom = rom_path.read_bytes()
    rom_data = extract_file(rom, file_id)
    port_data = port_path.read_bytes()

    print()
    print(f"ROM-extracted (truth):  {len(rom_data)} bytes")
    print(f"port dump:              {len(port_data)} bytes")

    if len(rom_data) != len(port_data):
        print(f"SIZE MISMATCH: rom={len(rom_data)} port={len(port_data)}")

    cmp_len = min(len(rom_data), len(port_data))
    diffs = []
    for i in range(cmp_len):
        if rom_data[i] != port_data[i]:
            diffs.append(i)

    if not diffs and len(rom_data) == len(port_data):
        print("\nIDENTICAL — no differences found")
        return 0

    print(f"\n{len(diffs)} byte differences in first {cmp_len} bytes")

    # Print first 30 diffs with context
    print("\nFirst 30 differing bytes (offset: rom -> port):")
    for off in diffs[:30]:
        print(f"  0x{off:06X}: 0x{rom_data[off]:02X} -> 0x{port_data[off]:02X}")

    # Find runs (regions of contiguous diffs) for higher-level analysis
    runs = []
    if diffs:
        run_start = diffs[0]
        run_end = diffs[0]
        for d in diffs[1:]:
            if d == run_end + 1:
                run_end = d
            else:
                runs.append((run_start, run_end))
                run_start = d
                run_end = d
        runs.append((run_start, run_end))

    print(f"\n{len(runs)} contiguous diff runs:")
    for start, end in runs[:20]:
        size = end - start + 1
        print(f"  0x{start:06X} .. 0x{end:06X}  ({size} bytes)")
        # Hex dump of the run, both sides
        hex_rom  = " ".join(f"{b:02X}" for b in rom_data[start:end + 1])
        hex_port = " ".join(f"{b:02X}" for b in port_data[start:end + 1])
        print(f"    rom : {hex_rom}")
        print(f"    port: {hex_port}")

    return 1


def main(argv):
    if len(argv) < 1:
        print(__doc__)
        return 2
    cmd = argv[0]
    if cmd == "extract":
        return cmd_extract(argv[1:])
    elif cmd == "diff":
        return cmd_diff(argv[1:])
    else:
        print(f"unknown command: {cmd}", file=sys.stderr)
        print(__doc__)
        return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

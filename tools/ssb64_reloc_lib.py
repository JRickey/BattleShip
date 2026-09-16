"""ssb64_reloc_lib — shared SSB64 reloc-file primitives for tools/.

One home for the logic every reloc tool needs, pinned to the same semantics
as torch's SSB64:RELOC factory (torch/src/factories/ssb64/RelocFactory.cpp)
and the port's chain walker (port/bridge/lbreloc_bridge.cpp:667-840):

  - per-version reloc table constants (VERSIONS)
  - reloc table reader
  - VPK0 decompressor (ported from debug_tools/reloc_extract/reloc_extract.py)
  - extern file-id extraction (the u16 region between a file's compressed
    data end and the next file's data start)
  - intern/extern relocation chain walkers

Chain encoding (big-endian u32 at word offset `cur`, starting from the table
entry's reloc_intern_offset / reloc_extern_offset, 0xFFFF terminates):
    bits [31:16] = next slot word offset
    bits [15:0]  = target word offset (intern: within this file;
                   extern: within the Nth extern dependency, N = entry order)

The "relocated view" used by torch's ssb64_reloc_parent slicing rewrites each
INTERN slot to `target_word * 4` (a plain byte offset); extern slots keep
their descriptor form. See docs/deblob.md (abstraction tower, L1/L2).
"""

import hashlib
import struct
from pathlib import Path

RELOC_TABLE_ENTRY_SIZE = 12  # bytes

# Per-ROM-version reloc table location + file count. Addresses from the decomp
# splat configs; counts from tools/relocFileDescriptions.<v>.txt. Must match
# tools/generate_yamls.py VERSIONS and torch's GetRelocLayout().
VERSIONS = {
    "us": {"reloc_table_rom_addr": 0x001AC870, "reloc_file_count": 2132},
    "jp": {"reloc_table_rom_addr": 0x001ACAF0, "reloc_file_count": 2107},
}


class RelocLayout:
    """Resolved table geometry for one ROM version."""

    def __init__(self, version):
        cfg = VERSIONS[version]
        self.version = version
        self.table_rom_addr = cfg["reloc_table_rom_addr"]
        self.file_count = cfg["reloc_file_count"]
        self.table_size = (self.file_count + 1) * RELOC_TABLE_ENTRY_SIZE
        self.data_start = self.table_rom_addr + self.table_size


def rom_sha1(rom_bytes):
    return hashlib.sha1(rom_bytes).hexdigest()


def load_rom(path):
    rom = Path(path).read_bytes()
    if rom[:4] != bytes([0x80, 0x37, 0x12, 0x40]):
        raise ValueError(f"{path}: not a big-endian z64 ROM (header {rom[:4].hex()})")
    return rom


# ---------------------------------------------------------------------------
# Reloc table
# ---------------------------------------------------------------------------

def read_table_entry(rom, layout, file_id):
    """Read one table entry (+ the next entry's data offset, needed for the
    ROM footprint / extern-id region). Same field decode as RelocFactory."""
    if file_id >= layout.file_count:
        raise ValueError(f"file_id {file_id} out of range (max {layout.file_count - 1})")

    off = layout.table_rom_addr + file_id * RELOC_TABLE_ENTRY_SIZE
    te = rom[off:off + RELOC_TABLE_ENTRY_SIZE * 2]
    if len(te) < RELOC_TABLE_ENTRY_SIZE * 2:
        raise ValueError("ROM too small for table entry")

    first_word, reloc_intern, compressed_words, reloc_extern, decompressed_words = \
        struct.unpack(">IHHHH", te[:12])
    next_first_word = struct.unpack(">I", te[12:16])[0]

    return {
        "file_id": file_id,
        "is_compressed": bool(first_word & 0x80000000),
        "data_offset": first_word & 0x7FFFFFFF,
        "reloc_intern_offset": reloc_intern,
        "reloc_extern_offset": reloc_extern,
        "compressed_bytes": compressed_words * 4,
        "decompressed_bytes": decompressed_words * 4,
        "next_data_offset": next_first_word & 0x7FFFFFFF,
    }


# ---------------------------------------------------------------------------
# VPK0 decoder — ported from torch/lib/libvpk0/vpk0.c via
# debug_tools/reloc_extract/reloc_extract.py (kept byte-for-byte compatible).
# ---------------------------------------------------------------------------

class _BitStream:
    __slots__ = ("data", "size", "pos", "bits", "avail")

    def __init__(self, data):
        self.data = data
        self.size = len(data)
        self.pos = 0
        self.bits = 0
        self.avail = 0

    def read(self, n):
        while self.avail < n:
            if self.pos >= self.size:
                raise ValueError("VPK0: bitstream underflow")
            self.bits = (self.bits << 8) | self.data[self.pos]
            self.pos += 1
            self.avail += 8
        self.avail -= n
        return (self.bits >> self.avail) & ((1 << n) - 1)


class _HuffNode:
    __slots__ = ("left", "right", "value")

    def __init__(self):
        self.left = None
        self.right = None
        self.value = 0


def _build_tree(bs):
    stack = []
    while True:
        bit = bs.read(1)
        if bit != 0 and len(stack) < 2:
            break
        node = _HuffNode()
        if bit != 0:
            node.left = stack[-2]
            node.right = stack[-1]
            stack[-2:] = [node]
        else:
            node.value = bs.read(8)
            stack.append(node)
    return stack[0]


def _tree_decode(bs, root):
    n = root
    while n.left is not None:
        n = n.right if bs.read(1) else n.left
    return n.value


def vpk0_decode(src):
    if len(src) < 9 or src[:4] != b"vpk0":
        raise ValueError("VPK0: bad magic")

    dec_size = struct.unpack(">I", src[4:8])[0]
    bs = _BitStream(src[4:])
    bs.read(16)
    bs.read(16)
    sample_method = bs.read(8)

    offsets_tree = _build_tree(bs)
    lengths_tree = _build_tree(bs)

    out = bytearray()
    while len(out) < dec_size:
        if bs.read(1) == 0:
            out.append(bs.read(8))
        else:
            if sample_method != 0:
                sub_offset = 0
                extra_bits = _tree_decode(bs, offsets_tree)
                value = bs.read(extra_bits) if extra_bits else 0
                if value <= 2:
                    sub_offset = value + 1
                    extra_bits2 = _tree_decode(bs, offsets_tree)
                    value = bs.read(extra_bits2) if extra_bits2 else 0
                copy_src_idx = len(out) - value * 4 - sub_offset + 8
            else:
                extra_bits = _tree_decode(bs, offsets_tree)
                value = bs.read(extra_bits) if extra_bits else 0
                copy_src_idx = len(out) - value

            len_bits = _tree_decode(bs, lengths_tree)
            length = bs.read(len_bits) if len_bits else 0
            for _ in range(length):
                out.append(out[copy_src_idx])
                copy_src_idx += 1

    return bytes(out[:dec_size])


# ---------------------------------------------------------------------------
# File extraction + extern ids
# ---------------------------------------------------------------------------

def extract_file(rom, layout, file_id):
    """Return (info, decompressed_bytes) for one reloc file."""
    info = read_table_entry(rom, layout, file_id)
    data_addr = layout.data_start + info["data_offset"]
    raw = rom[data_addr:data_addr + info["compressed_bytes"]]
    if len(raw) < info["compressed_bytes"]:
        raise ValueError(f"ROM too small for file {file_id} data")

    if info["is_compressed"]:
        data = vpk0_decode(raw)
        if len(data) != info["decompressed_bytes"]:
            raise ValueError(
                f"file {file_id}: vpk0 produced {len(data)} bytes,"
                f" table says {info['decompressed_bytes']}")
    else:
        data = raw[:info["decompressed_bytes"]]
    return info, data


def read_extern_ids(rom, layout, info):
    """Big-endian u16 extern file ids stored between this file's compressed
    data end and the next file's data start (RelocFactory.cpp:184-206)."""
    start = layout.data_start + info["data_offset"] + info["compressed_bytes"]
    end = layout.data_start + info["next_data_offset"]
    ids = []
    for addr in range(start, end - 1, 2):
        ids.append(struct.unpack(">H", rom[addr:addr + 2])[0])
    return ids


# ---------------------------------------------------------------------------
# Chain walkers
# ---------------------------------------------------------------------------

def walk_chain(data, start_word_offset):
    """Walk one relocation chain over big-endian file bytes.

    Returns a list of {slot_off, target_words, next_words} in CHAIN ORDER
    (byte offsets for slot_off, word offsets for the rest). Raises on any
    slot outside the file or a cycle — a fighters_main bundle from ROM must
    always have a pristine chain (invariant I4 precondition).
    """
    entries = []
    seen = set()
    cur = start_word_offset
    num_words = len(data) // 4

    while cur != 0xFFFF:
        if cur >= num_words:
            raise ValueError(f"chain slot word offset 0x{cur:X} outside file")
        if cur in seen:
            raise ValueError(f"chain cycle at word offset 0x{cur:X}")
        seen.add(cur)

        word = struct.unpack(">I", data[cur * 4:cur * 4 + 4])[0]
        nxt = (word >> 16) & 0xFFFF
        target = word & 0xFFFF
        entries.append({
            "slot_off": cur * 4,
            "target_words": target,
            "next_words": nxt,
        })
        cur = nxt

    return entries


def make_relocated_view(data, intern_chain):
    """Torch's ssb64_reloc_parent transform: copy of the file with every
    INTERN chain slot rewritten to target_word*4 (a plain byte offset).
    Extern slots are untouched (Decompressor.cpp GetSSB64RelocatedChunk)."""
    view = bytearray(data)
    for e in intern_chain:
        struct.pack_into(">I", view, e["slot_off"], e["target_words"] * 4)
    return bytes(view)


def restore_chain_form(view, intern_chain):
    """Inverse of make_relocated_view — used by the I4 identity check."""
    data = bytearray(view)
    for e in intern_chain:
        struct.pack_into(">I", data, e["slot_off"],
                         ((e["next_words"] & 0xFFFF) << 16) | (e["target_words"] & 0xFFFF))
    return bytes(data)


# ---------------------------------------------------------------------------
# Parent yaml reading (reloc_<category>.yml — generated, stable format)
# ---------------------------------------------------------------------------

def read_parent_yaml_entries(yaml_path):
    """Parse a generated reloc_<category>.yml into [(name, file_id)] in file
    order. The format is emitted by tools/generate_yamls.py and is regular
    enough for line parsing (no PyYAML dependency in tools/)."""
    entries = []
    name = None
    for line in Path(yaml_path).read_text().splitlines():
        if line and not line.startswith((" ", "#", ":")) and line.rstrip().endswith(":"):
            name = line.rstrip()[:-1]
        elif name is not None and line.strip().startswith("file_id:"):
            entries.append((name, int(line.split(":", 1)[1].strip(), 0)))
            name = None
    return entries

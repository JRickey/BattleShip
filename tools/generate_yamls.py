#!/usr/bin/env python3
"""
generate_yamls.py — Reads the SSB64 ROM's LBReloc file table and generates
Torch YAML extraction configs for every relocatable file as BLOBs.

Usage:
    python tools/generate_yamls.py [--version {us,jp}] [path/to/baserom.z64]

With no arguments: version=us, ROM=baserom.us.z64, output=yamls/us
(the legacy contract — CMake invokes it argument-less, US output is
byte-for-byte unchanged from before versioning was added).

  --version us : reloc table @ 0x1AC870, 2132 files → yamls/us,
                 names parsed from include/reloc_data.us.h
  --version jp : reloc table @ 0x1ACAF0, 2107 files → yamls/jp,
                 names parsed from include/reloc_data.jp.h

The optional positional ROM path overrides the per-version default.
"""

import argparse
import os
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from deblob_categories import DEBLOBBED_CATEGORIES, EMIT_ARCHIVE_FALSE

# ============================================================================
#  Constants
# ============================================================================

RELOC_TABLE_ENTRY_SIZE = 12  # bytes

# Per-ROM-version reloc table location + file count. The reloc table ROM
# address comes from the upstream decomp splat configs
# (smashbrothers.<v>.yaml: `- [<addr>, bin, relocData]`); file counts come
# from tools/relocFileDescriptions.<v>.txt (FILE_COUNT header).
VERSIONS = {
    "us": {"reloc_table_rom_addr": 0x001AC870, "reloc_file_count": 2132},
    "jp": {"reloc_table_rom_addr": 0x001ACAF0, "reloc_file_count": 2107},
}

# Set by select_version() from the chosen --version; default to US so the
# module-level constants stay valid for any code importing this module.
RELOC_TABLE_ROM_ADDR = VERSIONS["us"]["reloc_table_rom_addr"]
RELOC_FILE_COUNT = VERSIONS["us"]["reloc_file_count"]
# Table has FILE_COUNT + 1 entries (sentinel at end)
RELOC_TABLE_SIZE = (RELOC_FILE_COUNT + 1) * RELOC_TABLE_ENTRY_SIZE
RELOC_DATA_START = RELOC_TABLE_ROM_ADDR + RELOC_TABLE_SIZE  # where file data begins


def select_version(version):
    """Point the module-level reloc-table globals at the chosen version.

    read_reloc_table() / compute_file_info() reference these as globals, so
    the US code path is byte-for-byte unchanged when version == 'us'.
    """
    global RELOC_TABLE_ROM_ADDR, RELOC_FILE_COUNT, RELOC_TABLE_SIZE, RELOC_DATA_START
    cfg = VERSIONS[version]
    RELOC_TABLE_ROM_ADDR = cfg["reloc_table_rom_addr"]
    RELOC_FILE_COUNT = cfg["reloc_file_count"]
    RELOC_TABLE_SIZE = (RELOC_FILE_COUNT + 1) * RELOC_TABLE_ENTRY_SIZE
    RELOC_DATA_START = RELOC_TABLE_ROM_ADDR + RELOC_TABLE_SIZE

# ============================================================================
#  Supplemental names for files that reloc_data.h only knows as ll_NNN_FileID
# ============================================================================

# Stage wallpaper sprites (same decompressed size 0x26CD0 as named wallpapers)
SUPPLEMENTAL_NAMES = {
    91:  ("StageHyruleWallpaper",       "stages"),
    96:  ("StageYamabukiWallpaper",     "stages"),
    97:  ("StageInishieWallpaper",      "stages"),
    98:  ("StageLastWallpaper",         "stages"),
    119: ("StageMetalWallpaper",        "stages"),

    # Extern data banks — display list / texture data loaded as dependencies
    # by GR map files, EF effects, IT items, and fighter models.
    # Files 100-118, 120-135: stage/effect/item texture+DL banks
    100: ("ExternDataBank100",  "extern_data"),
    101: ("ExternDataBank101",  "extern_data"),
    102: ("ExternDataBank102",  "extern_data"),
    103: ("ExternDataBank103",  "extern_data"),
    104: ("ExternDataBank104",  "extern_data"),
    105: ("ExternDataBank105",  "extern_data"),
    106: ("ExternDataBank106",  "extern_data"),
    107: ("ExternDataBank107",  "extern_data"),
    108: ("ExternDataBank108",  "extern_data"),
    109: ("ExternDataBank109",  "extern_data"),
    110: ("ExternDataBank110",  "extern_data"),
    111: ("ExternDataBank111",  "extern_data"),
    112: ("ExternDataBank112",  "extern_data"),
    113: ("ExternDataBank113",  "extern_data"),
    114: ("ExternDataBank114",  "extern_data"),
    115: ("ExternDataBank115",  "extern_data"),
    116: ("ExternDataBank116",  "extern_data"),
    117: ("ExternDataBank117",  "extern_data"),
    118: ("ExternDataBank118",  "extern_data"),
    120: ("ExternDataBank120",  "extern_data"),
    121: ("ExternDataBank121",  "extern_data"),
    122: ("ExternDataBank122",  "extern_data"),
    123: ("ExternDataBank123",  "extern_data"),
    124: ("ExternDataBank124",  "extern_data"),
    125: ("ExternDataBank125",  "extern_data"),
    126: ("ExternDataBank126",  "extern_data"),
    127: ("ExternDataBank127",  "extern_data"),
    128: ("ExternDataBank128",  "extern_data"),
    129: ("ExternDataBank129",  "extern_data"),
    130: ("ExternDataBank130",  "extern_data"),
    131: ("ExternDataBank131",  "extern_data"),
    132: ("ExternDataBank132",  "extern_data"),
    133: ("ExternDataBank133",  "extern_data"),
    134: ("ExternDataBank134",  "extern_data"),
    135: ("ExternDataBank135",  "extern_data"),

    # Bonus stage texture/DL data banks (BtT + BtP stage assets)
    137: ("BonusDataBank137",   "bonus"),
    138: ("BonusDataBank138",   "bonus"),
    139: ("BonusDataBank139",   "bonus"),
    140: ("BonusDataBank140",   "bonus"),
    141: ("BonusDataBank141",   "bonus"),
    142: ("BonusDataBank142",   "bonus"),
    143: ("BonusDataBank143",   "bonus"),
    144: ("BonusDataBank144",   "bonus"),
    145: ("BonusDataBank145",   "bonus"),
    146: ("BonusDataBank146",   "bonus"),
    147: ("BonusDataBank147",   "bonus"),
    148: ("BonusDataBank148",   "bonus"),
    149: ("BonusDataBank149",   "bonus"),
    150: ("BonusDataBank150",   "bonus"),

    # Misc data banks (fighter/interface dependencies)
    152: ("MiscDataBank152",    "extern_data"),
    153: ("MiscDataBank153",    "extern_data"),
    154: ("MiscDataBank154",    "extern_data"),
    155: ("MiscDataBank155",    "extern_data"),
    156: ("MiscDataBank156",    "extern_data"),
    157: ("MiscDataBank157",    "extern_data"),
    158: ("MiscDataBank158",    "extern_data"),
    159: ("MiscDataBank159",    "extern_data"),
    160: ("MiscDataBank160",    "extern_data"),

    # Misc scattered unnamed
    29:  ("MiscData029",        "extern_data"),
    39:  ("MiscData039",        "extern_data"),
    47:  ("MiscData047",        "extern_data"),
    86:  ("MiscData086",        "extern_data"),
    162: ("MiscData162",        "extern_data"),
    201: ("MiscData201",        "extern_data"),
    230: ("MiscData230",        "extern_data"),
    253: ("MiscData253",        "extern_data"),
    299: ("MiscData299",        "extern_data"),
    302: ("MiscData302",        "extern_data"),
    315: ("MiscData315",        "extern_data"),
    326: ("MiscData326",        "extern_data"),
}

# ============================================================================
#  Sub-motion animation names (files 357-498)
#  Fighter-specific: entry poses, taunts, victory poses, results anims
# ============================================================================

def _build_submotion_names():
    """Build name table for sub-motion animation files 357-498."""
    names = {}

    # Per-fighter sub-motion blocks from scsubsysdata*.c analysis
    fighters = [
        # (start_id, fighter_name, count, slot_names)
        (357, "Mario", 13, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3", "IdleAlt",
            "Results1", "Results2", "Results3", "Results4",
        ]),
        (370, "Fox", 11, [
            "AppearR", "AppearL", "AppearAlt", "Taunt",
            "Victory1", "Victory2", "Victory3",
            "Results1", "Results2", "Results3", "Results4",
        ]),
        (381, "Donkey", 12, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3", "VictoryAlt",
            "Results1", "Results2", "Results3",
        ]),
        (393, "Samus", 11, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3",
            "Results1", "Results2", "Results3",
        ]),
        (404, "Link", 12, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3", "IdleAlt",
            "Results1", "Results2", "Results3",
        ]),
        (416, "Kirby", 13, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3", "AppearSpecial", "IdleAlt",
            "Results1", "Results2", "Results3",
        ]),
        (429, "Captain", 7, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2",
            "Victory2", "Victory3", "Results1",
        ]),
        (436, "Ness", 7, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2",
            "Victory2", "Victory3", "Results1",
        ]),
        (443, "Yoshi", 15, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3",
            "Results1", "Results2", "Results3", "Results4",
            "ResultsAlt1", "ResultsAlt2", "ResultsAlt3",
        ]),
        (458, "Boss", 4, [
            "Attack1", "Attack2", "Attack3", "Attack4",
        ]),
        (462, "Luigi", 7, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2",
            "Victory2", "Victory3", "Results1",
        ]),
        (469, "Purin", 7, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2",
            "Victory2", "Victory3", "Results1",
        ]),
        (476, "Pikachu", 12, [
            "AppearR", "AppearL", "AppearAlt1", "AppearAlt2", "Taunt",
            "Victory1", "Victory2", "Victory3", "IdleAlt",
            "Results1", "Results2", "Results3",
        ]),
    ]

    for start_id, fighter, count, slot_names in fighters:
        for i in range(count):
            fid = start_id + i
            slot = slot_names[i] if i < len(slot_names) else f"Slot{i}"
            names[fid] = (f"FT{fighter}SubMotion{slot}", "submotions")

    # Costume variant idle overrides (488-498)
    variant_fighters = [
        (488, "MMario"), (489, "NMario"), (490, "NFox"), (491, "NDonkey"),
        (492, "NSamus"), (493, "NLink"), (494, "NYoshi"), (495, "NCaptain"),
        (496, "NKirby"), (497, "NPikachu"), (498, "NNess"),
    ]
    for fid, variant in variant_fighters:
        names[fid] = (f"FT{variant}SubMotionIdle", "submotions")

    return names

SUBMOTION_NAMES = _build_submotion_names()

# ============================================================================
#  Main motion animation ranges (files 499-2131)
#  Per-fighter blocks of combat/common animations
# ============================================================================

# (start_id, end_id_inclusive, fighter_name)
MAIN_MOTION_RANGES = [
    (499,  641,  "Mario"),
    (642,  799,  "Fox"),
    (800,  952,  "Donkey"),
    (953,  1102, "Samus"),
    (1103, 1114, "Luigi"),       # Luigi-exclusive (shares most with Mario)
    (1115, 1258, "Link"),
    (1259, 1444, "Kirby"),
    (1445, 1503, "KirbyCopy"),   # Kirby copy ability anims (shared with Purin)
    (1504, 1511, "Purin"),       # Purin-exclusive (shares most with Kirby)
    (1512, 1663, "Captain"),
    (1664, 1814, "Ness"),
    (1815, 1956, "Yoshi"),
    (1957, 2097, "Pikachu"),
    (2098, 2131, "MasterHand"),
]

def get_main_motion_name(file_id):
    """Get fighter name and index for a main motion animation file."""
    for start, end, fighter in MAIN_MOTION_RANGES:
        if start <= file_id <= end:
            idx = file_id - start
            return f"FT{fighter}Anim{idx:03d}", "animations"
    return None, None

# ============================================================================
#  File ID → Name mapping (parsed from reloc_data.h)
# ============================================================================

def parse_reloc_data_header(header_path):
    """Parse include/reloc_data.h to build file_id -> (symbol_name, clean_name) map."""
    id_to_name = {}
    pattern = re.compile(r'#define\s+(ll(\w+)FileID)\s+\(\(intptr_t\)(0x[0-9a-fA-F]+)\)')

    with open(header_path, 'r') as f:
        for line in f:
            m = pattern.match(line.strip())
            if m:
                symbol = m.group(1)   # e.g. llMNCommonFileID
                name = m.group(2)     # e.g. MNCommon
                file_id = int(m.group(3), 16)
                id_to_name[file_id] = (symbol, name)

    return id_to_name

# Maps this ROM version's actual file_id -> the US ordinal embedded in its
# numeric `ll_<N>_FileID` symbol name. SUPPLEMENTAL_NAMES / SUBMOTION_NAMES /
# MAIN_MOTION_RANGES below are all hand-keyed by US ordinals; JP shifts every
# file_id (US 2132 vs JP 2107 files), so those tables must be looked up by the
# US ordinal, not the raw version file_id. Without this, JP submotion files
# (US 357-498) land at JP ids 332-473 and fall through to "file_NNNN" — which
# the port's portRelocIsFighterFigatreeFile() then misclassifies as non-
# figatree (it keys on the "reloc_submotions/FT" path prefix), skipping the
# AObj halfswap fixup and leaving CSS/opening fighters T-posing.
#
# Built from the header itself: the decomp emits `ll_<N>_FileID` for every
# file the pretty-name tables don't cover, where <N> is the US ordinal and the
# value is the per-version file_id. parse_reloc_data_header() captures these
# with clean name "_<N>_". For US this map is the identity (ll_357 == 357), so
# US yaml output is byte-for-byte unchanged.
_VERID_TO_USORD = {}

def build_verid_to_usord(id_to_name):
    """{version_file_id: us_ordinal} from the header's ll_<N>_FileID symbols."""
    m = {}
    for fid, (_sym, clean) in id_to_name.items():
        mo = re.match(r'^_(\d+)_$', clean)
        if mo:
            m[fid] = int(mo.group(1))
    return m

def resolve_name(file_id, id_to_name):
    """Resolve a file ID to (name, category_override_or_None).

    Priority: reloc_data.h named > supplemental > submotion > main motion > fallback.
    """
    # Check reloc_data.h first — but skip unnamed patterns like "_NNN_"
    if file_id in id_to_name:
        _, clean = id_to_name[file_id]
        if not re.match(r'^_\d+_$', clean):
            return clean, None  # Named in header, no category override

    # The US-ordinal-keyed tables below are version-agnostic only when looked
    # up by US ordinal; identity for US, the -25-ish shift undone for JP.
    usord = _VERID_TO_USORD.get(file_id, file_id)

    # Supplemental names (stage wallpapers, extern data banks, etc.)
    if usord in SUPPLEMENTAL_NAMES:
        name, cat = SUPPLEMENTAL_NAMES[usord]
        return name, cat

    # Sub-motion animations (US ordinals 357-498)
    if usord in SUBMOTION_NAMES:
        name, cat = SUBMOTION_NAMES[usord]
        return name, cat

    # Main motion animations (US ordinals 499-2131)
    name, cat = get_main_motion_name(usord)
    if name:
        return name, cat

    # Fallback
    return f"file_{file_id:04d}", None

# ============================================================================
#  File categorization
# ============================================================================

# Category rules: (category_name, match_function)
# Order matters — first match wins.
CATEGORIES = [
    ("menus", lambda fid, name: name.startswith("MN") or name.startswith("MNCommon")),
    ("transitions", lambda fid, name: "Transition" in name or "LBTransition" in name),
    ("movies", lambda fid, name: name.startswith("MV") or name.startswith("MVOpening")),
    ("interface", lambda fid, name: name.startswith("IF") or name.startswith("IFCommon")),
    ("effects", lambda fid, name: name.startswith("EF") or name.startswith("EFCommon")),
    ("stages", lambda fid, name: name.startswith("Stage") or name.startswith("GR")),
    ("fighters_main", lambda fid, name: (
        any(name.startswith(p) for p in [
            "Mario", "Fox", "Donkey", "Samus", "Luigi", "Link", "Kirby",
            "Purin", "Captain", "Ness", "Pikachu", "Yoshi", "Boss",
            "MMario", "NMario", "NFox", "NDonkey", "NSamus", "NLuigi",
            "NLink", "NKirby", "NPurin", "NCaptain", "NNess", "NPikachu",
            "NYoshi", "GDonkey", "MasterHand", "DkIcon",
        ]) and not name.startswith("FT")
    )),
    ("animations", lambda fid, name: "Anim" in name and name.startswith("FT")),
    ("fighters_common", lambda fid, name: (
        name.startswith("FTEmblem") or name.startswith("FTStocks") or
        name.startswith("FTManager")
    )),
    ("items", lambda fid, name: name.startswith("IT")),
    ("scene", lambda fid, name: name.startswith("SC")),
    ("bonus", lambda fid, name: "Bonus" in name),
    ("congra", lambda fid, name: "Congra" in name),
    ("misc_named", lambda fid, name: (
        name.startswith("N64Logo") or name.startswith("SCStaffroll") or
        name.startswith("SY") or name.startswith("CharacterNames") or
        name.startswith("GRWallpaper") or name.startswith("MNSoundTest") or
        name.startswith("SCExplain") or name.startswith("SC1PTraining")
    )),
    # Catch-all for anything still uncategorized
    ("misc", lambda fid, name: True),
]

def categorize_file(file_id, name):
    """Return category name for a given file."""
    for cat_name, match_fn in CATEGORIES:
        if match_fn(file_id, name):
            return cat_name
    return "misc"

# ============================================================================
#  ROM reading
# ============================================================================

def read_reloc_table(rom_path):
    """Read the LBReloc file table from the ROM. Returns list of table entries."""
    entries = []
    with open(rom_path, 'rb') as f:
        f.seek(RELOC_TABLE_ROM_ADDR)
        for i in range(RELOC_FILE_COUNT + 1):
            data = f.read(RELOC_TABLE_ENTRY_SIZE)
            if len(data) < RELOC_TABLE_ENTRY_SIZE:
                raise ValueError(f"Unexpected EOF reading table entry {i}")

            first_word, reloc_intern, compressed_size, reloc_extern, decompressed_size = \
                struct.unpack('>IHHHH', data)

            is_compressed = bool(first_word & 0x80000000)
            data_offset = first_word & 0x7FFFFFFF

            entries.append({
                'id': i,
                'is_compressed': is_compressed,
                'data_offset': data_offset,
                'reloc_intern_offset': reloc_intern,
                'compressed_size': compressed_size,  # in 32-bit words
                'reloc_extern_offset': reloc_extern,
                'decompressed_size': decompressed_size,  # in 32-bit words
            })

    return entries

def compute_file_info(entries):
    """Compute ROM offset and byte size for each file from the table entries."""
    files = []
    for i in range(len(entries) - 1):
        entry = entries[i]
        next_entry = entries[i + 1]

        rom_offset = RELOC_DATA_START + entry['data_offset']
        # Size in ROM = distance to next file's data
        rom_size = next_entry['data_offset'] - entry['data_offset']
        decompressed_bytes = entry['decompressed_size'] * 4

        files.append({
            'id': i,
            'rom_offset': rom_offset,
            'rom_size': rom_size,
            'decompressed_size': decompressed_bytes,
            'is_compressed': entry['is_compressed'],
            'reloc_intern_offset': entry['reloc_intern_offset'],
            'reloc_extern_offset': entry['reloc_extern_offset'],
        })

    return files

# ============================================================================
#  YAML generation
# ============================================================================

def generate_yaml_header():
    """Generate the :config: block for reloc file YAMLs."""
    # Use segment 0x00 mapped to ROM 0x000000 so offsets are absolute ROM addresses
    return (
        ":config:\n"
        "  segments:\n"
        "    - [0x00, 0x000000]\n"
        "\n"
    )

def generate_reloc_entry(name, rom_offset, rom_size, file_id, is_compressed, decompressed_size,
                         category=None):
    """Generate a single SSB64:RELOC YAML entry.

    The Torch SSB64:RELOC factory reads the relocation table from ROM directly
    using the file_id, so offset/size are kept for documentation but the factory
    derives all metadata from the table entry.

    Deblobbed categories (tools/deblob_categories.py) get `archive: false`
    once EMIT_ARCHIVE_FALSE is flipped: the bundle still parses (children
    slice from it via ssb64_reloc_parent) but its whole-file blob stops
    shipping — the port synthesizes it from the slices (docs/deblob.md).
    Living here means RegenerateRelocYamls can never clobber the flag.
    """
    lines = []
    lines.append(f"{name}:")
    lines.append(f"  type: SSB64:RELOC")
    lines.append(f"  offset: 0x{rom_offset:X}")
    lines.append(f"  size: 0x{rom_size:X}")
    lines.append(f"  symbol: {name}")
    lines.append(f"  file_id: {file_id}")
    if EMIT_ARCHIVE_FALSE and category in DEBLOBBED_CATEGORIES:
        lines.append(f"  archive: false")
    # Keep these as comments for human reference
    lines.append(f"  # compressed: {is_compressed}")
    lines.append(f"  # decompressed_size: 0x{decompressed_size:X}")
    return "\n".join(lines)

def write_yaml_files(files, id_to_name, output_dir):
    """Group files by category and write YAML files."""
    # Resolve names and categories
    groups = {}
    for f in files:
        name, cat_override = resolve_name(f['id'], id_to_name)
        cat = cat_override if cat_override else categorize_file(f['id'], name)
        if cat not in groups:
            groups[cat] = []
        groups[cat].append((f, name))

    os.makedirs(output_dir, exist_ok=True)
    written_files = []

    for cat_name, file_list in sorted(groups.items()):
        yaml_path = os.path.join(output_dir, f"reloc_{cat_name}.yml")
        with open(yaml_path, 'w', encoding='utf-8', newline='\n') as yf:
            yf.write(f"# SSB64 relocatable files — {cat_name}\n")
            yf.write(f"# Auto-generated by tools/generate_yamls.py\n")
            yf.write(f"# {len(file_list)} files\n\n")
            yf.write(generate_yaml_header())

            for i, (f, name) in enumerate(file_list):
                if i > 0:
                    yf.write("\n\n")
                yf.write(generate_reloc_entry(
                    name=name,
                    rom_offset=f['rom_offset'],
                    rom_size=f['rom_size'],
                    file_id=f['id'],
                    is_compressed=f['is_compressed'],
                    decompressed_size=f['decompressed_size'],
                    category=cat_name,
                ))
            yf.write("\n")

        written_files.append((yaml_path, len(file_list)))
        print(f"  Wrote {yaml_path} ({len(file_list)} files)")

    return written_files

# ============================================================================
#  Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description="Generate Torch reloc YAMLs from the SSB64 ROM.")
    parser.add_argument("--version", choices=sorted(VERSIONS), default="us",
                        help="ROM version (default: us)")
    parser.add_argument("rom", nargs="?", default=None,
                        help="ROM path (default: baserom.<version>.z64 in repo root)")
    args = parser.parse_args()
    version = args.version
    select_version(version)

    repo_root = Path(__file__).resolve().parent.parent
    rom_path = args.rom if args.rom else str(repo_root / f"baserom.{version}.z64")

    if not os.path.isfile(rom_path):
        print(f"Error: ROM not found at {rom_path}")
        sys.exit(1)

    # Per-version generated header (reloc_data.us.h / reloc_data.jp.h);
    # the decomp's selector shim picks the right one by REGION_* at compile.
    header_name = f"reloc_data.{version}.h"
    header_path = str(repo_root / "include" / header_name)
    if not os.path.isfile(header_path):
        print(f"Error: {header_name} not found at {header_path}")
        sys.exit(1)

    output_dir = str(repo_root / "yamls" / version)

    print(f"ROM: {rom_path}")
    print(f"Header: {header_path}")
    print(f"Output: {output_dir}")
    print()

    # Parse file ID names from header
    print("Parsing reloc_data.h...")
    id_to_name = parse_reloc_data_header(header_path)
    global _VERID_TO_USORD
    _VERID_TO_USORD = build_verid_to_usord(id_to_name)
    print(f"  Found {len(id_to_name)} named file IDs"
          f" ({len(_VERID_TO_USORD)} numeric ll_<N> ordinal remaps)")
    print()

    # Read ROM file table
    print(f"Reading reloc table at ROM 0x{RELOC_TABLE_ROM_ADDR:X}...")
    entries = read_reloc_table(rom_path)
    print(f"  Read {len(entries)} table entries (including sentinel)")
    print()

    # Compute file offsets and sizes
    files = compute_file_info(entries)
    print(f"  {len(files)} files total")

    compressed_count = sum(1 for f in files if f['is_compressed'])
    print(f"  {compressed_count} compressed (VPK0)")
    print(f"  {len(files) - compressed_count} uncompressed")

    total_rom_bytes = sum(f['rom_size'] for f in files)
    print(f"  Total ROM footprint: {total_rom_bytes:,} bytes ({total_rom_bytes / 1024 / 1024:.2f} MB)")

    # Sanity checks
    zero_size = [f for f in files if f['rom_size'] == 0]
    if zero_size:
        print(f"  Warning: {len(zero_size)} files have zero ROM size")

    # Count resolved names
    unnamed_count = 0
    for f in files:
        name, _ = resolve_name(f['id'], id_to_name)
        if name.startswith("file_"):
            unnamed_count += 1
    print(f"  Fully named: {len(files) - unnamed_count}, still unnamed: {unnamed_count}")

    print()

    # Clean old YAML files before regenerating
    for old_file in Path(output_dir).glob("reloc_*.yml"):
        old_file.unlink()

    # Generate YAMLs
    print("Generating YAML files...")
    written = write_yaml_files(files, id_to_name, output_dir)
    print()
    print(f"Done! Generated {len(written)} YAML files with {sum(n for _, n in written)} total entries.")

if __name__ == '__main__':
    main()

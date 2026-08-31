/**
 * port_css_stage_assets.cpp — Generic CSS stage asset loader (port-only).
 *
 * Loads full-resolution CSS wallpaper PNGs and 48x36 icon PNGs that were
 * extracted from baserom.us.z64 at CMake build time by
 * tools/derive_stage_assets.py, converts them to RGBA16 BE (the format the
 * Fast3D renderer expects for N64 sprite textures), and wraps each in a
 * synthetic Sprite + Bitmap array.
 *
 * The PNGs live at runtime under:
 *   <app-data>/assets/css_icons/<name>_background.png
 *   <app-data>/assets/css_icons/<name>_small.png
 *   <app-data>/assets/css_icons/<name>_name.png   (optional — see below)
 *
 * PNGs are only present on dev builds (CMake derives them from the baserom)
 * or when a user drops in custom art — shipped release builds contain no
 * ROM-derived files, and the packaging scripts never bundle css_icons
 * (docs/bugs/desktop_extra_stage_css_icons_2026-08-31.md). So for the
 * background and icon sprites there is a second source: the wallpaper
 * Sprite inside the stage's reloc file in the user's own extracted
 * BattleShip.o2r, parsed and downscaled at runtime. Lookup order:
 *   1. <app-data>/assets/css_icons/<name><suffix>.png   (moddable override)
 *   2. same path relative to the real app bundle dir (portable builds)
 *   3. derive from the o2r wallpaper sprite (background + icon only) —
 *      but only when no PNG exists at all: a PNG that exists and fails to
 *      load is a broken override, surfaced as the question-mark fallback
 *      rather than silently papered over with the ROM-derived image.
 *
 * Nameplates have no o2r source (they are synthesized text, no ROM sprite
 * exists) — when the PNG is absent the getter returns NULL and mnmaps.c
 * falls back to rendering the stage name with the in-game subtitle font.
 * Emblems likewise stay PNG-only (and are currently skipped by
 * mnMapsMakeEmblem anyway).
 *
 * Sprite construction mirrors the old port_css_fd_background.cpp /
 * port_css_fd_icon.cpp pattern:
 *   - Background: 300x220, 44 bitmaps x 5 rows each (bmheight=5, bmHreal=6),
 *                 ndisplist=552, RGBA16.
 *   - Icon: 48x36, 1 bitmap, ndisplist=36, RGBA16.
 *   - Name: 96x10, 1 bitmap, ndisplist=10, RGBA16.
 *     Dimensions match ROM IA4 nameplate sprites (MNMaps/file 30).
 *     Only port-introduced stages with a synthesized nameplate PNG return
 *     non-NULL; ROM-resident stages return NULL so caller uses ROM sprite.
 * These constants are derived from dStageLastBackground_0x26c88 (ROM-verified
 * in the old port_css_fd_background.h header comment).
 *
 * Adding a new stage: add one entry to STAGE_TABLE below.
 */

#ifdef PORT

#include "port_css_stage_assets.h"

#include "../app_paths.h"
#include "../bridge/lbreloc_byteswap.h"
#include "../resource/RelocFile.h"
#include "../resource/RelocFileTable.h"
#include "../resource/RelocPointerTable.h"
#include "../port_log.h"

#include <ship/resource/ResourceManager.h>

// stb_image's implementation lives in libultraship's stb_impl.c.
// Only include the header to pick up declarations.
#include <stb_image.h>

#include <libultraship/libultraship.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* PR/sp.h uses _Static_assert (C11) and cannot be included from C++.
 * Opaque type tags. */
struct sprite;
struct bitmap;
typedef struct sprite Sprite;
typedef struct bitmap Bitmap;

namespace {

// ---------------------------------------------------------------------------
// Stage table.
//
// To add a new port-introduced stage:
//   1. Append a StageEntry here.
//   2. Add a matching entry to STAGES in tools/derive_stage_assets.py.
//   3. Add the CMake OUTPUT + DEPENDS + POST_BUILD copy for the new PNGs.
// ---------------------------------------------------------------------------

struct StageEntry {
    int         gkind;       // GRKind enum value
    const char *name;        // filename stem (e.g. "final_destination")

    // Background sprite geometry (ROM-verified from dStageLastBackground_0x26c88):
    int         bg_w;        // full image width  (pixels)
    int         bg_h;        // full image height (pixels)
    int         bg_nbitmaps; // number of TMEM-slice bitmaps
    int         bg_bm_h;    // rendered rows per bitmap (bmheight)
    int         bg_bm_hreal;// N64 bmHreal (rows in the TMEM load including fringe)
    int         bg_ndisplist;// ndisplist from ROM sprite

    // Nameplate sprite: set to true when a synthesized <name>_name.png exists.
    // False for stages that ship a ROM-resident IA4 nameplate (the 9 starters).
    bool        has_name_png; // if false, portCSSGetStageNameSprite returns NULL

    // Emblem sprite: set to true when a synthesized <name>_emblem.png exists.
    // False for stages that use a ROM-resident 64x48 IA4 emblem (the 9 starters).
    // The PNG is loaded as a silhouette mask (alpha channel) and packed to IA4
    // single-bitmap at runtime — matches the ROM FT emblem format byte-for-byte
    // so no multi-bitmap strip geometry is needed.
    bool        has_emblem_png; // if false, portCSSGetStageEmblemSprite returns NULL

    // o2r wallpaper source — reloc file id + byte offset of the 300x220
    // RGBA16 wallpaper Sprite inside that file's (big-endian) data. Used to
    // derive the background/icon at runtime when no PNG exists (release
    // builds). Mirrors STAGES in tools/derive_stage_assets.py, which reads
    // the same sprite out of the same file at build time.
    uint32_t    wp_file_id;
    uint32_t    wp_sprite_off;
};

static constexpr StageEntry STAGE_TABLE[] = {
    {
        /* nGRKindLast = 16 */
        16, "final_destination",
        300, 220,
        44,  /* bg_nbitmaps: 44 * 5 rendered rows = 220 total */
        5,   /* bg_bm_h:    rows rendered per bitmap */
        6,   /* bg_bm_hreal: rows in the TMEM load   */
        552, /* bg_ndisplist from dStageLastBackground_0x26c88 */
        true,/* has_name_png: synthesized 96x10 nameplate */
        true,/* has_emblem_png: upscaled 64x48 emblem from MasterHand icon */
        96, 0x26c88, /* wallpaper: reloc file 96 (StageLastFile1), dStageLastBackground */
    },
    {
        /* nGRKindMetal = 13 — Meta Crystal (Metal Cavern in the port CSS).
         * Stage data file 0x62 ships the same 300x220/44-bitmap RGBA16
         * wallpaper format as FD at offset 0x26c88 — geometry matches FD
         * byte-for-byte so the same buildBackgroundSprite parameters work. */
        13, "metal_cavern",
        300, 220,
        44, 5, 6, 552,
        true,  /* synthesized "METAL CAVERN" nameplate */
        false, /* no emblem — same skip path as FD until IA4 render is fixed */
        0x62, 0x26c88, /* wallpaper: reloc file 0x62 — Meta Crystal stage data */
    },
    {
        /* nGRKindZako = 14 — Duel Zone (Battlefield in the port CSS).
         * Same wallpaper geometry as FD/Metal. */
        14, "battlefield",
        300, 220,
        44, 5, 6, 552,
        true,  /* synthesized "BATTLEFIELD" nameplate */
        false, /* no emblem */
        0x61, 0x26c88, /* wallpaper: reloc file 0x61 — Duel Zone stage data */
    },
    // { next_gkind, "next_name", bg_w, bg_h, bg_nbitmaps, bg_bm_h, bg_bm_hreal,
    //   bg_ndisplist, has_name_png, has_emblem_png, wp_file_id, wp_sprite_off },
};

static constexpr int kStageCount = (int)(sizeof(STAGE_TABLE) / sizeof(STAGE_TABLE[0]));

// Icon dimensions — same for all stages (matches N64 CSS format).
constexpr int      kIconW  = 48;
constexpr int      kIconH  = 36;
constexpr uint8_t  kFmtRGBA16 = 0; /* G_IM_FMT_RGBA */
constexpr uint8_t  kFmtIA     = 4; /* G_IM_FMT_IA   — emblems, nameplates */
constexpr uint8_t  kSiz16b    = 2; /* G_IM_SIZ_16b  */
constexpr uint8_t  kSiz4b     = 0; /* G_IM_SIZ_4b   — IA4: 2 pixels/byte */

// ---------------------------------------------------------------------------
// Cache entries.
// ---------------------------------------------------------------------------

enum AssetKind { kBackground = 0, kIcon = 1, kName = 2, kEmblem = 3, kAssetKindCount = 4 };

struct CacheEntry {
    Sprite   *sprite       = nullptr;
    Bitmap   *bitmaps      = nullptr;  // pointer to first Bitmap in the array
    uint8_t  *rgba16_buf   = nullptr;  // heap-allocated RGBA16 BE pixel data
    int       nbitmaps     = 0;
    bool      derive_failed = false;   // o2r derivation failed once — don't
                                       // re-parse/re-log on every CSS rebuild
                                       // (the archive can't change mid-session;
                                       // a PNG dropped in later is still probed)
};

// Flat 2D array: cache[gkind_index][asset_kind]
static CacheEntry  sCache[kStageCount][kAssetKindCount];
static std::mutex  sCacheMutex;

// ---------------------------------------------------------------------------
// Sprite builder helpers.
// ---------------------------------------------------------------------------

static inline void put_s16(uint8_t *b, unsigned off, int16_t v) {
    uint16_t u; std::memcpy(&u, &v, 2);
    b[off+0] = (uint8_t)(u & 0xFF); b[off+1] = (uint8_t)(u >> 8);
}
static inline void put_u16(uint8_t *b, unsigned off, uint16_t v) {
    b[off+0] = (uint8_t)(v & 0xFF); b[off+1] = (uint8_t)(v >> 8);
}
static inline void put_u32(uint8_t *b, unsigned off, uint32_t v) {
    b[off+0] = (uint8_t)(v & 0xFF);
    b[off+1] = (uint8_t)((v >> 8) & 0xFF);
    b[off+2] = (uint8_t)((v >> 16) & 0xFF);
    b[off+3] = (uint8_t)(v >> 24);
}
static inline void put_f32(uint8_t *b, unsigned off, float v) {
    uint32_t bits; std::memcpy(&bits, &v, 4); put_u32(b, off, bits);
}

// ---------------------------------------------------------------------------
// RGBA8888 -> RGBA16 BE converter.
//
// stb_image returns 4-byte RGBA8888. The Fast3D renderer expects RGBA16 BE
// (R5G5B5A1) with no TMEM odd-row swizzle (synthetic sprites bypass the
// portFixupSpriteBitmapData swizzle via portMarkSyntheticSprite).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// RGBA8888 -> IA4 silhouette packer.
//
// IA4 is **3 bits of intensity + 1 bit of alpha** per pixel (NOT 4-bit
// intensity — see libultraship/src/fast/interpreter.cpp ImportTextureIA4).
// 2 pixels per byte; high nibble = even pixel, low nibble = odd pixel.
//
// For a silhouette emblem source (RGB ignored, alpha encodes the mask):
//   Visible pixel (alpha >= 128): pack 0xF = intensity 7 + alpha 1.
//     Intensity 7 = max, so SObj-side colour modulation (0x5C/0x22/0x00 brown)
//     tints the visible pixels to their authored colour, exactly like every
//     ROM FT emblem.
//   Transparent pixel: pack 0x0 = intensity 0 + alpha 0.
//
// IA4's 1-bit alpha can't represent partial transparency — LANCZOS-upscaled
// edges threshold to fully visible or fully invisible. That's the cost of
// matching the ROM IA4 format exactly; if anti-aliased edges become important,
// CI4 (4-bit indexed) is the next-format-up that fits TMEM at 64x48.
// Width must be even (always true for the emblem dims we target).
// ---------------------------------------------------------------------------

static uint8_t *convertAlphaToIA4(const uint8_t *rgba8, int w, int h) {
    if (w % 2 != 0) return nullptr; // IA4 packs 2 pixels per byte
    const size_t out_bytes = (size_t)w * (size_t)h / 2;
    uint8_t *out = static_cast<uint8_t *>(std::malloc(out_bytes));
    if (!out) return nullptr;

    const uint8_t *src = rgba8;
    uint8_t *dst = out;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; x += 2) {
            uint8_t a0 = src[(y * w + x + 0) * 4 + 3];
            uint8_t a1 = src[(y * w + x + 1) * 4 + 3];
            // Threshold each pixel to a 1-bit alpha mask; visible pixels get
            // full intensity (nibble = 0xF = 0b1111 = I=7 + A=1).
            uint8_t n0 = (a0 >= 128) ? 0xF : 0x0;
            uint8_t n1 = (a1 >= 128) ? 0xF : 0x0;
            *dst++ = (uint8_t)((n0 << 4) | n1);
        }
    }
    return out;
}

static uint8_t *convertToRGBA16BE(const uint8_t *rgba8, int w, int h) {
    if (w <= 0 || h <= 0) return nullptr;
    const size_t npixels = (size_t)w * (size_t)h;
    uint8_t *out = static_cast<uint8_t *>(std::malloc(npixels * 2));
    if (!out) return nullptr;

    for (size_t i = 0; i < npixels; ++i) {
        uint8_t r = rgba8[i * 4 + 0];
        uint8_t g = rgba8[i * 4 + 1];
        uint8_t b = rgba8[i * 4 + 2];
        uint8_t a = rgba8[i * 4 + 3];
        uint16_t r5 = r >> 3;
        uint16_t g5 = g >> 3;
        uint16_t b5 = b >> 3;
        uint16_t a1 = a >= 128 ? 1 : 0;
        uint16_t word = (r5 << 11) | (g5 << 6) | (b5 << 1) | a1;
        // Store big-endian.
        out[i * 2 + 0] = (uint8_t)(word >> 8);
        out[i * 2 + 1] = (uint8_t)(word & 0xFF);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Build a multi-bitmap background Sprite from heap-allocated RGBA16 data.
//
// Layout matches dStageLastBackground_0x26c88: w x h image split into
// nbitmaps strips of bm_h rendered rows each.
// ---------------------------------------------------------------------------

static Sprite *buildBackgroundSprite(const StageEntry &se,
                                     uint8_t *rgba16_buf,
                                     CacheEntry *entry)
{
    const int w           = se.bg_w;
    const int h           = se.bg_h;
    const int nbitmaps    = se.bg_nbitmaps;
    const int bm_h        = se.bg_bm_h;
    const int bm_hreal    = se.bg_bm_hreal;
    const int ndisplist   = se.bg_ndisplist;
    const int bm_bytes    = w * bm_h * 2; // bytes per bitmap strip (rendered rows only)

    uint8_t *sp_raw = static_cast<uint8_t *>(std::malloc(68));
    uint8_t *bm_raw = static_cast<uint8_t *>(std::malloc((size_t)nbitmaps * 16));
    void   **buf_ptrs = static_cast<void **>(std::malloc((size_t)nbitmaps * sizeof(void *)));
    if (!sp_raw || !bm_raw || !buf_ptrs) {
        std::free(sp_raw); std::free(bm_raw); std::free(buf_ptrs);
        return nullptr;
    }
    std::memset(sp_raw, 0, 68);
    std::memset(bm_raw, 0, (size_t)nbitmaps * 16);

    for (int i = 0; i < nbitmaps; ++i) {
        uint8_t *bm       = bm_raw + i * 16;
        uint8_t *strip    = rgba16_buf + (size_t)i * bm_bytes;
        put_s16(bm, 0x00, (int16_t)w);
        put_s16(bm, 0x02, (int16_t)w);
        put_s16(bm, 0x04, 0);
        put_s16(bm, 0x06, 0);
        put_u32(bm, 0x08, portRelocRegisterPointer(strip));
        put_s16(bm, 0x0C, (int16_t)bm_h);
        put_s16(bm, 0x0E, 0);
        buf_ptrs[i] = strip;
    }

    Bitmap *bm0 = reinterpret_cast<Bitmap *>(bm_raw);

    put_s16(sp_raw, 0x00, 0);
    put_s16(sp_raw, 0x02, 0);
    put_s16(sp_raw, 0x04, (int16_t)w);
    put_s16(sp_raw, 0x06, (int16_t)h);
    put_f32(sp_raw, 0x08, 1.0f);
    put_f32(sp_raw, 0x0C, 1.0f);
    put_s16(sp_raw, 0x10, 0); put_s16(sp_raw, 0x12, 0);
    put_u16(sp_raw, 0x14, 0); put_s16(sp_raw, 0x16, 0);
    sp_raw[0x18] = 0xFF; sp_raw[0x19] = 0xFF;
    sp_raw[0x1A] = 0xFF; sp_raw[0x1B] = 0xFF;
    put_s16(sp_raw, 0x1C, 0); put_s16(sp_raw, 0x1E, 0);
    put_u32(sp_raw, 0x20, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x24, 0); put_s16(sp_raw, 0x26, 1);
    put_s16(sp_raw, 0x28, (int16_t)nbitmaps);
    put_s16(sp_raw, 0x2A, (int16_t)ndisplist);
    put_s16(sp_raw, 0x2C, (int16_t)bm_h);
    put_s16(sp_raw, 0x2E, (int16_t)bm_hreal);
    sp_raw[0x30] = kFmtRGBA16; sp_raw[0x31] = kSiz16b;
    sp_raw[0x32] = 0;          sp_raw[0x33] = 0;
    put_u32(sp_raw, 0x34, portRelocRegisterPointer(bm0));
    put_u32(sp_raw, 0x38, portRelocRegisterPointer(nullptr));
    put_u32(sp_raw, 0x3C, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x40, 0); put_s16(sp_raw, 0x42, 0);

    Sprite *sp = reinterpret_cast<Sprite *>(sp_raw);
    portMarkSyntheticSprite(sp, bm0, (unsigned int)nbitmaps, buf_ptrs);
    std::free(buf_ptrs);

    entry->sprite     = sp;
    entry->bitmaps    = bm0;
    entry->rgba16_buf = rgba16_buf;
    entry->nbitmaps   = nbitmaps;
    return sp;
}

// ---------------------------------------------------------------------------
// Build a single-bitmap icon Sprite (48x36).
// ---------------------------------------------------------------------------

static Sprite *buildIconSprite(uint8_t *rgba16_buf, CacheEntry *entry)
{
    const int w         = kIconW;
    const int h         = kIconH;
    const int nbitmaps  = 1;
    const int ndisplist = h;

    uint8_t *sp_raw = static_cast<uint8_t *>(std::malloc(68));
    uint8_t *bm_raw = static_cast<uint8_t *>(std::malloc(16));
    if (!sp_raw || !bm_raw) { std::free(sp_raw); std::free(bm_raw); return nullptr; }
    std::memset(sp_raw, 0, 68);
    std::memset(bm_raw, 0, 16);

    put_s16(bm_raw, 0x00, (int16_t)w);
    put_s16(bm_raw, 0x02, (int16_t)w);
    put_s16(bm_raw, 0x04, 0);
    put_s16(bm_raw, 0x06, 0);
    put_u32(bm_raw, 0x08, portRelocRegisterPointer(rgba16_buf));
    put_s16(bm_raw, 0x0C, (int16_t)h);
    put_s16(bm_raw, 0x0E, 0);

    Bitmap *bm = reinterpret_cast<Bitmap *>(bm_raw);

    put_s16(sp_raw, 0x00, 0);
    put_s16(sp_raw, 0x02, 0);
    put_s16(sp_raw, 0x04, (int16_t)w);
    put_s16(sp_raw, 0x06, (int16_t)h);
    put_f32(sp_raw, 0x08, 1.0f);
    put_f32(sp_raw, 0x0C, 1.0f);
    put_s16(sp_raw, 0x10, 0); put_s16(sp_raw, 0x12, 0);
    put_u16(sp_raw, 0x14, 0); put_s16(sp_raw, 0x16, 0);
    sp_raw[0x18] = 0xFF; sp_raw[0x19] = 0xFF;
    sp_raw[0x1A] = 0xFF; sp_raw[0x1B] = 0xFF;
    put_s16(sp_raw, 0x1C, 0); put_s16(sp_raw, 0x1E, 0);
    put_u32(sp_raw, 0x20, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x24, 0); put_s16(sp_raw, 0x26, 1);
    put_s16(sp_raw, 0x28, (int16_t)nbitmaps);
    put_s16(sp_raw, 0x2A, (int16_t)ndisplist);
    put_s16(sp_raw, 0x2C, (int16_t)h);
    put_s16(sp_raw, 0x2E, (int16_t)h);
    sp_raw[0x30] = kFmtRGBA16; sp_raw[0x31] = kSiz16b;
    sp_raw[0x32] = 0;          sp_raw[0x33] = 0;
    put_u32(sp_raw, 0x34, portRelocRegisterPointer(bm));
    put_u32(sp_raw, 0x38, portRelocRegisterPointer(nullptr));
    put_u32(sp_raw, 0x3C, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x40, 0); put_s16(sp_raw, 0x42, 0);

    Sprite *sp = reinterpret_cast<Sprite *>(sp_raw);

    void *buf_ptrs[1] = { rgba16_buf };
    portMarkSyntheticSprite(sp, bm, (unsigned int)nbitmaps, buf_ptrs);

    entry->sprite     = sp;
    entry->bitmaps    = bm;
    entry->rgba16_buf = rgba16_buf;
    entry->nbitmaps   = nbitmaps;
    return sp;
}

// ---------------------------------------------------------------------------
// Build a single-bitmap nameplate Sprite (96x10).
//
// Matches the ROM IA4 nameplate sprite dimensions from reloc file 30 / MNMaps:
//   width=96, height=10, 1 bitmap, bmheight=10, bmHreal=10, ndisplist=36.
// Format is stored as RGBA16 (same as background/icon) because convertToRGBA16BE
// is applied at load time and portMarkSyntheticSprite bypasses the TMEM swizzle.
// The caller (mnmaps.c) must force red=green=blue=0x00 on the SObj so the
// sprite renders as black text on transparent — identical to ROM nameplate style.
// ---------------------------------------------------------------------------

static constexpr int kNameW  = 96;
static constexpr int kNameH  = 10;

static Sprite *buildNameSprite(uint8_t *rgba16_buf, CacheEntry *entry)
{
    const int w         = kNameW;
    const int h         = kNameH;
    const int nbitmaps  = 1;
    // ndisplist=36 matches ROM nameplate sprites (PeachsCastleText etc.)
    const int ndisplist = 36;

    uint8_t *sp_raw = static_cast<uint8_t *>(std::malloc(68));
    uint8_t *bm_raw = static_cast<uint8_t *>(std::malloc(16));
    if (!sp_raw || !bm_raw) { std::free(sp_raw); std::free(bm_raw); return nullptr; }
    std::memset(sp_raw, 0, 68);
    std::memset(bm_raw, 0, 16);

    put_s16(bm_raw, 0x00, (int16_t)w);
    put_s16(bm_raw, 0x02, (int16_t)w);
    put_s16(bm_raw, 0x04, 0);
    put_s16(bm_raw, 0x06, 0);
    put_u32(bm_raw, 0x08, portRelocRegisterPointer(rgba16_buf));
    put_s16(bm_raw, 0x0C, (int16_t)h);
    put_s16(bm_raw, 0x0E, 0);

    Bitmap *bm = reinterpret_cast<Bitmap *>(bm_raw);

    put_s16(sp_raw, 0x00, 0);
    put_s16(sp_raw, 0x02, 0);
    put_s16(sp_raw, 0x04, (int16_t)w);
    put_s16(sp_raw, 0x06, (int16_t)h);
    put_f32(sp_raw, 0x08, 1.0f);
    put_f32(sp_raw, 0x0C, 1.0f);
    put_s16(sp_raw, 0x10, 0); put_s16(sp_raw, 0x12, 0);
    put_u16(sp_raw, 0x14, 0); put_s16(sp_raw, 0x16, 0);
    sp_raw[0x18] = 0xFF; sp_raw[0x19] = 0xFF;
    sp_raw[0x1A] = 0xFF; sp_raw[0x1B] = 0xFF;
    put_s16(sp_raw, 0x1C, 0); put_s16(sp_raw, 0x1E, 0);
    put_u32(sp_raw, 0x20, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x24, 0); put_s16(sp_raw, 0x26, 1);
    put_s16(sp_raw, 0x28, (int16_t)nbitmaps);
    put_s16(sp_raw, 0x2A, (int16_t)ndisplist);
    put_s16(sp_raw, 0x2C, (int16_t)h);
    put_s16(sp_raw, 0x2E, (int16_t)h);
    sp_raw[0x30] = kFmtRGBA16; sp_raw[0x31] = kSiz16b;
    sp_raw[0x32] = 0;          sp_raw[0x33] = 0;
    put_u32(sp_raw, 0x34, portRelocRegisterPointer(bm));
    put_u32(sp_raw, 0x38, portRelocRegisterPointer(nullptr));
    put_u32(sp_raw, 0x3C, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x40, 0); put_s16(sp_raw, 0x42, 0);

    Sprite *sp = reinterpret_cast<Sprite *>(sp_raw);

    void *buf_ptrs[1] = { rgba16_buf };
    portMarkSyntheticSprite(sp, bm, (unsigned int)nbitmaps, buf_ptrs);

    entry->sprite     = sp;
    entry->bitmaps    = bm;
    entry->rgba16_buf = rgba16_buf;
    entry->nbitmaps   = nbitmaps;
    return sp;
}

// ---------------------------------------------------------------------------
// Build the FD-style emblem Sprite — IA4 single-bitmap, 64x48.
//
// Every ROM FT emblem in reloc file 20 (llFTEmblemSprites*) uses the same
// fields, byte-for-byte:
//   width=64 height=48 nbitmaps=1 ndisplist=36
//   bmheight=48 bmHreal=48 bmfmt=4 (IA) bmsiz=0 (IA4)
//   attr=0x0220 scalex=scaley=1.0
//
// IA4 = 4 bits intensity per pixel, 2 pixels per byte. 64*48/2 = 1536 bytes —
// fits TMEM (4 KB) as a single bitmap, so no multi-strip layout needed. The
// caller sets SObj red/green/blue to the franchise brown (0x5C/0x22/0x00) and
// the combiner tints the intensity texels — identical to how the ROM emblems
// render.
//
// This deliberately matches the ROM Sprite struct field-for-field. Don't
// invent a new layout; if the 9 ROM emblems render correctly, ours does too.
// ---------------------------------------------------------------------------

static constexpr int kEmblemW  = 64;
static constexpr int kEmblemH  = 48;

static Sprite *buildEmblemSprite(uint8_t *ia4_buf, CacheEntry *entry)
{
    const int w         = kEmblemW;
    const int h         = kEmblemH;
    const int nbitmaps  = 1;
    const int ndisplist = 36; // matches ROM FT emblems exactly

    uint8_t *sp_raw = static_cast<uint8_t *>(std::malloc(68));
    uint8_t *bm_raw = static_cast<uint8_t *>(std::malloc(16));
    if (!sp_raw || !bm_raw) { std::free(sp_raw); std::free(bm_raw); return nullptr; }
    std::memset(sp_raw, 0, 68);
    std::memset(bm_raw, 0, 16);

    // Bitmap struct (single bitmap covers the full sprite).
    put_s16(bm_raw, 0x00, (int16_t)w);
    put_s16(bm_raw, 0x02, (int16_t)w);
    put_s16(bm_raw, 0x04, 0);
    put_s16(bm_raw, 0x06, 0);
    put_u32(bm_raw, 0x08, portRelocRegisterPointer(ia4_buf));
    put_s16(bm_raw, 0x0C, (int16_t)h);
    put_s16(bm_raw, 0x0E, 0);

    Bitmap *bm = reinterpret_cast<Bitmap *>(bm_raw);

    put_s16(sp_raw, 0x00, 0);
    put_s16(sp_raw, 0x02, 0);
    put_s16(sp_raw, 0x04, (int16_t)w);
    put_s16(sp_raw, 0x06, (int16_t)h);
    put_f32(sp_raw, 0x08, 1.0f);
    put_f32(sp_raw, 0x0C, 1.0f);
    put_s16(sp_raw, 0x10, 0); put_s16(sp_raw, 0x12, 0);
    put_u16(sp_raw, 0x14, 0); put_s16(sp_raw, 0x16, 0);
    sp_raw[0x18] = 0xFF; sp_raw[0x19] = 0xFF;
    sp_raw[0x1A] = 0xFF; sp_raw[0x1B] = 0xFF;
    put_s16(sp_raw, 0x1C, 0); put_s16(sp_raw, 0x1E, 0);
    put_u32(sp_raw, 0x20, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x24, 0); put_s16(sp_raw, 0x26, 1);
    put_s16(sp_raw, 0x28, (int16_t)nbitmaps);
    put_s16(sp_raw, 0x2A, (int16_t)ndisplist);
    put_s16(sp_raw, 0x2C, (int16_t)h);
    put_s16(sp_raw, 0x2E, (int16_t)h);
    sp_raw[0x30] = kFmtIA;     sp_raw[0x31] = kSiz4b;
    sp_raw[0x32] = 0;          sp_raw[0x33] = 0;
    put_u32(sp_raw, 0x34, portRelocRegisterPointer(bm));
    put_u32(sp_raw, 0x38, portRelocRegisterPointer(nullptr));
    put_u32(sp_raw, 0x3C, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x40, 0); put_s16(sp_raw, 0x42, 0);

    Sprite *sp = reinterpret_cast<Sprite *>(sp_raw);

    void *buf_ptrs[1] = { ia4_buf };
    portMarkSyntheticSprite(sp, bm, (unsigned int)nbitmaps, buf_ptrs);

    entry->sprite     = sp;
    entry->bitmaps    = bm;
    entry->rgba16_buf = ia4_buf;   // field name is historical; holds the IA4 packed buffer
    entry->nbitmaps   = nbitmaps;
    return sp;
}

// ---------------------------------------------------------------------------
// Re-register fixup tracking on every call (scene-boundary safety).
// portResetStructFixups() clears sStructU16Fixups / sDeswizzle4cFixups at
// scene boundaries, so portMarkSyntheticSprite must be called again each time
// the CSS is entered to keep portFixup* as no-ops for these synthetic sprites.
// ---------------------------------------------------------------------------

static void reregisterEntry(const CacheEntry &e) {
    if (!e.sprite || !e.bitmaps || e.nbitmaps == 0) return;
    void **buf_ptrs = static_cast<void **>(std::malloc((size_t)e.nbitmaps * sizeof(void *)));
    if (!buf_ptrs) return;
    // Walk the Bitmap array, reading each bitmap's buf reloc token (LE u32 at
    // offset +0x08 within each 16-byte Bitmap struct) and resolving it back to
    // the actual pointer, so portMarkSyntheticSprite can re-register all entries.
    const uint8_t *bm_base = reinterpret_cast<const uint8_t *>(e.bitmaps);
    for (int i = 0; i < e.nbitmaps; ++i) {
        uint32_t token;
        std::memcpy(&token, bm_base + (size_t)i * 16 + 0x08, 4);
        buf_ptrs[i] = portRelocResolvePointer(token);
    }
    portMarkSyntheticSprite(e.sprite, e.bitmaps, (unsigned int)e.nbitmaps, buf_ptrs);
    std::free(buf_ptrs);
}

// ---------------------------------------------------------------------------
// PNG loader — loads a PNG and converts to RGBA16 BE.
// Expected dimensions are validated; returns nullptr on mismatch/error.
// ---------------------------------------------------------------------------

static uint8_t *loadPNGAsRGBA16BE(const std::string &path, int expected_w, int expected_h) {
    int w = 0, h = 0, channels = 0;
    uint8_t *rgba8 = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!rgba8) {
        port_log("CSS stage assets: stbi_load failed: %s (%s)\n",
                 path.c_str(),
                 stbi_failure_reason() ? stbi_failure_reason() : "?");
        return nullptr;
    }

    if (w != expected_w || h != expected_h) {
        port_log("CSS stage assets: %s — expected %dx%d, got %dx%d; skipping.\n",
                 path.c_str(), expected_w, expected_h, w, h);
        stbi_image_free(rgba8);
        return nullptr;
    }

    uint8_t *rgba16 = convertToRGBA16BE(rgba8, w, h);
    stbi_image_free(rgba8);

    if (!rgba16) {
        port_log("CSS stage assets: convertToRGBA16BE OOM for %s\n", path.c_str());
    }
    return rgba16;
}

// Loads a PNG and returns the alpha channel packed as IA4 (2 px/byte, w*h/2 bytes).
// Used for emblem sprites where matching the ROM's IA4 single-bitmap format
// avoids TMEM-overflow / multi-bitmap-strip rendering bugs entirely.
static uint8_t *loadPNGAsIA4(const std::string &path, int expected_w, int expected_h) {
    int w = 0, h = 0, channels = 0;
    uint8_t *rgba8 = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!rgba8) {
        port_log("CSS stage assets: stbi_load failed: %s (%s)\n",
                 path.c_str(),
                 stbi_failure_reason() ? stbi_failure_reason() : "?");
        return nullptr;
    }
    if (w != expected_w || h != expected_h) {
        port_log("CSS stage assets: %s — expected %dx%d, got %dx%d; skipping.\n",
                 path.c_str(), expected_w, expected_h, w, h);
        stbi_image_free(rgba8);
        return nullptr;
    }
    uint8_t *ia4 = convertAlphaToIA4(rgba8, w, h);
    stbi_image_free(rgba8);
    if (!ia4) {
        port_log("CSS stage assets: convertAlphaToIA4 OOM for %s\n", path.c_str());
    }
    return ia4;
}

// ---------------------------------------------------------------------------
// o2r wallpaper derivation — the release-build source for background + icon.
//
// Shipped builds contain no css_icons PNGs (they are ROM-derived and cannot
// be distributed), but the user's own extracted BattleShip.o2r carries each
// stage's 300x220 RGBA16 CSS wallpaper Sprite inside the reloc file named
// by STAGE_TABLE.wp_file_id. RelocFile::Data is the pristine decompressed
// big-endian file image (the lbreloc bridge memcpy's it into game RAM
// before patching, so the cached resource is never mutated) — parse the
// Sprite exactly the way tools/derive_stage_assets.py does at build time:
// resolve the bitmap array via the RELOC pointer encoding
// ((raw & 0xFFFF) * 4), un-swizzle the TMEM odd-row XOR4 pattern per strip,
// and keep only the bmheight rendered rows of each strip.
//
// Every parse step is bounds-checked and geometry-validated against
// STAGE_TABLE; any mismatch (e.g. a JP archive where the sprite moved)
// returns nullptr and the caller falls back to the question-mark sprite —
// never a garbage image.
// ---------------------------------------------------------------------------

static inline uint16_t be_u16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] << 8 | (uint16_t)p[1]);
}
static inline int16_t be_s16(const uint8_t *p) { return (int16_t)be_u16(p); }
static inline uint32_t be_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

// SSB64 RELOC internal-pointer slots encode a file-relative byte offset in
// the low 16 bits, in u32-word units (high 16 bits are the reloc chain link).
static inline uint32_t relocPtrToFileOff(const uint8_t *p) {
    return (be_u32(p) & 0xFFFFu) * 4u;
}

// TMEM odd-row un-swizzle for 16bpp strips (strip-local row indexing):
// within every 8-byte qword of each odd row, swap the two 4-byte halves.
// Reference: docs/bugs/sprite_texel_tmem_swizzle_2026-04-10.md
static void unswizzleRGBA16Strip(uint8_t *strip, int width, int height) {
    const int row_bytes = width * 2;
    for (int row = 1; row < height; row += 2) {
        uint8_t *row_data = strip + (size_t)row * row_bytes;
        for (int off = 0; off + 7 < row_bytes; off += 8) {
            std::swap(row_data[off + 0], row_data[off + 4]);
            std::swap(row_data[off + 1], row_data[off + 5]);
            std::swap(row_data[off + 2], row_data[off + 6]);
            std::swap(row_data[off + 3], row_data[off + 7]);
        }
    }
}

// Decode the stage's o2r wallpaper Sprite into the final full-image RGBA16 BE
// buffer (se.bg_w x se.bg_h, malloc'd — same contract as loadPNGAsRGBA16BE).
// The unswizzled strip rows already ARE the target format, so the background
// is a straight memcpy stitch of each strip's rendered rows — byte-identity
// with the source texels is structural, not a property of a conversion
// round-trip. Returns nullptr on any validation failure.
static uint8_t *deriveWallpaperRGBA16(const StageEntry &se) {
    if (se.wp_file_id >= RELOC_FILE_COUNT) {
        return nullptr;
    }
    auto ctx = Ship::Context::GetInstance();
    if (!ctx) {
        return nullptr;
    }
    auto resource = ctx->GetResourceManager()->LoadResource(
        std::string(gRelocFileTable[se.wp_file_id]));
    if (!resource) {
        port_log("CSS stage assets: %s — reloc file %u (%s) not in archive\n",
                 se.name, se.wp_file_id, gRelocFileTable[se.wp_file_id]);
        return nullptr;
    }
    // Guarded downcast, matching lbreloc_bridge.cpp: a shadowing archive (mod
    // o2r, fromsource o2r) could serve this path as a different resource type.
    auto relocFile = std::dynamic_pointer_cast<RelocFile>(resource);
    if (!relocFile) {
        port_log("CSS stage assets: %s — %s is not a RelocFile resource; skipping derivation\n",
                 se.name, gRelocFileTable[se.wp_file_id]);
        return nullptr;
    }
    const std::vector<uint8_t> &data = relocFile->Data;

    const size_t so = se.wp_sprite_off;
    if (so + 68 > data.size()) {
        port_log("CSS stage assets: %s — sprite offset 0x%zX out of range (file %zu bytes)\n",
                 se.name, so, data.size());
        return nullptr;
    }
    const uint8_t *sp = data.data() + so;
    const int w        = be_s16(sp + 0x04);
    const int h        = be_s16(sp + 0x06);
    const int nbitmaps = be_s16(sp + 0x28);
    const int bm_h     = be_s16(sp + 0x2C);
    const uint8_t fmt  = sp[0x30];
    const uint8_t siz  = sp[0x31];

    if (w != se.bg_w || h != se.bg_h || nbitmaps != se.bg_nbitmaps ||
        bm_h != se.bg_bm_h || fmt != kFmtRGBA16 || siz != kSiz16b) {
        port_log("CSS stage assets: %s — wallpaper sprite geometry mismatch "
                 "(%dx%d nbm=%d bmh=%d fmt=%d siz=%d); skipping derivation\n",
                 se.name, w, h, nbitmaps, bm_h, fmt, siz);
        return nullptr;
    }

    const uint32_t bm_array = relocPtrToFileOff(sp + 0x34);
    const size_t out_bytes = (size_t)w * h * 2u;
    uint8_t *out = static_cast<uint8_t *>(std::malloc(out_bytes));
    if (!out) {
        return nullptr;
    }
    size_t out_off = 0;

    std::vector<uint8_t> strip;
    for (int i = 0; i < nbitmaps; ++i) {
        const size_t bm_off = (size_t)bm_array + (size_t)i * 16;
        if (bm_off + 16 > data.size()) {
            std::free(out);
            return nullptr;
        }
        const uint8_t *bm = data.data() + bm_off;
        const int bm_w    = be_s16(bm + 0x00);
        const int bm_rows = be_s16(bm + 0x0C); // actualHeight (rendered + fringe)
        const uint32_t buf_off = relocPtrToFileOff(bm + 0x08);

        const size_t strip_bytes = (size_t)bm_w * (size_t)bm_rows * 2u;
        const int rendered = std::min(bm_h, bm_rows);
        const size_t rendered_bytes = (size_t)bm_w * (size_t)rendered * 2u;
        if (bm_w != w || bm_rows <= 0 ||
            (size_t)buf_off + strip_bytes > data.size() ||
            out_off + rendered_bytes > out_bytes) {
            port_log("CSS stage assets: %s — bitmap[%d] out of range/mismatch\n",
                     se.name, i);
            std::free(out);
            return nullptr;
        }

        strip.assign(data.data() + buf_off, data.data() + buf_off + strip_bytes);
        unswizzleRGBA16Strip(strip.data(), bm_w, bm_rows);

        std::memcpy(out + out_off, strip.data(), rendered_bytes);
        out_off += rendered_bytes;
    }

    if (out_off != out_bytes) {
        port_log("CSS stage assets: %s — assembled %zu bytes, expected %zu\n",
                 se.name, out_off, out_bytes);
        std::free(out);
        return nullptr;
    }
    return out;
}

// Expand an RGBA16 BE buffer to RGBA8888 (for the bilinear icon filter).
static std::vector<uint8_t> expandRGBA16BEToRGBA8888(const uint8_t *rgba16, size_t npixels) {
    std::vector<uint8_t> out;
    out.reserve(npixels * 4);
    for (size_t i = 0; i < npixels; ++i) {
        const uint16_t word =
            (uint16_t)((uint16_t)rgba16[i * 2] << 8 | rgba16[i * 2 + 1]);
        const uint8_t r5 = (uint8_t)((word >> 11) & 0x1F);
        const uint8_t g5 = (uint8_t)((word >> 6) & 0x1F);
        const uint8_t b5 = (uint8_t)((word >> 1) & 0x1F);
        out.push_back((uint8_t)((r5 << 3) | (r5 >> 2)));
        out.push_back((uint8_t)((g5 << 3) | (g5 >> 2)));
        out.push_back((uint8_t)((b5 << 3) | (b5 >> 2)));
        out.push_back((word & 1) ? 255 : 0);
    }
    return out;
}

// Center-crop to the icon aspect ratio, then bilinear-downscale to 48x36.
// Same crop/filter behavior as the Android runtime deriver
// (port/android_torch_bridge.cpp make_icon_bilinear).
static std::vector<uint8_t> downscaleIconBilinear(const std::vector<uint8_t> &src,
                                                  int src_w, int src_h) {
    std::vector<uint8_t> dst((size_t)kIconW * kIconH * 4);
    int crop_x = 0, crop_y = 0, crop_w = src_w, crop_h = src_h;
    const float src_aspect  = (float)src_w / (float)src_h;
    const float icon_aspect = (float)kIconW / (float)kIconH;

    if (src_aspect > icon_aspect) {
        crop_w = (int)((float)src_h * icon_aspect);
        crop_x = (src_w - crop_w) / 2;
    } else if (src_aspect < icon_aspect) {
        crop_h = (int)((float)src_w / icon_aspect);
        crop_y = (src_h - crop_h) / 2;
    }

    const float scale_x = (float)crop_w / (float)kIconW;
    const float scale_y = (float)crop_h / (float)kIconH;

    for (int y = 0; y < kIconH; ++y) {
        const float sy = (float)crop_y + ((float)y + 0.5f) * scale_y - 0.5f;
        const int y0 = std::max(0, std::min(src_h - 1, (int)sy));
        const int y1 = std::max(0, std::min(src_h - 1, y0 + 1));
        const float fy = std::max(0.0f, std::min(1.0f, sy - (float)y0));

        for (int x = 0; x < kIconW; ++x) {
            const float sx = (float)crop_x + ((float)x + 0.5f) * scale_x - 0.5f;
            const int x0 = std::max(0, std::min(src_w - 1, (int)sx));
            const int x1 = std::max(0, std::min(src_w - 1, x0 + 1));
            const float fx = std::max(0.0f, std::min(1.0f, sx - (float)x0));

            for (int c = 0; c < 4; ++c) {
                const float p00 = src[((size_t)y0 * src_w + x0) * 4 + c];
                const float p10 = src[((size_t)y0 * src_w + x1) * 4 + c];
                const float p01 = src[((size_t)y1 * src_w + x0) * 4 + c];
                const float p11 = src[((size_t)y1 * src_w + x1) * 4 + c];
                const float top = p00 * (1.0f - fx) + p10 * fx;
                const float bot = p01 * (1.0f - fx) + p11 * fx;
                const float v = top * (1.0f - fy) + bot * fy;
                dst[((size_t)y * kIconW + x) * 4 + c] =
                    (uint8_t)std::max(0.0f, std::min(255.0f, v + 0.5f));
            }
        }
    }
    return dst;
}

// Derive the background or icon pixel buffer (RGBA16 BE, malloc'd — same
// contract as loadPNGAsRGBA16BE) from the o2r wallpaper sprite.
static uint8_t *deriveFromArchive(const StageEntry &se, AssetKind asset_kind) {
    uint8_t *rgba16 = deriveWallpaperRGBA16(se);
    if (!rgba16 || asset_kind == kBackground) {
        return rgba16;
    }
    const std::vector<uint8_t> rgba8 =
        expandRGBA16BEToRGBA8888(rgba16, (size_t)se.bg_w * se.bg_h);
    std::free(rgba16);
    const std::vector<uint8_t> icon8 = downscaleIconBilinear(rgba8, se.bg_w, se.bg_h);
    return convertToRGBA16BE(icon8.data(), kIconW, kIconH);
}

// ---------------------------------------------------------------------------
// PNG override lookup.
//
// ssb64::LocateExistingFile probes the LUS app directory first (SDL pref
// path on NON_PORTABLE release builds, cwd on portable ones), then the real
// executable/bundle directory (where the CMake POST_BUILD step stages
// dev-build PNGs, and where a portable build launched from another cwd keeps
// its files). Returns "" when the PNG exists nowhere, which routes
// background/icon requests to the o2r derivation above.
// ---------------------------------------------------------------------------

static std::string locateCssAssetPNG(const std::string &rel_path) {
    return ssb64::LocateExistingFile(rel_path);
}

// ---------------------------------------------------------------------------
// Core getter implementation.
// ---------------------------------------------------------------------------

static Sprite *getSprite(int gkind, AssetKind asset_kind) {
    // Find the stage table entry.
    int stage_idx = -1;
    for (int i = 0; i < kStageCount; ++i) {
        if (STAGE_TABLE[i].gkind == gkind) {
            stage_idx = i;
            break;
        }
    }
    if (stage_idx < 0) return nullptr;  // gkind not in table

    std::lock_guard<std::mutex> guard(sCacheMutex);

    CacheEntry &entry = sCache[stage_idx][asset_kind];

    if (entry.sprite) {
        // Cache hit: re-register for scene-boundary safety.
        reregisterEntry(entry);
        return entry.sprite;
    }

    // Cache miss: load the PNG.
    const StageEntry &se = STAGE_TABLE[stage_idx];

    // For nameplate sprites: return NULL immediately if this stage has no PNG.
    if (asset_kind == kName && !se.has_name_png) {
        return nullptr;
    }

    // For emblem sprites: return NULL immediately if this stage has no PNG.
    if (asset_kind == kEmblem && !se.has_emblem_png) {
        return nullptr;
    }

    const char *suffix;
    int expected_w, expected_h;
    if (asset_kind == kBackground) {
        suffix     = "_background.png";
        expected_w = se.bg_w;
        expected_h = se.bg_h;
    } else if (asset_kind == kName) {
        suffix     = "_name.png";
        expected_w = kNameW;
        expected_h = kNameH;
    } else if (asset_kind == kEmblem) {
        suffix     = "_emblem.png";
        expected_w = kEmblemW;
        expected_h = kEmblemH;
    } else {
        suffix     = "_small.png";
        expected_w = kIconW;
        expected_h = kIconH;
    }

    std::string rel_path = std::string("assets/css_icons/") + se.name + suffix;
    std::string full_path = locateCssAssetPNG(rel_path);

    // Emblems use the ROM's IA4 single-bitmap format (alpha-mask source); all
    // other CSS sprites use RGBA16. Pick the converter accordingly.
    uint8_t *pixels = nullptr;
    if (!full_path.empty()) {
        // A PNG override exists on disk: it is authoritative. If it fails to
        // load (corrupt, wrong size — already logged above), return NULL so
        // the question mark surfaces the broken override instead of silently
        // substituting the ROM-derived image under a modder's art.
        pixels = (asset_kind == kEmblem)
                     ? loadPNGAsIA4(full_path, expected_w, expected_h)
                     : loadPNGAsRGBA16BE(full_path, expected_w, expected_h);
    } else if ((asset_kind == kBackground || asset_kind == kIcon) &&
               !entry.derive_failed) {
        // No PNG anywhere (the normal case on release builds, which cannot
        // ship ROM-derived files) — derive from the o2r wallpaper sprite.
        pixels = deriveFromArchive(se, asset_kind);
        if (pixels) {
            full_path = std::string(gRelocFileTable[se.wp_file_id]) + " (o2r-derived)";
        } else {
            entry.derive_failed = true;
        }
    }
    if (!pixels) {
        // Nothing to load or derive — return NULL so caller falls back to
        // the ROM sprite / question mark / subtitle-font text.
        return nullptr;
    }

    Sprite *sp = nullptr;
    if (asset_kind == kBackground) {
        sp = buildBackgroundSprite(se, pixels, &entry);
    } else if (asset_kind == kName) {
        sp = buildNameSprite(pixels, &entry);
    } else if (asset_kind == kEmblem) {
        sp = buildEmblemSprite(pixels, &entry);
    } else {
        sp = buildIconSprite(pixels, &entry);
    }

    if (!sp) {
        std::free(pixels);
        port_log("CSS stage assets: sprite build failed for %s\n", full_path.c_str());
        return nullptr;
    }

    port_log("CSS stage assets: loaded %s (%dx%d, %d bitmaps)\n",
             full_path.c_str(), expected_w, expected_h, entry.nbitmaps);
    return sp;
}

} // namespace

// ---------------------------------------------------------------------------
// Public C entry points.
// ---------------------------------------------------------------------------

extern "C" Sprite *portCSSGetStageBackgroundSprite(int gkind) {
    return getSprite(gkind, kBackground);
}

extern "C" Sprite *portCSSGetStageIconSprite(int gkind) {
    return getSprite(gkind, kIcon);
}

extern "C" Sprite *portCSSGetStageNameSprite(int gkind) {
    return getSprite(gkind, kName);
}

extern "C" Sprite *portCSSGetStageEmblemSprite(int gkind) {
    return getSprite(gkind, kEmblem);
}

#endif /* PORT */

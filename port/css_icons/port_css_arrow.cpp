/**
 * port_css_arrow.cpp — CSS scroll-arrow sprite for Final Destination (port-only).
 *
 * The RGBA16 pixel bytes are baked into the binary at compile time via the
 * generated header arrow_data.h (produced by tools/generate_arrow_svg.py +
 * tools/png_to_c_array.py at CMake build time).
 *
 * The sprite is a right-pointing orange chevron ">" (6x44 px, apex ~149 deg).
 * Mirror it horizontally (scalex = -1.0f on the SObj) for the left variant.
 *
 * Iteration: edit tools/generate_arrow_svg.py or arrow.png, then rebuild.
 * See port_css_arrow.h for full geometry notes.
 */

#ifdef PORT

#include "port_css_arrow.h"

// Baked RGBA16 pixel data (generated at build time from arrow.png).
// Defines: kArrowRGBA16[], kArrowWidth, kArrowHeight
#include "arrow_data.h"
// Mirrored variant (generate_arrow_svg.py --mirror).
// Defines: kArrowLeftRGBA16[], kArrowLeftWidth, kArrowLeftHeight
#include "arrow_left_data.h"

#include "../bridge/lbreloc_byteswap.h"
#include "../resource/RelocPointerTable.h"
#include "../port_log.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>

struct sprite;
struct bitmap;
typedef struct sprite Sprite;
typedef struct bitmap Bitmap;

// libultraship fast/interpreter.cpp. The interpreter's SETTIMG/G_VTX low-VA
// guards drop addresses <= 0x0FFFFFFF that aren't inside a loaded module —
// and deliberately exclude the main image, because a non-PIE build's own VA
// range numerically overlaps stale N64-segment leftovers. Our baked arrow
// arrays live in exactly that main-image .rodata range on non-PIE builds
// (PIE/CI builds load high and never trip the guard), so each array must be
// registered as trusted or its SETTIMG is silently dropped and the arrow
// renders whatever texture was previously bound (garbled strip on the CSS).
// See docs/bugs/css_arrow_low_va_guard_2026-09-04.md.
extern "C" void gfxRegisterTrustedLowVARange(const void *base, size_t size);

namespace {

/* Arrow sprite constants — same format for both right and left variants. */
constexpr uint8_t  kArrowFmt      = 0;   /* G_IM_FMT_RGBA */
constexpr uint8_t  kArrowSiz      = 2;   /* G_IM_SIZ_16b  */
constexpr int      kArrowNBitmaps = 1;

static_assert(kArrowWidth     == kArrowLeftWidth,
              "arrow / arrow_left width mismatch — regenerate both PNGs at the same W/H");
static_assert(kArrowHeight    == kArrowLeftHeight,
              "arrow / arrow_left height mismatch — regenerate both PNGs at the same W/H");
/* RGBA16 row stride must be a multiple of 8 bytes (TMEM line size) or Fast3D
 * shears the decode. generate_arrow_svg.py pads the canvas width up to the
 * next multiple of 4 px to satisfy this; assert it stayed honest. */
static_assert((kArrowWidth * 2) % 8 == 0,
              "arrow PNG width must be a multiple of 4 px (RGBA16 row stride alignment)");

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

/* One cache slot per variant: [0] = right, [1] = left.  Same Sprite layout. */
enum ArrowVariant { kArrowRight = 0, kArrowLeft = 1, kArrowVariantCount = 2 };

static Sprite *sCachedSprite[kArrowVariantCount] = { nullptr, nullptr };
static Bitmap *sCachedBitmap[kArrowVariantCount] = { nullptr, nullptr };
static const uint8_t *sCachedPixels[kArrowVariantCount] = { nullptr, nullptr };
static std::mutex sCacheMutex;

static Sprite *buildSpriteFor(int variant, const uint8_t *pixels)
{
    uint8_t *sp_raw = static_cast<uint8_t *>(std::malloc(68));
    uint8_t *bm_raw = static_cast<uint8_t *>(std::malloc(16));
    if (!sp_raw || !bm_raw) { std::free(sp_raw); std::free(bm_raw); return nullptr; }
    std::memset(sp_raw, 0, 68);
    std::memset(bm_raw, 0, 16);

    /* Bitmap */
    put_s16(bm_raw, 0x00, (int16_t)kArrowWidth);
    put_s16(bm_raw, 0x02, (int16_t)kArrowWidth);
    put_s16(bm_raw, 0x04, 0);
    put_s16(bm_raw, 0x06, 0);
    put_u32(bm_raw, 0x08, portRelocRegisterPointer(const_cast<void*>(static_cast<const void*>(pixels))));
    put_s16(bm_raw, 0x0C, (int16_t)kArrowHeight);
    put_s16(bm_raw, 0x0E, 0);

    Bitmap *bm = reinterpret_cast<Bitmap *>(bm_raw);

    /* Sprite */
    put_s16(sp_raw, 0x00, 0);
    put_s16(sp_raw, 0x02, 0);
    put_s16(sp_raw, 0x04, (int16_t)kArrowWidth);
    put_s16(sp_raw, 0x06, (int16_t)kArrowHeight);
    put_f32(sp_raw, 0x08, 1.0f);
    put_f32(sp_raw, 0x0C, 1.0f);
    put_s16(sp_raw, 0x10, 0); put_s16(sp_raw, 0x12, 0);
    put_u16(sp_raw, 0x14, 0); put_s16(sp_raw, 0x16, 0);
    sp_raw[0x18] = 0xFF; sp_raw[0x19] = 0xFF;
    sp_raw[0x1A] = 0xFF; sp_raw[0x1B] = 0xFF;
    put_s16(sp_raw, 0x1C, 0); put_s16(sp_raw, 0x1E, 0);
    put_u32(sp_raw, 0x20, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x24, 0); put_s16(sp_raw, 0x26, 1);
    put_s16(sp_raw, 0x28, (int16_t)kArrowNBitmaps);
    put_s16(sp_raw, 0x2A, (int16_t)kArrowHeight);    /* ndisplist = height */
    put_s16(sp_raw, 0x2C, (int16_t)kArrowHeight);
    put_s16(sp_raw, 0x2E, (int16_t)kArrowHeight);
    sp_raw[0x30] = kArrowFmt; sp_raw[0x31] = kArrowSiz;
    sp_raw[0x32] = 0;         sp_raw[0x33] = 0;
    put_u32(sp_raw, 0x34, portRelocRegisterPointer(bm));
    put_u32(sp_raw, 0x38, portRelocRegisterPointer(nullptr));
    put_u32(sp_raw, 0x3C, portRelocRegisterPointer(nullptr));
    put_s16(sp_raw, 0x40, 0); put_s16(sp_raw, 0x42, 0);

    Sprite *sp = reinterpret_cast<Sprite *>(sp_raw);

    void *buf_ptrs[1] = { const_cast<void*>(static_cast<const void*>(pixels)) };
    portMarkSyntheticSprite(sp, bm, (unsigned int)kArrowNBitmaps, buf_ptrs);

    sCachedSprite[variant] = sp;
    sCachedBitmap[variant] = bm;
    sCachedPixels[variant] = pixels;
    return sp;
}

static Sprite *getOrBuild(int variant, const uint8_t *pixels, const char *label)
{
    std::lock_guard<std::mutex> guard(sCacheMutex);
    // Idempotent; must cover every draw, not just the first build, in case the
    // interpreter side ever gains a reset. Cheap (two tiny ranges).
    gfxRegisterTrustedLowVARange(kArrowRGBA16, sizeof(kArrowRGBA16));
    gfxRegisterTrustedLowVARange(kArrowLeftRGBA16, sizeof(kArrowLeftRGBA16));
    if (sCachedSprite[variant]) {
        /* Re-register on every entry — scene-boundary cleanups clear the
         * fixup tracking sets. Insertion is idempotent. */
        void *buf_ptrs[1] = { const_cast<void*>(static_cast<const void*>(sCachedPixels[variant])) };
        portMarkSyntheticSprite(sCachedSprite[variant], sCachedBitmap[variant],
                                (unsigned int)kArrowNBitmaps, buf_ptrs);
        return sCachedSprite[variant];
    }
    Sprite *sp = buildSpriteFor(variant, pixels);
    if (!sp) {
        port_log("CSS arrow (%s): buildSprite allocation failed\n", label);
        return nullptr;
    }
    port_log("CSS arrow (%s): built %dx%d RGBA16 from baked data\n",
             label, kArrowWidth, kArrowHeight);
    return sp;
}

} // namespace

extern "C" Sprite *portCSSGetScrollArrowSprite(void)
{
    return getOrBuild(kArrowRight, kArrowRGBA16, "right");
}

extern "C" Sprite *portCSSGetScrollArrowLeftSprite(void)
{
    return getOrBuild(kArrowLeft, kArrowLeftRGBA16, "left");
}

#endif /* PORT */

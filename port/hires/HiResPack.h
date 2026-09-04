#pragma once

/* Hi-res texture pack registry.
 *
 * Scans <app-data>/mods/ recursively at startup for PNG files whose names
 * follow the grammar:
 *
 *   <anything>#<rgba8CRC32>#<fmt>#<siz>[_<anything>].png
 *
 * A pack may be either a folder of loose PNGs OR a .zip dropped straight into
 * mods/ — the distributed pack format. Zips are read in place (libzip +
 * stbi_load_from_memory at decode time), so there is no extraction step on
 * any platform: desktop users drop the zip into mods/, and the Android
 * importer copies the downloaded zip there. Loose PNGs and zip members are
 * indexed identically; only the decode source differs.
 *
 * The hash key is a CRC32-IEEE over the *decoded* RGBA8 image — the same
 * tightly-packed pixel buffer the GPU receives at upload time. This makes
 * the key independent of N64 source byte layout (Sprite/Bitmap/raw/CI all
 * decode to the same RGBA8 and hash the same), so the runtime hook can
 * sit downstream of every format-specific decoder and the offline pack-
 * conversion tool can derive the same key by decoding each pack PNG.
 *
 * Each parsed entry is stored in an in-memory map keyed by
 * (rgba8CRC, fmt, siz). PNG decoding is deferred to first lookup; Init()
 * only builds the index.
 *
 * Packs work additively — drop multiple subfolders and/or zips under mods/
 * and the union of their PNGs is indexed. If two PNGs hash-collide the later
 * scan wins (alphabetical order).
 */

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace ssb64::hires {

// ── Platform-scaled tuning (shared by HiResPack.cpp / HiResHook.cpp /
//    PortMenu.cpp so the runtime default and the menu widget default agree) ──

// Master-enable CVar (gHiResTextures.Enabled) default. Hi-res substitution
// is on by default on desktop but OPT-IN on Android: a pack's decoded-RGBA8
// working set (see kDefaultLruBudgetMB) plus its uncompressed GPU uploads can
// push a phone past the OS low-memory-killer threshold, so a mobile user must
// enable it deliberately rather than have a dropped-in pack allocate silently.
#if defined(__ANDROID__)
inline constexpr int kHiResEnabledDefault = 0;
#else
inline constexpr int kHiResEnabledDefault = 1;
#endif

// Decoded-RGBA8 LRU budget default, in MB (overridable at runtime via the
// gHiResTextures.CacheBudgetMB CVar, read in HiResPack::Init). Desktop can
// afford a large cache; Android runs under a far tighter per-app footprint
// (the LMK reaps the foreground app well below desktop RAM limits), so it
// defaults much lower. Floored at kMinLruBudgetMB so a too-small value can't
// make the cache thrash (re-decoding on every miss stalls the render thread).
#if defined(__ANDROID__)
inline constexpr int kDefaultLruBudgetMB = 128;
#else
inline constexpr int kDefaultLruBudgetMB = 512;
#endif
inline constexpr int kMinLruBudgetMB = 16;

// Per-texture upscale ceiling, in decoded pixels. A pack PNG decoding to more
// than this many texels is rejected (the native texture renders instead) so a
// single pathological upscale can't blow the budget / GPU upload in one shot —
// the LRU never evicts its just-inserted tail, so a lone over-budget entry
// would otherwise exceed the cap outright. 0 = uncapped (desktop).
#if defined(__ANDROID__)
inline constexpr uint32_t kMaxPackTexels = 2048u * 2048u; // 16 MB RGBA8
#else
inline constexpr uint32_t kMaxPackTexels = 0u;
#endif

// Preload (gHiResTextures.Preload CVar) default: background-decode the pack
// into the LRU at boot so first use of a texture doesn't stall the game/render
// thread on a synchronous PNG decode (issue #215). Off by default on Android —
// preloading front-loads the pack's whole decoded working set, which is
// exactly the allocation pattern the opt-in kHiResEnabledDefault guards
// against on phones.
#if defined(__ANDROID__)
inline constexpr int kHiResPreloadDefault = 0;
#else
inline constexpr int kHiResPreloadDefault = 1;
#endif

struct DecodedTexture {
    std::vector<uint8_t> rgba; // tightly packed RGBA8, w * h * 4 bytes
    uint16_t w = 0;
    uint16_t h = 0;
};

struct HashKey {
    uint32_t rgba8Crc;
    uint8_t fmt;
    uint8_t siz;
    uint8_t pad0 = 0, pad1 = 0;

    bool operator==(const HashKey&) const noexcept = default;
};

struct PackStats {
    size_t scannedFiles;     // every file under mods/, .png or not
    size_t indexedTextures;  // PNGs whose names parsed successfully
    size_t skippedFilenames; // PNGs that did not match the grammar
    size_t collisions;       // duplicate hash keys (later scan wins)
};

struct LookupStats {
    uint64_t lookups;     // every Lookup() call (one per cache miss)
    uint64_t hits;        // resolved to a decoded RGBA8 buffer
    uint64_t decodeFails; // index hit but stbi_load failed (entry now removed)
};

class HiResPack {
public:
    static HiResPack& Get();

    /* Resolves <app-data>/mods/, walks every file recursively, parses each
     * .png filename, and builds the in-memory index. Safe to call before
     * Window/ResourceManager are up — only depends on Ship::Context being
     * alive. Returns true if at least one mods/ subdirectory existed; the
     * pack itself is allowed to be empty. */
    bool Init();

    /* Diagnostic snapshot of the last Init() pass. */
    const PackStats& Stats() const noexcept { return mStats; }

    /* Resolved absolute path to the scanned mods/ root, for logging /
     * "Open mods folder" UI button. Empty before Init(). */
    const char* ModsRoot() const noexcept;

    /* Start background preload workers that decode the indexed pack PNGs
     * into the LRU until the cache budget fills (issue #215) — first use of
     * a preloaded texture then skips the synchronous disk read + PNG decode
     * that otherwise stalls the game/render thread. Call once after Init();
     * no-op if the gHiResTextures.Preload or .Enabled CVars are off or the
     * index is empty. Threads stop on budget exhaustion, work exhaustion,
     * or StopPreload(). */
    void StartPreload();

    /* Signal preload workers to stop and join them. Safe to call at any
     * time (including with no preload running); registered via atexit so
     * workers never outlive the module's statics. */
    void StopPreload();

    /* Hash the decoded RGBA8 image (CRC32-IEEE over `width*height*4` bytes)
     * and return a substitute decoded RGBA8 buffer if the pack contains a
     * matching PNG. Decode of the pack PNG is lazy + memoized (or already
     * warm from StartPreload's background workers); a small LRU caps total
     * decoded RAM at kDefaultLruBudgetMB (512 MB desktop / 128 MB Android,
     * overridable via the gHiResTextures.CacheBudgetMB CVar). The returned
     * pointer stays valid through the calling frame's UploadTexture: Lookup
     * holds a reference to the returned texture until the next Lookup call,
     * so a concurrent preload insert evicting the entry cannot free it out
     * from under the caller. Returns nullptr on miss / decode failure.
     *
     * Format-independent and byte-layout-independent: the input buffer is
     * the exact tightly-packed RGBA8 image the GPU is about to receive, so
     * Sprite-fixup / Bitmap-fixup / raw / CI-with-palette paths all collapse
     * to the same key for the same visible texture content.
     */
    const DecodedTexture* Lookup(uint8_t fmt, uint8_t siz,
                                 const uint8_t* rgba8,
                                 uint16_t width, uint16_t height);

    /* Live counters since process start (or last ResetLookupStats). */
    LookupStats LookupStatsSnapshot() const noexcept { return mLookupStats; }
    void ResetLookupStats() noexcept { mLookupStats = {}; }

    /* When enabled (gHiResTextures.DumpSource CVar), dumps the source N64
     * byte run + dimensions + palette to a .bin file under
     * <app-data>/hires_dump/. Used by the offline GPL pack conversion tool
     * (see tools/hires-pack-convert/convert.py).
     *
     * NOTE: dormant since the post-decode hook refactor — Lookup() no
     * longer has source bytes. Kept available for a future re-wire into a
     * pre-decode dump hook on the libultraship side. */
    void MaybeDumpSource(uint8_t fmt, uint8_t siz,
                         const uint8_t* texels, uint16_t width, uint16_t height, uint32_t bpl,
                         const uint8_t* palette, uint32_t paletteBytes);

private:
    HiResPack() = default;
    HiResPack(const HiResPack&) = delete;
    HiResPack& operator=(const HiResPack&) = delete;

    PackStats mStats{};
    LookupStats mLookupStats{};

    /* Pins the texture returned by the most recent Lookup() so LRU eviction
     * (from a background preload insert) can't free it before the caller's
     * UploadTexture runs. Only touched under the module lock; only the
     * game/render thread calls Lookup. */
    std::shared_ptr<const DecodedTexture> mLastReturned;
};

} // namespace ssb64::hires

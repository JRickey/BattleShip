/**
 * particle_bank_bridge.cpp — Particle script+texture bank loader for PORT
 *
 * Replaces the empty-bank stub at src/ef/efparticle.c ifdef PORT. Extracts
 * the bank BLOBs out of the O2R archive (via LUS ResourceManager), byte-swaps
 * them from N64 big-endian to native little-endian, and hands them to
 * lbParticleSetupBankID so the decomp bytecode interpreter can run.
 *
 * Six named banks are supported: EFCommon, ITManager, GRPupupu, GRHyrule,
 * GRYoster, MNTitle. Each caller passes (uintptr_t)&l<Module>ParticleScriptBankLo
 * as the scripts_lo key; we map that address to the matching O2R resource
 * pair by comparing against the addresses of the externs below.
 *
 * Byte-swap scope (first-pass, see Phase 3 in CLAUDE notes for bytecode):
 *   - Script bank:   scripts_num, offset table, each LBScript struct header
 *                    (non-bytecode fields through 0x30).
 *   - Texture bank:  textures_num, offset table, each LBTexture header
 *                    including its inline data[] offset list. Texture pixel
 *                    and palette data are left untouched (handled later by
 *                    the TMEM/sprite decoder pipeline).
 */

#include <ship/Context.h>
#include <ship/resource/Resource.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/type/Blob.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include "bridge/particle_bank_bridge.h"
extern void port_log(const char *fmt, ...);

/* Stub symbols from port/stubs/segment_symbols.c. Their addresses are the
 * per-bank keys passed into efParticleGetLoadBankID. Types (u/i ptr_t) match
 * the stub declarations exactly — getting this wrong would change the
 * external linkage name via C++ name mangling only if we were using C++
 * linkage, but under extern "C" they all resolve to the same symbol name
 * regardless of signedness. Keep them matched for clarity. */
extern uintptr_t lEFCommonParticleScriptBankLo;
extern intptr_t  lITManagerParticleScriptBankLo;
extern intptr_t  lGRPupupuParticleScriptBankLo;
extern intptr_t  lGRHyruleParticleScriptBankLo;
extern intptr_t  lGRYosterParticleScriptBankLo;
extern uintptr_t lMNTitleParticleScriptBankLo;

/* Fighter-data-side banks: ftdata.c passes (uintptr_t)&particles_unk{0,1,2}_scb_ROM_START
 * from FTData structs, distinct from the stub-address convention used by the six
 * named banks above. Declared as s32 in port/stubs/segment_symbols.c. */
extern int32_t particles_unk0_scb_ROM_START;
extern int32_t particles_unk1_scb_ROM_START;
extern int32_t particles_unk2_scb_ROM_START;

/* lbParticleSetupBankID is declared in src/lb/lbparticle.h with LBScriptDesc*
 * and LBTextureDesc* types. We don't want to pull in the decomp headers on
 * the C++ side (they shadow system headers), so re-declare with opaque void*
 * — the function reads the first field (scripts_num / textures_num) and
 * an offset array that follows it, which we lay out correctly via byteswap. */
extern void lbParticleSetupBankID(int32_t bank_id, void *script_desc, void *texture_desc);
}

namespace {

/* ========================================================================= */
/*  Per-bank configuration table                                             */
/* ========================================================================= */

/* We can't make this a true constexpr because the stub-symbol addresses
 * aren't constant expressions. Static init fills it lazily on first use. */
struct BankRow {
    uintptr_t scripts_lo_stub_addr;   /* == (uintptr_t)&l<Module>ParticleScriptBankLo */
    const char *script_path;          /* archive path to pass to LoadResource */
    const char *texture_path;
    const char *debug_name;
};

static BankRow *bankTable() {
    static BankRow table[] = {
        { (uintptr_t)&lEFCommonParticleScriptBankLo,
          "particles/efcommon_particle_scb",
          "particles/efcommon_particle_txb",
          "EFCommon" },
        { (uintptr_t)&lITManagerParticleScriptBankLo,
          "particles/itmanager_particle_scb",
          "particles/itmanager_particle_txb",
          "ITManager" },
        { (uintptr_t)&lGRPupupuParticleScriptBankLo,
          "particles/grpupupu_particle_scb",
          "particles/grpupupu_particle_txb",
          "GRPupupu" },
        { (uintptr_t)&lGRHyruleParticleScriptBankLo,
          "particles/grhyrule_particle_scb",
          "particles/grhyrule_particle_txb",
          "GRHyrule" },
        { (uintptr_t)&lGRYosterParticleScriptBankLo,
          "particles/gryoster_particle_scb",
          "particles/gryoster_particle_txb",
          "GRYoster" },
        { (uintptr_t)&lMNTitleParticleScriptBankLo,
          "particles/mntitle_particle_scb",
          "particles/mntitle_particle_txb",
          "MNTitle" },
        { (uintptr_t)&particles_unk0_scb_ROM_START,
          "particles/particles_unk0_scb",
          "particles/particles_unk0_txb",
          "unk0" },
        { (uintptr_t)&particles_unk1_scb_ROM_START,
          "particles/particles_unk1_scb",
          "particles/particles_unk1_txb",
          "unk1" },
        { (uintptr_t)&particles_unk2_scb_ROM_START,
          "particles/particles_unk2_scb",
          "particles/particles_unk2_txb",
          "unk2" },
    };
    return table;
}
static constexpr size_t kBankTableCount = 9;

/* ========================================================================= */
/*  Blob storage                                                             */
/* ========================================================================= */

/* Two-tier storage:
 *
 *   sPristineCache — keyed by O2R archive path. Holds the post-byte-swap,
 *     PRE-pointerization data. Loaded once per path, reused for every scene
 *     that needs the bank. Memory cost: ~400 KB for EFCommon (the largest);
 *     all nine banks together are under 600 KB.
 *
 *   sWorkingEntries — keyed by scripts_lo stub address. Holds the per-scene
 *     pointerized working copy that lbParticleSetupBankID wrote tokens into.
 *     When the same scripts_lo is re-requested (next scene re-entering the
 *     same stage/menu/battle), we evict the prior entry; its tokens were
 *     already invalidated by portRelocResetPointerTable() inside the scene
 *     transition's lbRelocInitSetup() call, so the old buffer is dead anyway.
 */

static std::unordered_map<std::string, std::vector<uint8_t>> &pristineCache() {
    static std::unordered_map<std::string, std::vector<uint8_t>> m;
    return m;
}

struct WorkingEntry {
    uintptr_t scripts_lo;
    std::vector<uint8_t> script_data;
    std::vector<uint8_t> texture_data;
};

static std::vector<std::unique_ptr<WorkingEntry>> sWorkingEntries;

/* ========================================================================= */
/*  Byte-swap helpers                                                        */
/* ========================================================================= */

static inline void bswap16At(uint8_t *p, size_t off) {
    uint8_t a = p[off], b = p[off + 1];
    p[off]     = b;
    p[off + 1] = a;
}

static inline void bswap32At(uint8_t *p, size_t off) {
    uint8_t a = p[off], b = p[off + 1], c = p[off + 2], d = p[off + 3];
    p[off]     = d;
    p[off + 1] = c;
    p[off + 2] = b;
    p[off + 3] = a;
}

static inline uint32_t readNativeU32(const uint8_t *p, size_t off) {
    uint32_t v;
    std::memcpy(&v, p + off, 4);
    return v;
}

/* Script bank layout on ROM (IDO BE):
 *   +0x00  s32    scripts_num
 *   +0x04  u32[]  offset table (scripts_num entries, each an offset from blob base)
 *   +...   raw    LBScript records at each offset:
 *          +0x00  u16 kind
 *          +0x02  u16 texture_id
 *          +0x04  u16 generator_lifetime
 *          +0x06  u16 particle_lifetime
 *          +0x08  u32 flags
 *          +0x0C  f32 gravity
 *          +0x10  f32 friction
 *          +0x14  Vec3f vel (3 × f32)
 *          +0x20  f32 unk_0x20
 *          +0x24  f32 unk_0x24
 *          +0x28  f32 update_rate
 *          +0x2C  f32 size
 *          +0x30  u8[] bytecode (variable length; NOT swapped here)
 */
static void byteSwapScriptBank(uint8_t *data, size_t size, const char *dbg) {
    if (size < 4) {
        port_log("particle_bank[%s]: script blob too small (%zu bytes)\n", dbg, size);
        return;
    }
    bswap32At(data, 0);
    int32_t scripts_num = static_cast<int32_t>(readNativeU32(data, 0));
    if (scripts_num <= 0 || scripts_num > 1024) {
        port_log("particle_bank[%s]: absurd scripts_num=%d\n", dbg, scripts_num);
        return;
    }
    const size_t table_end = 4 + static_cast<size_t>(scripts_num) * 4;
    if (table_end > size) {
        port_log("particle_bank[%s]: script offset table (%zu) overflows blob (%zu)\n",
                 dbg, table_end, size);
        return;
    }
    for (int32_t i = 0; i < scripts_num; i++) {
        bswap32At(data, 4 + i * 4);
    }
    for (int32_t i = 0; i < scripts_num; i++) {
        uint32_t script_off = readNativeU32(data, 4 + i * 4);
        if (script_off + 0x30 > size) {
            port_log("particle_bank[%s]: script %d header at 0x%X runs past end\n",
                     dbg, i, script_off);
            continue;
        }
        /* 4 u16 fields at 0x00..0x06 */
        bswap16At(data, script_off + 0x00);
        bswap16At(data, script_off + 0x02);
        bswap16At(data, script_off + 0x04);
        bswap16At(data, script_off + 0x06);
        /* 10 u32/f32 fields at 0x08..0x2C */
        for (size_t k = 0; k < 10; k++) {
            bswap32At(data, script_off + 0x08 + k * 4);
        }
        /* Bytecode at 0x30 onward not touched in Phase 1. */
    }
}

/* Texture bank layout on ROM (IDO BE):
 *   +0x00  s32    textures_num
 *   +0x04  u32[]  offset table (textures_num entries)
 *   +...   raw    LBTexture records:
 *          +0x00  u32 count
 *          +0x04  s32 fmt        (G_IM_FMT_*)
 *          +0x08  s32 siz        (G_IM_SIZ_*)
 *          +0x0C  s32 width
 *          +0x10  s32 height
 *          +0x14  u32 flags      (bit 0: shared palette when fmt == CI)
 *          +0x18  u32 data[]     (count entries, then 1 or count palettes for CI)
 *   Pixel data and palette data (outside the LBTexture struct) are raw and
 *   not swapped here — the TMEM/sprite pipeline handles their byte order.
 */
static void byteSwapTextureBank(uint8_t *data, size_t size, const char *dbg) {
    static constexpr int32_t kImFmtCi = 2;  /* G_IM_FMT_CI */
    if (size < 4) {
        port_log("particle_bank[%s]: texture blob too small (%zu bytes)\n", dbg, size);
        return;
    }
    bswap32At(data, 0);
    int32_t textures_num = static_cast<int32_t>(readNativeU32(data, 0));
    if (textures_num <= 0 || textures_num > 1024) {
        port_log("particle_bank[%s]: absurd textures_num=%d\n", dbg, textures_num);
        return;
    }
    const size_t table_end = 4 + static_cast<size_t>(textures_num) * 4;
    if (table_end > size) {
        port_log("particle_bank[%s]: texture offset table (%zu) overflows blob (%zu)\n",
                 dbg, table_end, size);
        return;
    }
    for (int32_t i = 0; i < textures_num; i++) {
        bswap32At(data, 4 + i * 4);
    }
    for (int32_t i = 0; i < textures_num; i++) {
        uint32_t tex_off = readNativeU32(data, 4 + i * 4);
        if (tex_off + 0x18 > size) {
            port_log("particle_bank[%s]: texture %d header at 0x%X runs past end\n",
                     dbg, i, tex_off);
            continue;
        }
        /* 6 u32/s32 header fields */
        for (size_t k = 0; k < 6; k++) {
            bswap32At(data, tex_off + k * 4);
        }
        uint32_t count = readNativeU32(data, tex_off + 0x00);
        int32_t  fmt   = static_cast<int32_t>(readNativeU32(data, tex_off + 0x04));
        uint32_t flags = readNativeU32(data, tex_off + 0x14);
        if (count > 4096) {
            port_log("particle_bank[%s]: texture %d absurd count=%u\n", dbg, i, count);
            continue;
        }
        uint32_t total_data_entries = count;
        if (fmt == kImFmtCi) {
            total_data_entries += (flags & 1) ? 1u : count;
        }
        const size_t data_end = tex_off + 0x18 + static_cast<size_t>(total_data_entries) * 4;
        if (data_end > size) {
            port_log("particle_bank[%s]: texture %d data[] (%zu) overflows blob (%zu)\n",
                     dbg, i, data_end, size);
            continue;
        }
        for (uint32_t k = 0; k < total_data_entries; k++) {
            bswap32At(data, tex_off + 0x18 + k * 4);
        }
    }
}

/* ========================================================================= */
/*  Pristine cache: load + byte-swap once per archive path                   */
/* ========================================================================= */

/* Ensure a pristine (byte-swapped, pre-pointerization) copy of the blob
 * at `archive_path` lives in sPristineCache. Returns a pointer to the
 * cached vector on success, nullptr on failure. */
static const std::vector<uint8_t> *ensurePristine(const char *archive_path,
                                                  bool is_script_bank,
                                                  const char *debug_name) {
    auto &cache = pristineCache();
    auto it = cache.find(archive_path);
    if (it != cache.end()) {
        return &it->second;
    }

    auto ctx = Ship::Context::GetInstance();
    if (!ctx) {
        port_log("particle_bank[%s]: no Ship::Context for '%s'\n", debug_name, archive_path);
        return nullptr;
    }
    std::string lookup = std::string("__OTR__") + archive_path;
    auto res = ctx->GetResourceManager()->LoadResource(lookup);
    if (!res) {
        port_log("particle_bank[%s]: LoadResource('%s') returned null\n", debug_name, lookup.c_str());
        return nullptr;
    }
    auto blob = std::dynamic_pointer_cast<Ship::Blob>(res);
    if (!blob) {
        port_log("particle_bank[%s]: '%s' is not a Blob resource\n", debug_name, lookup.c_str());
        return nullptr;
    }

    std::vector<uint8_t> data(blob->Data.begin(), blob->Data.end());
    if (is_script_bank) {
        byteSwapScriptBank(data.data(), data.size(), debug_name);
    } else {
        byteSwapTextureBank(data.data(), data.size(), debug_name);
    }
    auto [ins_it, inserted] = cache.emplace(std::string(archive_path), std::move(data));
    return &ins_it->second;
}

} // namespace

/* ========================================================================= */
/*  Public entry point                                                       */
/* ========================================================================= */

extern "C" int portParticleLoadBank(uintptr_t scripts_lo, int bank_id) {
    const BankRow *match = nullptr;
    for (size_t i = 0; i < kBankTableCount; i++) {
        if (bankTable()[i].scripts_lo_stub_addr == scripts_lo) {
            match = &bankTable()[i];
            break;
        }
    }
    if (!match) {
        port_log("particle_bank: no table entry for scripts_lo=0x%llX (bank_id=%d)\n",
                 (unsigned long long)scripts_lo, bank_id);
        return -1;
    }

    const std::vector<uint8_t> *pristine_script  = ensurePristine(match->script_path,  true,  match->debug_name);
    if (!pristine_script)  return -2;
    const std::vector<uint8_t> *pristine_texture = ensurePristine(match->texture_path, false, match->debug_name);
    if (!pristine_texture) return -3;

    /* Evict any prior working entry for this scripts_lo. Its pointerization
     * was invalidated when lbRelocInitSetup() reset the token table between
     * scenes, so anyone still holding a raw pointer into the old buffer is
     * already broken — dropping the backing memory just matches.
     *
     * Before the backing vectors are freed, purge every port-side cache
     * keyed by raw addresses inside them — the same lockstep-eviction
     * contract lbRelocLoadAndRelocFile honors for reloc loads. glibc
     * routinely hands a freed chunk straight back to the next same-sized
     * allocation, so without this a Fast3D TextureCacheKey whose
     * texture_addr pointed into the old bank survives, and a later bank
     * texture that lands on the aliased (addr, shape) key renders the
     * previous bank's texels: intermittent corrupted particle textures
     * whose occurrence depends on heap layout, i.e. varies per launch. */
    for (const auto &e : sWorkingEntries) {
        if (!e || e->scripts_lo != scripts_lo)
            continue;
        extern void portTextureCacheDeleteRange(const void *base, size_t size);
        extern void portPackedDisplayListCacheDeleteRange(const void *base, size_t size);
        extern void portEvictStructFixupsInRange(const void *begin, size_t size);
        const std::vector<uint8_t> *bufs[2] = { &e->script_data, &e->texture_data };
        for (const std::vector<uint8_t> *buf : bufs) {
            if (buf->empty())
                continue;
            portTextureCacheDeleteRange(buf->data(), buf->size());
            portPackedDisplayListCacheDeleteRange(buf->data(), buf->size());
            portEvictStructFixupsInRange(buf->data(), buf->size());
        }
    }
    sWorkingEntries.erase(
        std::remove_if(sWorkingEntries.begin(), sWorkingEntries.end(),
                       [scripts_lo](const std::unique_ptr<WorkingEntry> &e) {
                           return e && e->scripts_lo == scripts_lo;
                       }),
        sWorkingEntries.end());

    auto working = std::make_unique<WorkingEntry>();
    working->scripts_lo    = scripts_lo;
    working->script_data   = *pristine_script;   /* copy */
    working->texture_data  = *pristine_texture;  /* copy */

    int32_t scripts_num  = static_cast<int32_t>(readNativeU32(working->script_data.data(),  0));
    int32_t textures_num = static_cast<int32_t>(readNativeU32(working->texture_data.data(), 0));

    port_log("particle_bank[%s]: bank_id=%d scripts=%d (%zu bytes) textures=%d (%zu bytes)\n",
             match->debug_name, bank_id, scripts_num, working->script_data.size(),
             textures_num, working->texture_data.size());

    void *script_desc  = working->script_data.data();
    void *texture_desc = working->texture_data.data();

    lbParticleSetupBankID(bank_id, script_desc, texture_desc);

    sWorkingEntries.push_back(std::move(working));
    return 0;
}

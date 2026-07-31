/**
 * frame_interpolation.cpp — Enhanced framerate mode (interpolated rendering).
 *
 * See frame_interpolation.h and docs/frame_interpolation_design_2026-07-30.md
 * for the architecture. Summary of the per-tick flow:
 *
 *   game draw phase (decomp)  -> portInterpRecordMtx() per emitted gSPMatrix
 *   port_drain_pending_display_list:
 *     portInterpActiveSubframes()   -> k, applies SetTargetFps(60*k)
 *     portInterpBeginDraw()         -> decode + pair cur vs prev by key
 *     for j in 1..k:
 *       portInterpGetReplacements(j, k) -> lerped map (empty when j == k)
 *       DrawAndRunGraphicsCommands(dl, map)  (each call paces 1/(60k) s)
 *     portInterpEndDraw()           -> rotate prev <- cur
 *   end of PortPushFrame: portInterpNoteTicDuration() -> auto-throttle
 */

#include "frame_interpolation.h"

#include <libultraship/libultraship.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <fast/Fast3dWindow.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../port_log.h"

extern "C" int gbi_trace_is_enabled(void);

/* ========================================================================= */
/*  State                                                                    */
/* ========================================================================= */

namespace {

constexpr size_t kMaxRecords = 8192;
constexpr int kMaxSubframes = 4;
/* Sustained-overrun threshold for the auto-throttle: ~4.5% over the 16667 us
 * tick budget, comfortably above pacing jitter but far below the 2x budget a
 * vsync-capped display produces. */
constexpr double kThrottleEmaUs = 17400.0;
constexpr int kThrottleTics = 90;
constexpr int kWarmupTics = 120;

struct RecordEntry {
    void *mtx;
    void *owner;
    int32_t ordinal;
    int32_t tag;
};

struct SnapEntry {
    uint64_t key;
    void *mtx;
    int32_t tag;
    float m[4][4];
};

struct PairEntry {
    Mtx *mtx;
    float prev[4][4];
    float cur[4][4];
};

std::vector<RecordEntry> sRecords;
bool sRecordArmed = false;
bool sRecordOverflow = false;

std::vector<SnapEntry> sCur;
std::vector<SnapEntry> sPrev;
std::unordered_map<uint64_t, uint32_t> sPrevIndex;
std::vector<PairEntry> sPairs;
std::unordered_map<Mtx *, MtxF> sReplacements;
const std::unordered_map<Mtx *, MtxF> sEmptyReplacements;

bool sConfigInited = false;
int sConfigK = 1;       /* k requested by CVar/env */
int sThrottleCapK = 4;  /* upper bound imposed by the auto-throttle */
int sAppliedK = 1;      /* last k applied via SetTargetFps */
bool sForceDisabled = false;
float sSnapDist = 500.0f;
bool sStatsLog = false;

double sTicEmaUs = 16667.0;
int sOverrunTics = 0;
int sWarmupRemaining = 0;

/* Per-second stats (SSB64_INTERP_LOG=1). */
int sStatTics = 0;
int sStatRecorded = 0;
int sStatPaired = 0;
int sStatSnapped = 0;

/* ========================================================================= */
/*  Helpers                                                                  */
/* ========================================================================= */

/* Decode a fixed-point s15.16 N64 Mtx into floats, using the exact formula
 * Fast3D's Interpreter::GfxSpMatrix uses, so an "identity" replacement is
 * numerically identical to the interpreter's own load. */
void decode_mtx(const void *mtx, float out[4][4])
{
    const int32_t *addr = static_cast<const int32_t *>(mtx);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j += 2) {
            int32_t int_part = addr[i * 2 + j / 2];
            uint32_t frac_part = static_cast<uint32_t>(addr[8 + i * 2 + j / 2]);
            out[i][j] = static_cast<int32_t>((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.0f;
            out[i][j + 1] = static_cast<int32_t>((int_part << 16) | (frac_part & 0xffff)) / 65536.0f;
        }
    }
}

uint64_t mix_key(const void *owner, int32_t ordinal, int32_t tag)
{
    uint64_t h = reinterpret_cast<uintptr_t>(owner);
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDull;
    h ^= static_cast<uint64_t>(static_cast<uint32_t>(ordinal)) * 0x9E3779B97F4A7C15ull;
    h ^= static_cast<uint64_t>(static_cast<uint32_t>(tag)) << 56;
    return h;
}

/* Guards: decide whether a prev->cur pair may be lerped or must snap. */
bool pair_interpolatable(const SnapEntry &prev, const SnapEntry &cur, float snapDist)
{
    if (cur.tag != PORT_INTERP_TAG_DOBJ) {
        /* Camera matrices (view, projection, or combined view*persp): eye
         * translation scales with zoom distance and projection rows are not
         * world-space, so a fixed distance gate misfires — gate on relative
         * Frobenius delta instead. Cuts change the matrix wholesale (snap);
         * pans/zooms change it gradually (lerp). */
        double dsum = 0.0, csum = 0.0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                double d = static_cast<double>(cur.m[i][j]) - prev.m[i][j];
                dsum += d * d;
                csum += static_cast<double>(cur.m[i][j]) * cur.m[i][j];
            }
        }
        return dsum <= 0.25 * (csum > 1.0 ? csum : 1.0);
    }

    /* Row-vector convention: rows 0-2 are the rotated basis, row 3 is the
     * translation. Large per-tick translation = teleport (respawn, camera
     * cut, recycled object slot) -> snap. */
    for (int j = 0; j < 3; j++) {
        if (std::fabs(cur.m[3][j] - prev.m[3][j]) > snapDist) {
            return false;
        }
    }
    /* Basis flip beyond 90 degrees in one tick -> snap (also catches a
     * mispaired recycled slot whose orientation is unrelated). */
    for (int r = 0; r < 3; r++) {
        float dot = cur.m[r][0] * prev.m[r][0] + cur.m[r][1] * prev.m[r][1] + cur.m[r][2] * prev.m[r][2];
        if (dot < 0.0f) {
            return false;
        }
    }
    return true;
}

Fast::Fast3dWindow *get_fast3d_window()
{
    auto context = Ship::Context::GetInstance();
    if (!context) {
        return nullptr;
    }
    return dynamic_cast<Fast::Fast3dWindow *>(context->GetWindow().get());
}

void apply_effective_k(int k)
{
    if (k == sAppliedK) {
        return;
    }
    auto *window = get_fast3d_window();
    if (window == nullptr) {
        return; /* window not up yet; retry next tick */
    }
    window->SetTargetFps(60 * k);
    port_log("SSB64: interp — render rate set to %d fps (%d subframe%s per tick)\n",
             60 * k, k, (k == 1) ? "" : "s");
    sAppliedK = k;
}

} // namespace

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

extern "C" void portInterpApplyConfig(void)
{
    sConfigInited = true;

    /* One-shot env reads. */
    static bool sEnvInited = false;
    static int sEnvFps = -1;
    if (!sEnvInited) {
        sEnvInited = true;
        const char *env = std::getenv("SSB64_INTERP_FPS");
        sEnvFps = (env != nullptr && env[0] != '\0') ? std::atoi(env) : -1;
        const char *snap = std::getenv("SSB64_INTERP_SNAP_DIST");
        if (snap != nullptr && snap[0] != '\0') {
            sSnapDist = static_cast<float>(std::atof(snap));
        }
        sStatsLog = (std::getenv("SSB64_INTERP_LOG") != nullptr);
        /* Draw-dump diagnostics expect one interpreter walk per tick. */
        sForceDisabled = (std::getenv("SSB64_DUMP_DRAWS") != nullptr);
        sRecords.reserve(kMaxRecords);
    }

    int fps = (sEnvFps >= 0) ? sEnvFps : CVarGetInteger(PORT_INTERP_CVAR_FPS, 0);
    int k = (fps >= 60) ? (fps + 30) / 60 : 1;
    if (k < 1) k = 1;
    if (k > kMaxSubframes) k = kMaxSubframes;

    sConfigK = sForceDisabled ? 1 : k;
    sThrottleCapK = kMaxSubframes; /* a config change re-arms the throttle */
    sOverrunTics = 0;
    sWarmupRemaining = kWarmupTics;
    sTicEmaUs = 16667.0;
    sRecordArmed = (sConfigK > 1);

    if (sConfigK > 1) {
        port_log("SSB64: interp — enhanced framerate requested: %d fps (snap_dist=%.0f)\n",
                 60 * sConfigK, static_cast<double>(sSnapDist));
    }
}

extern "C" int portInterpActiveSubframes(void)
{
    if (!sConfigInited) {
        portInterpApplyConfig();
    }
    if (sConfigK <= 1) {
        apply_effective_k(1);
        return 1;
    }
    if (gbi_trace_is_enabled()) {
        apply_effective_k(1);
        return 1;
    }
    int k = (sConfigK < sThrottleCapK) ? sConfigK : sThrottleCapK;
    apply_effective_k(k);
    return k;
}

extern "C" void portInterpRecordMtx(void *mtx, void *owner, int ordinal, int tag)
{
    if (!sRecordArmed || mtx == nullptr) {
        return;
    }
    if (sRecords.size() >= kMaxRecords) {
        sRecordOverflow = true;
        return;
    }
    sRecords.push_back({ mtx, owner, ordinal, tag });
}

extern "C" void portInterpBeginDraw(void)
{
    sPairs.clear();
    sCur.clear();

    if (sRecordOverflow) {
        /* Snap-only tick; keep snapshots consistent by treating it as if
         * nothing was recorded. */
        static bool sWarned = false;
        if (!sWarned) {
            sWarned = true;
            port_log("SSB64: interp — WARNING: record buffer overflow (>%zu), tick rendered without interpolation\n",
                     kMaxRecords);
        }
        return;
    }
    if (sRecords.empty()) {
        return;
    }

    /* Decode this tick's matrices and assign occurrence-disambiguated keys
     * (a camera re-prepped between layer groups records the same owner
     * multiple times; occurrence order is stable across ticks). */
    std::unordered_map<uint64_t, uint32_t> occurrence;
    occurrence.reserve(sRecords.size());
    sCur.reserve(sRecords.size());
    for (const RecordEntry &rec : sRecords) {
        uint64_t base = mix_key(rec.owner, rec.ordinal, rec.tag);
        uint32_t occ = occurrence[base]++;
        SnapEntry entry;
        entry.key = base + static_cast<uint64_t>(occ) * 0xC2B2AE3D27D4EB4Full;
        entry.mtx = rec.mtx;
        entry.tag = rec.tag;
        decode_mtx(rec.mtx, entry.m);
        sCur.push_back(entry);
    }

    /* Pair against the previous drawn tick. Missing key = spawned this tick
     * -> no entry -> interpreter reads game memory (snap). Identical values
     * (static geometry, cached XObj matrices) need no replacement either. */
    int paired = 0, snapped = 0;
    for (const SnapEntry &cur : sCur) {
        auto it = sPrevIndex.find(cur.key);
        if (it == sPrevIndex.end()) {
            continue;
        }
        const SnapEntry &prev = sPrev[it->second];
        if (std::memcmp(prev.m, cur.m, sizeof(cur.m)) == 0) {
            continue;
        }
        if (!pair_interpolatable(prev, cur, sSnapDist)) {
            snapped++;
            continue;
        }
        PairEntry pair;
        pair.mtx = static_cast<Mtx *>(cur.mtx);
        std::memcpy(pair.prev, prev.m, sizeof(pair.prev));
        std::memcpy(pair.cur, cur.m, sizeof(pair.cur));
        sPairs.push_back(pair);
        paired++;
    }

    sStatTics++;
    sStatRecorded += static_cast<int>(sRecords.size());
    sStatPaired += paired;
    sStatSnapped += snapped;
    if (sStatsLog && sStatTics >= 60) {
        port_log("SSB64: interp — %d tics: recorded=%d paired=%d snapped=%d ema=%.0fus capK=%d\n",
                 sStatTics, sStatRecorded, sStatPaired, sStatSnapped, sTicEmaUs, sThrottleCapK);
        sStatTics = sStatRecorded = sStatPaired = sStatSnapped = 0;
    }
}

const std::unordered_map<Mtx *, MtxF> &portInterpGetReplacements(int subframe, int total)
{
    if (subframe >= total || sPairs.empty()) {
        return sEmptyReplacements;
    }
    float f = static_cast<float>(subframe) / static_cast<float>(total);
    sReplacements.clear();
    sReplacements.reserve(sPairs.size());
    for (const PairEntry &pair : sPairs) {
        MtxF value;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                value.mf[i][j] = pair.prev[i][j] + (pair.cur[i][j] - pair.prev[i][j]) * f;
            }
        }
        sReplacements.emplace(pair.mtx, value);
    }
    return sReplacements;
}

extern "C" void portInterpEndDraw(void)
{
    /* Rotate prev <- cur only when this tick actually drew (sCur populated
     * or a genuine empty draw). A frozen tick never reaches here, so the
     * next drawn tick pairs against the last *drawn* one — motion resumes
     * smoothly across authored freeze frames. */
    sPrev.swap(sCur);
    sPrevIndex.clear();
    sPrevIndex.reserve(sPrev.size());
    for (uint32_t i = 0; i < sPrev.size(); i++) {
        sPrevIndex.emplace(sPrev[i].key, i);
    }
    sCur.clear();
    sPairs.clear();
    sRecords.clear();
    sRecordOverflow = false;
}

extern "C" void portInterpNoteTicDuration(long long micros)
{
    sTicEmaUs += (static_cast<double>(micros) - sTicEmaUs) * (1.0 / 32.0);

    if (sAppliedK <= 1) {
        sOverrunTics = 0;
        return;
    }
    if (sWarmupRemaining > 0) {
        sWarmupRemaining--;
        return;
    }
    if (sTicEmaUs > kThrottleEmaUs) {
        if (++sOverrunTics >= kThrottleTics) {
            sThrottleCapK = sAppliedK - 1;
            sOverrunTics = 0;
            sWarmupRemaining = kWarmupTics;
            port_log("SSB64: interp — host cannot sustain %d fps (tick avg %.1f ms > 16.7 ms); "
                     "stepping down to %d fps to protect the 60 Hz game clock\n",
                     60 * sAppliedK, sTicEmaUs / 1000.0, 60 * sThrottleCapK);
        }
    } else {
        sOverrunTics = 0;
    }
}

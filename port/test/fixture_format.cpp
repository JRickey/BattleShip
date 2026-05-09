#include "fixture_format.h"

#include <ship/utils/StrHash64.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace harness {

namespace {

template <typename T>
void AppendLE(std::vector<uint8_t> &buf, T value)
{
    static_assert(std::is_integral_v<T>, "AppendLE requires integral type");
    /* Host is LE on every platform we run tests on (M1, x86-64). */
    const uint8_t *src = reinterpret_cast<const uint8_t *>(&value);
    buf.insert(buf.end(), src, src + sizeof(T));
}

void AppendBytes(std::vector<uint8_t> &buf, const void *src, size_t n)
{
    if (n == 0) return;
    const uint8_t *p = static_cast<const uint8_t *>(src);
    buf.insert(buf.end(), p, p + n);
}

template <typename T>
bool ReadLE(const uint8_t *&p, const uint8_t *end, T &out)
{
    if (static_cast<size_t>(end - p) < sizeof(T)) return false;
    std::memcpy(&out, p, sizeof(T));
    p += sizeof(T);
    return true;
}

bool ReadBytes(const uint8_t *&p, const uint8_t *end, void *dst, size_t n)
{
    if (static_cast<size_t>(end - p) < n) return false;
    std::memcpy(dst, p, n);
    p += n;
    return true;
}

} // namespace

std::string FixtureFilename(uint32_t file_id, const std::string &path)
{
    char id_buf[8];
    std::snprintf(id_buf, sizeof(id_buf), "%04u", file_id);
    std::string sanitized = path;
    /* Strip a leading "reloc_" prefix if present. */
    if (sanitized.rfind("reloc_", 0) == 0) {
        sanitized.erase(0, 6);
    }
    /* Replace path separators. */
    for (char &c : sanitized) {
        if (c == '/' || c == '\\') c = '_';
    }
    return std::string(id_buf) + "_" + sanitized + ".fixt";
}

bool WriteFixture(const std::filesystem::path &path, const Snapshot &snap)
{
    std::vector<uint8_t> body;
    body.reserve(snap.data.size() + 256);

    uint32_t flags = 0;

    /* DATA */
    flags |= kSectionData;
    AppendLE<uint32_t>(body, static_cast<uint32_t>(snap.data.size()));
    AppendBytes(body, snap.data.data(), snap.data.size());

    /* HALFSWAP_RANGES — only emit if non-empty (saves a few bytes for the 99%+ that aren't fighter figatree). */
    if (!snap.halfswap_ranges.empty()) {
        flags |= kSectionHalfswap;
        AppendLE<uint32_t>(body, static_cast<uint32_t>(snap.halfswap_ranges.size()));
        for (const auto &r : snap.halfswap_ranges) {
            AppendLE<uint32_t>(body, r.base_offset);
            AppendLE<uint32_t>(body, r.size);
        }
    }

    /* TRACKERS */
    if (!snap.trackers.empty()) {
        flags |= kSectionTrackers;
        AppendLE<uint32_t>(body, static_cast<uint32_t>(snap.trackers.size()));
        for (const auto &t : snap.trackers) {
            AppendLE<uint32_t>(body, t.byte_offset);
            uint8_t pad[4] = {t.family, 0, 0, 0};
            AppendBytes(body, pad, 4);
        }
    }

    /* Split chain targets by family. */
    {
        uint32_t intern_count = 0, extern_count = 0;
        for (const auto &c : snap.chain_targets) {
            if (c.dep_file_id < 0) intern_count++; else extern_count++;
        }
        if (intern_count > 0) {
            flags |= kSectionChainIntern;
            AppendLE<uint32_t>(body, intern_count);
            for (const auto &c : snap.chain_targets) {
                if (c.dep_file_id < 0) {
                    AppendLE<uint32_t>(body, c.slot_offset);
                    AppendLE<uint32_t>(body, c.target_offset);
                }
            }
        }
        if (extern_count > 0) {
            flags |= kSectionChainExtern;
            AppendLE<uint32_t>(body, extern_count);
            for (const auto &c : snap.chain_targets) {
                if (c.dep_file_id >= 0) {
                    AppendLE<uint32_t>(body, c.slot_offset);
                    AppendLE<uint32_t>(body, static_cast<uint32_t>(c.dep_file_id));
                    AppendLE<uint32_t>(body, c.target_offset);
                }
            }
        }
    }

    uint64_t crc = ::crc64(body.data(), static_cast<uint32_t>(body.size()));

    /* Build header. */
    std::vector<uint8_t> hdr;
    hdr.reserve(32);
    AppendLE<uint32_t>(hdr, kFixtureMagic);
    AppendLE<uint32_t>(hdr, kFixtureVersion);
    AppendLE<uint32_t>(hdr, snap.file_id);
    AppendLE<uint32_t>(hdr, flags);
    AppendLE<uint32_t>(hdr, static_cast<uint32_t>(body.size()));
    AppendLE<uint64_t>(hdr, crc);
    AppendLE<uint32_t>(hdr, 0u);  /* reserved */

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    out.write(reinterpret_cast<const char *>(body.data()), body.size());
    return out.good();
}

bool ReadFixture(const std::filesystem::path &path, Snapshot &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::vector<uint8_t> all((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    if (all.size() < 32) return false;

    const uint8_t *p = all.data();
    const uint8_t *end = p + all.size();

    uint32_t magic = 0, version = 0, file_id = 0, flags = 0, body_size = 0, reserved = 0;
    uint64_t body_crc = 0;
    if (!ReadLE(p, end, magic)) return false;
    if (magic != kFixtureMagic) return false;
    if (!ReadLE(p, end, version)) return false;
    if (version != kFixtureVersion) return false;
    if (!ReadLE(p, end, file_id)) return false;
    if (!ReadLE(p, end, flags)) return false;
    if (!ReadLE(p, end, body_size)) return false;
    if (!ReadLE(p, end, body_crc)) return false;
    if (!ReadLE(p, end, reserved)) return false;

    if (static_cast<size_t>(end - p) < body_size) return false;
    const uint8_t *body_start = p;
    const uint8_t *body_end = p + body_size;

    /* Verify CRC before any further parsing. */
    if (::crc64(body_start, body_size) != body_crc) return false;

    Snapshot snap;
    snap.file_id = file_id;

    if (flags & kSectionData) {
        uint32_t size = 0;
        if (!ReadLE(p, body_end, size)) return false;
        snap.data.resize(size);
        if (!ReadBytes(p, body_end, snap.data.data(), size)) return false;
    }
    if (flags & kSectionHalfswap) {
        uint32_t count = 0;
        if (!ReadLE(p, body_end, count)) return false;
        snap.halfswap_ranges.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            if (!ReadLE(p, body_end, snap.halfswap_ranges[i].base_offset)) return false;
            if (!ReadLE(p, body_end, snap.halfswap_ranges[i].size)) return false;
        }
    }
    if (flags & kSectionTrackers) {
        uint32_t count = 0;
        if (!ReadLE(p, body_end, count)) return false;
        snap.trackers.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            if (!ReadLE(p, body_end, snap.trackers[i].byte_offset)) return false;
            uint8_t pad[4];
            if (!ReadBytes(p, body_end, pad, 4)) return false;
            snap.trackers[i].family = pad[0];
        }
    }
    if (flags & kSectionChainIntern) {
        uint32_t count = 0;
        if (!ReadLE(p, body_end, count)) return false;
        for (uint32_t i = 0; i < count; i++) {
            ChainTarget c;
            c.dep_file_id = -1;
            if (!ReadLE(p, body_end, c.slot_offset)) return false;
            if (!ReadLE(p, body_end, c.target_offset)) return false;
            snap.chain_targets.push_back(c);
        }
    }
    if (flags & kSectionChainExtern) {
        uint32_t count = 0;
        if (!ReadLE(p, body_end, count)) return false;
        for (uint32_t i = 0; i < count; i++) {
            ChainTarget c;
            uint32_t dep = 0;
            if (!ReadLE(p, body_end, c.slot_offset)) return false;
            if (!ReadLE(p, body_end, dep)) return false;
            if (!ReadLE(p, body_end, c.target_offset)) return false;
            c.dep_file_id = static_cast<int32_t>(dep);
            snap.chain_targets.push_back(c);
        }
    }

    snap.Canonicalize();
    out = std::move(snap);
    return true;
}

} // namespace harness

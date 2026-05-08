#include "snapshot.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace harness {

void Snapshot::Canonicalize()
{
    std::sort(chain_targets.begin(), chain_targets.end(),
              [](const ChainTarget &a, const ChainTarget &b) {
                  if (a.slot_offset != b.slot_offset) return a.slot_offset < b.slot_offset;
                  if (a.dep_file_id != b.dep_file_id) return a.dep_file_id < b.dep_file_id;
                  return a.target_offset < b.target_offset;
              });
    std::sort(trackers.begin(), trackers.end(),
              [](const TrackerEntry &a, const TrackerEntry &b) {
                  if (a.byte_offset != b.byte_offset) return a.byte_offset < b.byte_offset;
                  return a.family < b.family;
              });
    std::sort(halfswap_ranges.begin(), halfswap_ranges.end(),
              [](const HalfswapRange &a, const HalfswapRange &b) {
                  if (a.base_offset != b.base_offset) return a.base_offset < b.base_offset;
                  return a.size < b.size;
              });
}

std::string Snapshot::Diff(const Snapshot &other) const
{
    std::ostringstream oss;
    oss << "Snapshot diff (this=" << file_id << " path='" << path << "', other="
        << other.file_id << " path='" << other.path << "'):\n";

    if (file_id != other.file_id) {
        oss << "  file_id: " << file_id << " vs " << other.file_id << "\n";
        return oss.str();
    }

    if (data.size() != other.data.size()) {
        oss << "  data.size: " << data.size() << " vs " << other.data.size() << "\n";
    }
    {
        size_t n = std::min(data.size(), other.data.size());
        size_t first_div = n;
        for (size_t i = 0; i < n; ++i) {
            if (data[i] != other.data[i]) { first_div = i; break; }
        }
        if (first_div < n) {
            char hex[256];
            size_t span_end = std::min(first_div + 16, n);
            std::snprintf(hex, sizeof(hex), "  data first divergence at byte 0x%zx:\n    this : ", first_div);
            oss << hex;
            for (size_t i = first_div; i < span_end; ++i) {
                std::snprintf(hex, sizeof(hex), "%02X ", data[i]);
                oss << hex;
            }
            oss << "\n    other: ";
            for (size_t i = first_div; i < span_end; ++i) {
                std::snprintf(hex, sizeof(hex), "%02X ", other.data[i]);
                oss << hex;
            }
            oss << "\n";
        }
    }

    if (chain_targets.size() != other.chain_targets.size()) {
        oss << "  chain_targets.size: " << chain_targets.size()
            << " vs " << other.chain_targets.size() << "\n";
    } else {
        for (size_t i = 0; i < chain_targets.size(); ++i) {
            if (!(chain_targets[i] == other.chain_targets[i])) {
                char hex[256];
                std::snprintf(hex, sizeof(hex),
                    "  chain_targets[%zu]: this {slot=0x%X dep=%d tgt=0x%X} vs other {slot=0x%X dep=%d tgt=0x%X}\n",
                    i,
                    chain_targets[i].slot_offset, chain_targets[i].dep_file_id, chain_targets[i].target_offset,
                    other.chain_targets[i].slot_offset, other.chain_targets[i].dep_file_id, other.chain_targets[i].target_offset);
                oss << hex;
                break;
            }
        }
    }

    if (trackers.size() != other.trackers.size()) {
        oss << "  trackers.size: " << trackers.size() << " vs " << other.trackers.size() << "\n";
    } else {
        for (size_t i = 0; i < trackers.size(); ++i) {
            if (!(trackers[i] == other.trackers[i])) {
                char hex[256];
                std::snprintf(hex, sizeof(hex),
                    "  trackers[%zu]: this {off=0x%X fam=%u} vs other {off=0x%X fam=%u}\n",
                    i,
                    trackers[i].byte_offset, trackers[i].family,
                    other.trackers[i].byte_offset, other.trackers[i].family);
                oss << hex;
                break;
            }
        }
    }

    if (halfswap_ranges.size() != other.halfswap_ranges.size()) {
        oss << "  halfswap_ranges.size: " << halfswap_ranges.size()
            << " vs " << other.halfswap_ranges.size() << "\n";
    } else {
        for (size_t i = 0; i < halfswap_ranges.size(); ++i) {
            if (!(halfswap_ranges[i] == other.halfswap_ranges[i])) {
                char hex[256];
                std::snprintf(hex, sizeof(hex),
                    "  halfswap_ranges[%zu]: this {base=0x%X size=0x%X} vs other {base=0x%X size=0x%X}\n",
                    i,
                    halfswap_ranges[i].base_offset, halfswap_ranges[i].size,
                    other.halfswap_ranges[i].base_offset, other.halfswap_ranges[i].size);
                oss << hex;
                break;
            }
        }
    }

    return oss.str();
}

} // namespace harness

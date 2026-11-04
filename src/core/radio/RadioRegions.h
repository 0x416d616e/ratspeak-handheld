#pragma once

#include "config/Config.h"
#include "config/UserConfig.h"

// Region presets and the regional limits they carry.
//
// Most regions only choose a default center frequency. A region with limits
// also applies a full link profile when chosen, and keeps the link inside its
// legal envelope while the radio stays in that region's band: link presets and
// manual edits are clamped instead of silently going out of bounds.
namespace RadioRegions {

struct Limits {
    uint32_t minHz;        // Band edges; limits apply while tuned inside them
    uint32_t maxHz;
    uint32_t maxBandwidth; // Widest allowed LoRa bandwidth in Hz
    int8_t maxTxPower;     // Highest allowed radio output in dBm
};

// India, IN865 (WPC licence-exempt band): 865-867 MHz, at most 1 W (30 dBm)
// transmitter power and 200 kHz carrier bandwidth. 125 kHz is the widest
// standard LoRa bandwidth under 200 kHz. The SX1262 tops out at 22 dBm, so
// the power ceiling is the radio's own.
inline constexpr Limits INDIA_LIMITS = {865000000UL, 867000000UL, 125000UL, 22};

// The India link profile is Medium Fast (the device default) narrowed to
// 125 kHz, so it still reads as Medium Fast in the link preset list.
inline constexpr uint8_t INDIA_SF = 9;
inline constexpr uint32_t INDIA_BW = 125000UL;
inline constexpr uint8_t INDIA_CR = 5;
inline constexpr int8_t INDIA_TX_POWER = 17;
inline constexpr long INDIA_PREAMBLE = 18;

inline constexpr const Limits* limits(uint8_t region) {
    return region == REGION_INDIA ? &INDIA_LIMITS : nullptr;
}

// The limits that apply to these settings now, if any: the region has limits
// and the radio is tuned inside that region's band.
inline const Limits* activeLimits(const UserScalars& s) {
    const Limits* l = limits(s.radioRegion);
    if (!l || s.loraFrequency < l->minHz || s.loraFrequency > l->maxHz) return nullptr;
    return l;
}

// The bandwidth a link preset would use under these settings' limits.
inline uint32_t allowedBandwidth(const UserScalars& s, uint32_t hz) {
    const Limits* l = activeLimits(s);
    return l && hz > l->maxBandwidth ? l->maxBandwidth : hz;
}

// Clamp settings into the active region's limits. Safe to call repeatedly.
inline void enforce(UserScalars& s) {
    const Limits* l = activeLimits(s);
    if (!l) return;
    if (s.loraBW > l->maxBandwidth) s.loraBW = l->maxBandwidth;
    if (s.loraTxPower > l->maxTxPower) s.loraTxPower = l->maxTxPower;
}

// Explicitly choosing a region restores its default frequency, and for a
// region with limits, its full compliant link profile.
inline void apply(UserScalars& s, int region) {
    if (region < 0 || region >= REGION_COUNT) return;
    s.radioRegion = uint8_t(region);
    s.loraFrequency = REGION_FREQ[region];
    if (region == REGION_INDIA) {
        s.loraSF = INDIA_SF;
        s.loraBW = INDIA_BW;
        s.loraCR = INDIA_CR;
        s.loraTxPower = INDIA_TX_POWER;
        s.loraPreamble = INDIA_PREAMBLE;
    }
    enforce(s);
}

} // namespace RadioRegions

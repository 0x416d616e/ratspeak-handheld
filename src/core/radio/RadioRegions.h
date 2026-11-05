#pragma once

#include <stddef.h>
#include <stdint.h>
#include "radio/RadioPresets.h"

// Radio regions: the single source of truth for each region's band, power
// limit, bandwidth limit, default frequency and default link preset.
//
// The values summarise each region's licence-exempt rules as best we know
// them; they are not legal advice. Duty-cycle limits are shown to the user but
// not enforced, because the firmware does not account airtime yet.
namespace radio {

// Persisted as user.json "radio_region". Permanent: never renumber or reuse.
enum class RegionId : uint8_t {
    Americas = 0,
    Europe = 1,
    AuNz = 2,
    Asia = 3,
    India = 4,
    Custom = 254,  // Developer only: SX1262 hardware limits, no regional rules.
};

enum class PowerRef : uint8_t { Conducted, Erp, Eirp };

struct SubBand {
    uint32_t minHz;
    uint32_t maxHz;          // The whole signal (centre +/- bandwidth/2) must fit.
    int8_t limitDbm;         // Legal power limit, measured as `ref` states.
    PowerRef ref;
    uint16_t dutyPermille;   // 1000 = no limit. Shown, not enforced.
};

struct Region {
    RegionId id;
    const char* label;       // ASCII, short enough for a settings row.
    const SubBand* bands;    // nullptr: no regional rules (Custom).
    uint8_t bandCount;
    uint32_t defaultHz;      // Fits every preset the region allows (host test).
    uint32_t maxBandwidthHz;
    PresetId defaultPreset;
};

// FCC Part 15: 902-928 MHz, 1 W conducted.
inline constexpr SubBand AMERICAS_BANDS[] = {
    {902000000UL, 928000000UL, 30, PowerRef::Conducted, 1000},
};

// ERC Rec 70-03 Annex 1 / EN 300 220 sub-bands. 865-868.6 merges h1.4 and
// h1.5 (both 25 mW ERP, 1%). The 500 mW band 47b tier needs adaptive power
// control, which the firmware lacks, so it is not offered.
inline constexpr SubBand EUROPE_BANDS[] = {
    {863000000UL, 865000000UL, 14, PowerRef::Erp, 1},
    {865000000UL, 868600000UL, 14, PowerRef::Erp, 10},
    {868700000UL, 869200000UL, 14, PowerRef::Erp, 1},
    {869400000UL, 869650000UL, 27, PowerRef::Erp, 100},
    {869700000UL, 870000000UL, 14, PowerRef::Erp, 10},
};

// Australia LIPD class licence: 915-928 MHz, 1 W EIRP. New Zealand limits
// 915-921 MHz to 3 mW EIRP, so the default sits in 921-928 MHz.
inline constexpr SubBand AUNZ_BANDS[] = {
    {915000000UL, 928000000UL, 30, PowerRef::Eirp, 1000},
};

// AS923 920-925 MHz at the lowest common limit (Singapore, 100 mW).
inline constexpr SubBand ASIA_BANDS[] = {
    {920000000UL, 925000000UL, 20, PowerRef::Conducted, 1000},
};

// India G.S.R. 853(E) (2021), general tier: 25 mW ERP, 1% duty cycle. The
// 500 mW tier requires adaptive power control, which the firmware lacks. The
// band stays inside 865-867 MHz, which the older 2005 rule also allowed, and
// bandwidth stays at or below 125 kHz, under every tier's 200 kHz cap.
inline constexpr SubBand INDIA_BANDS[] = {
    {865000000UL, 867000000UL, 14, PowerRef::Erp, 10},
};

template <size_t N>
constexpr uint8_t bandCount(const SubBand (&)[N]) { return static_cast<uint8_t>(N); }

// Display order. Custom stays last: the settings lists drop it outside
// Developer Mode.
inline constexpr Region REGIONS[] = {
    {RegionId::Americas, "Americas 902-928", AMERICAS_BANDS, bandCount(AMERICAS_BANDS),
     915000000UL, 500000UL, PresetId::MediumFast},
    {RegionId::Europe, "Europe 863-870", EUROPE_BANDS, bandCount(EUROPE_BANDS),
     868000000UL, 500000UL, PresetId::MediumFast},
    {RegionId::AuNz, "Aus/NZ 915-928", AUNZ_BANDS, bandCount(AUNZ_BANDS),
     925875000UL, 500000UL, PresetId::MediumFast},
    {RegionId::Asia, "Asia 920-925", ASIA_BANDS, bandCount(ASIA_BANDS),
     923000000UL, 500000UL, PresetId::MediumFast},
    // 866.6 MHz keeps a 125 kHz signal clear of Meshtastic India's default
    // channels (865.5-866.5 MHz) and of the 2 W RFID channels at 866.3 and
    // 866.9 MHz.
    {RegionId::India, "India 865-867", INDIA_BANDS, bandCount(INDIA_BANDS),
     866600000UL, 125000UL, PresetId::LiteFast},
    {RegionId::Custom, "Custom", nullptr, 0,
     0, 500000UL, PresetId::MediumFast},
};
inline constexpr int REGION_COUNT = sizeof(REGIONS) / sizeof(REGIONS[0]);

inline constexpr const Region* findRegion(uint8_t id) {
    for (const auto& region : REGIONS) {
        if (static_cast<uint8_t>(region.id) == id) return &region;
    }
    return nullptr;
}

inline constexpr uint8_t regionValue(RegionId id) { return static_cast<uint8_t>(id); }

} // namespace radio

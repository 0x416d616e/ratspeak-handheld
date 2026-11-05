#pragma once

#include <stdint.h>

// LoRa link presets: a name for one SF/bandwidth/coding-rate tuple, the same
// in every region. Reticulum peers must share frequency, bandwidth and SF; the
// coding rate travels in the LoRa header and TX power need not match, so
// presets do not set power. Names follow Meshtastic's modem presets.
namespace radio {

enum class PresetId : uint8_t {
    ShortTurbo, ShortFast, ShortSlow, MediumFast, MediumSlow,
    LongTurbo, LongFast, LongModerate, LiteFast, LiteSlow,
};

struct Preset {
    PresetId id;
    const char* name;
    uint8_t sf;
    uint32_t bandwidthHz;
    uint8_t cr;
};

// Frozen: a published preset never changes its tuple, or saved links would be
// renamed. Add new presets at the end; never edit or reorder these.
inline constexpr Preset PRESETS[] = {
    {PresetId::ShortTurbo,   "Short Turbo",    7, 500000, 5},
    {PresetId::ShortFast,    "Short Fast",     7, 250000, 5},
    {PresetId::ShortSlow,    "Short Slow",     8, 250000, 5},
    {PresetId::MediumFast,   "Medium Fast",    9, 250000, 5},
    {PresetId::MediumSlow,   "Medium Slow",   10, 250000, 5},
    {PresetId::LongTurbo,    "Long Turbo",    11, 500000, 8},
    {PresetId::LongFast,     "Long Fast",     11, 250000, 5},
    {PresetId::LongModerate, "Long Moderate", 11, 125000, 8},
    {PresetId::LiteFast,     "Lite Fast",      9, 125000, 5},
    {PresetId::LiteSlow,     "Lite Slow",     10, 125000, 5},
};
inline constexpr int PRESET_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);

inline constexpr PresetId DEFAULT_PRESET = PresetId::MediumFast;
inline constexpr uint8_t DEFAULT_PREAMBLE = 18;

inline constexpr const Preset& preset(PresetId id) { return PRESETS[static_cast<int>(id)]; }

} // namespace radio

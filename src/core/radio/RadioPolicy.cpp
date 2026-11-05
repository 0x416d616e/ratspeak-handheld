#include "radio/RadioPolicy.h"

#include <stdio.h>
#include "radio/RadioBandwidth.h"
#include "radio/RadioFrequency.h"

namespace radio {
namespace policy {
namespace {

template <class T>
T clampValue(T value, T lo, T hi) { return value < lo ? lo : (value > hi ? hi : value); }

const Region& customRegion() { return *findRegion(regionValue(RegionId::Custom)); }

uint32_t widestBandwidthAtMost(uint32_t hz) {
    uint32_t best = RadioBandwidth::values[0].hz;
    for (const auto& value : RadioBandwidth::values) {
        if (value.hz <= hz) best = value.hz;
    }
    return best;
}

// Keep the signal in the sub-band that holds its centre, else use the default.
uint32_t fitFrequency(const Region& region, uint32_t hz, uint32_t bandwidthHz) {
    for (uint8_t i = 0; i < region.bandCount; ++i) {
        const SubBand& band = region.bands[i];
        if (hz >= band.minHz && hz <= band.maxHz && band.maxHz - band.minHz >= bandwidthHz) {
            return clampValue(hz, band.minHz + bandwidthHz / 2, band.maxHz - bandwidthHz / 2);
        }
    }
    return region.defaultHz;
}

void applyPreset(Link& link, const Preset& value) {
    link.sf = value.sf;
    link.bandwidthHz = value.bandwidthHz;
    link.cr = value.cr;
    link.preamble = DEFAULT_PREAMBLE;
}

uint16_t difference(const Link& a, const Link& b) {
    return (a.region != b.region ? ChangedRegion : 0) |
           (a.hz != b.hz ? ChangedFrequency : 0) |
           (a.bandwidthHz != b.bandwidthHz ? ChangedBandwidth : 0) |
           (a.sf != b.sf ? ChangedSpreading : 0) |
           (a.cr != b.cr ? ChangedCoding : 0) |
           (a.txDbm != b.txDbm ? ChangedTxPower : 0) |
           (a.preamble != b.preamble ? ChangedPreamble : 0);
}

uint16_t fieldMask(Field field) {
    switch (field) {
        case Field::Frequency: return ChangedFrequency;
        case Field::Bandwidth: return ChangedBandwidth;
        case Field::Spreading: return ChangedSpreading;
        case Field::Coding: return ChangedCoding;
        case Field::TxPower: return ChangedTxPower;
        case Field::Preamble: return ChangedPreamble;
    }
    return 0;
}

const char* regionRefusal(Field field) {
    switch (field) {
        case Field::Frequency: return "Outside region band";
        case Field::Bandwidth: return "Too wide for region";
        case Field::TxPower: return "Above region power limit";
        default: return "Not allowed in region";
    }
}

size_t finish(int written, size_t size) {
    if (written < 0) return 0;
    return static_cast<size_t>(written) < size ? static_cast<size_t>(written) : (size ? size - 1 : 0);
}

const char* bandwidthLabel(uint32_t hz) {
    for (const auto& value : RadioBandwidth::values) {
        if (value.hz == hz) return value.label;
    }
    return "?";
}

} // namespace

const Region& regionOf(const Link& link) {
    const Region* region = findRegion(link.region);
    return region ? *region : customRegion();
}

bool hasLimits(const Region& region) { return region.bandCount > 0; }

bool regionSelectable(const Region& region, bool devMode) {
    return hasLimits(region) || devMode;
}

const SubBand* subBandFor(const Region& region, uint32_t hz, uint32_t bandwidthHz) {
    const uint32_t half = bandwidthHz / 2;
    for (uint8_t i = 0; i < region.bandCount; ++i) {
        const SubBand& band = region.bands[i];
        if (hz >= band.minHz + half && hz + half <= band.maxHz) return &band;
    }
    return nullptr;
}

int8_t conductedLimitDbm(const SubBand& band, int8_t antennaGainDbi) {
    // Work in tenths of a dB and round down, so the cap never exceeds the rule.
    int tenths = band.limitDbm * 10;
    if (band.ref == PowerRef::Eirp) {
        tenths -= antennaGainDbi * 10;
    } else if (band.ref == PowerRef::Erp && antennaGainDbi * 10 > 21) {
        tenths -= antennaGainDbi * 10 - 21;  // ERP is relative to a 2.15 dBi dipole.
    }
    return static_cast<int8_t>(tenths >= 0 ? tenths / 10 : -((-tenths + 9) / 10));
}

int8_t txCapDbm(const Link& link, const BoardCaps& caps) {
    int cap = caps.maxTxDbm;
    const Region& region = regionOf(link);
    if (const SubBand* band = subBandFor(region, link.hz, link.bandwidthHz)) {
        const int legal = conductedLimitDbm(*band, caps.antennaGainDbi);
        if (legal < cap) cap = legal;
    }
    return static_cast<int8_t>(cap < caps.minTxDbm ? caps.minTxDbm : cap);
}

bool presetAllowed(uint8_t regionId, PresetId id) {
    const Preset& value = preset(id);
    if (RadioBandwidth::normalize(value.bandwidthHz) != value.bandwidthHz) return false;
    const Region* region = findRegion(regionId);
    return !region || !hasLimits(*region) || value.bandwidthHz <= region->maxBandwidthHz;
}

int matchPreset(const Link& link) {
    for (int i = 0; i < PRESET_COUNT; ++i) {
        const Preset& value = PRESETS[i];
        if (value.sf == link.sf && value.bandwidthHz == link.bandwidthHz && value.cr == link.cr) return i;
    }
    return -1;
}

const char* presetName(const Link& link) {
    const int index = matchPreset(link);
    return index < 0 ? "Custom" : PRESETS[index].name;
}

uint16_t conform(Link& link, const BoardCaps& caps) {
    const Link before = link;
    if (!findRegion(link.region)) link.region = regionValue(RegionId::Custom);
    link.sf = clampValue<uint8_t>(link.sf, 5, 12);
    link.cr = clampValue<uint8_t>(link.cr, 5, 8);
    link.preamble = clampValue<uint8_t>(link.preamble, 6, 65);
    link.bandwidthHz = RadioBandwidth::normalize(link.bandwidthHz);
    const Region& region = regionOf(link);
    if (hasLimits(region)) {
        if (link.bandwidthHz > region.maxBandwidthHz) {
            link.bandwidthHz = widestBandwidthAtMost(region.maxBandwidthHz);
        }
        if (!subBandFor(region, link.hz, link.bandwidthHz)) {
            link.hz = fitFrequency(region, link.hz, link.bandwidthHz);
        }
    } else if (!loRaFrequencyBand(link.hz)) {
        link.hz = caps.fallbackHz;
    }
    link.txDbm = clampValue<int8_t>(link.txDbm, caps.minTxDbm, txCapDbm(link, caps));
    return difference(before, link);
}

Outcome selectRegion(Link& link, uint8_t regionId, const BoardCaps& caps) {
    const Region* region = findRegion(regionId);
    if (!region) return {Verdict::Refused, 0, "Unknown region"};
    Link next = link;
    next.region = regionId;
    if (hasLimits(*region)) {
        next.hz = region->defaultHz;
        applyPreset(next, preset(region->defaultPreset));
        next.txDbm = caps.defaultTxDbm;
    }
    conform(next, caps);
    const uint16_t changed = difference(link, next) & ~ChangedRegion;
    link = next;
    return {changed ? Verdict::Adjusted : Verdict::Ok, changed, nullptr};
}

Outcome selectPreset(Link& link, PresetId id, const BoardCaps& caps) {
    if (!presetAllowed(link.region, id)) return {Verdict::Refused, 0, "Not allowed in region"};
    Link next = link;
    applyPreset(next, preset(id));
    const uint16_t sideEffects = conform(next, caps) &
        ~(ChangedSpreading | ChangedBandwidth | ChangedCoding | ChangedPreamble);
    link = next;
    return {sideEffects ? Verdict::Adjusted : Verdict::Ok, sideEffects, nullptr};
}

Outcome edit(Link& link, Field field, int32_t value, const BoardCaps& caps) {
    Link next = link;
    switch (field) {
        case Field::Frequency:
            if (value <= 0 || !loRaFrequencyBand(static_cast<uint32_t>(value))) {
                return {Verdict::Refused, 0, "Unsupported frequency"};
            }
            next.hz = static_cast<uint32_t>(value);
            break;
        case Field::Bandwidth:
            if (value <= 0 || RadioBandwidth::normalize(static_cast<uint32_t>(value)) != static_cast<uint32_t>(value)) {
                return {Verdict::Refused, 0, "Unsupported bandwidth"};
            }
            next.bandwidthHz = static_cast<uint32_t>(value);
            break;
        case Field::Spreading:
            if (value < 5 || value > 12) return {Verdict::Refused, 0, "SF: 5-12"};
            next.sf = static_cast<uint8_t>(value);
            break;
        case Field::Coding:
            if (value < 5 || value > 8) return {Verdict::Refused, 0, "CR: 5-8"};
            next.cr = static_cast<uint8_t>(value);
            break;
        case Field::TxPower:
            if (value < caps.minTxDbm || value > caps.maxTxDbm) return {Verdict::Refused, 0, "Outside radio power range"};
            next.txDbm = static_cast<int8_t>(value);
            break;
        case Field::Preamble:
            if (value < 6 || value > 65) return {Verdict::Refused, 0, "Preamble: 6-65"};
            next.preamble = static_cast<uint8_t>(value);
            break;
    }
    const uint16_t changed = conform(next, caps);
    if (changed & fieldMask(field)) return {Verdict::Refused, 0, regionRefusal(field)};
    link = next;
    return {changed ? Verdict::Adjusted : Verdict::Ok, changed, nullptr};
}

void migrateLegacy(Link& link, int32_t storedRegion) {
    // Default frequencies of the five regions the older firmware offered.
    static constexpr uint32_t LEGACY_DEFAULT_HZ[] = {915000000UL, 868000000UL, 915000000UL, 923000000UL, 866000000UL};
    constexpr int32_t LEGACY_COUNT = sizeof(LEGACY_DEFAULT_HZ) / sizeof(LEGACY_DEFAULT_HZ[0]);
    if (storedRegion < 0 || storedRegion >= LEGACY_COUNT) {
        link.region = regionValue(RegionId::Custom);
        return;
    }
    // The older sanitizer replaced an unusable frequency with its region default.
    if (!loRaFrequencyBand(link.hz)) link.hz = LEGACY_DEFAULT_HZ[storedRegion];
    const Region& region = *findRegion(static_cast<uint8_t>(storedRegion));
    const uint32_t bandwidthHz = RadioBandwidth::normalize(link.bandwidthHz);
    const bool fits = bandwidthHz <= region.maxBandwidthHz && subBandFor(region, link.hz, bandwidthHz);
    link.region = fits ? static_cast<uint8_t>(storedRegion) : regionValue(RegionId::Custom);
}

size_t formatMhz(uint32_t hz, char* out, size_t size) {
    char fraction[8];
    snprintf(fraction, sizeof(fraction), "%06lu", static_cast<unsigned long>(hz % 1000000UL));
    int digits = 6;
    while (digits > 0 && fraction[digits - 1] == '0') --digits;
    fraction[digits] = '\0';
    const int written = digits
        ? snprintf(out, size, "%lu.%s", static_cast<unsigned long>(hz / 1000000UL), fraction)
        : snprintf(out, size, "%lu", static_cast<unsigned long>(hz / 1000000UL));
    return finish(written, size);
}

size_t formatLink(const Link& link, char* out, size_t size) {
    char mhz[16];
    formatMhz(link.hz, mhz, sizeof(mhz));
    return finish(snprintf(out, size, "%s MHz %s SF%u CR%u", mhz, bandwidthLabel(link.bandwidthHz),
                           static_cast<unsigned>(link.sf), static_cast<unsigned>(link.cr)), size);
}

size_t formatRules(const Link& link, const BoardCaps& caps, char* out, size_t size) {
    const Region& region = regionOf(link);
    const SubBand* band = subBandFor(region, link.hz, link.bandwidthHz);
    if (!hasLimits(region) || !band) return finish(snprintf(out, size, "No region limits"), size);
    char low[16], high[16], width[16] = "", duty[24] = "";
    formatMhz(band->minHz, low, sizeof(low));
    formatMhz(band->maxHz, high, sizeof(high));
    if (region.maxBandwidthHz < RadioBandwidth::values[RadioBandwidth::count - 1].hz) {
        snprintf(width, sizeof(width), ", <=%s", bandwidthLabel(region.maxBandwidthHz));
    }
    if (band->dutyPermille < 1000) {
        if (band->dutyPermille % 10 == 0) {
            snprintf(duty, sizeof(duty), ", %u%% duty", static_cast<unsigned>(band->dutyPermille / 10));
        } else {
            snprintf(duty, sizeof(duty), ", %u.%u%% duty", static_cast<unsigned>(band->dutyPermille / 10),
                     static_cast<unsigned>(band->dutyPermille % 10));
        }
    }
    return finish(snprintf(out, size, "%s-%s MHz%s, %d dBm%s", low, high, width,
                           static_cast<int>(txCapDbm(link, caps)), duty), size);
}

} // namespace policy
} // namespace radio

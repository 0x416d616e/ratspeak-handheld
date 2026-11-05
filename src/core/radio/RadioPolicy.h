#pragma once

#include <stddef.h>
#include <stdint.h>
#include "radio/RadioPresets.h"
#include "radio/RadioRegions.h"

// The only place that decides what a LoRa link may be. Pure (no Arduino), so
// the host tests in tests/host cover it directly.
//
// conform() is the single clamp: settings load and save call it, and it is
// idempotent. The settings screens change the link only through
// selectRegion(), selectPreset() and edit(), which say what they changed or
// why they refused, so the UI can tell the user instead of silently moving
// values.
namespace radio {

struct Link {
    uint8_t region;
    uint32_t hz;
    uint8_t sf;
    uint32_t bandwidthHz;
    uint8_t cr;
    int8_t txDbm;
    uint8_t preamble;
};

struct BoardCaps {
    int8_t minTxDbm;
    int8_t maxTxDbm;
    int8_t defaultTxDbm;
    int8_t antennaGainDbi;   // Assumed, to turn ERP/EIRP limits into a conducted cap.
    uint32_t fallbackHz;     // Tune here when a Custom link has no usable frequency.
};

enum Change : uint16_t {
    ChangedRegion = 1, ChangedFrequency = 2, ChangedBandwidth = 4, ChangedSpreading = 8,
    ChangedCoding = 16, ChangedTxPower = 32, ChangedPreamble = 64,
};

enum class Field : uint8_t { Frequency, Bandwidth, Spreading, Coding, TxPower, Preamble };
enum class Verdict : uint8_t { Ok, Adjusted, Refused };

struct Outcome {
    Verdict verdict;
    uint16_t changed;        // Change bits beyond the field or choice itself.
    const char* reason;      // Set when refused.
};

namespace policy {

// The region to apply limits from; unknown IDs behave as Custom.
const Region& regionOf(const Link& link);
bool hasLimits(const Region& region);
bool regionSelectable(const Region& region, bool devMode);

// The sub-band holding the whole signal, or nullptr.
const SubBand* subBandFor(const Region& region, uint32_t hz, uint32_t bandwidthHz);
int8_t conductedLimitDbm(const SubBand& band, int8_t antennaGainDbi);
int8_t txCapDbm(const Link& link, const BoardCaps& caps);

bool presetAllowed(uint8_t region, PresetId id);
int matchPreset(const Link& link);          // Exact SF/BW/CR; -1 when none.
const char* presetName(const Link& link);   // "Custom" when none.

uint16_t conform(Link& link, const BoardCaps& caps);

// Applies the region's default frequency, preset and TX power (capped).
// Custom keeps the current link.
Outcome selectRegion(Link& link, uint8_t region, const BoardCaps& caps);
Outcome selectPreset(Link& link, PresetId id, const BoardCaps& caps);
// Refuses a value the region would have to change; reports other fields it moved.
Outcome edit(Link& link, Field field, int32_t value, const BoardCaps& caps);

// Settings saved before regions had limits (no "radio_schema" key). Keeps the
// saved link working: a link that does not fit its old region becomes Custom.
void migrateLegacy(Link& link, int32_t storedRegion);

// ASCII text for the settings screens.
size_t formatLink(const Link& link, char* out, size_t size);    // "866.600 MHz 125k SF9 CR5"
size_t formatRules(const Link& link, const BoardCaps& caps, char* out, size_t size);
size_t formatMhz(uint32_t hz, char* out, size_t size);          // "868.6", "869.525"

} // namespace policy
} // namespace radio

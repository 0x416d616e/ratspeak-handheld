#pragma once

// Firmware glue between the saved settings (UserScalars) and the pure radio
// policy. Every settings screen and first-setup flow changes the radio link
// through these helpers, so the region rules apply the same way everywhere.

#include "config/BoardConfig.h"
#include "config/UserConfig.h"
#include "radio/RadioPolicy.h"

#ifndef LORA_MIN_TX_POWER
#define LORA_MIN_TX_POWER -9
#endif
#ifndef LORA_MAX_TX_POWER
#define LORA_MAX_TX_POWER 22
#endif
#ifndef LORA_ANTENNA_GAIN_DBI
#define LORA_ANTENNA_GAIN_DBI 2   // Assumed for stock antennas; converts ERP/EIRP limits.
#endif

inline constexpr radio::BoardCaps BOARD_RADIO_CAPS = {
    LORA_MIN_TX_POWER, LORA_MAX_TX_POWER, LORA_DEFAULT_TX_POWER, LORA_ANTENNA_GAIN_DBI,
    LORA_DEFAULT_FREQ,
};

static_assert(LORA_DEFAULT_SF == radio::preset(radio::DEFAULT_PRESET).sf &&
              LORA_DEFAULT_BW == radio::preset(radio::DEFAULT_PRESET).bandwidthHz &&
              LORA_DEFAULT_CR == radio::preset(radio::DEFAULT_PRESET).cr &&
              LORA_DEFAULT_PREAMBLE == radio::DEFAULT_PREAMBLE,
              "Board LoRa defaults must equal the default link preset");

inline radio::Link radioLink(const UserScalars& s) {
    const long preamble = s.loraPreamble < 0 ? 0 : (s.loraPreamble > 255 ? 255 : s.loraPreamble);
    return radio::Link{s.radioRegion, s.loraFrequency, s.loraSF, s.loraBW, s.loraCR, s.loraTxPower,
                       static_cast<uint8_t>(preamble)};
}

inline void setRadioLink(UserScalars& s, const radio::Link& link) {
    s.radioRegion = link.region;
    s.loraFrequency = link.hz;
    s.loraSF = link.sf;
    s.loraBW = link.bandwidthHz;
    s.loraCR = link.cr;
    s.loraTxPower = link.txDbm;
    s.loraPreamble = link.preamble;
}

inline uint16_t conformRadio(UserScalars& s) {
    radio::Link link = radioLink(s);
    const uint16_t changed = radio::policy::conform(link, BOARD_RADIO_CAPS);
    setRadioLink(s, link);
    return changed;
}

// Each action leaves the settings untouched when it refuses.
inline radio::Outcome selectRadioRegion(UserScalars& s, uint8_t region) {
    radio::Link link = radioLink(s);
    const radio::Outcome outcome = radio::policy::selectRegion(link, region, BOARD_RADIO_CAPS);
    if (outcome.verdict != radio::Verdict::Refused) setRadioLink(s, link);
    return outcome;
}

inline radio::Outcome selectRadioPreset(UserScalars& s, radio::PresetId id) {
    radio::Link link = radioLink(s);
    const radio::Outcome outcome = radio::policy::selectPreset(link, id, BOARD_RADIO_CAPS);
    if (outcome.verdict != radio::Verdict::Refused) setRadioLink(s, link);
    return outcome;
}

inline radio::Outcome editRadio(UserScalars& s, radio::Field field, int32_t value) {
    radio::Link link = radioLink(s);
    const radio::Outcome outcome = radio::policy::edit(link, field, value, BOARD_RADIO_CAPS);
    if (outcome.verdict != radio::Verdict::Refused) setRadioLink(s, link);
    return outcome;
}

inline const radio::Region& radioRegionOf(const UserScalars& s) {
    return radio::policy::regionOf(radioLink(s));
}

inline const char* radioPresetName(const UserScalars& s) {
    return radio::policy::presetName(radioLink(s));
}

inline int8_t radioTxCapDbm(const UserScalars& s) {
    return radio::policy::txCapDbm(radioLink(s), BOARD_RADIO_CAPS);
}

inline size_t formatRadioLink(const UserScalars& s, char* out, size_t size) {
    return radio::policy::formatLink(radioLink(s), out, size);
}

inline size_t formatRadioRules(const UserScalars& s, char* out, size_t size) {
    return radio::policy::formatRules(radioLink(s), BOARD_RADIO_CAPS, out, size);
}

// A short message for an outcome's side effects, or nullptr when there were
// none worth telling the user about.
inline const char* radioOutcomeNote(const radio::Outcome& outcome) {
    if (outcome.verdict == radio::Verdict::Refused) return outcome.reason;
    if (outcome.changed & radio::ChangedFrequency) return "Frequency moved to stay in band";
    if (outcome.changed & radio::ChangedBandwidth) return "Bandwidth narrowed for region";
    if (outcome.changed & radio::ChangedTxPower) return "TX power capped for region";
    return nullptr;
}

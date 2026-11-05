// Host tests for the radio region and link-preset policy.
// Build and run with tests/host/run.sh (plain g++, no ESP32 toolchain).
#include "radio/RadioPolicy.h"
#include "radio/RadioBandwidth.h"
#include "radio/RadioFrequency.h"

#include <stdio.h>
#include <string.h>

using namespace radio;

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_STR(actual, expected) do { ++checks; if (strcmp((actual), (expected)) != 0) { ++failures; \
    fprintf(stderr, "%s:%d: got \"%s\", expected \"%s\"\n", __FILE__, __LINE__, (actual), (expected)); } } while (0)

static constexpr BoardCaps CAPS = {-9, 22, 17, 2, 915000000UL};

static uint8_t id(RegionId region) { return regionValue(region); }

static Link link(RegionId region, uint32_t hz, uint8_t sf, uint32_t bw, uint8_t cr, int8_t tx, uint8_t pre = 18) {
    return Link{id(region), hz, sf, bw, cr, tx, pre};
}

static bool same(const Link& a, const Link& b) {
    return a.region == b.region && a.hz == b.hz && a.sf == b.sf && a.bandwidthHz == b.bandwidthHz &&
           a.cr == b.cr && a.txDbm == b.txDbm && a.preamble == b.preamble;
}

static void testPresetTableIsFrozen() {
    struct Expected { const char* name; uint8_t sf; uint32_t bw; uint8_t cr; };
    static const Expected expected[] = {
        {"Short Turbo", 7, 500000, 5}, {"Short Fast", 7, 250000, 5}, {"Short Slow", 8, 250000, 5},
        {"Medium Fast", 9, 250000, 5}, {"Medium Slow", 10, 250000, 5}, {"Long Turbo", 11, 500000, 8},
        {"Long Fast", 11, 250000, 5}, {"Long Moderate", 11, 125000, 8}, {"Lite Fast", 9, 125000, 5},
        {"Lite Slow", 10, 125000, 5},
    };
    CHECK(PRESET_COUNT == int(sizeof(expected) / sizeof(expected[0])));
    for (int i = 0; i < PRESET_COUNT; ++i) {
        CHECK(int(PRESETS[i].id) == i);
        CHECK_STR(PRESETS[i].name, expected[i].name);
        CHECK(PRESETS[i].sf == expected[i].sf);
        CHECK(PRESETS[i].bandwidthHz == expected[i].bw);
        CHECK(PRESETS[i].cr == expected[i].cr);
    }
}

static void testRegionTable() {
    // Persisted IDs are permanent.
    CHECK(id(RegionId::Americas) == 0 && id(RegionId::Europe) == 1 && id(RegionId::AuNz) == 2);
    CHECK(id(RegionId::Asia) == 3 && id(RegionId::India) == 4 && id(RegionId::Custom) == 254);
    CHECK(REGIONS[REGION_COUNT - 1].id == RegionId::Custom);
    for (int i = 0; i < REGION_COUNT; ++i) {
        const Region& region = REGIONS[i];
        CHECK(findRegion(id(region.id)) == &region);
        for (int j = i + 1; j < REGION_COUNT; ++j) CHECK(REGIONS[j].id != region.id);
        CHECK(strlen(region.label) <= 18);
        for (const char* c = region.label; *c; ++c) CHECK(*c >= 0x20 && *c < 0x7f);
        if (!policy::hasLimits(region)) continue;
        CHECK(policy::presetAllowed(id(region.id), region.defaultPreset));
        for (uint8_t b = 0; b < region.bandCount; ++b) {
            const SubBand& band = region.bands[b];
            CHECK(band.minHz < band.maxHz);
            // Every legal sub-band must be tunable by the SX1262 driver.
            const LoRaFrequencyBand* cal = loRaFrequencyBand(band.minHz);
            CHECK(cal && band.maxHz <= cal->maxHz);
        }
        // The default frequency must fit every preset the region allows, so
        // selecting a region and then any offered preset never moves it.
        for (int p = 0; p < PRESET_COUNT; ++p) {
            if (!policy::presetAllowed(id(region.id), PresetId(p))) continue;
            CHECK(policy::subBandFor(region, region.defaultHz, PRESETS[p].bandwidthHz) != nullptr);
        }
    }
}

static void testPowerLimits() {
    // 25 mW e.r.p. is 13.98 dBm; ERP adds 2.15 dB over a 2 dBi antenna's EIRP.
    // Caps keep a 1 dB tolerance margin and round down.
    const SubBand erp25 = {865000000, 868000000, LIMIT_25_MW, PowerRef::Erp, 10};
    CHECK(policy::conductedLimitDbm(erp25, 0) == 15);   // 13.98 - 1 + 2.15 = 15.13
    CHECK(policy::conductedLimitDbm(erp25, 2) == 13);   // 13.13
    CHECK(policy::conductedLimitDbm(erp25, 5) == 10);   // 10.13
    const SubBand eirp1w = {915000000, 928000000, LIMIT_1_W, PowerRef::Eirp, 1000};
    CHECK(policy::conductedLimitDbm(eirp1w, 2) == 27);
    const SubBand conducted100 = {920000000, 925000000, LIMIT_100_MW, PowerRef::Conducted, 1000};
    CHECK(policy::conductedLimitDbm(conducted100, 6) == 19);  // gain does not change a conducted limit
    const SubBand tiny = {865000000, 868000000, -50, PowerRef::Eirp, 10};
    CHECK(policy::conductedLimitDbm(tiny, 2) == -4);    // -3.5 rounds down
    // With a 2 dBi antenna the India cap stays under 25 mW e.r.p. with margin:
    // 13 dBm + 2 dBi - 2.15 = 12.85 dBm e.r.p. = 19.3 mW.
    CHECK(13 * 100 + 200 - 215 <= LIMIT_25_MW - TX_MARGIN_CENTI_DB);
}

static void testIndia() {
    Link l = link(RegionId::Americas, 915000000, 11, 250000, 5, 22);
    const Outcome outcome = policy::selectRegion(l, id(RegionId::India), CAPS);
    CHECK(outcome.verdict == Verdict::Adjusted);
    CHECK(same(l, link(RegionId::India, 866600000, 9, 125000, 5, 13)));
    CHECK_STR(policy::presetName(l), "Lite Fast");

    // Only presets at or below 125 kHz are offered.
    CHECK(policy::presetAllowed(id(RegionId::India), PresetId::LiteFast));
    CHECK(policy::presetAllowed(id(RegionId::India), PresetId::LiteSlow));
    CHECK(policy::presetAllowed(id(RegionId::India), PresetId::LongModerate));
    CHECK(!policy::presetAllowed(id(RegionId::India), PresetId::MediumFast));
    CHECK(!policy::presetAllowed(id(RegionId::India), PresetId::ShortTurbo));
    CHECK(policy::selectPreset(l, PresetId::LongFast, CAPS).verdict == Verdict::Refused);
    CHECK(policy::selectPreset(l, PresetId::LiteSlow, CAPS).verdict == Verdict::Ok);
    CHECK(l.sf == 10 && l.bandwidthHz == 125000 && l.txDbm == 13);

    // Manual edits that would leave the band or the limits are refused.
    Link before = l;
    CHECK(policy::edit(l, Field::TxPower, 14, CAPS).verdict == Verdict::Refused);
    CHECK(policy::edit(l, Field::Frequency, 865000000, CAPS).verdict == Verdict::Refused);
    CHECK(policy::edit(l, Field::Frequency, 868000000, CAPS).verdict == Verdict::Refused);
    CHECK(policy::edit(l, Field::Frequency, 867950000, CAPS).verdict == Verdict::Refused);
    CHECK(policy::edit(l, Field::Bandwidth, 250000, CAPS).verdict == Verdict::Refused);
    CHECK(same(l, before));
    CHECK(policy::edit(l, Field::Frequency, 865062500, CAPS).verdict == Verdict::Ok);
    CHECK(l.hz == 865062500);
    // G.S.R. 853(E) covers 865-868 MHz.
    CHECK(policy::edit(l, Field::Frequency, 867937500, CAPS).verdict == Verdict::Ok);
    CHECK(policy::edit(l, Field::Frequency, 867000000, CAPS).verdict == Verdict::Ok);
    CHECK(policy::edit(l, Field::Bandwidth, 62500, CAPS).verdict == Verdict::Ok);
    CHECK(policy::edit(l, Field::TxPower, 10, CAPS).verdict == Verdict::Ok);

    char text[64];
    Link shown = link(RegionId::India, 866600000, 9, 125000, 5, 13);
    policy::formatLink(shown, text, sizeof(text));
    CHECK_STR(text, "866.6 MHz 125k SF9 CR5");
    policy::formatRules(shown, CAPS, text, sizeof(text));
    CHECK_STR(text, "865-868 MHz, <=125k, 13 dBm, 1% duty");
}

static void testEurope() {
    Link l = link(RegionId::Americas, 915000000, 9, 250000, 5, 17);
    policy::selectRegion(l, id(RegionId::Europe), CAPS);
    CHECK(same(l, link(RegionId::Europe, 868000000, 9, 250000, 5, 13)));
    char text[64];
    policy::formatRules(l, CAPS, text, sizeof(text));
    CHECK_STR(text, "865-868.6 MHz, 13 dBm, 1% duty");

    // 869.525 MHz is the 500 mW sub-band: the radio's own 22 dBm is allowed.
    CHECK(policy::edit(l, Field::Frequency, 869525000, CAPS).verdict == Verdict::Ok);
    CHECK(policy::edit(l, Field::TxPower, 22, CAPS).verdict == Verdict::Ok);
    policy::formatRules(l, CAPS, text, sizeof(text));
    CHECK_STR(text, "869.4-869.65 MHz, 22 dBm, 10% duty");
    // Moving back lowers the power and says so.
    const Outcome back = policy::edit(l, Field::Frequency, 868000000, CAPS);
    CHECK(back.verdict == Verdict::Adjusted && (back.changed & ChangedTxPower) && l.txDbm == 13);
    // A 500 kHz preset does not fit 869.4-869.65 and moves to the default.
    CHECK(policy::edit(l, Field::Frequency, 869525000, CAPS).verdict == Verdict::Ok);
    const Outcome turbo = policy::selectPreset(l, PresetId::ShortTurbo, CAPS);
    CHECK(turbo.verdict == Verdict::Adjusted && (turbo.changed & ChangedFrequency) && l.hz == 868000000);
    // Between sub-bands is refused.
    CHECK(policy::edit(l, Field::Frequency, 869300000, CAPS).verdict == Verdict::Refused);
}

static void testOtherRegions() {
    Link l = link(RegionId::Americas, 915000000, 9, 250000, 5, 22);
    CHECK(policy::conform(l, CAPS) == 0);
    policy::selectRegion(l, id(RegionId::AuNz), CAPS);
    CHECK(same(l, link(RegionId::AuNz, 925875000, 9, 250000, 5, 17)));
    CHECK(policy::edit(l, Field::Frequency, 915000000, CAPS).verdict == Verdict::Refused);
    policy::selectRegion(l, id(RegionId::Asia), CAPS);
    CHECK(l.hz == 923000000 && l.txDbm == 17);
    CHECK(policy::edit(l, Field::TxPower, 20, CAPS).verdict == Verdict::Refused);
    CHECK(policy::edit(l, Field::TxPower, 19, CAPS).verdict == Verdict::Ok);

    // Custom keeps the link and only applies hardware limits.
    Link custom = link(RegionId::India, 866600000, 9, 125000, 5, 14);
    CHECK(policy::selectRegion(custom, id(RegionId::Custom), CAPS).verdict == Verdict::Ok);
    CHECK(custom.region == id(RegionId::Custom) && custom.hz == 866600000);
    CHECK(policy::edit(custom, Field::Frequency, 433500000, CAPS).verdict == Verdict::Ok);
    CHECK(policy::edit(custom, Field::TxPower, 22, CAPS).verdict == Verdict::Ok);
    CHECK(policy::edit(custom, Field::Frequency, 600000000, CAPS).verdict == Verdict::Refused);
    CHECK(policy::selectPreset(custom, PresetId::ShortTurbo, CAPS).verdict == Verdict::Ok);
    char text[64];
    policy::formatRules(custom, CAPS, text, sizeof(text));
    CHECK_STR(text, "No region limits");

    // Custom is selectable only in Developer Mode.
    CHECK(!policy::regionSelectable(*findRegion(id(RegionId::Custom)), false));
    CHECK(policy::regionSelectable(*findRegion(id(RegionId::Custom)), true));
    CHECK(policy::regionSelectable(*findRegion(id(RegionId::India)), false));
    CHECK(policy::selectRegion(custom, 77, CAPS).verdict == Verdict::Refused);
}

static void testConformIdempotentAndStable() {
    static const uint8_t regions[] = {0, 1, 2, 3, 4, 254, 9};
    static const uint32_t freqs[] = {433000000, 600000000, 863000000, 865000000, 865062500, 866000000,
        866600000, 867000000, 868000000, 868650000, 869300000, 869525000, 869650000, 902000000,
        914900000, 915000000, 920000000, 923000000, 925875000, 928000000, 960000000};
    static const uint32_t bws[] = {7800, 62500, 100000, 125000, 250000, 500000, 900000};
    static const uint8_t sfs[] = {4, 5, 9, 12, 13};
    static const uint8_t crs[] = {4, 5, 8, 9};
    static const int8_t txs[] = {-20, -9, 14, 17, 22, 30};
    static const uint8_t pres[] = {0, 6, 18, 65, 90};
    int cases = 0;
    for (uint8_t region : regions) for (uint32_t hz : freqs) for (uint32_t bw : bws) for (uint8_t sf : sfs)
    for (uint8_t cr : crs) for (int8_t tx : txs) for (uint8_t pre : pres) {
        Link l{region, hz, sf, bw, cr, tx, pre};
        policy::conform(l, CAPS);
        Link again = l;
        CHECK(policy::conform(again, CAPS) == 0);
        CHECK(same(again, l));
        // A conformed link is always within the region's rules.
        const Region& r = policy::regionOf(l);
        CHECK(findRegion(l.region) != nullptr);
        if (policy::hasLimits(r)) {
            CHECK(policy::subBandFor(r, l.hz, l.bandwidthHz) != nullptr);
            CHECK(l.bandwidthHz <= r.maxBandwidthHz);
        } else {
            CHECK(loRaFrequencyBand(l.hz) != nullptr);
        }
        CHECK(l.txDbm >= CAPS.minTxDbm && l.txDbm <= policy::txCapDbm(l, CAPS));
        CHECK(RadioBandwidth::normalize(l.bandwidthHz) == l.bandwidthHz);
        ++cases;
    }
    printf("%d conform cases\n", cases);
}

static void testSelectRegionIsDeterministic() {
    for (int i = 0; i < REGION_COUNT; ++i) {
        Link a = link(RegionId::Americas, 915000000, 11, 250000, 5, 22);
        Link b = link(RegionId::Custom, 433000000, 7, 62500, 8, -3, 40);
        policy::selectRegion(a, id(REGIONS[i].id), CAPS);
        policy::selectRegion(b, id(REGIONS[i].id), CAPS);
        if (policy::hasLimits(REGIONS[i])) CHECK(same(a, b));
        Link again = a;
        CHECK(policy::conform(again, CAPS) == 0);
    }
}

static void testMigration() {
    // Legacy records have no "radio_schema"; their links must keep working.
    Link india = link(RegionId::India, 866000000, 9, 125000, 5, 22);   // this branch's earlier India
    policy::migrateLegacy(india, 4);
    CHECK(india.region == id(RegionId::India) && india.hz == 866000000);
    CHECK(policy::conform(india, CAPS) == ChangedTxPower && india.txDbm == 13 && india.hz == 866000000);

    Link europe = link(RegionId::Americas, 868000000, 9, 250000, 5, 17);
    policy::migrateLegacy(europe, 1);
    CHECK(europe.region == id(RegionId::Europe));
    CHECK(policy::conform(europe, CAPS) == ChangedTxPower && europe.txDbm == 13);

    Link oldAustralia = link(RegionId::Americas, 915000000, 9, 250000, 5, 17);
    policy::migrateLegacy(oldAustralia, 2);   // spills below 915 MHz
    CHECK(oldAustralia.region == id(RegionId::Custom));
    CHECK(policy::conform(oldAustralia, CAPS) == 0 && oldAustralia.hz == 915000000);

    Link manual = link(RegionId::Americas, 868100000, 9, 250000, 5, 17);
    policy::migrateLegacy(manual, 0);
    CHECK(manual.region == id(RegionId::Custom) && manual.hz == 868100000);

    Link americas = link(RegionId::Americas, 915000000, 11, 250000, 5, 22);
    policy::migrateLegacy(americas, 0);
    CHECK(americas.region == id(RegionId::Americas));
    CHECK(policy::conform(americas, CAPS) == 0);

    Link unusable = link(RegionId::Americas, 600000000, 9, 250000, 5, 17);
    policy::migrateLegacy(unusable, 3);
    CHECK(unusable.region == id(RegionId::Asia) && unusable.hz == 923000000);

    Link unknown = link(RegionId::Americas, 915000000, 9, 250000, 5, 17);
    policy::migrateLegacy(unknown, 9);
    CHECK(unknown.region == id(RegionId::Custom) && unknown.hz == 915000000);
    policy::migrateLegacy(unknown, -1);
    CHECK(unknown.region == id(RegionId::Custom));
}

static void testFormatting() {
    char text[32];
    policy::formatMhz(868600000, text, sizeof(text)); CHECK_STR(text, "868.6");
    policy::formatMhz(869525000, text, sizeof(text)); CHECK_STR(text, "869.525");
    policy::formatMhz(865062500, text, sizeof(text)); CHECK_STR(text, "865.0625");
    policy::formatMhz(915000000, text, sizeof(text)); CHECK_STR(text, "915");
    char tiny[8];
    CHECK(policy::formatLink(link(RegionId::India, 866600000, 9, 125000, 5, 14), tiny, sizeof(tiny)) == 7);
    CHECK(strlen(tiny) == 7);
    char rules[64];
    for (int i = 0; i < REGION_COUNT; ++i) {
        Link l = link(RegionId::Americas, 915000000, 9, 250000, 5, 17);
        policy::selectRegion(l, id(REGIONS[i].id), CAPS);
        policy::formatRules(l, CAPS, rules, sizeof(rules));
        CHECK(strlen(rules) <= 40);  // fits one Cardputer row
    }
}

int main() {
    testPresetTableIsFrozen();
    testRegionTable();
    testPowerLimits();
    testIndia();
    testEurope();
    testOtherRegions();
    testConformIdempotentAndStable();
    testSelectRegionIsDeterministic();
    testMigration();
    testFormatting();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

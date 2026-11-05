#pragma once

#include "Screen.h"
#include "widgets/ScrollList.h"
#include "config/Config.h"
#include "radio/RadioRegions.h"
#include <functional>

struct TimezoneEntry {
    const char* label;
    const char* posixTZ;
    int8_t baseOffset;
    radio::RegionId radioRegion;  // Region suggested during first setup
};

static const TimezoneEntry TIMEZONE_TABLE[] = {
    {"Honolulu (UTC-10)",     "HST10",                                    -10, radio::RegionId::Americas},
    {"Anchorage (UTC-9)",     "AKST9AKDT,M3.2.0,M11.1.0",               -9,  radio::RegionId::Americas},
    {"Los Angeles (UTC-8)",   "PST8PDT,M3.2.0,M11.1.0",                 -8,  radio::RegionId::Americas},
    {"Denver (UTC-7)",        "MST7MDT,M3.2.0,M11.1.0",                 -7,  radio::RegionId::Americas},
    {"Phoenix (UTC-7)",       "MST7",                                    -7,  radio::RegionId::Americas},
    {"Chicago (UTC-6)",       "CST6CDT,M3.2.0,M11.1.0",                 -6,  radio::RegionId::Americas},
    {"New York (UTC-5)",      "EST5EDT,M3.2.0,M11.1.0",                  -5,  radio::RegionId::Americas},
    {"San Juan (UTC-4)",      "AST4",                                     -4,  radio::RegionId::Americas},
    {"Sao Paulo (UTC-3)",     "<-03>3",                                   -3,  radio::RegionId::Americas},
    {"London (UTC+0)",        "GMT0BST,M3.5.0/1,M10.5.0",                0,   radio::RegionId::Europe},
    {"Paris (UTC+1)",         "CET-1CEST,M3.5.0,M10.5.0/3",              1,   radio::RegionId::Europe},
    {"Cairo (UTC+2)",         "EET-2",                                    2,   radio::RegionId::Europe},
    {"Moscow (UTC+3)",        "MSK-3",                                    3,   radio::RegionId::Europe},
    {"Dubai (UTC+4)",         "<+04>-4",                                  4,   radio::RegionId::Europe},
    {"Karachi (UTC+5)",       "PKT-5",                                    5,   radio::RegionId::Europe},
    {"Kolkata (UTC+5:30)",    "IST-5:30",                                 5,   radio::RegionId::India},
    {"Bangkok (UTC+7)",       "<+07>-7",                                  7,   radio::RegionId::Asia},
    {"Singapore (UTC+8)",     "<+08>-8",                                  8,   radio::RegionId::Asia},
    {"Tokyo (UTC+9)",         "JST-9",                                    9,   radio::RegionId::Asia},
    {"Sydney (UTC+10)",       "AEST-10AEDT,M10.1.0,M4.1.0/3",           10,   radio::RegionId::AuNz},
    {"Auckland (UTC+12)",     "NZST-12NZDT,M9.5.0,M4.1.0/3",            12,   radio::RegionId::AuNz},
};

static constexpr int TIMEZONE_COUNT = sizeof(TIMEZONE_TABLE) / sizeof(TIMEZONE_TABLE[0]);

class TimezoneScreen : public Screen {
public:
    void render(M5Canvas& canvas) override;
    bool handleKey(const KeyEvent& event) override;
    void onEnter() override;
    const char* title() const override { return "Timezone"; }

    using DoneCb = std::function<void(int tzIndex)>;
    void setDoneCallback(DoneCb cb) { _doneCb = cb; }
    void setSelectedIndex(int idx) { _selectedIdx = idx; }

    void setSaveStatus(const char* status, bool pending) { _saveStatus = status; _savePending = pending; }

private:
    const char* _saveStatus = nullptr;
    bool _savePending = false;
    DoneCb _doneCb;
    ScrollList _list;
    int _selectedIdx = 0;  // Start at top of list
};

#pragma once
#include "config/Config.h"
#include "radio/RadioRegions.h"

struct TimezoneEntry {
    const char* label;      // Display name (e.g., "New York (EST/EDT)")
    const char* posixTZ;    // POSIX TZ string for setenv("TZ", ...)
    int8_t baseOffset;      // Base UTC offset for display (e.g., -5 for EST)
    radio::RegionId radioRegion;  // Region suggested during first setup
};

// Major world timezones with DST rules where applicable
static const TimezoneEntry TIMEZONE_TABLE[] = {
    {"Honolulu",              "HST10",                                    -10, radio::RegionId::Americas},
    {"Anchorage",             "AKST9AKDT,M3.2.0,M11.1.0",               -9,  radio::RegionId::Americas},
    {"Los Angeles",           "PST8PDT,M3.2.0,M11.1.0",                 -8,  radio::RegionId::Americas},
    {"Denver",                "MST7MDT,M3.2.0,M11.1.0",                 -7,  radio::RegionId::Americas},
    {"Phoenix",               "MST7",                                    -7,  radio::RegionId::Americas},
    {"Chicago",               "CST6CDT,M3.2.0,M11.1.0",                 -6,  radio::RegionId::Americas},
    {"New York",              "EST5EDT,M3.2.0,M11.1.0",                  -5,  radio::RegionId::Americas},
    {"San Juan",              "AST4",                                     -4,  radio::RegionId::Americas},
    {"Sao Paulo",             "<-03>3",                                   -3,  radio::RegionId::Americas},
    {"London",                "GMT0BST,M3.5.0/1,M10.5.0",                0,   radio::RegionId::Europe},
    {"Paris",                 "CET-1CEST,M3.5.0,M10.5.0/3",              1,   radio::RegionId::Europe},
    {"Cairo",                 "EET-2",                                    2,   radio::RegionId::Europe},
    {"Moscow",                "MSK-3",                                    3,   radio::RegionId::Europe},
    {"Dubai",                 "<+04>-4",                                  4,   radio::RegionId::Europe},
    {"Karachi",               "PKT-5",                                    5,   radio::RegionId::Europe},
    {"Kolkata",               "IST-5:30",                                 5,   radio::RegionId::India},
    {"Bangkok",               "<+07>-7",                                  7,   radio::RegionId::Asia},
    {"Singapore",             "<+08>-8",                                  8,   radio::RegionId::Asia},
    {"Tokyo",                 "JST-9",                                    9,   radio::RegionId::Asia},
    {"Sydney",                "AEST-10AEDT,M10.1.0,M4.1.0/3",           10,   radio::RegionId::AuNz},
    {"Auckland",              "NZST-12NZDT,M9.5.0,M4.1.0/3",            12,   radio::RegionId::AuNz},
};

static constexpr int TIMEZONE_COUNT = sizeof(TIMEZONE_TABLE) / sizeof(TIMEZONE_TABLE[0]);

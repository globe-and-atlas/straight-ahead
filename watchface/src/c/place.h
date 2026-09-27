#pragma once
#include <stdbool.h>
#include <stdint.h>

// The day ring: at `minute` minutes past local midnight the ring is `minute` nautical miles out
// (1 arcminute of great circle = 1 nmi), reaching the disc's rim (24 degrees) at midnight.
#define PLACE_DAY_NM 1440

// Label window around the point where the ring crosses the heading.
#define PLACE_WINDOW_NM 25
#define PLACE_WINDOW_DEG10 120  // +-12 degrees

// Region cells from the phone: 36 bearings (every 10 degrees) x 24 bands (every 60 nmi).
#define PLACE_BEARINGS 36
#define PLACE_BANDS 24

typedef struct {
  uint16_t nm;         // great-circle distance from the wearer
  uint16_t bearing10;  // initial bearing, tenths of a degree clockwise from north
  const char *name;    // not NUL-terminated
  uint8_t name_len;
} Place;

// Ring radius in pixels for a disc of `disc_r` pixels.
int place_ring_px(int minute, int disc_r);

// Smallest angle between two bearings, tenths of a degree (0..1800).
int place_delta10(int a10, int b10);

// Index of the city to name at `minute` facing `heading10`, or -1: within the window, smallest
// radial miss plus lateral miss (d * angle), both in nmi.
int place_pick(const Place *cities, int count, int minute, int heading10);

// Region cell index (bearing-major) for the ring point at `minute` facing `heading10`.
int place_cell(int minute, int heading10);

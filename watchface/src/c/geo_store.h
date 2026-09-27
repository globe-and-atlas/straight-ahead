#pragma once
#include <pebble.h>

#include "place.h"

// Receives the phone's projected map in chunks (see src/pkjs/index.js) and keeps it in static
// buffers. Coastlines are int8 x, y pairs (north up, 127 = the view's rim) with (-128, -128)
// lifting the pen.

#define GEO_PEN_UP -128
#define GEO_UNIT 127
#define GEO_MAX_CITIES 160

typedef enum { GEO_DAY_COAST = 0, GEO_WORLD_COAST, GEO_CITIES, GEO_CELLS, GEO_NAMES, GEO_KINDS } GeoKind;

typedef void (*GeoHandler)(void);

void geo_store_start(GeoHandler on_update);

bool geo_has_fix(void);
bool geo_ready(GeoKind kind);

// Coastline bytes for a view; *len is the byte count (pairs = len / 2).
const int8_t *geo_coast(bool world, int *len);
const Place *geo_cities(int *count);

// Name of a region cell, or NULL (unnamed or not yet received).
const char *geo_region(int cell, int *len);

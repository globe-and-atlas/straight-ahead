#include "geo_store.h"

#define COAST_BYTES 3600  // 1,500 points + pen-up markers
#define CITY_BYTES (GEO_MAX_CITIES * 27)
#define CELL_BYTES (PLACE_BEARINGS * PLACE_BANDS)
#define NAME_BYTES 3000
#define MAX_NAMES 255

static uint8_t s_day[COAST_BYTES], s_world[COAST_BYTES], s_city_raw[CITY_BYTES];
static uint8_t s_cells[CELL_BYTES], s_names_raw[NAME_BYTES];
static uint8_t *const BUFFERS[GEO_KINDS] = {s_day, s_world, s_city_raw, s_cells, s_names_raw};
static const uint16_t CAPACITY[GEO_KINDS] = {COAST_BYTES, COAST_BYTES, CITY_BYTES, CELL_BYTES, NAME_BYTES};

static uint16_t s_len[GEO_KINDS];
static bool s_ready[GEO_KINDS];
static bool s_fix;

static Place s_cities[GEO_MAX_CITIES];
static int s_city_count;
static uint16_t s_name_off[MAX_NAMES];
static uint8_t s_name_len[MAX_NAMES];
static int s_name_count;

static GeoHandler s_handler;

static void parse_cities(void) {
  s_city_count = 0;
  int i = 0;
  while (i + 5 <= s_len[GEO_CITIES] && s_city_count < GEO_MAX_CITIES) {
    const uint8_t *r = s_city_raw + i;
    uint8_t n = r[4];
    if (i + 5 + n > s_len[GEO_CITIES]) break;
    s_cities[s_city_count++] = (Place){
        .nm = (uint16_t)(r[0] | r[1] << 8),
        .bearing10 = (uint16_t)(r[2] | r[3] << 8),
        .name = (const char *)r + 5,
        .name_len = n,
    };
    i += 5 + n;
  }
}

static void parse_names(void) {
  s_name_count = 0;
  int start = 0;
  for (int i = 0; i < s_len[GEO_NAMES] && s_name_count < MAX_NAMES; i++) {
    if (s_names_raw[i] != 0) continue;
    s_name_off[s_name_count] = start;
    s_name_len[s_name_count] = i - start;
    s_name_count++;
    start = i + 1;
  }
}

static void on_message(DictionaryIterator *iter, void *context) {
  Tuple *lat = dict_find(iter, MESSAGE_KEY_LAT_E6);
  if (lat) {
    // A new fix starts a new map: forget the old one so views never mix two centres.
    s_fix = true;
    for (int k = 0; k < GEO_KINDS; k++) s_ready[k] = false;
  }
  Tuple *kind_t = dict_find(iter, MESSAGE_KEY_KIND);
  Tuple *off_t = dict_find(iter, MESSAGE_KEY_OFFSET);
  Tuple *total_t = dict_find(iter, MESSAGE_KEY_TOTAL);
  Tuple *data_t = dict_find(iter, MESSAGE_KEY_DATA);
  if (kind_t && off_t && total_t && data_t) {
    int kind = kind_t->value->int32;
    int off = off_t->value->int32;
    int total = total_t->value->int32;
    int n = data_t->length;
    if (kind < 0 || kind >= GEO_KINDS || total > CAPACITY[kind] || off < 0 || off + n > total) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "geo: dropped kind %d off %d n %d total %d", kind, off, n, total);
      return;
    }
    if (off == 0) s_ready[kind] = false;
    memcpy(BUFFERS[kind] + off, data_t->value->data, n);
    if (off + n == total) {
      s_len[kind] = total;
      if (kind == GEO_CITIES) parse_cities();
      if (kind == GEO_NAMES) parse_names();
      s_ready[kind] = true;
    }
  }
  if (s_handler) s_handler();
}

void geo_store_start(GeoHandler on_update) {
  s_handler = on_update;
  app_message_register_inbox_received(on_message);
  app_message_open(1200, 64);
}

bool geo_has_fix(void) { return s_fix; }

bool geo_ready(GeoKind kind) { return s_ready[kind]; }

const int8_t *geo_coast(bool world, int *len) {
  GeoKind k = world ? GEO_WORLD_COAST : GEO_DAY_COAST;
  *len = s_ready[k] ? s_len[k] : 0;
  return (const int8_t *)BUFFERS[k];
}

const Place *geo_cities(int *count) {
  *count = s_ready[GEO_CITIES] ? s_city_count : 0;
  return s_cities;
}

const char *geo_region(int cell, int *len) {
  if (!s_ready[GEO_CELLS] || !s_ready[GEO_NAMES] || cell < 0 || cell >= CELL_BYTES) return NULL;
  int idx = s_cells[cell];
  if (idx <= 0 || idx >= s_name_count || s_name_len[idx] == 0) return NULL;
  *len = s_name_len[idx];
  return (const char *)s_names_raw + s_name_off[idx];
}

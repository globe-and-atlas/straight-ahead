// Straight Ahead: the world in azimuthal-equidistant projection, centred on the wearer and turned
// to their heading. Two views:
//   Day ring (default)  rim = 24 degrees. A ring sits `minutes since midnight` nautical miles out
//                       and the label names what lies where it crosses the bearing you face.
//   World               rim = the antipode. The vertical is the great circle you face.
// The phone projects everything (src/pkjs); the watch only rotates and draws.
#include <pebble.h>

#include "config.h"
#include "geo_store.h"
#include "heading.h"
#include "place.h"

#define AMBER GColorChromeYellow

#define DISC_CX 100
#define DISC_CY 140
#define DISC_R 84
#define HALF_CIRCUMFERENCE_KM 20015

static Window *s_window;
static Layer *s_canvas;
static GFont s_time_font, s_small_font, s_label_font;

static Heading s_heading;
static int s_minute;  // minutes since local midnight
static char s_time[8];

static int heading10(void) {
  if (!s_heading.live || s_heading.status < CompassStatusCalibrating) return 0;
  return (int)((s_heading.heading * 3600 + TRIG_MAX_ANGLE / 2) / TRIG_MAX_ANGLE) % 3600;
}

// Screen point at `bearing` (TRIG units clockwise from north) and `radius` px, turned so the
// current heading points up.
static GPoint polar(int32_t bearing, int32_t radius) {
  int32_t a = bearing - s_heading.heading;
  return GPoint(DISC_CX + radius * sin_lookup(a) / TRIG_MAX_RATIO,
                DISC_CY - radius * cos_lookup(a) / TRIG_MAX_RATIO);
}

static void update_time(struct tm *t) {
  strftime(s_time, sizeof(s_time), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  if (!clock_is_24h_style() && s_time[0] == '0') memmove(s_time, s_time + 1, sizeof(s_time) - 1);
#if SA_TEST_MINUTE >= 0
  s_minute = SA_TEST_MINUTE;
#else
  s_minute = t->tm_hour * 60 + t->tm_min;
#endif
}

// ---- Render -------------------------------------------------------------------------------

static void draw_rings(GContext *ctx, bool world) {
  GPoint c = GPoint(DISC_CX, DISC_CY);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorOxfordBlue);
  if (world) {
    for (int km = 5000; km < HALF_CIRCUMFERENCE_KM; km += 5000) {
      graphics_draw_circle(ctx, c, DISC_R * km / HALF_CIRCUMFERENCE_KM);
    }
  } else {
    // 06:00, 12:00 and 18:00 rings: the day ring passes them on the hour.
    for (int m = 360; m < PLACE_DAY_NM; m += 360) graphics_draw_circle(ctx, c, place_ring_px(m, DISC_R));
  }
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_circle(ctx, c, DISC_R);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, polar(0, DISC_R - 5), polar(0, DISC_R + 2));  // north tick
}

static void draw_coast(GContext *ctx, bool world) {
  int len;
  const int8_t *pts = geo_coast(world, &len);
  int32_t s = sin_lookup(s_heading.heading), c = cos_lookup(s_heading.heading);
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 1);
  bool pen = false;
  GPoint prev = GPointZero;
  for (int i = 0; i + 1 < len; i += 2) {
    int32_t x = pts[i], y = pts[i + 1];
    if (x == GEO_PEN_UP && y == GEO_PEN_UP) {
      pen = false;
      continue;
    }
    // Rotate by -heading (north-up units, y down), then scale to the disc.
    int32_t rx = (x * c + y * s) / TRIG_MAX_RATIO;
    int32_t ry = (y * c - x * s) / TRIG_MAX_RATIO;
    GPoint p = GPoint(DISC_CX + rx * DISC_R / GEO_UNIT, DISC_CY + ry * DISC_R / GEO_UNIT);
    if (pen) graphics_draw_line(ctx, prev, p);
    prev = p;
    pen = true;
  }
}

static void draw_facing(GContext *ctx, bool world) {
  GPoint c = GPoint(DISC_CX, DISC_CY);
  graphics_context_set_stroke_width(ctx, 1);
  if (world) {
    // Through the centre every great circle is straight: ahead runs up to the antipode.
    graphics_context_set_stroke_color(ctx, GColorLiberty);
    graphics_draw_line(ctx, c, GPoint(DISC_CX, DISC_CY + DISC_R));
    graphics_context_set_stroke_color(ctx, GColorCyan);
    graphics_context_set_stroke_width(ctx, 2);
  } else {
    graphics_context_set_stroke_color(ctx, GColorLiberty);
  }
  graphics_draw_line(ctx, c, GPoint(DISC_CX, DISC_CY - DISC_R));
}

// Returns the picked city index (or -1) so the label agrees with the dot.
static int draw_day_ring(GContext *ctx) {
  int r = place_ring_px(s_minute, DISC_R);
  graphics_context_set_stroke_color(ctx, GColorCyan);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(DISC_CX, DISC_CY), r);

  int count;
  const Place *cities = geo_cities(&count);
  int pick = place_pick(cities, count, s_minute, heading10());
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < count; i++) {
    int miss = cities[i].nm - s_minute;
    if (miss < -PLACE_WINDOW_NM || miss > PLACE_WINDOW_NM || i == pick) continue;
    graphics_fill_circle(ctx, polar(cities[i].bearing10 * TRIG_MAX_ANGLE / 3600,
                                    place_ring_px(cities[i].nm, DISC_R)), 1);
  }
  graphics_context_set_fill_color(ctx, AMBER);
  GPoint mark = pick >= 0 ? polar(cities[pick].bearing10 * TRIG_MAX_ANGLE / 3600,
                                  place_ring_px(cities[pick].nm, DISC_R))
                          : GPoint(DISC_CX, DISC_CY - r);
  graphics_fill_circle(ctx, mark, 2);
  return pick;
}

static void draw_you(GContext *ctx) {
  GPoint c = GPoint(DISC_CX, DISC_CY);
  graphics_context_set_fill_color(ctx, AMBER);
  graphics_context_set_stroke_color(ctx, AMBER);
  graphics_context_set_stroke_width(ctx, 1);
  if (geo_has_fix()) {
    graphics_fill_circle(ctx, c, 3);
  } else {
    graphics_draw_circle(ctx, c, 3);
  }
}

static void draw_header(GContext *ctx, bool world, int pick) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_time, s_time_font, GRect(4, -2, 110, 34), GTextOverflowModeFill,
                     GTextAlignmentLeft, NULL);

  char hdg[12], dist[16];
  if (s_heading.live && s_heading.status >= CompassStatusCalibrating) {
    snprintf(hdg, sizeof(hdg), "HDG %03d%s", heading10() / 10,
             s_heading.status == CompassStatusCalibrating ? "?" : "");
  } else {
    snprintf(hdg, sizeof(hdg), !s_heading.live                               ? "N-UP"
                               : s_heading.status == CompassStatusUnavailable ? "NO CMPS"
                                                                              : "HDG ...");
  }
  if (world) {
    snprintf(dist, sizeof(dist), "RIM 180°");
  } else {
    snprintf(dist, sizeof(dist), "%d°%02d'", s_minute / 60, s_minute % 60);
  }
  graphics_context_set_text_color(ctx, AMBER);
  graphics_draw_text(ctx, hdg, s_small_font, GRect(104, 2, 92, 16), GTextOverflowModeFill,
                     GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, dist, s_small_font, GRect(104, 16, 92, 16), GTextOverflowModeFill,
                     GTextAlignmentRight, NULL);

  // Label row: the place on the ring ahead.
  char label[24];
  if (!geo_has_fix()) {
    snprintf(label, sizeof(label), "NO FIX");
  } else if (world) {
    snprintf(label, sizeof(label), "WORLD");
  } else {
    int count, len = 0;
    const Place *cities = geo_cities(&count);
    const char *name = NULL;
    if (pick >= 0) {
      name = cities[pick].name;
      len = cities[pick].name_len;
    } else {
      name = geo_region(place_cell(s_minute, heading10()), &len);
    }
    if (name) {
      snprintf(label, sizeof(label), "%.*s", len, name);
    } else {
      snprintf(label, sizeof(label), "%s", geo_ready(GEO_CELLS) ? "OPEN WATER" : "...");
    }
  }
  graphics_draw_text(ctx, label, s_label_font, GRect(2, 30, 196, 22), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

#if SA_DEBUG
static void draw_debug(GContext *ctx, int pick) {
  char a[32];
  int count;
  geo_cities(&count);
  snprintf(a, sizeof(a), "m%d h%d c%d p%d s%d", s_minute, heading10(), count, pick,
           (int)s_heading.status);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, a, s_small_font, GRect(2, 212, 160, 16), GTextOverflowModeFill,
                     GTextAlignmentLeft, NULL);
}
#endif

static void canvas_update(Layer *layer, GContext *ctx) {
  bool world = s_heading.world;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_antialiased(ctx, false);
  draw_rings(ctx, world);
  draw_coast(ctx, world);
  draw_facing(ctx, world);
  int pick = world ? -1 : draw_day_ring(ctx);
  draw_you(ctx);
  draw_header(ctx, world, pick);
#if SA_DEBUG
  draw_debug(ctx, pick);
#endif
}

// ---- Events -------------------------------------------------------------------------------

static void on_heading(const Heading *h) {
  s_heading = *h;
#if SA_TEST_HEADING >= 0
  // Emulator fixture: the emery emulator has no compass, so freeze a calibrated heading.
  s_heading.heading = SA_TEST_HEADING * TRIG_MAX_ANGLE / 360;
  s_heading.live = true;
  s_heading.status = CompassStatusCalibrated;
#endif
  layer_mark_dirty(s_canvas);
}

static void on_geo(void) {
  layer_mark_dirty(s_canvas);
}

static void on_tick(struct tm *t, TimeUnits changed) {
  update_time(t);
  layer_mark_dirty(s_canvas);
}

// ---- Lifecycle ----------------------------------------------------------------------------

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
}

static void init(void) {
  s_time_font = fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS);
  s_small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_label_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  time_t now = time(NULL);
  update_time(localtime(&now));

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, on_tick);
  heading_start(on_heading);
#if SA_TEST_HEADING >= 0
  Heading fixture = {.world = false};
  on_heading(&fixture);
#endif
  geo_store_start(on_geo);
}

static void deinit(void) {
  heading_stop();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

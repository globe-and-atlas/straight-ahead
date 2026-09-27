// Straight Ahead: an azimuthal-equidistant world centred on the wearer and turned to their
// heading. Scaffold stage: disc, range rings, north tick and the facing great circle. Coastlines
// and the landfall label arrive from the phone in later milestones (see task.md).
#include <pebble.h>

#include "config.h"
#include "heading.h"

#define AMBER GColorChromeYellow

// The whole Earth fits the disc: its rim is the wearer's antipode, 20,015 km away.
#define DISC_CX 100
#define DISC_CY 130
#define DISC_R 92
#define HALF_CIRCUMFERENCE_KM 20015

static Window *s_window;
static Layer *s_canvas;
static GFont s_time_font;
static GFont s_small_font;

static Heading s_heading;
static bool s_has_fix;
static int32_t s_lat_e6, s_lon_e6;
static char s_time[8];

// Screen point at `bearing` (clockwise from north) and `radius` px, with the map turned so the
// current heading points up.
static GPoint polar(int32_t bearing, int32_t radius) {
  int32_t a = bearing - s_heading.heading;
  return GPoint(DISC_CX + radius * sin_lookup(a) / TRIG_MAX_RATIO,
                DISC_CY - radius * cos_lookup(a) / TRIG_MAX_RATIO);
}

static void update_time(struct tm *t) {
  strftime(s_time, sizeof(s_time), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  if (!clock_is_24h_style() && s_time[0] == '0') memmove(s_time, s_time + 1, sizeof(s_time) - 1);
}

// ---- Render -------------------------------------------------------------------------------

static void draw_disc(GContext *ctx) {
  GPoint c = GPoint(DISC_CX, DISC_CY);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorOxfordBlue);
  for (int km = 5000; km < HALF_CIRCUMFERENCE_KM; km += 5000) {
    graphics_draw_circle(ctx, c, DISC_R * km / HALF_CIRCUMFERENCE_KM);
  }
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_circle(ctx, c, DISC_R);

  // North tick on the rim: where true north lies once the map turns to the heading.
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, polar(0, DISC_R - 5), polar(0, DISC_R + 2));
}

// Through the centre of an azimuthal-equidistant map every great circle is a straight line, so
// the one you face is simply the vertical: ahead runs up to the antipode, behind runs down.
static void draw_great_circle(GContext *ctx) {
  GPoint c = GPoint(DISC_CX, DISC_CY);
  graphics_context_set_stroke_color(ctx, GColorLiberty);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, c, GPoint(DISC_CX, DISC_CY + DISC_R));
  graphics_context_set_stroke_color(ctx, GColorCyan);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, c, GPoint(DISC_CX, DISC_CY - DISC_R));

  graphics_context_set_fill_color(ctx, AMBER);
  graphics_context_set_stroke_color(ctx, AMBER);
  graphics_context_set_stroke_width(ctx, 1);
  if (s_has_fix) {
    graphics_fill_circle(ctx, c, 3);
  } else {
    graphics_draw_circle(ctx, c, 3);
  }
}

static void draw_header(GContext *ctx) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_time, s_time_font, GRect(4, -2, 110, 34), GTextOverflowModeFill,
                     GTextAlignmentLeft, NULL);

  char hdg[12];
  if (s_heading.live && s_heading.status >= CompassStatusCalibrating) {
    snprintf(hdg, sizeof(hdg), "HDG %03d%s", (int)(s_heading.heading * 360 / TRIG_MAX_ANGLE),
             s_heading.status == CompassStatusCalibrating ? "?" : "");
  } else {
    snprintf(hdg, sizeof(hdg), !s_heading.live                               ? "N-UP"
                               : s_heading.status == CompassStatusUnavailable ? "NO CMPS"
                                                                              : "HDG ...");
  }
  const char *second = s_has_fix ? "AHEAD --" : "NO FIX";
  graphics_context_set_text_color(ctx, AMBER);
  graphics_draw_text(ctx, hdg, s_small_font, GRect(104, 2, 92, 16), GTextOverflowModeFill,
                     GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, second, s_small_font, GRect(104, 16, 92, 16), GTextOverflowModeFill,
                     GTextAlignmentRight, NULL);
}

#if SA_DEBUG
static void draw_debug(GContext *ctx) {
  char a[32];
  snprintf(a, sizeof(a), "%ld %ld s%d", (long)(s_lat_e6 / 1000), (long)(s_lon_e6 / 1000),
           (int)s_heading.status);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, a, s_small_font, GRect(4, 210, 150, 16), GTextOverflowModeFill,
                     GTextAlignmentLeft, NULL);
}
#endif

static void canvas_update(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_antialiased(ctx, false);
  draw_disc(ctx);
  draw_great_circle(ctx);
  draw_header(ctx);
#if SA_DEBUG
  draw_debug(ctx);
#endif
}

// ---- Events -------------------------------------------------------------------------------

static void on_heading(const Heading *h) {
#if SA_TEST_HEADING >= 0
  return;
#endif
  s_heading = *h;
  layer_mark_dirty(s_canvas);
}

static void on_tick(struct tm *t, TimeUnits changed) {
  update_time(t);
  layer_mark_dirty(s_canvas);
}

static void on_message(DictionaryIterator *iter, void *context) {
  Tuple *lat = dict_find(iter, MESSAGE_KEY_LAT_E6);
  Tuple *lon = dict_find(iter, MESSAGE_KEY_LON_E6);
  if (!lat || !lon) return;
  s_lat_e6 = lat->value->int32;
  s_lon_e6 = lon->value->int32;
  s_has_fix = true;
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
  time_t now = time(NULL);
  update_time(localtime(&now));

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, on_tick);
  heading_start(on_heading);
#if SA_TEST_HEADING >= 0
  s_heading = (Heading){.heading = SA_TEST_HEADING * TRIG_MAX_ANGLE / 360, .live = true,
                        .status = CompassStatusCalibrated};
#endif
  app_message_register_inbox_received(on_message);
  app_message_open(128, 64);
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

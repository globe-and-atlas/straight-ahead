#include "heading.h"

#include "raise.h"

// A raise often registers as a flick too; flicks this soon after a session starts only extend it,
// so raising the wrist never toggles the world view by accident.
#define TAP_GRACE_MS 2000

static Heading s_heading;
static HeadingHandler s_handler;
static AppTimer *s_session;
static uint64_t s_session_start_ms;
static Raise s_raise;

static uint64_t now_ms(void) {
  time_t s;
  uint16_t ms;
  time_ms(&s, &ms);
  return (uint64_t)s * 1000 + ms;
}

static void notify(void) {
  if (s_handler) s_handler(&s_heading);
}

static void compass_handler(CompassHeadingData data) {
  s_heading.status = data.compass_status;
  // Unavailable (e.g. the emery emulator) and DataInvalid carry no usable heading: fall back to
  // north-up so the map, the label and the header never disagree.
  if (data.compass_status < CompassStatusCalibrating) {
    s_heading.heading = 0;
    notify();
    return;
  }
  // The SDK measures counter-clockwise; maps and bearings run clockwise.
  s_heading.heading = (TRIG_MAX_ANGLE - data.magnetic_heading) % TRIG_MAX_ANGLE;
  notify();
}

static void end_session(void *context) {
  s_session = NULL;
  compass_service_unsubscribe();
  s_heading.live = false;
  s_heading.world = false;
  s_heading.heading = 0;
  notify();
}

// Start a session, or extend the running one. Returns true if one was already running.
static bool start_or_extend(void) {
  if (s_session) {
    app_timer_reschedule(s_session, HEADING_SESSION_MS);
    return true;
  }
  s_session_start_ms = now_ms();
  s_heading.live = true;
  s_heading.status = CompassStatusDataInvalid;
  compass_service_set_heading_filter(HEADING_FILTER);
  compass_service_subscribe(compass_handler);
  s_session = app_timer_register(HEADING_SESSION_MS, end_session, NULL);
  notify();
  return false;
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (start_or_extend() && now_ms() - s_session_start_ms > TAP_GRACE_MS) {
    s_heading.world = !s_heading.world;
    notify();
  }
}

static void accel_handler(AccelData *data, uint32_t count) {
  bool raised = false;
  for (uint32_t i = 0; i < count; i++) {
    if (data[i].did_vibrate) continue;
    raised |= raise_step(&s_raise, data[i].z);
  }
  if (raised) start_or_extend();  // a raise never toggles the view
}

void heading_start(HeadingHandler on_change) {
  s_handler = on_change;
  s_heading = (Heading){.heading = 0, .live = false, .world = false, .status = CompassStatusDataInvalid};
  raise_reset(&s_raise);
  accel_tap_service_subscribe(tap_handler);
  accel_data_service_subscribe(RAISE_BATCH, accel_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  start_or_extend();  // opening the face counts as looking at it
}

void heading_stop(void) {
  accel_tap_service_unsubscribe();
  accel_data_service_unsubscribe();
  if (s_session) {
    app_timer_cancel(s_session);
    s_session = NULL;
    compass_service_unsubscribe();
  }
  s_handler = NULL;
}

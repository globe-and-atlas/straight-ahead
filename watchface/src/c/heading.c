#include "heading.h"

static Heading s_heading;
static HeadingHandler s_handler;
static AppTimer *s_session;

static void notify(void) {
  if (s_handler) s_handler(&s_heading);
}

static void compass_handler(CompassHeadingData data) {
  s_heading.status = data.compass_status;
  // Unavailable (e.g. the emery emulator) and DataInvalid carry no usable heading.
  if (data.compass_status < CompassStatusCalibrating) {
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
  s_heading.heading = 0;
  notify();
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_session) {
    app_timer_reschedule(s_session, HEADING_SESSION_MS);
    return;
  }
  s_heading.live = true;
  s_heading.status = CompassStatusDataInvalid;
  compass_service_set_heading_filter(HEADING_FILTER);
  compass_service_subscribe(compass_handler);
  s_session = app_timer_register(HEADING_SESSION_MS, end_session, NULL);
  notify();
}

void heading_start(HeadingHandler on_change) {
  s_handler = on_change;
  s_heading = (Heading){.heading = 0, .live = false, .status = CompassStatusDataInvalid};
  accel_tap_service_subscribe(tap_handler);
}

void heading_stop(void) {
  accel_tap_service_unsubscribe();
  if (s_session) {
    app_timer_cancel(s_session);
    s_session = NULL;
    compass_service_unsubscribe();
  }
  s_handler = NULL;
}

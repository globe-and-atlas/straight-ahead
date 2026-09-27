#include "place.h"

int place_ring_px(int minute, int disc_r) {
  if (minute < 0) minute = 0;
  if (minute >= PLACE_DAY_NM) minute = PLACE_DAY_NM - 1;
  return minute * disc_r / PLACE_DAY_NM;
}

int place_delta10(int a10, int b10) {
  int d = (a10 - b10) % 3600;
  if (d < 0) d += 3600;
  return d > 1800 ? 3600 - d : d;
}

int place_pick(const Place *cities, int count, int minute, int heading10) {
  int best = -1;
  int32_t best_score = 0;
  for (int i = 0; i < count; i++) {
    int radial = cities[i].nm - minute;
    if (radial < 0) radial = -radial;
    if (radial > PLACE_WINDOW_NM) continue;
    int dtheta = place_delta10(cities[i].bearing10, heading10);
    if (dtheta > PLACE_WINDOW_DEG10) continue;
    // Lateral miss d * angle, with angle = dtheta / 10 degrees and pi ~= 355 / 113.
    int32_t lateral = (int32_t)cities[i].nm * dtheta * 355 / (113 * 1800);
    int32_t score = radial + lateral;
    if (best < 0 || score < best_score) {
      best = i;
      best_score = score;
    }
  }
  return best;
}

int place_cell(int minute, int heading10) {
  int b = ((heading10 % 3600 + 3600) % 3600 + 50) / 100 % PLACE_BEARINGS;
  int k = minute / 60;
  if (k < 0) k = 0;
  if (k >= PLACE_BANDS) k = PLACE_BANDS - 1;
  return b * PLACE_BANDS + k;
}

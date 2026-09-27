// Host harness for place.c (pure C, no pebble.h). Used by tests/test_place.py.
//   ring MINUTE RADIUS            -> ring radius px
//   cell MINUTE HEADING10         -> region cell index
//   pick MINUTE HEADING10 < cities   (lines: "nm bearing10 NAME") -> index or -1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "place.h"

#define MAX 400

int main(int argc, char **argv) {
  if (argc == 4 && !strcmp(argv[1], "ring")) {
    printf("%d\n", place_ring_px(atoi(argv[2]), atoi(argv[3])));
    return 0;
  }
  if (argc == 4 && !strcmp(argv[1], "cell")) {
    printf("%d\n", place_cell(atoi(argv[2]), atoi(argv[3])));
    return 0;
  }
  if (argc == 4 && !strcmp(argv[1], "pick")) {
    static Place cities[MAX];
    static char names[MAX][32];
    int n = 0, nm, b10;
    while (n < MAX && scanf("%d %d %31[^\n]", &nm, &b10, names[n]) == 3) {
      cities[n] = (Place){.nm = (uint16_t)nm, .bearing10 = (uint16_t)b10, .name = names[n],
                          .name_len = (uint8_t)strlen(names[n])};
      n++;
    }
    printf("%d\n", place_pick(cities, n, atoi(argv[2]), atoi(argv[3])));
    return 0;
  }
  fprintf(stderr, "usage: place_harness ring M R | cell M H10 | pick M H10 < cities\n");
  return 2;
}

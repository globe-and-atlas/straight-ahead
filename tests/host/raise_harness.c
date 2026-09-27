// Host harness for raise.c: one z (mG) per stdin line -> one 0/1 per line (1 = raise).
#include <stdio.h>

#include "raise.h"

int main(void) {
  Raise r;
  raise_reset(&r);
  int z;
  while (scanf("%d", &z) == 1) printf("%d\n", raise_step(&r, (int16_t)z));
  return 0;
}

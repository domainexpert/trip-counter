/*
 * Simple loop example for numerical precision analysis.
 *
 * Copyright 2017 National University of Singapore
 *
 * See LICENSE.md for license information.
 */

#include <klee/klee.h>

int main(int argc, char **argv) {
  int c = 0;

  klee_make_symbolic(&c, sizeof(c), "c");
  klee_track_error(&c, "c_error");

  for (int i = 0; i < 3; ++i) {
    c++;
  }

  klee_bound_error(c, 1.3);

  return 0;
}

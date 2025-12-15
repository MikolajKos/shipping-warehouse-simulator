#include "utils.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void get_time(char* buffer, size_t size) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(buffer, size, "%H:%M:%S", t);
}

double get_volume(PackageType type) {
  if(type == PKG_A) return 0.019456;
  if(type == PKG_B) return 0.046208;
  if(type == PKG_C) return 0.099712;
  return 0.0;
}

double generate_weight(PackageType type) {
  double min = 0.1;
  double max = 25.0;

  // Generating random weight
  double d = (double)rand() / (double)RAND_MAX; // d == 0 or 1
  double weight = min + d * (max - min); // Multiplication result can't be more than 24.9

  // Satisfying requirements: smaller package -> smaller weight
  if (type == PKG_A && weight > 10.0) weight /= 3.0;
  if (type == PKG_B && weight > 10.0) weight /= 2.0;

  return weight;
}

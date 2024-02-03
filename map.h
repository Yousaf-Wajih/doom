#ifndef _MAP_H
#define _MAP_H

#include "vector.h"

#include <stddef.h>

typedef struct map {
  size_t  num_vertices;
  vec2_t *vertices;

  vec2_t min, max;
} map_t;

#endif // !_MAP_H

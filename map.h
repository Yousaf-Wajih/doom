#ifndef _MAP_H
#define _MAP_H

#include "vector.h"

#include <stddef.h>
#include <stdint.h>

#define LINEDEF_FLAGS_TWO_SIDED 0x0004

typedef struct linedef {
  uint16_t start_idx, end_idx;
  uint16_t flags;
} linedef_t;

typedef struct map {
  size_t  num_vertices;
  vec2_t *vertices;
  vec2_t  min, max;

  size_t     num_linedefs;
  linedef_t *linedefs;
} map_t;

#endif // !_MAP_H

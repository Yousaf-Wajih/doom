#ifndef _MAP_H
#define _MAP_H

#include "vector.h"

#include <stddef.h>
#include <stdint.h>

#define LINEDEF_FLAGS_TWO_SIDED      0x0004
#define LINEDEF_FLAGS_UPPER_UNPEGGED 0x0008
#define LINEDEF_FLAGS_LOWER_UNPEGGED 0x0010

enum thing_types {
  THING_P1_START = 1,
};

typedef struct thing_info {
  uint16_t type;
  int      height;
} thing_info_t;

extern int          map_num_thing_infos;
extern thing_info_t map_thing_info[];

typedef struct sector {
  int16_t floor, ceiling;
  int16_t light_level;
  int     floor_tex, ceiling_tex;
} sector_t;

typedef struct sidedef {
  int16_t  x_off, y_off;
  uint16_t upper, lower, middle;
  uint16_t sector_idx;
} sidedef_t;

typedef struct linedef {
  uint16_t start_idx, end_idx;
  uint16_t flags;
  uint16_t front_sidedef, back_sidedef;
} linedef_t;

typedef struct thing {
  uint16_t type;
  vec2_t   position;
  float    angle;
} thing_t;

typedef struct map {
  size_t  num_vertices;
  vec2_t *vertices;
  vec2_t  min, max;

  size_t     num_linedefs;
  linedef_t *linedefs;

  size_t     num_sidedefs;
  sidedef_t *sidedefs;

  size_t    num_sectors;
  sector_t *sectors;

  size_t   num_things;
  thing_t *things;
} map_t;

#endif // !_MAP_H

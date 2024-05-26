#ifndef _ENGINE_UTIL_H
#define _ENGINE_UTIL_H

#include "map.h"
#include "matrix.h"
#include "vector.h"

void      insert_stencil_quad(mat4_t transformation);
sector_t *map_get_sector(vec2_t position);

#endif

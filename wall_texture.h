#ifndef _WALL_TEXTURE_H
#define _WALL_TEXTURE_H

#include <GL/glew.h>
#include <stddef.h>
#include <stdint.h>

#include "vector.h"

typedef struct wall_tex {
  char     name[8];
  uint16_t width, height;
  uint8_t *data;
} wall_tex_t;

GLuint generate_wall_texture_array(const wall_tex_t *textures,
                                   size_t            num_textures,
                                   vec2_t           *max_coords_array);

#endif // !_WALL_TEXTURE_H

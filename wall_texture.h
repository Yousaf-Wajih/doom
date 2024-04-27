#ifndef _WALL_TEXTURE_H
#define _WALL_TEXTURE_H

#include <GL/glew.h>
#include <stdint.h>

typedef struct wall_tex {
  char     name[8];
  uint16_t width, height;
  uint8_t *data;
} wall_tex_t;

#endif // !_WALL_TEXTURE_H

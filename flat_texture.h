#ifndef _FLAT_TEXTURE_H
#define _FLAT_TEXTURE_H

#include <GL/glew.h>
#include <stdint.h>

#define FLAT_TEXTURE_SIZE 64

typedef struct flat_tex {
  uint8_t data[FLAT_TEXTURE_SIZE * FLAT_TEXTURE_SIZE];
} flat_tex_t;

GLuint generate_flat_texture(const flat_tex_t *flat);

#endif // !_FLAT_TEXTURE_H

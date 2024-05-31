#ifndef _FLAT_TEXTURE_H
#define _FLAT_TEXTURE_H

#include <GL/glew.h>
#include <stddef.h>
#include <stdint.h>

#define FLAT_TEXTURE_SIZE 64

typedef struct flat_tex {
  uint8_t data[FLAT_TEXTURE_SIZE * FLAT_TEXTURE_SIZE];
  char    name[9];
} flat_tex_t;

GLuint generate_flat_texture_array(const flat_tex_t *flats, size_t num_flats);

#endif // !_FLAT_TEXTURE_H

#ifndef _PALETTE_H
#define _PALETTE_H

#include <GL/glew.h>
#include <stdint.h>

#define NUM_COLORS 256

typedef struct palette {
  uint8_t colors[NUM_COLORS * 3];
} palette_t;

GLuint palettes_generate_texture(const palette_t *palettes, size_t num);

#endif // !_PALETTE_H

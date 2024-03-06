#include "flat_texture.h"

#include <GL/glew.h>

GLuint generate_flat_texture_array(const flat_tex_t *flats, size_t num_flats) {
  GLuint tex_id;
  glGenTextures(1, &tex_id);
  glBindTexture(GL_TEXTURE_2D_ARRAY, tex_id);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8UI, FLAT_TEXTURE_SIZE,
                 FLAT_TEXTURE_SIZE, num_flats);

  for (int i = 0; i < num_flats; i++) {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, FLAT_TEXTURE_SIZE,
                    FLAT_TEXTURE_SIZE, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE,
                    flats[i].data);
  }

  return tex_id;
}

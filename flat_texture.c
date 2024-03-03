#include "flat_texture.h"

#include <GL/glew.h>

GLuint generate_flat_texture(const flat_tex_t *flat) {
  GLuint tex_id;
  glGenTextures(1, &tex_id);
  glBindTexture(GL_TEXTURE_2D, tex_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, FLAT_TEXTURE_SIZE, FLAT_TEXTURE_SIZE,
               0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, flat->data);

  return tex_id;
}

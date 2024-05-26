#include "gl_helpers.h"

#include <GL/glew.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

GLuint compile_shader(GLenum type, const char *src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infolog[512];
    glGetShaderInfoLog(shader, sizeof(infolog), NULL, infolog);
    fprintf(stderr, "Failed to compile shader!\nInfo Log:\n%s\n", infolog);
  }

  return shader;
}

GLuint link_program(size_t num_shaders, ...) {
  GLuint program = glCreateProgram();

  va_list va;
  va_start(va, num_shaders);
  for (size_t i = 0; i < num_shaders; i++) {
    glAttachShader(program, va_arg(va, GLuint));
  }
  va_end(va);

  glLinkProgram(program);
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infolog[512];
    glGetProgramInfoLog(program, sizeof(infolog), NULL, infolog);
    fprintf(stderr, "Failed to link shader program!\nInfo Log:\n%s\n", infolog);
  }

  return program;
}

GLuint generate_texture(uint16_t width, uint16_t height, uint8_t *data) {
  GLuint tex_id;
  glGenTextures(1, &tex_id);
  glBindTexture(GL_TEXTURE_2D, tex_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, width, height, 0, GL_RED_INTEGER,
               GL_UNSIGNED_BYTE, data);

  return tex_id;
}

#ifndef _GL_HELPERS_H
#define _GL_HELPERS_H

#include <GL/glew.h>
#include <stddef.h>

GLuint compile_shader(GLenum type, const char *src);
GLuint link_program(size_t num_shaders, ...);

GLuint generate_texture(uint16_t width, uint16_t height, uint8_t *data);

#endif // !_GL_HELPERS_H

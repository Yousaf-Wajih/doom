#ifndef _GL_HELPERS_H
#define _GL_HELPERS_H

#include <GL/glew.h>
#include <stddef.h>

GLuint compile_shader(GLenum type, const char *src);
GLuint link_program(size_t num_shaders, ...);

#endif // !_GL_HELPERS_H

#ifndef _RENDERER_H
#define _RENDERER_H

#include "matrix.h"
#include "mesh.h"
#include "vector.h"

void renderer_init(int width, int height);
void renderer_clear();

void renderer_set_palette_texture(GLuint palette_texture);
void renderer_set_wall_texture(GLuint texture);
void renderer_set_flat_texture(GLuint texture);
void renderer_set_projection(mat4_t projection);
void renderer_set_view(mat4_t view);

vec2_t renderer_get_size();

void renderer_draw_mesh(const mesh_t *mesh, mat4_t transformation);

#endif // !_RENDERER_H

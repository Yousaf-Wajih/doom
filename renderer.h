#ifndef _RENDERER_H
#define _RENDERER_H

#include "matrix.h"
#include "mesh.h"
#include "vector.h"

void renderer_init(int width, int height);
void renderer_clear();

void renderer_set_palette_texture(GLuint palette_texture);
void renderer_set_draw_texture(GLuint texture);
void renderer_set_projection(mat4_t projection);
void renderer_set_view(mat4_t view);

vec2_t renderer_get_size();

void renderer_draw_mesh(const mesh_t *mesh, mat4_t transformation, int color);
void renderer_draw_point(vec2_t point, float size, int color);
void renderer_draw_line(vec2_t p0, vec2_t p1, float width, int color);
void renderer_draw_quad(vec2_t center, vec2_t size, float angle, int color);

#endif // !_RENDERER_H

#ifndef _RENDERER_H
#define _RENDERER_H

#include "matrix.h"
#include "mesh.h"
#include "vector.h"

void renderer_init(int width, int height);
void renderer_clear();
void renderer_set_view(mat4_t view);

void renderer_draw_mesh(const mesh_t *mesh, mat4_t transformation,
                        vec4_t color);
void renderer_draw_point(vec2_t point, float size, vec4_t color);
void renderer_draw_line(vec2_t p0, vec2_t p1, float width, vec4_t color);
void renderer_draw_quad(vec2_t center, vec2_t size, float angle, vec4_t color);

#endif // !_RENDERER_H

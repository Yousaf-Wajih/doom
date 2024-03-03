#ifndef _MESH_H
#define _MESH_H

#include <GL/glew.h>
#include <stddef.h>
#include <stdint.h>

#include "vector.h"

typedef struct mesh {
  GLuint vao, vbo, ebo;
  size_t num_indices;
} mesh_t;

typedef struct vertex {
  vec3_t position;
  vec2_t tex_coords;
} vertex_t;

void mesh_create(mesh_t *mesh, size_t num_vertices, vertex_t *vertices,
                 size_t num_indices, uint32_t *indices);

#endif // !_MESH_H

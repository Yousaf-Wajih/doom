#ifndef _MESH_H
#define _MESH_H

#include <GL/glew.h>
#include <stddef.h>
#include <stdint.h>

#include "dynarray.h"
#include "vector.h"

typedef struct mesh {
  GLuint vao, vbo, ebo;
  size_t num_indices;
} mesh_t;

typedef struct vertex {
  vec3_t position;
  vec2_t tex_coords;
  int    texture_index;
  int    texture_type;
  float  light;
  vec2_t max_coords;
} vertex_t;

void mesh_create(mesh_t *mesh, size_t num_vertices, vertex_t *vertices,
                 size_t num_indices, uint32_t *indices);

typedef dynarray(vertex_t) vertexarray_t;
typedef dynarray(uint32_t) indexarray_t;

#endif // !_MESH_H

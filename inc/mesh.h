#ifndef _MESH_H
#define _MESH_H

#include <GL/glew.h>
#include <stdbool.h>
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

typedef enum vertex_layout {
  VERTEX_LAYOUT_PLAIN,
  VERTEX_LAYOUT_FULL,
} vertex_layout_t;

void mesh_create(mesh_t *mesh, vertex_layout_t vertex_layout,
                 size_t num_vertices, const void *vertices, size_t num_indices,
                 const uint32_t *indices, bool is_dynamic);

typedef dynarray(vertex_t) vertexarray_t;
typedef dynarray(uint32_t) indexarray_t;

#endif // !_MESH_H

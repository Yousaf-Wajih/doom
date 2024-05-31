#include "mesh.h"
#include "vector.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void mesh_create(mesh_t *mesh, vertex_layout_t vertex_layout,
                 size_t num_vertices, const void *vertices, size_t num_indices,
                 const uint32_t *indices, bool is_dynamic) {
  mesh->num_indices = num_indices;

  glGenVertexArrays(1, &mesh->vao);
  glGenBuffers(1, &mesh->vbo);
  glGenBuffers(1, &mesh->ebo);

  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

  switch (vertex_layout) {
  case VERTEX_LAYOUT_PLAIN:
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3_t) * num_vertices, vertices,
                 is_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), (void *)0);
    glEnableVertexAttribArray(0);
    break;
  case VERTEX_LAYOUT_FULL:
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * num_vertices, vertices,
                 is_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
                          (void *)offsetof(vertex_t, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
                          (void *)offsetof(vertex_t, tex_coords));
    glEnableVertexAttribArray(1);

    glVertexAttribIPointer(2, 1, GL_INT, sizeof(vertex_t),
                           (void *)offsetof(vertex_t, texture_index));
    glEnableVertexAttribArray(2);

    glVertexAttribIPointer(3, 1, GL_INT, sizeof(vertex_t),
                           (void *)offsetof(vertex_t, texture_type));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
                          (void *)offsetof(vertex_t, light));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
                          (void *)offsetof(vertex_t, max_coords));
    glEnableVertexAttribArray(5);

    break;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * num_indices, indices,
               GL_STATIC_DRAW);
}

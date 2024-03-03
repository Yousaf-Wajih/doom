#include "mesh.h"
#include <stddef.h>
#include <stdint.h>

void mesh_create(mesh_t *mesh, size_t num_vertices, vertex_t *vertices,
                 size_t num_indices, uint32_t *indices) {
  mesh->num_indices = num_indices;

  glGenVertexArrays(1, &mesh->vao);
  glGenBuffers(1, &mesh->vbo);
  glGenBuffers(1, &mesh->ebo);

  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * num_vertices, vertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
                        (void *)offsetof(vertex_t, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
                        (void *)offsetof(vertex_t, tex_coords));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * num_indices, indices,
               GL_STATIC_DRAW);
}

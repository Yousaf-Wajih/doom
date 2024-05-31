#include "engine/anim.h"
#include "mesh.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

flat_anim_t *flat_anim = NULL, **flat_anim_ptr = &flat_anim;

void update_animation(float dt) {
  for (flat_anim_t *anim = flat_anim; anim != NULL; anim = anim->next) {
    anim->time += dt;
    if (anim->time < TEX_ANIM_TIME) continue;

    anim->time -= TEX_ANIM_TIME;

    glBindBuffer(GL_ARRAY_BUFFER, anim->mesh->vbo);
    void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);

    for (size_t i = anim->vertex_index_start; i < anim->vertex_index_end; i++) {
      int *tex = ptr + i * sizeof(vertex_t) + offsetof(vertex_t, texture_index);
      if (++*tex > anim->max_tex) *tex = anim->min_tex;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
  }
}

void add_tex_anim(mesh_t *mesh, size_t vertex_index_start,
                  size_t vertex_index_end, int min_tex, int max_tex) {
  flat_anim_t *anim = malloc(sizeof(flat_anim_t));
  *flat_anim_ptr    = anim;
  flat_anim_ptr     = &anim->next;

  *anim = (flat_anim_t){
      mesh, vertex_index_start, vertex_index_end, min_tex, max_tex, 0.f, NULL,
  };
}

#ifndef _ANIM_H
#define _ANIM_H

#include "mesh.h"
#include <stddef.h>

#define TEX_ANIM_TIME (8.f / 35.f)

typedef struct tex_anim {
  mesh_t *mesh;
  size_t  vertex_index_start, vertex_index_end;
  int     min_tex, max_tex;
  float   time;

  struct tex_anim *next; // used as a linked link
} flat_anim_t;

void update_animation(float dt);

void add_tex_anim(mesh_t *mesh, size_t vertex_index_start,
                  size_t vertex_index_end, int min_tex, int max_tex);

#endif // !_ANIM_H

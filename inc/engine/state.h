#ifndef _STATE_H
#define _STATE_H

#include "gl_map.h"
#include "map.h"
#include "matrix.h"
#include "mesh.h"

typedef struct draw_node {
  mesh_t           *mesh;
  struct draw_node *front, *back;
} draw_node_t;

typedef struct stencil_node {
  mat4_t               transformation;
  struct stencil_node *next;
} stencil_node_t;

typedef struct stencil_quad_list {
  stencil_node_t *head, *tail;
} stencil_list_t;

typedef struct wall_tex_info {
  int width, height;
} wall_tex_info_t;

typedef struct tex_anim_def {
  const char *end_name, *start_name;
  int         start, end;
} tex_anim_def_t;

extern size_t           num_flats, num_wall_textures, num_palettes;
extern wall_tex_info_t *wall_textures_info;
extern vec2_t          *wall_max_coords;

extern map_t    map;
extern gl_map_t gl_map;
extern float    player_height;
extern float    max_sector_height;
extern int      sky_flat;

extern draw_node_t   *root_draw_node;
extern stencil_list_t stencil_list;

extern tex_anim_def_t tex_anim_defs[];
extern size_t         num_tex_anim_defs;

#endif

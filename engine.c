#include "engine.h"
#include "camera.h"
#include "flat_texture.h"
#include "gl_map.h"
#include "input.h"
#include "map.h"
#include "matrix.h"
#include "mesh.h"
#include "palette.h"
#include "renderer.h"
#include "util.h"
#include "vector.h"
#include "wad.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOV               (M_PI / 3.f)
#define PLAYER_SPEED      (5.f)
#define MOUSE_SENSITIVITY (.05f)

#define SCALE (100.f)

typedef struct wall_node {
  mat4_t            model;
  const sector_t   *sector;
  struct wall_node *next;
} wall_node_t, wall_list_t;

typedef struct flat_node {
  mesh_t            mesh;
  const sector_t   *sector;
  struct flat_node *next;
} flat_node_t, flat_list_t;

static void   generate_meshes(const map_t *map, const gl_map_t *gl_map);
static mat4_t model_from_vertices(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3);

static palette_t palette;
static size_t    num_flats;
static GLuint    flat_texture_array;

static camera_t camera;
static vec2_t   last_mouse;
static mesh_t   quad_mesh;

static wall_list_t *wall_list;
static flat_list_t *flat_list;

void engine_init(wad_t *wad, const char *mapname) {
  camera = (camera_t){
      .position = {0.f, 0.f, 3.f},
      .yaw      = M_PI_2,
      .pitch    = 0.f,
  };

  vec2_t size       = renderer_get_size();
  mat4_t projection = mat4_perspective(FOV, size.x / size.y, .1f, 1000.f);
  renderer_set_projection(projection);

  vertex_t vertices[] = {
      {.position = {1.f, 1.f, 0.f}}, // top-right
      {.position = {0.f, 1.f, 0.f}}, // bottom-right
      {.position = {0.f, 0.f, 0.f}}, // bottom-left
      {.position = {1.0, 0.f, 0.f}}, // top-left
  };

  uint32_t indices[] = {
      0, 1, 3, // 1st triangle
      1, 2, 3, // 2nd triangle
  };

  mesh_create(&quad_mesh, 4, vertices, 6, indices);

  map_t map;
  if (wad_read_map(mapname, &map, wad) != 0) {
    fprintf(stderr, "Failed to read map (%s) from WAD file\n", mapname);
    return;
  }

  char *gl_mapname = malloc(strlen(mapname) + 4);
  gl_mapname[0]    = 'G';
  gl_mapname[1]    = 'L';
  gl_mapname[2]    = '_';
  gl_mapname[3]    = 0;
  strcat(gl_mapname, mapname);

  gl_map_t gl_map;
  if (wad_read_gl_map(gl_mapname, &gl_map, wad) != 0) {
    fprintf(stderr, "Failed to read GL info for map (%s) from WAD file\n",
            mapname);
    return;
  }

  generate_meshes(&map, &gl_map);

  wad_read_playpal(&palette, wad);
  GLuint palette_texture = palette_generate_texture(&palette);
  renderer_set_palette_texture(palette_texture);

  flat_tex_t *flats  = wad_read_flats(&num_flats, wad);
  flat_texture_array = generate_flat_texture_array(flats, num_flats);
  free(flats);
}

void engine_update(float dt) {
  camera_update_direction_vectors(&camera);

  float speed =
      (is_button_pressed(KEY_LSHIFT) ? PLAYER_SPEED * 2.f : PLAYER_SPEED) * dt;

  if (is_button_pressed(KEY_W)) {
    camera.position =
        vec3_add(camera.position, vec3_scale(camera.forward, speed));
  }
  if (is_button_pressed(KEY_S)) {
    camera.position =
        vec3_add(camera.position, vec3_scale(camera.forward, -speed));
  }
  if (is_button_pressed(KEY_D)) {
    camera.position =
        vec3_add(camera.position, vec3_scale(camera.right, speed));
  }
  if (is_button_pressed(KEY_A)) {
    camera.position =
        vec3_add(camera.position, vec3_scale(camera.right, -speed));
  }

  if (is_button_pressed(MOUSE_RIGHT)) {
    if (!is_mouse_captured()) {
      last_mouse = get_mouse_position();
      set_mouse_captured(1);
    }

    vec2_t current_mouse = get_mouse_position();
    float  dx            = current_mouse.x - last_mouse.x;
    float  dy            = last_mouse.y - current_mouse.y;
    last_mouse           = current_mouse;

    camera.yaw += dx * MOUSE_SENSITIVITY * dt;
    camera.pitch += dy * MOUSE_SENSITIVITY * dt;

    camera.pitch = max(-M_PI_2 + 0.05, min(M_PI_2 - 0.05, camera.pitch));
  } else if (is_mouse_captured()) {
    set_mouse_captured(0);
  }
}

void engine_render() {
  mat4_t view = mat4_look_at(
      camera.position, vec3_add(camera.position, camera.forward), camera.up);
  renderer_set_view(view);

  renderer_set_draw_texture(0);
  for (wall_node_t *node = wall_list; node != NULL; node = node->next) {
    srand((uintptr_t)node->sector);
    int color = rand() % NUM_COLORS;
    renderer_draw_mesh(&quad_mesh, node->model, color);
  }

  renderer_set_draw_texture(flat_texture_array);
  for (flat_node_t *node = flat_list; node != NULL; node = node->next) {
    int floor_tex   = node->sector->floor_tex;
    int ceiling_tex = node->sector->ceiling_tex;

    if (floor_tex >= 0 && floor_tex < num_flats) {
      renderer_set_texture_index(floor_tex);
      renderer_draw_mesh(
          &node->mesh,
          mat4_translate((vec3_t){0.f, node->sector->floor / SCALE, 0.f}), 0);
    }

    if (ceiling_tex >= 0 && ceiling_tex < num_flats) {
      renderer_set_texture_index(ceiling_tex);
      renderer_draw_mesh(
          &node->mesh,
          mat4_translate((vec3_t){0.f, node->sector->ceiling / SCALE, 0.f}), 0);
    }
  }
}

static void generate_meshes(const map_t *map, const gl_map_t *gl_map) {
  flat_node_t **flat_node_ptr = &flat_list;
  for (int i = 0; i < gl_map->num_subsectors; i++) {
    flat_node_t *node = malloc(sizeof(flat_node_t));
    node->next        = NULL;
    node->sector      = NULL;

    gl_subsector_t *subsector  = &gl_map->subsectors[i];
    size_t          n_vertices = subsector->num_segs;
    vertex_t       *vertices   = malloc(sizeof(vertex_t) * n_vertices);

    for (int j = 0; j < subsector->num_segs; j++) {
      gl_segment_t *segment = &gl_map->segments[j + subsector->first_seg];

      if (node->sector == NULL && segment->linedef != 0xffff) {
        linedef_t *linedef = &map->linedefs[segment->linedef];
        int        sector  = -1;
        if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED && segment->side == 1) {
          sector = map->sidedefs[linedef->back_sidedef].sector_idx;
        } else {
          sector = map->sidedefs[linedef->front_sidedef].sector_idx;
        }

        if (sector >= 0) { node->sector = &map->sectors[sector]; }
      }

      vec2_t v;
      if (segment->start_vertex & VERT_IS_GL) {
        v = gl_map->vertices[segment->start_vertex & 0x7fff];
      } else {
        v = map->vertices[segment->start_vertex];
      }

      vertices[j] = (vertex_t){
          .position   = {v.x / SCALE, 0.f, -v.y / SCALE},
          .tex_coords = {v.x / FLAT_TEXTURE_SIZE, -v.y / FLAT_TEXTURE_SIZE},
      };
    }

    // Triangulation will form (n - 2) triangles, so 3*(n - 3) indices are
    // required
    size_t    n_indices = 3 * (n_vertices - 2);
    uint32_t *indices   = malloc(sizeof(uint32_t) * n_indices);
    for (int j = 0, k = 1; j < n_indices; j += 3, k++) {
      indices[j]     = 0;
      indices[j + 1] = k;
      indices[j + 2] = k + 1;
    }

    mesh_create(&node->mesh, n_vertices, vertices, n_indices, indices);
    free(vertices);
    free(indices);

    *flat_node_ptr = node;
    flat_node_ptr  = &node->next;
  }

  wall_node_t **wall_node_ptr = &wall_list;
  for (int i = 0; i < map->num_linedefs; i++) {
    linedef_t *linedef = &map->linedefs[i];

    if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED) {
      wall_node_t *floor_node = malloc(sizeof(wall_node_t));
      floor_node->next        = NULL;

      vec2_t start = map->vertices[linedef->start_idx];
      vec2_t end   = map->vertices[linedef->end_idx];

      sidedef_t *front_sidedef = &map->sidedefs[linedef->front_sidedef];
      sector_t  *front_sector  = &map->sectors[front_sidedef->sector_idx];

      sidedef_t *back_sidedef = &map->sidedefs[linedef->back_sidedef];
      sector_t  *back_sector  = &map->sectors[back_sidedef->sector_idx];

      vec3_t floor0 = {start.x, front_sector->floor, -start.y};
      vec3_t floor1 = {end.x, front_sector->floor, -end.y};
      vec3_t floor2 = {end.x, back_sector->floor, -end.y};
      vec3_t floor3 = {start.x, back_sector->floor, -start.y};

      floor_node->model  = model_from_vertices(floor0, floor1, floor2, floor3);
      floor_node->sector = front_sector;

      *wall_node_ptr = floor_node;
      wall_node_ptr  = &floor_node->next;

      wall_node_t *ceil_node = malloc(sizeof(wall_node_t));
      ceil_node->next        = NULL;

      vec3_t ceil0 = {start.x, front_sector->ceiling, -start.y};
      vec3_t ceil1 = {end.x, front_sector->ceiling, -end.y};
      vec3_t ceil2 = {end.x, back_sector->ceiling, -end.y};
      vec3_t ceil3 = {start.x, back_sector->ceiling, -start.y};

      ceil_node->model  = model_from_vertices(ceil0, ceil1, ceil2, ceil3);
      ceil_node->sector = front_sector;

      *wall_node_ptr = ceil_node;
      wall_node_ptr  = &ceil_node->next;
    } else {
      wall_node_t *node = malloc(sizeof(wall_node_t));
      node->next        = NULL;

      vec2_t start = map->vertices[linedef->start_idx];
      vec2_t end   = map->vertices[linedef->end_idx];

      sidedef_t *sidedef = &map->sidedefs[linedef->front_sidedef];
      sector_t  *sector  = &map->sectors[sidedef->sector_idx];

      vec3_t p0 = {start.x, sector->floor, -start.y};
      vec3_t p1 = {end.x, sector->floor, -end.y};
      vec3_t p2 = {end.x, sector->ceiling, -end.y};
      vec3_t p3 = {start.x, sector->ceiling, -start.y};

      node->model  = model_from_vertices(p0, p1, p2, p3);
      node->sector = sector;

      *wall_node_ptr = node;
      wall_node_ptr  = &node->next;
    }
  }
}

mat4_t model_from_vertices(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3) {
  p0 = vec3_scale(p0, 1.f / SCALE);
  p1 = vec3_scale(p1, 1.f / SCALE);
  p2 = vec3_scale(p2, 1.f / SCALE);
  p3 = vec3_scale(p3, 1.f / SCALE);

  float x = p1.x - p0.x, y = p1.z - p0.z;
  float length = sqrtf(x * x + y * y), height = p3.y - p0.y;
  float angle = atan2f(y, x);

  mat4_t translation = mat4_translate(p0);
  mat4_t scale       = mat4_scale((vec3_t){length, height, 1.f});
  mat4_t rotation    = mat4_rotate((vec3_t){0.f, 1.f, 0.f}, angle);
  return mat4_mul(mat4_mul(scale, rotation), translation);
}

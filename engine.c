#include "engine.h"
#include "camera.h"
#include "flat_texture.h"
#include "gl_helpers.h"
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
#include "wall_texture.h"

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

typedef struct draw_node {
  int               texture;
  mesh_t            mesh;
  const sector_t   *sector;
  struct draw_node *next;
} draw_node_t, draw_list_t;

typedef struct wall_tex_info {
  int width, height;
} wall_tex_info_t;

static void   generate_meshes(const map_t *map, const gl_map_t *gl_map);
static mat4_t model_from_vertices(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3);

static palette_t        palette;
static wall_tex_info_t *wall_textures_info;
static size_t           num_flats, num_wall_textures;
static GLuint           flat_texture_array;
static GLuint          *wall_textures;

static camera_t camera;
static vec2_t   last_mouse;
static mesh_t   quad_mesh;

static draw_list_t *draw_list;

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

  wad_read_playpal(&palette, wad);
  GLuint palette_texture = palette_generate_texture(&palette);
  renderer_set_palette_texture(palette_texture);

  flat_tex_t *flats  = wad_read_flats(&num_flats, wad);
  flat_texture_array = generate_flat_texture_array(flats, num_flats);
  free(flats);

  wall_tex_t *textures = wad_read_textures(&num_wall_textures, "TEXTURE1", wad);
  wall_textures        = malloc(sizeof(GLuint) * num_wall_textures);
  wall_textures_info   = malloc(sizeof(wall_tex_info_t) * num_wall_textures);
  for (int i = 0; i < num_wall_textures; i++) {
    wall_textures[i] = generate_texture(textures[i].width, textures[i].height,
                                        textures[i].data);
    wall_textures_info[i] =
        (wall_tex_info_t){textures[i].width, textures[i].height};
  }

  map_t map;
  if (wad_read_map(mapname, &map, wad, textures, num_wall_textures) != 0) {
    fprintf(stderr, "Failed to read map (%s) from WAD file\n", mapname);
    return;
  }

  generate_meshes(&map, &gl_map);
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

  renderer_set_draw_texture_array(flat_texture_array);
  for (draw_node_t *node = draw_list; node != NULL; node = node->next) {
    if (node->texture != -1) { renderer_set_draw_texture(node->texture); }
    renderer_draw_mesh(&node->mesh, mat4_identity());
  }
}

static void generate_meshes(const map_t *map, const gl_map_t *gl_map) {
  draw_node_t **draw_node_ptr = &draw_list;
  for (int i = 0; i < gl_map->num_subsectors; i++) {
    draw_node_t *floor_node = malloc(sizeof(draw_node_t));
    floor_node->next        = NULL;

    *draw_node_ptr = floor_node;
    draw_node_ptr  = &floor_node->next;

    draw_node_t *ceil_node = malloc(sizeof(draw_node_t));
    ceil_node->next        = NULL;

    *draw_node_ptr = ceil_node;
    draw_node_ptr  = &ceil_node->next;

    sector_t       *sector         = NULL;
    gl_subsector_t *subsector      = &gl_map->subsectors[i];
    size_t          n_vertices     = subsector->num_segs;
    vertex_t       *floor_vertices = malloc(sizeof(vertex_t) * n_vertices);
    vertex_t       *ceil_vertices  = malloc(sizeof(vertex_t) * n_vertices);

    for (int j = 0; j < subsector->num_segs; j++) {
      gl_segment_t *segment = &gl_map->segments[j + subsector->first_seg];

      if (sector == NULL && segment->linedef != 0xffff) {
        linedef_t *linedef    = &map->linedefs[segment->linedef];
        int        sector_idx = -1;
        if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED && segment->side == 1) {
          sector_idx = map->sidedefs[linedef->back_sidedef].sector_idx;
        } else {
          sector_idx = map->sidedefs[linedef->front_sidedef].sector_idx;
        }

        if (sector_idx >= 0) { sector = &map->sectors[sector_idx]; }
      }

      vec2_t v;
      if (segment->start_vertex & VERT_IS_GL) {
        v = gl_map->vertices[segment->start_vertex & 0x7fff];
      } else {
        v = map->vertices[segment->start_vertex];
      }

      floor_vertices[j] = ceil_vertices[j] = (vertex_t){
          .position     = {v.x / SCALE, 0.f, -v.y / SCALE},
          .tex_coords   = {v.x / FLAT_TEXTURE_SIZE, -v.y / FLAT_TEXTURE_SIZE},
          .texture_type = 1,
      };
    }

    for (int i = 0; i < n_vertices; i++) {
      int floor_tex = sector->floor_tex, ceil_tex = sector->ceiling_tex;

      floor_vertices[i].position.y = sector->floor / SCALE;
      floor_vertices[i].texture_index =
          floor_tex >= 0 && floor_tex < num_flats ? floor_tex : -1;

      ceil_vertices[i].position.y = sector->ceiling / SCALE;
      ceil_vertices[i].texture_index =
          ceil_tex >= 0 && ceil_tex < num_flats ? ceil_tex : -1;
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

    floor_node->texture = ceil_node->texture = -1;
    floor_node->sector = ceil_node->sector = sector;
    mesh_create(&floor_node->mesh, n_vertices, floor_vertices, n_indices,
                indices);
    mesh_create(&ceil_node->mesh, n_vertices, ceil_vertices, n_indices,
                indices);

    free(floor_vertices);
    free(ceil_vertices);
    free(indices);
  }

  uint32_t indices[] = {
      0, 1, 3, // 1st triangle
      1, 2, 3, // 2nd triangle
  };

  for (int i = 0; i < map->num_linedefs; i++) {
    linedef_t *linedef = &map->linedefs[i];

    if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED) {
      draw_node_t *floor_node = malloc(sizeof(draw_node_t));
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

      floor0 = vec3_scale(floor0, 1.f / SCALE);
      floor1 = vec3_scale(floor1, 1.f / SCALE);
      floor2 = vec3_scale(floor2, 1.f / SCALE);
      floor3 = vec3_scale(floor3, 1.f / SCALE);

      srand((uintptr_t)front_sector);
      int color = rand() % NUM_COLORS;

      vertex_t floor_vertices[] = {
          {.position = floor0, .texture_index = color, .texture_type = 0},
          {.position = floor1, .texture_index = color, .texture_type = 0},
          {.position = floor2, .texture_index = color, .texture_type = 0},
          {.position = floor3, .texture_index = color, .texture_type = 0},
      };

      mesh_create(&floor_node->mesh, 4, floor_vertices, 6, indices);
      floor_node->sector = front_sector;

      *draw_node_ptr = floor_node;
      draw_node_ptr  = &floor_node->next;

      draw_node_t *ceil_node = malloc(sizeof(draw_node_t));
      ceil_node->next        = NULL;

      vec3_t ceil0 = {start.x, front_sector->ceiling, -start.y};
      vec3_t ceil1 = {end.x, front_sector->ceiling, -end.y};
      vec3_t ceil2 = {end.x, back_sector->ceiling, -end.y};
      vec3_t ceil3 = {start.x, back_sector->ceiling, -start.y};

      ceil0 = vec3_scale(ceil0, 1.f / SCALE);
      ceil1 = vec3_scale(ceil1, 1.f / SCALE);
      ceil2 = vec3_scale(ceil2, 1.f / SCALE);
      ceil3 = vec3_scale(ceil3, 1.f / SCALE);

      vertex_t ceil_vertices[] = {
          {.position = ceil0, .texture_index = color, .texture_type = 0},
          {.position = ceil1, .texture_index = color, .texture_type = 0},
          {.position = ceil2, .texture_index = color, .texture_type = 0},
          {.position = ceil3, .texture_index = color, .texture_type = 0},
      };

      mesh_create(&ceil_node->mesh, 4, ceil_vertices, 6, indices);
      ceil_node->sector = front_sector;

      floor_node->texture = ceil_node->texture = -1;

      *draw_node_ptr = ceil_node;
      draw_node_ptr  = &ceil_node->next;
    } else {
      draw_node_t *node = malloc(sizeof(draw_node_t));
      node->next        = NULL;

      vec2_t start = map->vertices[linedef->start_idx];
      vec2_t end   = map->vertices[linedef->end_idx];

      sidedef_t *sidedef = &map->sidedefs[linedef->front_sidedef];
      sector_t  *sector  = &map->sectors[sidedef->sector_idx];

      vec3_t p0 = {start.x, sector->floor, -start.y};
      vec3_t p1 = {end.x, sector->floor, -end.y};
      vec3_t p2 = {end.x, sector->ceiling, -end.y};
      vec3_t p3 = {start.x, sector->ceiling, -start.y};

      const float x = p1.x - p0.x, y = p1.z - p0.z;
      const float width = sqrtf(x * x + y * y), height = p3.y - p0.y;

      p0 = vec3_scale(p0, 1.f / SCALE);
      p1 = vec3_scale(p1, 1.f / SCALE);
      p2 = vec3_scale(p2, 1.f / SCALE);
      p3 = vec3_scale(p3, 1.f / SCALE);

      srand((uintptr_t)sector);
      int color = rand() % NUM_COLORS;

      float min_x = sidedef->x_off, min_y = sidedef->y_off;
      float max_x = min_x + width / wall_textures_info[sidedef->middle].width;
      float max_y = min_y + height / wall_textures_info[sidedef->middle].height;

      vertex_t vertices[] = {
          {p0, {min_x, max_y}, 0, 2},
          {p1, {max_x, max_y}, 0, 2},
          {p2, {max_x, min_y}, 0, 2},
          {p3, {min_x, min_y}, 0, 2},
      };

      mesh_create(&node->mesh, 4, vertices, 6, indices);
      node->texture = wall_textures[sidedef->middle];
      node->sector  = sector;

      *draw_node_ptr = node;
      draw_node_ptr  = &node->next;
    }
  }
}

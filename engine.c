#include "engine.h"
#include "camera.h"
#include "input.h"
#include "map.h"
#include "matrix.h"
#include "mesh.h"
#include "renderer.h"
#include "util.h"
#include "vector.h"
#include "wad.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FOV               (M_PI / 3.f)
#define PLAYER_SPEED      (5.f)
#define MOUSE_SENSITIVITY (.05f)

#define SCALE (100.f)

typedef struct wall_node {
  mat4_t            model;
  const sector_t   *sector;
  struct wall_node *next;
} wall_node_t, wall_list_t;

static vec3_t get_random_color(const void *seed);

static camera_t camera;
static vec2_t   last_mouse;
static mesh_t   quad_mesh;

static wall_list_t *wall_list;

void engine_init(wad_t *wad, const char *mapname) {
  camera = (camera_t){
      .position = {0.f, 0.f, 3.f},
      .yaw      = -M_PI_2,
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
    fprintf(stderr, "Failed to read map (E1M1) from WAD file\n");
    return;
  }

  wall_node_t **wall_node_ptr = &wall_list;
  for (int i = 0; i < map.num_linedefs; i++) {
    linedef_t *linedef = &map.linedefs[i];

    if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED) {
      // TODO
    } else {
      wall_node_t *node = malloc(sizeof(wall_node_t));
      node->next        = NULL;

      vec2_t start = map.vertices[linedef->start_idx];
      vec2_t end   = map.vertices[linedef->end_idx];

      sector_t *sector =
          &map.sectors[map.sidedefs[linedef->front_sidedef].sector_idx];

      float floor   = (float)sector->floor / SCALE;
      float ceiling = (float)sector->ceiling / SCALE;

      start.x /= SCALE;
      start.y /= SCALE;
      end.x /= SCALE;
      end.y /= SCALE;

      float x = end.x - start.x, y = end.y - start.y;
      float length = sqrtf(x * x + y * y), height = ceiling - floor;
      float angle = atan2f(y, x);

      mat4_t translation = mat4_translate((vec3_t){start.x, floor, start.y});
      mat4_t scale       = mat4_scale((vec3_t){length, height, 1.f});
      mat4_t rotation    = mat4_rotate((vec3_t){0.f, 1.f, 0.f}, angle);
      node->model        = mat4_mul(mat4_mul(scale, rotation), translation);
      node->sector       = sector;

      *wall_node_ptr = node;
      wall_node_ptr  = &node->next;
    }
  }
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

  for (wall_node_t *node = wall_list; node != NULL; node = node->next) {
    vec3_t color = get_random_color(node->sector);

    renderer_draw_mesh(&quad_mesh, node->model,
                       (vec4_t){color.x, color.y, color.z, 1.f});
  }
}

vec3_t get_random_color(const void *seed) {
  srand((uintptr_t)seed);
  return (vec3_t){
      (float)rand() / RAND_MAX,
      (float)rand() / RAND_MAX,
      (float)rand() / RAND_MAX,
  };
}

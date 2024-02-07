#include "engine.h"
#include "camera.h"
#include "input.h"
#include "matrix.h"
#include "mesh.h"
#include "renderer.h"
#include "util.h"
#include "vector.h"
#include "wad.h"

#include <math.h>
#include <stdio.h>

#define FOV               (M_PI / 3.f)
#define PLAYER_SPEED      (5.f)
#define MOUSE_SENSITIVITY (.05f)

static camera_t camera;
static vec2_t   last_mouse;
static mesh_t   quad_mesh;

void engine_init(wad_t *wad, const char *mapname) {
  camera = (camera_t){
      .position = {0.f, 0.f, 3.f},
      .yaw      = -M_PI_2,
      .pitch    = 0.f,
  };

  vec2_t size       = renderer_get_size();
  mat4_t projection = mat4_perspective(FOV, size.x / size.y, .1f, 1000.f);
  renderer_set_projection(projection);

  map_t map;
  if (wad_read_map(mapname, &map, wad) != 0) {
    fprintf(stderr, "Failed to read map (E1M1) from WAD file\n");
    return;
  }

  vertex_t vertices[] = {
      {.position = {.5f, .5f, 0.f}},   // top-right
      {.position = {.5f, -.5f, 0.f}},  // bottom-right
      {.position = {-.5f, -.5f, 0.f}}, // bottom-left
      {.position = {-.5f, .5f, 0.f}},  // top-left
  };

  uint32_t indices[] = {
      0, 1, 3, // 1st triangle
      1, 2, 3, // 2nd triangle
  };

  mesh_create(&quad_mesh, 4, vertices, 6, indices);
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
  if (is_button_pressed(KEY_A)) {
    camera.position =
        vec3_add(camera.position, vec3_scale(camera.right, speed));
  }
  if (is_button_pressed(KEY_D)) {
    camera.position =
        vec3_add(camera.position, vec3_scale(camera.right, -speed));
  }

  if (is_button_pressed(MOUSE_RIGHT)) {
    if (!is_mouse_captured()) {
      last_mouse = get_mouse_position();
      set_mouse_captured(1);
    }

    vec2_t current_mouse = get_mouse_position();
    float  dx            = last_mouse.x - current_mouse.x;
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

  renderer_draw_mesh(&quad_mesh, mat4_identity(), (vec4_t){1.f, 1.f, 1.f, 1.f});
}

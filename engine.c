#include "engine.h"
#include "camera.h"
#include "matrix.h"
#include "mesh.h"
#include "renderer.h"
#include "vector.h"
#include "wad.h"

#include <math.h>
#include <stdio.h>

#define FOV (M_PI / 3.f)

static camera_t camera;
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

void engine_update(float dt) { camera_update_direction_vectors(&camera); }

void engine_render() {
  mat4_t view = mat4_look_at(
      camera.position, vec3_add(camera.position, camera.forward), camera.up);
  renderer_set_view(view);

  renderer_draw_mesh(&quad_mesh, mat4_identity(), (vec4_t){1.f, 1.f, 1.f, 1.f});
}

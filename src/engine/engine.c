#include "engine.h"
#include "camera.h"
#include "engine/anim.h"
#include "engine/meshgen.h"
#include "engine/state.h"
#include "engine/util.h"
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
#include "wall_texture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOV               (M_PI / 3.f)
#define PLAYER_SPEED      (500.f)
#define MOUSE_SENSITIVITY (.05f)

static void render_node(draw_node_t *node);

size_t           num_flats, num_wall_textures, num_palettes;
wall_tex_info_t *wall_textures_info;
vec2_t          *wall_max_coords;

map_t    map;
gl_map_t gl_map;
float    player_height;
float    max_sector_height;
int      sky_flat;

draw_node_t   *root_draw_node;
stencil_list_t stencil_list;
mesh_t         quad_mesh;

size_t         num_tex_anim_defs;
tex_anim_def_t tex_anim_defs[] = {
    {"NUKAGE3", "NUKAGE1"},
    {"FWATER4", "FWATER1"},
    {"SWATER4", "SWATER1"},
    {"LAVA4",   "LAVA1"  },
    {"BLOOD3",  "BLOOD1" },
};

static camera_t camera;
static vec2_t   last_mouse;

void engine_init(wad_t *wad, const char *mapname) {
  vec2_t size       = renderer_get_size();
  mat4_t projection = mat4_perspective(FOV, size.x / size.y, .1f, 10000.f);
  renderer_set_projection(projection);

  char *gl_mapname = malloc(strlen(mapname) + 4);
  gl_mapname[0]    = 'G';
  gl_mapname[1]    = 'L';
  gl_mapname[2]    = '_';
  gl_mapname[3]    = 0;
  strcat(gl_mapname, mapname);

  if (wad_read_gl_map(gl_mapname, &gl_map, wad) != 0) {
    fprintf(stderr, "Failed to read GL info for map (%s) from WAD file\n",
            mapname);
    return;
  }

  palette_t *palettes    = wad_read_playpal(&num_palettes, wad);
  GLuint palette_texture = palettes_generate_texture(palettes, num_palettes);

  sky_flat = wad_find_lump("F_SKY1", wad) - wad_find_lump("F_START", wad) - 1;

  num_tex_anim_defs = sizeof tex_anim_defs / sizeof tex_anim_defs[0];
  for (int i = 0; i < num_tex_anim_defs; i++) {
    tex_anim_defs[i].start = tex_anim_defs[i].end = -1;
  }

  flat_tex_t *flats         = wad_read_flats(&num_flats, wad);
  GLuint flat_texture_array = generate_flat_texture_array(flats, num_flats);
  for (int i = 0; i < num_flats; i++) {
    for (int j = 0; j < num_tex_anim_defs; j++) {
      if (strcmp_nocase(flats[i].name, tex_anim_defs[j].start_name) == 0) {
        tex_anim_defs[j].start = i;
      }

      if (strcmp_nocase(flats[i].name, tex_anim_defs[j].end_name) == 0) {
        tex_anim_defs[j].end = i;
      }
    }
  }
  free(flats);

  wall_tex_t *textures = wad_read_textures(&num_wall_textures, "TEXTURE1", wad);
  wall_textures_info   = malloc(sizeof(wall_tex_info_t) * num_wall_textures);
  wall_max_coords      = malloc(sizeof(vec2_t) * num_wall_textures);
  for (int i = 0; i < num_wall_textures; i++) {
    if (strcmp_nocase(textures[i].name, "SKY1") == 0) {
      renderer_set_sky_texture(generate_texture_cubemap(&textures[i]));
    }

    wall_textures_info[i] =
        (wall_tex_info_t){textures[i].width, textures[i].height};
  }
  GLuint wall_texture_array =
      generate_wall_texture_array(textures, num_wall_textures, wall_max_coords);
  wad_free_wall_textures(textures, num_wall_textures);

  if (wad_read_map(mapname, &map, wad, textures, num_wall_textures) != 0) {
    fprintf(stderr, "Failed to read map (%s) from WAD file\n", mapname);
    return;
  }

  for (int i = 0; i < map.num_things; i++) {
    thing_t *thing = &map.things[i];

    if (thing->type == THING_P1_START) {
      thing_info_t *info = NULL;

      for (int i = 0; i < map_num_thing_infos; i++) {
        if (thing->type == map_thing_info[i].type) {
          info = &map_thing_info[i];
          break;
        }
      }

      if (info == NULL) { continue; }

      player_height = info->height;

      camera = (camera_t){
          .position = {thing->position.x, player_height, thing->position.y},
          .yaw      = thing->angle,
          .pitch    = 0.f,
      };
    }
  }

  stencil_list = (stencil_list_t){NULL, NULL};
  generate_meshes();

  renderer_set_flat_texture(flat_texture_array);
  renderer_set_wall_texture(wall_texture_array);
  renderer_set_palette_texture(palette_texture);

  vec3_t stencil_quad_vertices[] = {
      {0.f, 0.f, 0.f},
      {0.f, 1.f, 0.f},
      {1.f, 1.f, 0.f},
      {1.f, 0.f, 0.f},
  };

  uint32_t stencil_quad_indices[] = {0, 2, 1, 0, 3, 2};

  mesh_create(&quad_mesh, VERTEX_LAYOUT_PLAIN, 4, stencil_quad_vertices, 6,
              stencil_quad_indices, false);
}

static int palette_index = 0;
void       engine_update(float dt) {
  if (is_button_just_pressed(KEY_O)) { palette_index--; }
  if (is_button_just_pressed(KEY_P)) { palette_index++; }

  palette_index = min(max(palette_index, 0), num_palettes - 1);

  camera_update_direction_vectors(&camera);

  vec2_t    position = {camera.position.x, camera.position.z};
  sector_t *sector   = map_get_sector(position);
  if (sector) { camera.position.y = sector->floor + player_height; }

  float speed =
      (is_button_pressed(KEY_LSHIFT) ? PLAYER_SPEED * 2.f : PLAYER_SPEED) * dt;

  vec3_t forward = camera.forward, right = camera.right;
  forward.y = right.y = 0.f;
  forward = vec3_normalize(forward), right = vec3_normalize(right);

  if (is_button_pressed(KEY_W) || is_button_pressed(KEY_UP)) {
    camera.position = vec3_add(camera.position, vec3_scale(forward, speed));
  }
  if (is_button_pressed(KEY_S) || is_button_pressed(KEY_DOWN)) {
    camera.position = vec3_add(camera.position, vec3_scale(forward, -speed));
  }
  if (is_button_pressed(KEY_D)) {
    camera.position = vec3_add(camera.position, vec3_scale(right, speed));
  }
  if (is_button_pressed(KEY_A)) {
    camera.position = vec3_add(camera.position, vec3_scale(right, -speed));
  }

  float turn_speed = 4.f * dt;
  if (is_button_pressed(KEY_RIGHT)) { camera.yaw -= turn_speed; }
  if (is_button_pressed(KEY_LEFT)) { camera.yaw += turn_speed; }

  if (is_button_pressed(KEY_ESCAPE)) { set_mouse_captured(0); }
  if (is_button_pressed(MOUSE_RIGHT)) { set_mouse_captured(1); }

  if (is_button_pressed(KEY_EQUAL)) { camera.pitch = 0.f; }

  static bool is_first = true;
  if (is_mouse_captured()) {
    if (is_first) {
      last_mouse = get_mouse_position();
      is_first   = false;
    }

    vec2_t current_mouse = get_mouse_position();
    float  dx            = current_mouse.x - last_mouse.x;
    float  dy            = last_mouse.y - current_mouse.y;
    last_mouse           = current_mouse;

    camera.yaw -= dx * MOUSE_SENSITIVITY * dt;
    camera.pitch += dy * MOUSE_SENSITIVITY * dt;

    camera.pitch = max(-M_PI_2 + 0.05, min(M_PI_2 - 0.05, camera.pitch));
  } else {
    is_first = true;
  }

  update_animation(dt);
}

void engine_render() {
  mat4_t view = mat4_look_at(
      camera.position, vec3_add(camera.position, camera.forward), camera.up);
  renderer_set_view(view);

  renderer_set_palette_index(palette_index);

  glStencilMask(0x00);
  render_node(root_draw_node);

  glStencilMask(0xff);
  for (stencil_node_t *node = stencil_list.head; node != NULL;
       node                 = node->next) {
    renderer_draw_mesh(&quad_mesh, SHADER_PLAIN, node->transformation);
  }

  renderer_draw_sky();
}

void render_node(draw_node_t *node) {
  if (node->mesh) {
    renderer_draw_mesh(node->mesh, SHADER_DEFAULT, mat4_identity());
  }

  if (node->front) { render_node(node->front); }
  if (node->back) { render_node(node->back); }
}

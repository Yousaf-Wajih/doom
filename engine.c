#include "engine.h"
#include "camera.h"
#include "dynarray.h"
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

typedef struct draw_node {
  mesh_t           *mesh;
  struct draw_node *front, *back;
} draw_node_t;

typedef struct wall_tex_info {
  int width, height;
} wall_tex_info_t;

static sector_t *map_get_sector(map_t *map, gl_map_t *gl_map, vec2_t position);
static void      render_node(draw_node_t *node);
static void      generate_meshes(const map_t *map, const gl_map_t *gl_map);
static void      generate_node(draw_node_t **draw_node_ptr, size_t id,
                               const map_t *map, const gl_map_t *gl_map);

static size_t           num_flats, num_wall_textures, num_palettes;
static wall_tex_info_t *wall_textures_info;
static vec2_t          *wall_max_coords;

static camera_t camera;
static vec2_t   last_mouse;

static map_t    map;
static gl_map_t gl_map;
static float    player_height;
static int      sky_flat;

static draw_node_t *root_draw_node;

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

  flat_tex_t *flats         = wad_read_flats(&num_flats, wad);
  GLuint flat_texture_array = generate_flat_texture_array(flats, num_flats);
  free(flats);

  wall_tex_t *textures = wad_read_textures(&num_wall_textures, "TEXTURE1", wad);
  wall_textures_info   = malloc(sizeof(wall_tex_info_t) * num_wall_textures);
  wall_max_coords      = malloc(sizeof(vec2_t) * num_wall_textures);
  for (int i = 0; i < num_wall_textures; i++) {
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

  generate_meshes(&map, &gl_map);

  renderer_set_flat_texture(flat_texture_array);
  renderer_set_wall_texture(wall_texture_array);
  renderer_set_palette_texture(palette_texture);
}

static int palette_index = 0;
void       engine_update(float dt) {
  if (is_button_just_pressed(KEY_O)) { palette_index--; }
  if (is_button_just_pressed(KEY_P)) { palette_index++; }

  palette_index = min(max(palette_index, 0), num_palettes - 1);

  camera_update_direction_vectors(&camera);

  vec2_t    position = {camera.position.x, camera.position.z};
  sector_t *sector   = map_get_sector(&map, &gl_map, position);
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
}

void engine_render() {
  mat4_t view = mat4_look_at(
      camera.position, vec3_add(camera.position, camera.forward), camera.up);
  renderer_set_view(view);

  renderer_set_palette_index(palette_index);
  render_node(root_draw_node);
}

void render_node(draw_node_t *node) {
  if (node->mesh) { renderer_draw_mesh(node->mesh, mat4_identity()); }
  if (node->front) { render_node(node->front); }
  if (node->back) { render_node(node->back); }
}

sector_t *map_get_sector(map_t *map, gl_map_t *gl_map, vec2_t position) {
  uint16_t id = gl_map->num_nodes - 1;
  while ((id & 0x8000) == 0) {
    if (id > gl_map->num_nodes) { return NULL; }

    gl_node_t *node = &gl_map->nodes[id];

    vec2_t delta      = vec2_sub(position, node->partition);
    bool   is_on_back = (delta.x * node->delta_partition.y -
                       delta.y * node->delta_partition.x) <= 0.f;

    if (is_on_back) {
      id = gl_map->nodes[id].back_child_id;
    } else {
      id = gl_map->nodes[id].front_child_id;
    }
  }

  if ((id & 0x7fff) >= gl_map->num_subsectors) { return NULL; }

  gl_subsector_t *subsector = &gl_map->subsectors[id & 0x7fff];
  gl_segment_t   *segment   = &gl_map->segments[subsector->first_seg];
  linedef_t      *linedef   = &map->linedefs[segment->linedef];

  sidedef_t *sidedef;
  if (segment->side == 0) {
    sidedef = &map->sidedefs[linedef->front_sidedef];
  } else {
    sidedef = &map->sidedefs[linedef->back_sidedef];
  }

  return &map->sectors[sidedef->sector_idx];
}

void generate_meshes(const map_t *map, const gl_map_t *gl_map) {
  generate_node(&root_draw_node, gl_map->num_nodes - 1, map, gl_map);
}

void generate_node(draw_node_t **draw_node_ptr, size_t id, const map_t *map,
                   const gl_map_t *gl_map) {
  draw_node_t *draw_node = malloc(sizeof(draw_node_t));
  *draw_node             = (draw_node_t){NULL, NULL, NULL};
  *draw_node_ptr         = draw_node;

  if (id & 0x8000) {
    gl_subsector_t *subsector = &gl_map->subsectors[id & 0x7fff];

    vertexarray_t vertices;
    indexarray_t  indices;
    dynarray_init(vertices, 0);
    dynarray_init(indices, 0);

    sector_t *the_sector = NULL;
    size_t    n_vertices = subsector->num_segs;
    if (n_vertices < 3) { return; }

    vertex_t *floor_vertices = malloc(sizeof(vertex_t) * n_vertices);
    vertex_t *ceil_vertices  = malloc(sizeof(vertex_t) * n_vertices);

    size_t start_idx = 0;
    for (int j = 0; j < subsector->num_segs; j++) {
      gl_segment_t *segment = &gl_map->segments[j + subsector->first_seg];

      vec2_t start, end;
      if (segment->start_vertex & VERT_IS_GL) {
        start = gl_map->vertices[segment->start_vertex & 0x7fff];
      } else {
        start = map->vertices[segment->start_vertex];
      }

      if (segment->end_vertex & VERT_IS_GL) {
        end = gl_map->vertices[segment->end_vertex & 0x7fff];
      } else {
        end = map->vertices[segment->end_vertex];
      }

      if (the_sector == NULL && segment->linedef != 0xffff) {
        linedef_t *linedef    = &map->linedefs[segment->linedef];
        int        sector_idx = -1;
        if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED && segment->side == 1) {
          sector_idx = map->sidedefs[linedef->back_sidedef].sector_idx;
        } else {
          sector_idx = map->sidedefs[linedef->front_sidedef].sector_idx;
        }

        if (sector_idx >= 0) { the_sector = &map->sectors[sector_idx]; }
      }

      floor_vertices[j] = ceil_vertices[j] = (vertex_t){
          .position     = {start.x, 0.f, start.y},
          .tex_coords   = {start.x / FLAT_TEXTURE_SIZE,
                           start.y / FLAT_TEXTURE_SIZE},
          .texture_type = 1,
      };

      if (segment->linedef == 0xffff) { continue; }
      linedef_t *linedef = &map->linedefs[segment->linedef];

      sidedef_t *front_sidedef = &map->sidedefs[linedef->front_sidedef];
      sidedef_t *back_sidedef  = &map->sidedefs[linedef->back_sidedef];

      if (segment->side) {
        sidedef_t *tmp = front_sidedef;
        front_sidedef  = back_sidedef;
        back_sidedef   = tmp;
      }

      sector_t *front_sector = &map->sectors[front_sidedef->sector_idx];
      sector_t *back_sector  = &map->sectors[back_sidedef->sector_idx];

      sidedef_t *sidedef = front_sidedef;
      sector_t  *sector  = front_sector;

      if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED) {
        if (sidedef->lower >= 0 && front_sector->floor < back_sector->floor) {
          vec3_t p0 = {start.x, front_sector->floor, start.y};
          vec3_t p1 = {end.x, front_sector->floor, end.y};
          vec3_t p2 = {end.x, back_sector->floor, end.y};
          vec3_t p3 = {start.x, back_sector->floor, start.y};

          const float x = p1.x - p0.x, y = p1.z - p0.z;
          const float width = sqrtf(x * x + y * y), height = fabsf(p3.y - p0.y);

          float tw = wall_textures_info[sidedef->lower].width;
          float th = wall_textures_info[sidedef->lower].height;

          float w = width / tw, h = height / th;
          float x_off = sidedef->x_off / tw, y_off = sidedef->y_off / th;
          if (linedef->flags & LINEDEF_FLAGS_LOWER_UNPEGGED) {
            y_off += (front_sector->ceiling - back_sector->floor) / th;
          }

          float tx0 = x_off, ty0 = y_off + h;
          float tx1 = x_off + w, ty1 = y_off;

          vec2_t max_coords = wall_max_coords[sidedef->lower];
          tx0 *= max_coords.x, tx1 *= max_coords.x;
          ty0 *= max_coords.y, ty1 *= max_coords.y;

          float    light = front_sector->light_level / 256.f;
          vertex_t v[]   = {
              {p0, {tx0, ty0}, sidedef->lower, 2, light, max_coords},
              {p1, {tx1, ty0}, sidedef->lower, 2, light, max_coords},
              {p2, {tx1, ty1}, sidedef->lower, 2, light, max_coords},
              {p3, {tx0, ty1}, sidedef->lower, 2, light, max_coords},
          };

          start_idx = vertices.count;
          for (int i = 0; i < 4; i++) {
            dynarray_push(vertices, v[i]);
          }

          dynarray_push(indices, start_idx + 0);
          dynarray_push(indices, start_idx + 1);
          dynarray_push(indices, start_idx + 3);
          dynarray_push(indices, start_idx + 1);
          dynarray_push(indices, start_idx + 2);
          dynarray_push(indices, start_idx + 3);
        }

        if (sidedef->upper >= 0 &&
            front_sector->ceiling > back_sector->ceiling &&
            !(front_sector->ceiling_tex == sky_flat &&
              back_sector->ceiling_tex == sky_flat)) {
          vec3_t p0 = {start.x, back_sector->ceiling, start.y};
          vec3_t p1 = {end.x, back_sector->ceiling, end.y};
          vec3_t p2 = {end.x, front_sector->ceiling, end.y};
          vec3_t p3 = {start.x, front_sector->ceiling, start.y};

          const float x = p1.x - p0.x, y = p1.z - p0.z;
          const float width  = sqrtf(x * x + y * y),
                      height = -fabsf(p3.y - p0.y);

          float tw = wall_textures_info[sidedef->upper].width;
          float th = wall_textures_info[sidedef->upper].height;

          float w = width / tw, h = height / th;
          float x_off = sidedef->x_off / tw, y_off = sidedef->y_off / th;
          if (linedef->flags & LINEDEF_FLAGS_UPPER_UNPEGGED) { y_off -= h; }

          float tx0 = x_off, ty0 = y_off;
          float tx1 = x_off + w, ty1 = y_off + h;

          vec2_t max_coords = wall_max_coords[sidedef->upper];
          tx0 *= max_coords.x, tx1 *= max_coords.x;
          ty0 *= max_coords.y, ty1 *= max_coords.y;

          float    light = front_sector->light_level / 256.f;
          vertex_t v[]   = {
              {p0, {tx0, ty0}, sidedef->upper, 2, light, max_coords},
              {p1, {tx1, ty0}, sidedef->upper, 2, light, max_coords},
              {p2, {tx1, ty1}, sidedef->upper, 2, light, max_coords},
              {p3, {tx0, ty1}, sidedef->upper, 2, light, max_coords},
          };

          start_idx = vertices.count;
          for (int i = 0; i < 4; i++) {
            dynarray_push(vertices, v[i]);
          }

          dynarray_push(indices, start_idx + 0);
          dynarray_push(indices, start_idx + 1);
          dynarray_push(indices, start_idx + 3);
          dynarray_push(indices, start_idx + 1);
          dynarray_push(indices, start_idx + 2);
          dynarray_push(indices, start_idx + 3);
        }
      } else {
        vec3_t p0 = {start.x, sector->floor, start.y};
        vec3_t p1 = {end.x, sector->floor, end.y};
        vec3_t p2 = {end.x, sector->ceiling, end.y};
        vec3_t p3 = {start.x, sector->ceiling, start.y};

        const float x = p1.x - p0.x, y = p1.z - p0.z;
        const float width = sqrtf(x * x + y * y), height = p3.y - p0.y;

        float tw = wall_textures_info[sidedef->middle].width;
        float th = wall_textures_info[sidedef->middle].height;

        float w = width / tw, h = height / th;
        float x_off = sidedef->x_off / tw, y_off = sidedef->y_off / th;
        if (linedef->flags & LINEDEF_FLAGS_LOWER_UNPEGGED) { y_off -= h; }

        float tx0 = x_off, ty0 = y_off + h;
        float tx1 = x_off + w, ty1 = y_off;

        vec2_t max_coords = wall_max_coords[sidedef->middle];
        tx0 *= max_coords.x, tx1 *= max_coords.x;
        ty0 *= max_coords.y, ty1 *= max_coords.y;

        float    light = sector->light_level / 256.f;
        vertex_t v[]   = {
            {p0, {tx0, ty0}, sidedef->middle, 2, light, max_coords},
            {p1, {tx1, ty0}, sidedef->middle, 2, light, max_coords},
            {p2, {tx1, ty1}, sidedef->middle, 2, light, max_coords},
            {p3, {tx0, ty1}, sidedef->middle, 2, light, max_coords},
        };

        start_idx = vertices.count;
        for (int i = 0; i < 4; i++) {
          dynarray_push(vertices, v[i]);
        }

        dynarray_push(indices, start_idx + 0);
        dynarray_push(indices, start_idx + 1);
        dynarray_push(indices, start_idx + 3);
        dynarray_push(indices, start_idx + 1);
        dynarray_push(indices, start_idx + 2);
        dynarray_push(indices, start_idx + 3);
      }
    }

    start_idx = vertices.count;
    for (int i = 0; i < n_vertices; i++) {
      int floor_tex = the_sector->floor_tex, ceil_tex = the_sector->ceiling_tex;

      floor_vertices[i].position.y = the_sector->floor;
      floor_vertices[i].texture_index =
          floor_tex >= 0 && floor_tex < num_flats ? floor_tex : -1;

      ceil_vertices[i].position.y = the_sector->ceiling;
      ceil_vertices[i].texture_index =
          ceil_tex >= 0 && ceil_tex < num_flats ? ceil_tex : -1;

      floor_vertices[i].light = ceil_vertices[i].light =
          the_sector->light_level / 256.f;
    }

    for (int i = 0; i < n_vertices; i++) {
      dynarray_push(vertices, floor_vertices[i]);
    }

    for (int i = 0; i < n_vertices; i++) {
      dynarray_push(vertices, ceil_vertices[i]);
    }

    // Triangulation will form (n - 2) triangles, so 2*3*(n - 3) indices are
    // required
    for (int j = 0, k = 1; j < n_vertices - 2; j++, k++) {
      dynarray_push(indices, start_idx + 0);
      dynarray_push(indices, start_idx + k + 1);
      dynarray_push(indices, start_idx + k);

      dynarray_push(indices, start_idx + n_vertices);
      dynarray_push(indices, start_idx + n_vertices + k);
      dynarray_push(indices, start_idx + n_vertices + k + 1);
    }

    free(floor_vertices);
    free(ceil_vertices);

    draw_node->mesh = malloc(sizeof(mesh_t));
    mesh_create(draw_node->mesh, vertices.count, vertices.data, indices.count,
                indices.data);
    dynarray_free(vertices);
    dynarray_free(indices);
  } else {
    gl_node_t *node = &gl_map->nodes[id];
    generate_node(&draw_node->front, node->front_child_id, map, gl_map);
    generate_node(&draw_node->back, node->back_child_id, map, gl_map);
  }
}

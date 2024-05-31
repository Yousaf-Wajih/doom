#include "engine/meshgen.h"
#include "dynarray.h"
#include "engine/anim.h"
#include "engine/state.h"
#include "engine/util.h"
#include "flat_texture.h"
#include "gl_map.h"
#include "map.h"
#include "matrix.h"
#include "vector.h"

#include <GL/glew.h>
#include <math.h>
#include <stdbool.h>

static void generate_node(draw_node_t **draw_node_ptr, size_t id);

void generate_meshes() {
  max_sector_height = 0.f;
  for (int i = 0; i < map.num_sectors; i++) {
    if (map.sectors[i].ceiling > max_sector_height) {
      max_sector_height = map.sectors[i].ceiling;
    }
  }
  max_sector_height += 1.f;

  float  width = map.max.x - map.min.x, height = map.max.y - map.min.y;
  vec3_t translate = {map.min.x, max_sector_height, map.max.y};

  mat4_t scale       = mat4_scale((vec3_t){width, height, 1.f});
  mat4_t translation = mat4_translate(translate);
  mat4_t rotation    = mat4_rotate((vec3_t){1.f, 0.f, 0.f}, M_PI / 2.f);
  mat4_t model       = mat4_mul(scale, mat4_mul(rotation, translation));
  insert_stencil_quad(model);

  generate_node(&root_draw_node, gl_map.num_nodes - 1);
}

void generate_node(draw_node_t **draw_node_ptr, size_t id) {
  draw_node_t *draw_node = malloc(sizeof(draw_node_t));
  *draw_node             = (draw_node_t){NULL, NULL, NULL};
  *draw_node_ptr         = draw_node;

  if (id & 0x8000) {
    gl_subsector_t *subsector = &gl_map.subsectors[id & 0x7fff];
    draw_node->mesh           = malloc(sizeof(mesh_t));

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
      gl_segment_t *segment = &gl_map.segments[j + subsector->first_seg];

      vec2_t start, end;
      if (segment->start_vertex & VERT_IS_GL) {
        start = gl_map.vertices[segment->start_vertex & 0x7fff];
      } else {
        start = map.vertices[segment->start_vertex];
      }

      if (segment->end_vertex & VERT_IS_GL) {
        end = gl_map.vertices[segment->end_vertex & 0x7fff];
      } else {
        end = map.vertices[segment->end_vertex];
      }

      if (the_sector == NULL && segment->linedef != 0xffff) {
        linedef_t *linedef    = &map.linedefs[segment->linedef];
        int        sector_idx = -1;
        if (linedef->flags & LINEDEF_FLAGS_TWO_SIDED && segment->side == 1) {
          sector_idx = map.sidedefs[linedef->back_sidedef].sector_idx;
        } else {
          sector_idx = map.sidedefs[linedef->front_sidedef].sector_idx;
        }

        if (sector_idx >= 0) { the_sector = &map.sectors[sector_idx]; }
      }

      floor_vertices[j] = ceil_vertices[j] = (vertex_t){
          .position     = {start.x, 0.f, start.y},
          .tex_coords   = {start.x / FLAT_TEXTURE_SIZE,
                           -start.y / FLAT_TEXTURE_SIZE},
          .texture_type = 1,
      };

      if (segment->linedef == 0xffff) { continue; }
      linedef_t *linedef = &map.linedefs[segment->linedef];

      sidedef_t *front_sidedef = &map.sidedefs[linedef->front_sidedef];
      sidedef_t *back_sidedef  = &map.sidedefs[linedef->back_sidedef];

      if (segment->side) {
        sidedef_t *tmp = front_sidedef;
        front_sidedef  = back_sidedef;
        back_sidedef   = tmp;
      }

      sector_t *front_sector = &map.sectors[front_sidedef->sector_idx];
      sector_t *back_sector  = &map.sectors[back_sidedef->sector_idx];

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

          if (sector->ceiling_tex == sky_flat) {
            float  quad_height = max_sector_height - p3.y;
            mat4_t scale       = mat4_scale((vec3_t){width, quad_height, 1.f});
            mat4_t translation = mat4_translate(p3);
            mat4_t rotation =
                mat4_rotate((vec3_t){0.f, 1.f, 0.f}, atan2f(y, x));
            mat4_t model = mat4_mul(scale, mat4_mul(rotation, translation));

            insert_stencil_quad(model);
          }
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

        if (sector->ceiling_tex == sky_flat) {
          float  quad_height = max_sector_height - p3.y;
          mat4_t scale       = mat4_scale((vec3_t){width, quad_height, 1.f});
          mat4_t translation = mat4_translate(p3);
          mat4_t rotation = mat4_rotate((vec3_t){0.f, 1.f, 0.f}, atan2f(y, x));
          mat4_t model    = mat4_mul(scale, mat4_mul(rotation, translation));

          insert_stencil_quad(model);
        }
      }
    }

    int floor_tex = the_sector->floor_tex, ceil_tex = the_sector->ceiling_tex;
    for (int i = 0; i < n_vertices; i++) {
      floor_vertices[i].position.y = the_sector->floor;
      floor_vertices[i].texture_index =
          floor_tex >= 0 && floor_tex < num_flats ? floor_tex : -1;

      ceil_vertices[i].position.y = the_sector->ceiling;
      ceil_vertices[i].texture_index =
          ceil_tex >= 0 && ceil_tex < num_flats ? ceil_tex : -1;

      floor_vertices[i].light = ceil_vertices[i].light =
          the_sector->light_level / 256.f;
    }

    start_idx = vertices.count;
    for (int i = 0; i < n_vertices; i++) {
      dynarray_push(vertices, floor_vertices[i]);
    }

    for (int i = 0; i < n_vertices; i++) {
      dynarray_push(vertices, ceil_vertices[i]);
    }

    for (int i = 0; i < num_tex_anim_defs; i++) {
      if (floor_tex >= tex_anim_defs[i].start &&
          floor_tex <= tex_anim_defs[i].end) {
        add_tex_anim(draw_node->mesh, start_idx, start_idx + n_vertices,
                     tex_anim_defs[i].start, tex_anim_defs[i].end);
      }

      if (ceil_tex >= tex_anim_defs[i].start &&
          ceil_tex <= tex_anim_defs[i].end) {
        add_tex_anim(draw_node->mesh, start_idx + n_vertices,
                     start_idx + 2 * n_vertices, tex_anim_defs[i].start,
                     tex_anim_defs[i].end);
      }
    }

    // Triangulation will form (n - 2) triangles, so 2*3*(n - 2) indices are
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

    mesh_create(draw_node->mesh, VERTEX_LAYOUT_FULL, vertices.count,
                vertices.data, indices.count, indices.data, true);
    dynarray_free(vertices);
    dynarray_free(indices);
  } else {
    gl_node_t *node = &gl_map.nodes[id];
    generate_node(&draw_node->front, node->front_child_id);
    generate_node(&draw_node->back, node->back_child_id);
  }
}

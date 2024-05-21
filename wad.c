#include "wad.h"
#include "flat_texture.h"
#include "gl_map.h"
#include "map.h"
#include "palette.h"
#include "patch.h"
#include "util.h"
#include "vector.h"
#include "wall_texture.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_I16(buffer, offset)                                               \
  ((buffer)[(offset)] | ((buffer)[(offset + 1)] << 8))

#define READ_I32(buffer, offset)                                               \
  ((buffer)[(offset)] | ((buffer)[(offset + 1)] << 8) |                        \
   ((buffer)[(offset + 2)] << 16) | ((buffer)[(offset + 3)] << 24))

int wad_load_from_file(const char *filename, wad_t *wad) {
  if (wad == NULL) { return 1; }

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) { return 2; }

  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t *buffer = malloc(size);
  fread(buffer, size, 1, fp);
  fclose(fp);

  // Read header
  if (size < 12) { return 3; }
  wad->id = malloc(5);
  memcpy(wad->id, buffer, 4);
  wad->id[4] = 0; // null terminator

  wad->num_lumps            = READ_I32(buffer, 4);
  uint32_t directory_offset = READ_I32(buffer, 8);

  wad->lumps = malloc(sizeof(lump_t) * wad->num_lumps);
  for (int i = 0; i < wad->num_lumps; i++) {
    uint32_t offset = directory_offset + i * 16;

    uint32_t lump_offset = READ_I32(buffer, offset);
    wad->lumps[i].size   = READ_I32(buffer, offset + 4);
    wad->lumps[i].name   = malloc(9);
    memcpy(wad->lumps[i].name, &buffer[offset + 8], 8);
    wad->lumps[i].name[8] = 0; // null terminator

    wad->lumps[i].data = malloc(wad->lumps[i].size);
    memcpy(wad->lumps[i].data, &buffer[lump_offset], wad->lumps[i].size);
  }

  free(buffer);
  return 0;
}

void wad_free(wad_t *wad) {
  if (wad == NULL) { return; }

  for (int i = 0; i < wad->num_lumps; i++) {
    free(wad->lumps[i].data);
  }

  free(wad->id);
  free(wad->lumps);

  wad->num_lumps = 0;
}

int wad_find_lump(const char *lumpname, const wad_t *wad) {
  for (int i = 0; i < wad->num_lumps; i++) {
    if (strcmp_nocase(wad->lumps[i].name, lumpname) == 0) { return i; }
  }

  return -1;
}

palette_t *wad_read_playpal(size_t *num, const wad_t *wad) {
  int playpal_index = wad_find_lump("PLAYPAL", wad);
  if (playpal_index < 0) { return NULL; }

  size_t palette_size = NUM_COLORS * 3;
  *num                = wad->lumps[playpal_index].size / palette_size;

  palette_t *palettes = malloc(sizeof(palette_t) * *num);
  for (int i = 0; i < *num; i++) {
    memcpy(palettes[i].colors,
           wad->lumps[playpal_index].data + i * palette_size, palette_size);
  }

  return palettes;
}

flat_tex_t *wad_read_flats(size_t *num, const wad_t *wad) {
  int f_start = wad_find_lump("F_START", wad);
  int f_end   = wad_find_lump("F_END", wad);

  if (num == NULL || f_end < 0 || f_start < 0) { return NULL; }

  *num              = f_end - f_start - 1;
  flat_tex_t *flats = malloc(sizeof(flat_tex_t) * *num);

  for (int i = f_start + 1; i < f_end; i++) {
    if (wad->lumps[i].size != FLAT_TEXTURE_SIZE * FLAT_TEXTURE_SIZE) {
      (*num)--;
      continue;
    }

    memcpy(flats[i - f_start - 1].data, wad->lumps[i].data,
           FLAT_TEXTURE_SIZE * FLAT_TEXTURE_SIZE);
  }

  return flats;
}

int wad_read_patch(patch_t *patch, const char *patch_name, const wad_t *wad) {
  *patch             = (patch_t){0};
  int patch_lump_idx = wad_find_lump(patch_name, wad);
  if (patch_lump_idx < 0) { return 1; }
  lump_t *patch_lump = &wad->lumps[patch_lump_idx];

  patch->width  = READ_I16(patch_lump->data, 0);
  patch->height = READ_I16(patch_lump->data, 2);
  patch->data   = malloc(patch->width * patch->height);
  memset(patch->data, 247, patch->width * patch->height);

  for (int16_t x = 0; x < patch->width; x++) {
    uint32_t column_offset = READ_I32(patch_lump->data, 8 + x * 4);
    uint8_t  post_topdelta = 0;
    for (;;) {
      post_topdelta = patch_lump->data[column_offset++];
      if (post_topdelta == 255) { break; }
      uint8_t post_length = patch_lump->data[column_offset++];
      column_offset++; // dummy value

      for (int y = 0; y < post_length; y++) {
        int data_byte = patch_lump->data[column_offset++];
        int tex_x = x, tex_y = y + post_topdelta;
        patch->data[tex_y * patch->width + tex_x] = data_byte;
      }
      column_offset++; // dummy value
    }
  }

  return 0;
}

patch_t *wad_read_patches(size_t *num, const wad_t *wad) {
  int     pnames_index = wad_find_lump("PNAMES", wad);
  lump_t *pnames_lump  = &wad->lumps[pnames_index];
  *num                 = READ_I32(pnames_lump->data, 0);
  patch_t *patches     = malloc(sizeof(patch_t) * *num);

  for (int i = 0; i < *num; i++) {
    char patch_name[9] = {0};
    memcpy(patch_name, &pnames_lump->data[i * 8 + 4], 8);
    wad_read_patch(&patches[i], patch_name, wad);
  }

  return patches;
}

void wad_free_patches(patch_t *patches, size_t num) {
  for (int i = 0; i < num; i++) {
    patches[i].width = patches[i].height = 0;
    free(patches[i].data);
  }
}

wall_tex_t *wad_read_textures(size_t *num, const char *lumpname,
                              const wad_t *wad) {
  size_t   num_patches;
  patch_t *patches = wad_read_patches(&num_patches, wad);

  int     lump_index = wad_find_lump(lumpname, wad);
  lump_t *tex_lump   = &wad->lumps[lump_index];
  *num               = READ_I32(tex_lump->data, 0);

  wall_tex_t *textures = malloc(sizeof(wall_tex_t) * *num);
  for (int i = 0; i < *num; i++) {
    uint32_t offset = READ_I32(tex_lump->data, 4 * i + 4);
    memcpy(textures[i].name, tex_lump->data + offset, 8);
    textures[i].width  = READ_I16(tex_lump->data, offset + 12);
    textures[i].height = READ_I16(tex_lump->data, offset + 14);

    textures[i].data = malloc(textures[i].width * textures[i].height);
    memset(textures[i].data, 247, textures[i].width * textures[i].height);

    uint16_t num_patches = READ_I16(tex_lump->data, offset + 20);
    for (int j = 0; j < num_patches; j++) {
      int16_t  origin_x  = READ_I16(tex_lump->data, offset + 22 + j * 10);
      int16_t  origin_y  = READ_I16(tex_lump->data, offset + 24 + j * 10);
      uint16_t patch_idx = READ_I16(tex_lump->data, offset + 26 + j * 10);

      patch_t patch = patches[patch_idx];
      for (int x = 0; x < patch.width; x++) {
        for (int y = 0; y < patch.height; y++) {
          uint8_t data_byte = patch.data[y * patch.width + x];
          int     tex_x = x + origin_x, tex_y = y + origin_y;

          if (tex_x >= 0 && tex_x < textures[i].width && tex_y >= 0 &&
              tex_y < textures[i].height && data_byte != 247) {
            textures[i].data[tex_y * textures[i].width + tex_x] = data_byte;
          }
        }
      }
    }
  }

  wad_free_patches(patches, num_patches);
  return textures;
}

void wad_free_wall_textures(wall_tex_t *textures, size_t num) {
  for (int i = 0; i < num; i++) {
    textures[i].width = textures[i].height = 0;
    free(textures[i].data);
  }
}

#define THINGS_IDX   1
#define LINEDEFS_IDX 2
#define SIDEDEFS_IDX 3
#define VERTEXES_IDX 4
#define SEGS_IDX     5
#define SSECTORS_IDX 6
#define NODES_IDX    7
#define SECTORS_IDX  8

static void read_vertices(map_t *map, const lump_t *lump);
static void read_linedefs(map_t *map, const lump_t *lump);
static void read_things(map_t *map, const lump_t *lump);
static void read_sectors(map_t *map, const lump_t *lump, const wad_t *wad);
static void read_sidedefs(map_t *map, const lump_t *lump, const wall_tex_t *tex,
                          int num_tex);

int wad_read_map(const char *mapname, map_t *map, const wad_t *wad,
                 const wall_tex_t *tex, int num_tex) {
  int map_index = wad_find_lump(mapname, wad);
  if (map_index < 0) { return 1; }

  read_vertices(map, &wad->lumps[map_index + VERTEXES_IDX]);
  read_linedefs(map, &wad->lumps[map_index + LINEDEFS_IDX]);
  read_things(map, &wad->lumps[map_index + THINGS_IDX]);
  read_sidedefs(map, &wad->lumps[map_index + SIDEDEFS_IDX], tex, num_tex);
  read_sectors(map, &wad->lumps[map_index + SECTORS_IDX], wad);

  return 0;
}

void wad_free_map(map_t *map) {
  map->num_vertices = map->num_things = map->num_sectors = map->num_linedefs =
      map->num_sidedefs                                  = 0;

  free(map->vertices);
  free(map->things);
  free(map->sectors);
  free(map->linedefs);
  free(map->sidedefs);
}

void read_vertices(map_t *map, const lump_t *lump) {
  map->num_vertices = lump->size / 4; // each vertex is 2+2=4 bytes
  map->vertices     = malloc(sizeof(vec2_t) * map->num_vertices);

  map->min = (vec2_t){INFINITY, INFINITY};
  map->max = (vec2_t){-INFINITY, -INFINITY};

  for (int i = 0, j = 0; i < lump->size; i += 4, j++) {
    map->vertices[j].x = (int16_t)READ_I16(lump->data, i);
    map->vertices[j].y = (int16_t)READ_I16(lump->data, i + 2);

    if (map->vertices[j].x < map->min.x) { map->min.x = map->vertices[j].x; }
    if (map->vertices[j].y < map->min.y) { map->min.y = map->vertices[j].y; }
    if (map->vertices[j].x > map->max.x) { map->max.x = map->vertices[j].x; }
    if (map->vertices[j].y > map->max.y) { map->max.y = map->vertices[j].y; }
  }
}

void read_linedefs(map_t *map, const lump_t *lump) {
  map->num_linedefs = lump->size / 14; // each linedef is 14 bytes
  map->linedefs     = malloc(sizeof(linedef_t) * map->num_linedefs);

  for (int i = 0, j = 0; i < lump->size; i += 14, j++) {
    map->linedefs[j].start_idx     = READ_I16(lump->data, i);
    map->linedefs[j].end_idx       = READ_I16(lump->data, i + 2);
    map->linedefs[j].flags         = READ_I16(lump->data, i + 4);
    map->linedefs[j].front_sidedef = READ_I16(lump->data, i + 10);
    map->linedefs[j].back_sidedef  = READ_I16(lump->data, i + 12);
  }
}

void read_things(map_t *map, const lump_t *lump) {
  map->num_things = lump->size / 10; // each thing is 10 bytes
  map->things     = malloc(sizeof(thing_t) * map->num_things);

  for (int i = 0, j = 0; i < lump->size; i += 10, j++) {
    map->things[j].position.x = (int16_t)READ_I16(lump->data, i);
    map->things[j].position.y = (int16_t)READ_I16(lump->data, i + 2);
    map->things[j].type       = READ_I16(lump->data, i + 6);

    float angle          = (int16_t)READ_I16(lump->data, i + 4);
    map->things[j].angle = angle * M_PI / 180.f;
  }
}

void read_sidedefs(map_t *map, const lump_t *lump, const wall_tex_t *tex,
                   int num_tex) {
  map->num_sidedefs = lump->size / 30; // each sidedef is 30 bytes
  map->sidedefs     = malloc(sizeof(sidedef_t) * map->num_sidedefs);

  for (int i = 0, j = 0; i < lump->size; i += 30, j++) {
    map->sidedefs[j].lower = map->sidedefs[j].upper = map->sidedefs[j].middle =
        -1;

    for (int k = 0; k < num_tex; k++) {
      if (strncmp_nocase((char *)lump->data + i + 4, tex[k].name, 8) == 0) {
        map->sidedefs[j].upper = k;
        break;
      }
    }

    for (int k = 0; k < num_tex; k++) {
      if (strncmp_nocase((char *)lump->data + i + 12, tex[k].name, 8) == 0) {
        map->sidedefs[j].lower = k;
        break;
      }
    }

    for (int k = 0; k < num_tex; k++) {
      if (strncmp_nocase((char *)lump->data + i + 20, tex[k].name, 8) == 0) {
        map->sidedefs[j].middle = k;
        break;
      }
    }

    map->sidedefs[j].x_off      = (int16_t)READ_I16(lump->data, i);
    map->sidedefs[j].y_off      = (int16_t)READ_I16(lump->data, i + 2);
    map->sidedefs[j].sector_idx = READ_I16(lump->data, i + 28);
  }
}

void read_sectors(map_t *map, const lump_t *lump, const wad_t *wad) {
  map->num_sectors = lump->size / 26; // each sector is 26 bytes
  map->sectors     = malloc(sizeof(sector_t) * map->num_sectors);

  int f_start = wad_find_lump("F_START", wad);
  int f_end   = wad_find_lump("F_END", wad);
  for (int i = 0, j = 0; i < lump->size; i += 26, j++) {
    map->sectors[j].floor       = (int16_t)READ_I16(lump->data, i);
    map->sectors[j].ceiling     = (int16_t)READ_I16(lump->data, i + 2);
    map->sectors[j].light_level = (int16_t)READ_I16(lump->data, i + 20);

    char name[9] = {0};

    memcpy(name, &lump->data[i + 4], 8);
    int floor = wad_find_lump(name, wad);
    if (floor <= f_start || floor >= f_end - 1) {
      map->sectors[j].floor_tex = -1;
    } else {
      map->sectors[j].floor_tex = floor - f_start - 1;
    }

    memcpy(name, &lump->data[i + 12], 8);
    int ceiling = wad_find_lump(name, wad);
    if (ceiling <= f_start || ceiling >= f_end - 1) {
      map->sectors[j].ceiling_tex = -1;
    } else {
      map->sectors[j].ceiling_tex = ceiling - f_start - 1;
    }
  }
}

#define GL_VERTICES_IDX 1
#define GL_SEGS_IDX     2
#define GL_SSECTORS_IDX 3
#define GL_NODES_IDX    4

static void read_gl_vertices(gl_map_t *map, const lump_t *lump);
static void read_gl_segments(gl_map_t *map, const lump_t *lump);
static void read_gl_subsectors(gl_map_t *map, const lump_t *lump);
static void read_gl_nodes(gl_map_t *map, const lump_t *lump);

int wad_read_gl_map(const char *gl_mapname, gl_map_t *map, const wad_t *wad) {
  int map_index = wad_find_lump(gl_mapname, wad);
  if (map_index < 0) { return 1; }

  if (strncmp((const char *)wad->lumps[map_index + GL_VERTICES_IDX].data,
              "gNd2", 4) != 0) {
    return 2;
  }

  if (strncmp((const char *)wad->lumps[map_index + GL_SEGS_IDX].data, "gNd3",
              4) == 0) {
    return 2;
  }

  read_gl_vertices(map, &wad->lumps[map_index + GL_VERTICES_IDX]);
  read_gl_segments(map, &wad->lumps[map_index + GL_SEGS_IDX]);
  read_gl_subsectors(map, &wad->lumps[map_index + GL_SSECTORS_IDX]);
  read_gl_nodes(map, &wad->lumps[map_index + GL_NODES_IDX]);

  return 0;
}

void read_gl_vertices(gl_map_t *map, const lump_t *lump) {
  map->num_vertices = (lump->size - 4) / 8; // each vertex is 4+4=8 bytes
  map->vertices     = malloc(sizeof(vec2_t) * map->num_vertices);

  map->min = (vec2_t){INFINITY, INFINITY};
  map->max = (vec2_t){-INFINITY, -INFINITY};

  for (int i = 4, j = 0; i < lump->size; i += 8, j++) {
    map->vertices[j].x = (float)((int32_t)READ_I32(lump->data, i)) / (1 << 16);
    map->vertices[j].y =
        (float)((int32_t)READ_I32(lump->data, i + 4)) / (1 << 16);

    if (map->vertices[j].x < map->min.x) { map->min.x = map->vertices[j].x; }
    if (map->vertices[j].y < map->min.y) { map->min.y = map->vertices[j].y; }
    if (map->vertices[j].x > map->max.x) { map->max.x = map->vertices[j].x; }
    if (map->vertices[j].y > map->max.y) { map->max.y = map->vertices[j].y; }
  }
}

void read_gl_segments(gl_map_t *map, const lump_t *lump) {
  map->num_segments = lump->size / 10; // each segment is 10 bytes
  map->segments     = malloc(sizeof(gl_segment_t) * map->num_segments);

  for (int i = 0, j = 0; i < lump->size; i += 10, j++) {
    map->segments[j].start_vertex = READ_I16(lump->data, i);
    map->segments[j].end_vertex   = READ_I16(lump->data, i + 2);
    map->segments[j].linedef      = READ_I16(lump->data, i + 4);
    map->segments[j].side         = READ_I16(lump->data, i + 6);
  }
}

void read_gl_subsectors(gl_map_t *map, const lump_t *lump) {
  map->num_subsectors = lump->size / 4; // each subsector is 4 bytes
  map->subsectors     = malloc(sizeof(gl_subsector_t) * map->num_subsectors);

  for (int i = 0, j = 0; i < lump->size; i += 4, j++) {
    map->subsectors[j].num_segs  = READ_I16(lump->data, i);
    map->subsectors[j].first_seg = READ_I16(lump->data, i + 2);
  }
}

void read_gl_nodes(gl_map_t *map, const lump_t *lump) {
  map->num_nodes = lump->size / 28; // each node is 28 bytes
  map->nodes     = malloc(sizeof(gl_node_t) * map->num_nodes);

  for (int i = 0, j = 0; i < lump->size; i += 28, j++) {
    map->nodes[j].partition.x       = (int16_t)READ_I16(lump->data, i);
    map->nodes[j].partition.y       = (int16_t)READ_I16(lump->data, i + 2);
    map->nodes[j].delta_partition.x = (int16_t)READ_I16(lump->data, i + 4);
    map->nodes[j].delta_partition.y = (int16_t)READ_I16(lump->data, i + 6);

    map->nodes[j].front_bbox[0] = READ_I16(lump->data, i + 8);
    map->nodes[j].front_bbox[1] = READ_I16(lump->data, i + 10);
    map->nodes[j].front_bbox[2] = READ_I16(lump->data, i + 12);
    map->nodes[j].front_bbox[3] = READ_I16(lump->data, i + 14);

    map->nodes[j].back_bbox[0] = READ_I16(lump->data, i + 16);
    map->nodes[j].back_bbox[1] = READ_I16(lump->data, i + 18);
    map->nodes[j].back_bbox[2] = READ_I16(lump->data, i + 20);
    map->nodes[j].back_bbox[3] = READ_I16(lump->data, i + 22);

    map->nodes[j].front_child_id = READ_I16(lump->data, i + 24);
    map->nodes[j].back_child_id  = READ_I16(lump->data, i + 26);
  }
}

void wad_free_gl_map(gl_map_t *map) {
  map->num_vertices = map->num_segments = map->num_subsectors = 0;
  free(map->vertices);
  free(map->segments);
  free(map->subsectors);
}

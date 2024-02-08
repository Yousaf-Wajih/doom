#include "wad.h"
#include "map.h"
#include "vector.h"

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
    if (strcmp(wad->lumps[i].name, lumpname) == 0) { return i; }
  }

  return -1;
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
static void read_sidedefs(map_t *map, const lump_t *lump);
static void read_sectors(map_t *map, const lump_t *lump);

int wad_read_map(const char *mapname, map_t *map, const wad_t *wad) {
  int map_index = wad_find_lump(mapname, wad);
  if (map_index < 0) { return 1; }

  read_vertices(map, &wad->lumps[map_index + VERTEXES_IDX]);
  read_linedefs(map, &wad->lumps[map_index + LINEDEFS_IDX]);
  read_sidedefs(map, &wad->lumps[map_index + SIDEDEFS_IDX]);
  read_sectors(map, &wad->lumps[map_index + SECTORS_IDX]);

  return 0;
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

void read_sidedefs(map_t *map, const lump_t *lump) {
  map->num_sidedefs = lump->size / 30; // each sidedef is 30 bytes
  map->sidedefs     = malloc(sizeof(sidedef_t) * map->num_sidedefs);

  for (int i = 0, j = 0; i < lump->size; i += 30, j++) {
    map->sidedefs[j].sector_idx = READ_I16(lump->data, i + 28);
  }
}

void read_sectors(map_t *map, const lump_t *lump) {
  map->num_sectors = lump->size / 26; // each sector is 26 bytes
  map->sectors     = malloc(sizeof(sector_t) * map->num_sectors);

  for (int i = 0, j = 0; i < lump->size; i += 26, j++) {
    map->sectors[j].floor   = (int16_t)READ_I16(lump->data, i);
    map->sectors[j].ceiling = (int16_t)READ_I16(lump->data, i + 2);
  }
}

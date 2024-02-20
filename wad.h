#ifndef _WAD_H
#define _WAD_H

#include "gl_map.h"
#include "map.h"
#include <stdint.h>

typedef struct lump {
  char    *name;
  uint8_t *data;
  uint32_t size;
} lump_t;

typedef struct wad {
  char    *id;
  uint32_t num_lumps;

  lump_t *lumps;
} wad_t;

int  wad_load_from_file(const char *filename, wad_t *wad);
void wad_free(wad_t *wad);

int wad_find_lump(const char *lumpname, const wad_t *wad);
int wad_read_map(const char *mapname, map_t *map, const wad_t *wad);
int wad_read_gl_map(const char *gl_mapname, gl_map_t *map, const wad_t *wad);

#endif // !_WAD_H

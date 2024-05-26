#ifndef _WAD_H
#define _WAD_H

#include <stdint.h>

#include "flat_texture.h"
#include "gl_map.h"
#include "map.h"
#include "palette.h"
#include "patch.h"
#include "wall_texture.h"

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
int wad_read_gl_map(const char *gl_mapname, gl_map_t *map, const wad_t *wad);
int wad_read_map(const char *mapname, map_t *map, const wad_t *wad,
                 const wall_tex_t *tex, int num_tex);

int wad_read_patch(patch_t *patch, const char *patch_name, const wad_t *wad);
palette_t  *wad_read_playpal(size_t *num, const wad_t *wad);
flat_tex_t *wad_read_flats(size_t *num, const wad_t *wad);
patch_t    *wad_read_patches(size_t *num, const wad_t *wad);
wall_tex_t *wad_read_textures(size_t *num, const char *lumpname,
                              const wad_t *wad);

void wad_free_map(map_t *map);
void wad_free_gl_map(gl_map_t *map);
void wad_free_patches(patch_t *patches, size_t num);
void wad_free_wall_textures(wall_tex_t *textures, size_t num);

#endif // !_WAD_H

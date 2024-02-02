#ifndef _WAD_H
#define _WAD_H

#include <stdint.h>

typedef struct lump {
  char    *name;
  uint8_t *data;
  uint32_t offset;
  uint32_t size;
} lump_t;

typedef struct wad {
  char    *id;
  uint32_t num_lumps;
  uint32_t directory_offset;

  lump_t *lumps;
} wad_t;

int  wad_load_from_file(const char *filename, wad_t *wad);
void wad_free(wad_t *wad);

#endif // !_WAD_H

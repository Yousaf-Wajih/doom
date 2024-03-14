#ifndef _PATCH_H
#define _PATCH_H

#include <stdint.h>

typedef struct patch {
  uint16_t width, height;
  uint8_t *data;
} patch_t;

#endif // !_PATCH_H

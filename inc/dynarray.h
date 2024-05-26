#pragma once

#include <stddef.h>
#include <stdlib.h>

#define dynarray(type)                                                         \
  struct {                                                                     \
    size_t count, capacity, elem_size;                                         \
    type  *data;                                                               \
    type   __dummy;                                                            \
  }

#define dynarray_init(array, init_cap)                                         \
  do {                                                                         \
    array.count     = 0;                                                       \
    array.capacity  = init_cap;                                                \
    array.elem_size = sizeof(array.__dummy);                                   \
    if (init_cap > 0) { array.data = malloc(array.elem_size * init_cap); }     \
  } while (0)

#define dynarray_free(array)                                                   \
  do {                                                                         \
    array.count    = 0;                                                        \
    array.capacity = 0;                                                        \
    if (array.capacity > 0) { free(array.data); }                              \
  } while (0)

#define dynarray_push(array, value)                                            \
  do {                                                                         \
    if (array.count >= array.capacity) {                                       \
      if (array.capacity < 8) {                                                \
        array.capacity = 8;                                                    \
        array.data     = malloc(array.elem_size * array.capacity);             \
      } else {                                                                 \
        array.capacity *= 2;                                                   \
        array.data = realloc(array.data, array.elem_size * array.capacity);    \
      }                                                                        \
    }                                                                          \
    array.data[array.count++] = (value);                                       \
  } while (0)

#ifndef _CAMERA_H
#define _CAMERA_H

#include "vector.h"

#define WORLD_UP ((vec3_t){0.f, 1.f, 0.f})

typedef struct camera {
  vec3_t position;
  float  yaw, pitch;

  vec3_t forward, right, up;
} camera_t;

void camera_update_direction_vectors(camera_t *camera);

#endif // !_CAMERA_H

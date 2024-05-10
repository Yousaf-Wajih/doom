#ifndef _VECTOR_H
#define _VECTOR_H

typedef union vec2 {
  struct {
    float x, y;
  };
  float v[2];
} vec2_t;

typedef union vec3 {
  struct {
    float x, y, z;
  };
  struct {
    float r, g, b;
  };
  float v[3];
} vec3_t;

typedef union vec4 {
  struct {
    float x, y, z, w;
  };
  struct {
    float r, g, b, a;
  };
  float v[4];
} vec4_t;

vec2_t vec2_add(vec2_t a, vec2_t b);
vec2_t vec2_sub(vec2_t a, vec2_t b);

vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_sub(vec3_t a, vec3_t b);
vec3_t vec3_scale(vec3_t v, float s);

float  vec3_length(vec3_t v);
vec3_t vec3_normalize(vec3_t v);

float  vec3_dot(vec3_t a, vec3_t b);
vec3_t vec3_cross(vec3_t a, vec3_t b);

#endif // !_VECTOR_H

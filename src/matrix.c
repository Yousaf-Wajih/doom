#include "matrix.h"
#include <math.h>

mat4_t mat4_identity() {
  mat4_t mat = {0};

  mat.a1 = 1.f;
  mat.b2 = 1.f;
  mat.c3 = 1.f;
  mat.d4 = 1.f;

  return mat;
}

mat4_t mat4_mul(mat4_t m1, mat4_t m2) {
  mat4_t result;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      result.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] +
                       m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
    }
  }

  return result;
}

mat4_t mat4_translate(vec3_t translation) {
  mat4_t mat = mat4_identity();

  mat.d1 = translation.x;
  mat.d2 = translation.y;
  mat.d3 = translation.z;

  return mat;
}

mat4_t mat4_scale(vec3_t scale) {
  mat4_t mat = {0};

  mat.a1 = scale.x;
  mat.b2 = scale.y;
  mat.c3 = scale.z;
  mat.d4 = 1.f;

  return mat;
}

mat4_t mat4_rotate(vec3_t axis, float angle) {
  float c = cosf(angle);
  float s = sinf(angle);
  float t = 1.f - c;

  vec3_t axis_normalized = vec3_normalize(axis);
  float  x = axis_normalized.x, y = axis_normalized.y, z = axis_normalized.z;

  mat4_t result = {
      .a1 = t * x * x + c,
      .a2 = t * x * y - s * z,
      .a3 = t * x * z + s * y,
      .a4 = 0.0f,
      .b1 = t * x * y + s * z,
      .b2 = t * y * y + c,
      .b3 = t * y * z - s * x,
      .b4 = 0.0f,
      .c1 = t * x * z - s * y,
      .c2 = t * y * z + s * x,
      .c3 = t * z * z + c,
      .c4 = 0.0f,
      .d1 = 0.0f,
      .d2 = 0.0f,
      .d3 = 0.0f,
      .d4 = 1.0f,
  };

  return result;
}

mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up) {
  vec3_t zaxis = vec3_normalize(vec3_sub(target, eye));
  vec3_t xaxis = vec3_normalize(vec3_cross(up, zaxis));
  vec3_t yaxis = vec3_cross(zaxis, xaxis);

  mat4_t result = {
      .a1 = xaxis.x,
      .a2 = yaxis.x,
      .a3 = -zaxis.x,
      .a4 = 0.0f,
      .b1 = xaxis.y,
      .b2 = yaxis.y,
      .b3 = -zaxis.y,
      .b4 = 0.0f,
      .c1 = xaxis.z,
      .c2 = yaxis.z,
      .c3 = -zaxis.z,
      .c4 = 0.0f,
      .d1 = -vec3_dot(xaxis, eye),
      .d2 = -vec3_dot(yaxis, eye),
      .d3 = vec3_dot(zaxis, eye),
      .d4 = 1.0f,
  };

  return result;
}

mat4_t mat4_perspective(float fov, float aspect_ratio, float near, float far) {
  mat4_t result = {0};

  float tan_half_fov = tanf(fov / 2.0f);
  float range_inv    = 1.0f / (near - far);

  result.a1 = 1.0f / (aspect_ratio * tan_half_fov);
  result.b2 = 1.0f / tan_half_fov;
  result.c3 = (near + far) * range_inv;
  result.c4 = -1.0f;
  result.d3 = 2.0f * near * far * range_inv;
  result.d4 = 0.0f;

  return result;
}

mat4_t mat4_ortho(float left, float right, float bottom, float top, float near,
                  float far) {
  mat4_t mat = {0};

  mat.a1 = 2.f / (right - left);
  mat.b2 = 2.f / (top - bottom);
  mat.c3 = -2.f / (far - near);
  mat.d1 = -(right + left) / (right - left);
  mat.d2 = -(top + bottom) / (top - bottom);
  mat.d3 = -(far + near) / (far - near);
  mat.d4 = 1.f;

  return mat;
}

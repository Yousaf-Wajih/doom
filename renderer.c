#include "renderer.h"
#include "gl_helpers.h"
#include "matrix.h"
#include "mesh.h"
#include "vector.h"

#include <GL/glew.h>
#include <math.h>
#include <stdint.h>

static void init_quad();
static void init_shader();
static void init_projection();

const char *vertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 pos;\n"
    "layout(location = 1) in vec2 texCoords;\n"
    "out vec2 TexCoords;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "  gl_Position = projection * view * model * vec4(pos, 1.0);\n"
    "  TexCoords = texCoords;\n"
    "}\n";

const char *fragSrc =
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 fragColor;\n"
    "uniform bool useTexture;\n"
    "uniform usampler2DArray tex;\n"
    "uniform int texIdx;\n"
    "uniform sampler1D palette;\n"
    "uniform int color;\n"
    "void main() {\n"
    "  if (useTexture) {\n"
    "    fragColor = texelFetch(palette, int(texture(tex, vec3(TexCoords, "
    "texIdx)).r), 0);\n"
    "  } else {\n"
    "    fragColor = texelFetch(palette, color, 0);\n"
    "  }\n"
    "}\n";

static mesh_t quad_mesh;
static float  width, height;
static GLuint program;
static GLuint model_location, view_location, projection_location;
static GLuint color_location, use_texture_location, texture_index_location;

void renderer_init(int w, int h) {
  width  = w;
  height = h;

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glEnable(GL_DEPTH_TEST);

  init_quad();
  init_shader();
  init_projection();
}

void renderer_clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void renderer_set_view(mat4_t view) {
  glUniformMatrix4fv(view_location, 1, GL_FALSE, view.v);
}

void renderer_set_projection(mat4_t projection) {
  glUniformMatrix4fv(projection_location, 1, GL_FALSE, projection.v);
}

void renderer_set_palette_texture(GLuint palette_texture) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_1D, palette_texture);
}

void renderer_set_texture_index(int index) {
  glUniform1i(texture_index_location, index);
}

void renderer_set_draw_texture(GLuint texture) {
  if (texture == 0) {
    glUniform1i(use_texture_location, 0);
  } else {
    glUniform1i(use_texture_location, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
  }
}

vec2_t renderer_get_size() { return (vec2_t){width, height}; }

void renderer_draw_mesh(const mesh_t *mesh, mat4_t transformation, int color) {
  glUniform1i(color_location, color);
  glUniformMatrix4fv(model_location, 1, GL_FALSE, transformation.v);

  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, NULL);
}

void renderer_draw_point(vec2_t point, float size, int color) {
  mat4_t translation = mat4_translate((vec3_t){point.x, point.y, 0.f});
  mat4_t scale       = mat4_scale((vec3_t){size, size, 1.f});
  mat4_t model       = mat4_mul(scale, translation);

  renderer_draw_mesh(&quad_mesh, model, color);
}

void renderer_draw_line(vec2_t p0, vec2_t p1, float width, int color) {
  float x = p1.x - p0.x, y = p0.y - p1.y;
  float r = sqrtf(x * x + y * y), angle = atan2f(y, x);

  mat4_t translation =
      mat4_translate((vec3_t){(p0.x + p1.x) / 2.f, (p0.y + p1.y) / 2.f, 0.f});
  mat4_t scale    = mat4_scale((vec3_t){r, width, 1.f});
  mat4_t rotation = mat4_rotate((vec3_t){0.f, 0.f, 1.f}, angle);
  mat4_t model    = mat4_mul(mat4_mul(scale, rotation), translation);

  renderer_draw_mesh(&quad_mesh, model, color);
}

void renderer_draw_quad(vec2_t center, vec2_t size, float angle, int color) {
  mat4_t translation = mat4_translate((vec3_t){center.x, center.y, 0.f});
  mat4_t scale       = mat4_scale((vec3_t){size.x, size.y, 1.f});
  mat4_t rotation    = mat4_rotate((vec3_t){0.f, 0.f, 1.f}, angle);
  mat4_t model       = mat4_mul(mat4_mul(scale, rotation), translation);

  renderer_draw_mesh(&quad_mesh, model, color);
}

static void init_shader() {
  GLuint vertex   = compile_shader(GL_VERTEX_SHADER, vertSrc);
  GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragSrc);
  program         = link_program(2, vertex, fragment);

  glUseProgram(program);

  projection_location    = glGetUniformLocation(program, "projection");
  model_location         = glGetUniformLocation(program, "model");
  view_location          = glGetUniformLocation(program, "view");
  color_location         = glGetUniformLocation(program, "color");
  use_texture_location   = glGetUniformLocation(program, "useTexture");
  texture_index_location = glGetUniformLocation(program, "texIdx");

  GLuint palette_location = glGetUniformLocation(program, "palette");
  glUniform1i(palette_location, 0);

  GLuint texture_location = glGetUniformLocation(program, "tex");
  glUniform1i(texture_location, 1);
}

static void init_quad() {
  vertex_t vertices[] = {
      {.position = {.5f, .5f, 0.f},   .tex_coords = {1.f, 1.f}}, // top-right
      {.position = {.5f, -.5f, 0.f},  .tex_coords = {1.f, 0.f}}, // bottom-right
      {.position = {-.5f, -.5f, 0.f}, .tex_coords = {0.f, 0.f}}, // bottom-left
      {.position = {-.5f, .5f, 0.f},  .tex_coords = {0.f, 1.f}}, // top-left
  };

  uint32_t indices[] = {
      0, 1, 3, // 1st triangle
      1, 2, 3, // 2nd triangle
  };

  mesh_create(&quad_mesh, 4, vertices, 6, indices);
}

void init_projection() {
  mat4_t projection          = mat4_ortho(0.f, width, height, 0.f, -1.f, 1.f);
  GLuint projection_location = glGetUniformLocation(program, "projection");
  glUniformMatrix4fv(projection_location, 1, GL_FALSE, projection.v);
  glUniformMatrix4fv(view_location, 1, GL_FALSE, mat4_identity().v);
}

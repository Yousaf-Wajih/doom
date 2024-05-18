#include "renderer.h"
#include "gl_helpers.h"
#include "matrix.h"
#include "mesh.h"
#include "vector.h"

#include <GL/glew.h>
#include <stdint.h>

static void init_quad();
static void init_shader();
static void init_projection();

const char *vertSrc =
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "layout (location = 1) in vec2 texCoords;\n"
    "layout (location = 2) in int texIndex;\n"
    "layout (location = 3) in int texType;\n"
    "layout (location = 4) in float light;\n"
    "layout (location = 5) in vec2 maxTexCoords;\n"
    "out vec2 TexCoords;\n"
    "flat out int TexIndex;\n"
    "flat out int TexType;\n"
    "flat out vec2 MaxTexCoords;"
    "out float Light;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "  gl_Position = projection * view * model * vec4(pos, 1.0);\n"
    "  TexIndex = texIndex;\n"
    "  TexType = texType;\n"
    "  TexCoords = texCoords;\n"
    "  Light = light;\n"
    "  MaxTexCoords = maxTexCoords;\n"
    "}\n";

const char *fragSrc =
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "flat in int TexIndex;\n"
    "flat in int TexType;\n"
    "flat in vec2 MaxTexCoords;"
    "in float Light;\n"
    "out vec4 fragColor;\n"
    "uniform usampler2DArray flat_tex;\n"
    "uniform usampler2DArray wall_tex;\n"
    "uniform sampler1DArray palettes;\n"
    "uniform int palette_index;\n"
    "void main() {\n"
    "  vec3 color;"
    "  if (TexIndex == -1) { discard; }\n"
    "  else if (TexType == 0) {\n"
    "    color = vec3(texelFetch(palettes, ivec2(TexIndex, palette_index), "
    "             0));\n"
    "  } else if (TexType == 1) {\n"
    "    color = vec3(texelFetch(palettes, ivec2(int(texture(flat_tex, "
    "                 vec3(TexCoords, TexIndex)).r), palette_index), 0));\n"
    "  } else if (TexType == 2) {\n"
    "    color = vec3(texelFetch(palettes, ivec2(int(texture(wall_tex, "
    "                 vec3(fract(TexCoords / MaxTexCoords) * MaxTexCoords, "
    "                             TexIndex)).r), palette_index), 0));\n"
    "  }\n"
    "  fragColor = vec4(color * Light, 1.0);\n"
    "}\n";

static mesh_t quad_mesh;
static float  width, height;
static GLuint program;
static GLuint model_location, view_location, projection_location;
static GLuint palette_index_location;

void renderer_init(int w, int h) {
  width  = w;
  height = h;

  glClearColor(.1f, .1f, .1f, 1.f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

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
  glBindTexture(GL_TEXTURE_1D_ARRAY, palette_texture);
}

void renderer_set_palette_index(int index) {
  glUniform1i(palette_index_location, index);
}

void renderer_set_wall_texture(GLuint texture) {
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
}

void renderer_set_flat_texture(GLuint texture) {
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
}

vec2_t renderer_get_size() { return (vec2_t){width, height}; }

void renderer_draw_mesh(const mesh_t *mesh, mat4_t transformation) {
  glUniformMatrix4fv(model_location, 1, GL_FALSE, transformation.v);

  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, NULL);
}

static void init_shader() {
  GLuint vertex   = compile_shader(GL_VERTEX_SHADER, vertSrc);
  GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragSrc);
  program         = link_program(2, vertex, fragment);

  glUseProgram(program);

  projection_location    = glGetUniformLocation(program, "projection");
  model_location         = glGetUniformLocation(program, "model");
  view_location          = glGetUniformLocation(program, "view");
  palette_index_location = glGetUniformLocation(program, "palette_index");

  GLuint palette_location = glGetUniformLocation(program, "palettes");
  glUniform1i(palette_location, 0);

  GLuint texture_array_location = glGetUniformLocation(program, "flat_tex");
  glUniform1i(texture_array_location, 1);

  GLuint texture_location = glGetUniformLocation(program, "wall_tex");
  glUniform1i(texture_location, 2);
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

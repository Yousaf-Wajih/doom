#include "engine.h"
#include "gl_helpers.h"
#include "input.h"
#include "renderer.h"
#include "vector.h"
#include "wad.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH  1200
#define HEIGHT 675

int main(int argc, char **argv) {
  if (glfwInit() != GLFW_TRUE) {
    fprintf(stderr, "Failed to initalize GLFW\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "DooM", NULL, NULL);
  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initalize GLEW\n");
    return 1;
  }

  input_init(window);
  glfwSetKeyCallback(window, input_key_callback);
  glfwSetMouseButtonCallback(window, input_mouse_button_callback);
  glfwSetCursorPosCallback(window, input_mouse_position_callback);

  wad_t wad;
  if (wad_load_from_file("doom1.wad", &wad) != 0) {
    printf("Failed to load WAD file (doom1.wad)\n");
    return 2;
  }

  renderer_init(WIDTH, HEIGHT);
  renderer_set_draw_texture(0);
  // engine_init(&wad, "E1M1");

  palette_t palette;
  wad_read_playpal(&palette, &wad);
  GLuint palette_texture = palette_generate_texture(&palette);
  renderer_set_palette_texture(palette_texture);

  size_t   num_textures;
  patch_t *patches = wad_read_patches(&num_textures, &wad);
  GLuint  *tex     = malloc(sizeof(GLuint) * num_textures);
  for (int i = 0; i < num_textures; i++) {
    tex[i] =
        generate_texture(patches[i].width, patches[i].height, patches[i].data);
  }

  size_t index = 0;
  float  time  = .5f;

  char  title[128];
  float last = 0.f;
  while (!glfwWindowShouldClose(window)) {
    float now   = glfwGetTime();
    float delta = now - last;
    last        = now;

    time -= delta;
    if (time <= 0.f) {
      time = .5f;
      if (++index >= num_textures) { index = 0; }
    }

    // engine_update(delta);

    glfwPollEvents();
    snprintf(title, 128, "DooM | %.0f", 1.f / delta);
    glfwSetWindowTitle(window, title);

    renderer_clear();
    // engine_render();
    renderer_set_draw_texture(tex[index]);
    renderer_set_texture_index(0);
    renderer_draw_quad(
        (vec2_t){WIDTH / 2.f, HEIGHT / 2.f},
        (vec2_t){patches[index].width * 5.f, patches[index].height * 5.f}, 0.f,
        0);
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}

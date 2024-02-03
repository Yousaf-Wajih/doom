#include "map.h"
#include "renderer.h"
#include "util.h"
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

  wad_t wad;
  if (wad_load_from_file("doom1.wad", &wad) != 0) {
    printf("Failed to load WAD file (doom1.wad)\n");
    return 2;
  }

  map_t map;
  if (wad_read_map("E1M1", &map, &wad) != 0) {
    printf("Failed to read map (E1M1) from WAD file\n");
    return 3;
  }

  vec2_t  out_min = {20.f, 20.f}, out_max = {WIDTH - 20.f, HEIGHT - 20.f};
  vec2_t *remapped_vertices = malloc(sizeof(vec2_t) * map.num_vertices);
  for (size_t i = 0; i < map.num_vertices; i++) {
    remapped_vertices[i] = (vec2_t){
        .x = (max(map.min.x, min(map.vertices[i].x, map.max.x)) - map.min.x) *
                 (out_max.x - out_min.x) / (map.max.x - map.min.x) +
             out_min.x,
        .y = HEIGHT -
             (max(map.min.y, min(map.vertices[i].y, map.max.y)) - map.min.y) *
                 (out_max.y - out_min.y) / (map.max.y - map.min.y) -
             out_min.y,
    };
  }

  renderer_init(WIDTH, HEIGHT);

  char  title[128];
  float angle = 0.f;
  float last  = 0.f;
  while (!glfwWindowShouldClose(window)) {
    float now   = glfwGetTime();
    float delta = now - last;
    last        = now;

    angle += .7f * delta;

    glfwPollEvents();
    snprintf(title, 128, "DooM | %.0f", 1.f / delta);
    glfwSetWindowTitle(window, title);

    renderer_clear();
    for (size_t i = 0; i < map.num_vertices; i++) {
      renderer_draw_point(remapped_vertices[i], 3.f,
                          (vec4_t){1.f, 1.f, 0.f, 1.f});
    }
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}

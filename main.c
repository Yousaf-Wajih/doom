#include "engine.h"
#include "input.h"
#include "renderer.h"
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
  glfwSwapInterval(1);

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
  engine_init(&wad, "E1M1");

  char  title[128];
  float last = 0.f;
  while (!glfwWindowShouldClose(window)) {
    float now   = glfwGetTime();
    float delta = now - last;
    last        = now;

    engine_update(delta);

    glfwPollEvents();
    snprintf(title, 128, "DooM | %.0f", 1.f / delta);
    glfwSetWindowTitle(window, title);

    renderer_clear();
    engine_render();
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}

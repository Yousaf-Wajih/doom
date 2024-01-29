#include "renderer.h"
#include "vector.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

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
    renderer_draw_line((vec2_t){0.f, 0.f}, (vec2_t){WIDTH, HEIGHT}, 5.f,
                       (vec4_t){0.f, 0.f, 1.f, 1.f});
    renderer_draw_line((vec2_t){WIDTH, 0.f}, (vec2_t){0.f, HEIGHT}, 5.f,
                       (vec4_t){0.f, 0.f, 1.f, 1.f});

    renderer_draw_point((vec2_t){100.f, 100.f}, 2.f,
                        (vec4_t){1.f, 1.f, 1.f, 1.f});
    renderer_draw_quad((vec2_t){900.f, 100.f}, (vec2_t){50.f, 50.f}, angle,
                       (vec4_t){1.f, 1.f, 0.f, 1.f});
    glfwSwapBuffers(window);
  }

  glfwTerminate();
  return 0;
}

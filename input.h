#ifndef _INPUT_H
#define _INPUT_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "vector.h"

typedef enum button {
  KEY_UNKNOWN,
  KEY_SPACE,
  KEY_APOSTROPHE,
  KEY_COMMA,
  KEY_MINUS,
  KEY_PERIOD,
  KEY_SLASH,
  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_SEMICOLON,
  KEY_EQUAL,
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_LBRACKET,
  KEY_BACKSLASH,
  KEY_RBRACKET,
  KEY_GRAVE_ACCENT,
  KEY_TILDE,
  KEY_ESCAPE,
  KEY_LSHIFT,
  KEY_UP,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_DOWN,

  MOUSE_LEFT,
  MOUSE_RIGHT,
  MOUSE_MIDDLE,

  BUTTON_COUNT, // special; must be at the end
} button_t;

int    is_button_pressed(button_t button);
vec2_t get_mouse_position();
int    is_mouse_captured();
void   set_mouse_captured(int is_mouse_captured);

void input_init(GLFWwindow *window);
void input_key_callback(GLFWwindow *window, int key, int scancode, int action,
                        int mods);
void input_mouse_button_callback(GLFWwindow *window, int button, int action,
                                 int mods);
void input_mouse_position_callback(GLFWwindow *window, double x, double y);

#endif // !_INPUT_H

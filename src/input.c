#include "input.h"
#include "vector.h"

#include <GLFW/glfw3.h>
#include <string.h>

const int key_mapping[GLFW_KEY_LAST + 1] = {
    [GLFW_KEY_SPACE]         = KEY_SPACE,
    [GLFW_KEY_APOSTROPHE]    = KEY_APOSTROPHE,
    [GLFW_KEY_COMMA]         = KEY_COMMA,
    [GLFW_KEY_MINUS]         = KEY_MINUS,
    [GLFW_KEY_PERIOD]        = KEY_PERIOD,
    [GLFW_KEY_SLASH]         = KEY_SLASH,
    [GLFW_KEY_0]             = KEY_0,
    [GLFW_KEY_1]             = KEY_1,
    [GLFW_KEY_2]             = KEY_2,
    [GLFW_KEY_3]             = KEY_3,
    [GLFW_KEY_4]             = KEY_4,
    [GLFW_KEY_5]             = KEY_5,
    [GLFW_KEY_6]             = KEY_6,
    [GLFW_KEY_7]             = KEY_7,
    [GLFW_KEY_8]             = KEY_8,
    [GLFW_KEY_9]             = KEY_9,
    [GLFW_KEY_SEMICOLON]     = KEY_SEMICOLON,
    [GLFW_KEY_EQUAL]         = KEY_EQUAL,
    [GLFW_KEY_A]             = KEY_A,
    [GLFW_KEY_B]             = KEY_B,
    [GLFW_KEY_C]             = KEY_C,
    [GLFW_KEY_D]             = KEY_D,
    [GLFW_KEY_E]             = KEY_E,
    [GLFW_KEY_F]             = KEY_F,
    [GLFW_KEY_G]             = KEY_G,
    [GLFW_KEY_H]             = KEY_H,
    [GLFW_KEY_I]             = KEY_I,
    [GLFW_KEY_J]             = KEY_J,
    [GLFW_KEY_K]             = KEY_K,
    [GLFW_KEY_L]             = KEY_L,
    [GLFW_KEY_M]             = KEY_M,
    [GLFW_KEY_N]             = KEY_N,
    [GLFW_KEY_O]             = KEY_O,
    [GLFW_KEY_P]             = KEY_P,
    [GLFW_KEY_Q]             = KEY_Q,
    [GLFW_KEY_R]             = KEY_R,
    [GLFW_KEY_S]             = KEY_S,
    [GLFW_KEY_T]             = KEY_T,
    [GLFW_KEY_U]             = KEY_U,
    [GLFW_KEY_V]             = KEY_V,
    [GLFW_KEY_W]             = KEY_W,
    [GLFW_KEY_X]             = KEY_X,
    [GLFW_KEY_Y]             = KEY_Y,
    [GLFW_KEY_Z]             = KEY_Z,
    [GLFW_KEY_LEFT_BRACKET]  = KEY_LBRACKET,
    [GLFW_KEY_BACKSLASH]     = KEY_BACKSLASH,
    [GLFW_KEY_RIGHT_BRACKET] = KEY_RBRACKET,
    [GLFW_KEY_GRAVE_ACCENT]  = KEY_GRAVE_ACCENT,
    [GLFW_KEY_WORLD_1]       = KEY_TILDE,
    [GLFW_KEY_ESCAPE]        = KEY_ESCAPE,
    [GLFW_KEY_LEFT_SHIFT]    = KEY_LSHIFT,
    [GLFW_KEY_UP]            = KEY_UP,
    [GLFW_KEY_DOWN]          = KEY_DOWN,
    [GLFW_KEY_LEFT]          = KEY_LEFT,
    [GLFW_KEY_RIGHT]         = KEY_RIGHT,
};

const int mouse_mapping[GLFW_MOUSE_BUTTON_LAST + 1] = {
    [GLFW_MOUSE_BUTTON_LEFT]   = MOUSE_LEFT,
    [GLFW_MOUSE_BUTTON_RIGHT]  = MOUSE_RIGHT,
    [GLFW_MOUSE_BUTTON_MIDDLE] = MOUSE_MIDDLE,
};

static GLFWwindow *window;
static vec2_t      mouse_position;
static int         buttons[BUTTON_COUNT], previous_buttons[BUTTON_COUNT];

int is_button_pressed(button_t button) {
  if (button >= 0 && button < BUTTON_COUNT) { return buttons[button]; }
  return 0;
}

int is_button_just_pressed(button_t button) {
  if (button >= 0 && button < BUTTON_COUNT) {
    return buttons[button] && !previous_buttons[button];
  }

  return 0;
}

vec2_t get_mouse_position() { return mouse_position; }

int is_mouse_captured() {
  return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED ? 1 : 0;
}

void set_mouse_captured(int is_mouse_captured) {
  glfwSetInputMode(window, GLFW_CURSOR,
                   is_mouse_captured ? GLFW_CURSOR_DISABLED
                                     : GLFW_CURSOR_NORMAL);
}

void input_init(GLFWwindow *win) { window = win; }

void input_tick() {
  memcpy(previous_buttons, buttons, BUTTON_COUNT * sizeof(int));
}

void input_key_callback(GLFWwindow *window, int key, int scancode, int action,
                        int mods) {
  if (key < 0) { return; }
  buttons[key_mapping[key]] = action == GLFW_RELEASE ? 0 : 1;
}

void input_mouse_button_callback(GLFWwindow *window, int button, int action,
                                 int mods) {
  if (buttons < 0) { return; }
  buttons[mouse_mapping[button]] = action == GLFW_RELEASE ? 0 : 1;
}

void input_mouse_position_callback(GLFWwindow *window, double x, double y) {
  mouse_position.x = x;
  mouse_position.y = y;
}

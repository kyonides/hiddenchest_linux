/*
** input.h
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** Copyright (C) 2018-2026 Kyonides Arkanthes <kyonides@gmail.com>
**
** HiddenChest is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** HiddenChest is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with HiddenChest. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INPUT_H
#define INPUT_H

#include "etc.h"

struct InputPrivate;
struct RGSSThreadData;

class Input
{
public:
  enum ButtonCode
  {
    None = 0,
    Down = 2, Left = 4, Right = 6, Up = 8,
    A = 11, B = 12, C = 13, X = 14, Y = 15, Z = 16, L = 17, R = 18,
    Shift = 19, Ctrl = 20, Alt = 21,
    // Non-standard extensions
    F1 = 10, F2 = 22, F3 = 23, F4 = 24, F5 = 25, F6 = 26, F7 = 27,
    F8 = 28, F9 = 29, F10 = 30, F11 = 31, F12 = 32,
    KeyA = 33, KeyB = 34, KeyC = 35, KeyD = 36, KeyE = 37, KeyF = 38,
    KeyG = 39, KeyH = 40, KeyI = 41, KeyJ = 42, KeyK = 43, KeyL = 44,
    KeyM = 45, KeyN = 46, KeyO = 47, KeyP = 48, KeyQ = 49, KeyR = 50,
    KeyS = 51, KeyT = 52, KeyU = 53, KeyV = 54, KeyW = 55, KeyX = 56,
    KeyY = 57, KeyZ = 58,
    N1 = 59, N2 = 60, N3 = 61, N4 = 62, N5 = 63,
    N6 = 64, N7 = 65, N8 = 66, N9 = 67, N0 = 68,
    Return = 69, Escape = 70, Backspace = 71, Tab = 72, Space = 73,
    Minus = 74, Equals = 75, LeftBracket = 76, RightBracket = 77,
    BackSlash = 78, Semicolon = 79, Apostrophe = 80, Grave = 81,
    Comma = 82, Period = 83, Slash = 84, CapsLock = 85, PrintScreen = 86,
    ScrollLock = 87, Pause = 88, Insert = 89, Home = 90, PageUp = 91,
    Delete = 92, End = 93, PageDown = 94,
    NumPadDivide = 95, NumPadMultiply = 96, NumPadMinus = 97,
    NumPadPlus = 98, Enter = 99, NumPad1 = 100, NumPad2 = 101,
    NumPad3 = 102, NumPad4 = 103, NumPad5 = 104, NumPad6 = 105,
    NumPad7 = 106, NumPad8 = 107, NumPad9 = 108, NumPad0 = 109, NumPadDot = 110,
    LessOrGreater = 111, APP = 112, NumPadEquals = 113,
    LeftCtrl = 114, LeftShift = 115, LeftAlt = 116,
    RightCtrl = 117, RightShift = 118, RightAlt = 119,
    LeftMeta = 120, RightMeta = 121, L2 = 122, R2 = 123,
    Web = 124, Mail = 125, Calculator = 126, Computer = 127,
    APP1 = 128, APP2 = 129,
    MouseLeft = 130, MouseMiddle = 131, MouseRight = 132,
    JS0 = 133, JS1 = 134, JS2 = 135, JS3 = 136, JS4 = 137, JS5 = 138,
    JS6 = 139, JS7 = 140, JS8 = 141, JS9 = 142, JS10 = 143, JS11 = 144,
    JS12 = 145, JS13 = 146, JS14 = 147, JS15 = 148, JS16 = 149, JS17 = 150,
    JS18 = 151, JS19 = 152, JS20 = 153,
  };
  void update();
  int timer() const;
  int trigger_timer() const;
  int trigger_base_timer() const;
  void set_trigger_base_timer(int timer);
  int click_timer() const;
  int click_base_timer() const;
  void set_click_base_timer(int timer);
  int scroll_factor() const;
  void set_scroll_factor(int value);
  int mouse_scroll_x() const;
  int mouse_scroll_y() const;
  void mouse_scroll_reset();
  void clear_clicks();
  bool is_left_click();
  bool is_middle_click();
  bool is_right_click();
  bool is_double_left_click();
  bool is_double_right_click();
  bool is_double_click(int btn);
  bool press_left_click();
  bool press_right_click();
  bool is_mouse_scroll_x(bool go_up);
  bool is_mouse_scroll_y(bool go_up);
  bool isPressed(int button);
  bool isTriggered(int button);
  bool isRepeated(int button);
  bool is_pressed_any();
  bool is_triggered_any();
  bool is_triggered_double(int button);
  bool is_last_key();
  int text_input();
  void set_text_input(int value);
  int last_key();
  void last_key_clear();
  void triggered_bind_clear();
  int triggered_kind();
  int triggered_js_value();
  int triggered_js_axis();
  int triggered_js_dir();
  int triggered_last();
  int triggered_old();
  int dir4Value();
  int dir8Value();
  bool is_dir4();
  bool is_dir8();
  // Mouse Extensions
  int mouseX();
  int mouseY();
  int mouse_ox() const;
  int mouse_oy() const;
  void mouse_set_xy(int x, int y);
  void mouse_set_ox(int n);
  void mouse_set_oy(int n);
  bool mouse_is_inside(int index, Rect *rect);
  bool has_joystick();
  const char* joystick_name();
  int joysticks_total();
  int joystick_vendor();
  int joystick_kind();
  int joystick_power();
  std::vector<int> joystick_basic_values();
  int joystick_axis_number();
  int joystick_hat_number();
  int joystick_button_number();
  bool joystick_has_rumble();
  int joystick_set_rumble(int lfr, int rfr, int ms);
  int joystick_change();

private:
  Input(const RGSSThreadData &rtData);
  ~Input();
  void main_update();
  void text_update();
  void bind_update();
  friend struct SharedStatePrivate;
  InputPrivate *p;
};

#endif // INPUT_H

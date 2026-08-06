/*
** input-binding.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** 2018-2025 Extended by Kyonides-Arkanthes <kyonides@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "clicks.h"
#include "input/input.h"
#include "input_buttons.h"
#include "input_vendors.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "input/keybindings.h"
#include "exception.h"
#include "binding-util.h"
#include "util.h"
#include "hcextras.h"
#include <SDL_keyboard.h>
#include "debugwriter.h"

static VALUE gamepad;

static void add_keyboard_key(BDescVec &d, int source, int target)
{
  SourceDesc src;
  src.type = Key;
  src.d.scan = (SDL_Scancode)source;
  BindingDesc desc;
  desc.src = src;
  desc.target = (Input::ButtonCode)target;
  d.push_back(desc);
}

static void add_joystick_button(BDescVec &d, int source, int target)
{
  SourceDesc src;
  src.type = JButton;
  src.d.jb = source;
  BindingDesc desc;
  desc.src = src;
  desc.target = (Input::ButtonCode)target;
  d.push_back(desc);
}

static void add_axis_binding(BDescVec &d, int axis, int dir, int target)
{
  SourceDesc src;
  src.type = JAxis;
  src.d.ja.axis = axis;
  src.d.ja.dir = dir == 1 ? Positive : Negative;
  BindingDesc desc;
  desc.src = src;
  desc.target = (Input::ButtonCode)target;
  d.push_back(desc);
}

static void add_hat_binding(BDescVec &d, uint8_t hat, uint8_t pos, int target)
{
  SourceDesc src;
  src.type = JHat;
  src.d.jh.hat = hat;
  src.d.jh.pos = pos;
  BindingDesc desc;
  desc.src = src;
  desc.target = (Input::ButtonCode)target;
  d.push_back(desc);
}

static void gamepad_set_sdl_binding(BDescVec &bind, int target, int type, VALUE b)
{
  VALUE value, rdir;
  value = rb_iv_get(b, "@value");
  int dir, v = FIX2INT(value);
  switch (type)
  {
  case 0:
    break;
  case 1:
    value = rb_iv_get(b, "@scancode");
    v = FIX2INT(value);
    add_keyboard_key(bind, v, target);
    break;
  case 2:
    add_joystick_button(bind, v, target);
    break;
  case 3:
    rdir = rb_iv_get(b, "@dir");
    dir = FIX2INT(rdir);
    add_axis_binding(bind, v, dir, target);
    break;
  case 4:
    rdir = rb_iv_get(b, "@dir");
    dir = FIX2INT(rdir);
    add_hat_binding(bind, v, dir, target);
    break;
  }
}

static void gamepad_set_sdl_bound_values(VALUE list, int total, int index, BDescVec &bind)
{
  VALUE bg, data, b, type;
  int target, kind;
  for (int n = 0; n < total; n++) {
    target = target_buttons[n];
    bg = rb_ary_entry(list, n);
    data = rb_iv_get(bg, "@data");
    for (int i = 0; i < 4; i++) {
      b = rb_ary_entry(data, i);
      type = rb_iv_get(b, "@type");
      kind = FIX2INT(type);
      gamepad_set_sdl_binding(bind, target, kind, b);
    }
  }
}

static VALUE input_reset_sdl_bindings(VALUE self, VALUE pos)
{
  BDescVec bind;
  VALUE list;
  int index = FIXNUM_P(pos) ? FIX2INT(pos) : 2;
  list = rb_iv_get(self, "@bindings");
  list = rb_hash_aref(list, pos);
  if (list == Qnil) {
    shState->rtData().post_bindings(index, bind);
    return Qfalse;
  }
  list = rb_iv_get(list, "@list");
  int total = RARRAY_LEN(list);
  gamepad_set_sdl_bound_values(list, total, index, bind);
  shState->rtData().post_bindings(index, bind);
  return Qtrue;
}

static VALUE input_gamepad_number_axis(VALUE self)
{
  if (rb_iv_get(self, "@active") == Qtrue) {
    VALUE pos = rb_iv_get(self, "@id");
    int n = FIX2INT(pos);
    n = SDL_JoystickNumAxes(shState->rtData().joysticks[n]->js);
    return INT2FIX(n);
  } else
    return INT2FIX(0);
}

static VALUE input_gamepad_number_hats(VALUE self)
{
  if (rb_iv_get(self, "@active") == Qtrue) {
    VALUE pos = rb_iv_get(self, "@id");
    int n = FIX2INT(pos);
    n = SDL_JoystickNumHats(shState->rtData().joysticks[n]->js);
    return INT2FIX(n);
  } else
    return INT2FIX(0);
}

static VALUE input_gamepad_number_buttons(VALUE self)
{
  if (rb_iv_get(self, "@active") == Qtrue) {
    VALUE pos = rb_iv_get(self, "@id");
    int n = FIX2INT(pos);
    n = SDL_JoystickNumButtons(shState->rtData().joysticks[n]->js);
    return INT2FIX(n);
  } else
    return INT2FIX(0);
}

static VALUE input_gamepad_set_rumble(VALUE self, VALUE lint, VALUE rint, VALUE ms)
{
  if (rb_iv_get(self, "@rumble") == Qfalse)
    return rb_iv_set(self, "@last_rumble", Qfalse);
  VALUE pos = rb_iv_get(self, "@id");
  int n = FIX2INT(pos);
  int lfreq = FIX2INT(lint);
  int rfreq = FIX2INT(rint);
  int milli = FIX2INT(ms);
  int result = shState->input().joystick_set_rumble(n, lfreq, rfreq, milli);
  return rb_iv_set(self, "@last_rumble", !result ? Qtrue : Qfalse);
}

static VALUE input_update_internal(VALUE self)
{
  shState->input().update();
  return Qnil;
}
// FIXME: RMXP allows only few more types that don't make sense (symbols in pre 3, floats)
static int getButtonArg(VALUE input, VALUE number)
{
  if (FIXNUM_P(number))
    return FIX2INT(number);
  if (SYMBOL_P(number)) {
    VALUE sym_hash = rb_const_get(input, rb_intern("BUTTON_CODES"));
    return FIX2INT(rb_hash_aref(sym_hash, number));
  }
  return 0;
}

static VALUE input_press(int argc, VALUE *args, VALUE self)
{
  if (!argc || argc > 2)
    return Qfalse;
  int pos = argc == 2 ? FIX2INT(args[1]) : 2;
  int num = getButtonArg(self, args[0]);
  return shState->input().is_pressed(pos, num) ? Qtrue : Qfalse;
}

static VALUE input_trigger(int argc, VALUE *args, VALUE self)
{
  if (!argc || argc > 2)
    return Qfalse;
  int pos = argc == 2 ? FIX2INT(args[1]) : 2;
  int num = getButtonArg(self, args[0]);
  return shState->input().is_triggered(pos, num) ? Qtrue : Qfalse;
}

static VALUE input_repeat(int argc, VALUE *args, VALUE self)
{
  if (!argc || argc > 2)
    return Qfalse;
  int pos = argc == 2 ? FIX2INT(args[1]) : 2;
  int num = getButtonArg(self, args[0]);
  return shState->input().is_repeated(pos, num) ? Qtrue : Qfalse;
}

static VALUE input_press_any(int argc, VALUE *args, VALUE self)
{
  VALUE pos = !argc ? INT2FIX(2) : args[0];
  int n = FIX2INT(pos);
  return shState->input().is_pressed_any(n)? Qtrue : Qfalse;
}

static VALUE input_trigger_any(int argc, VALUE *args, VALUE self)
{
  VALUE pos = !argc ? INT2FIX(2) : args[0];
  int n = FIX2INT(pos);
  return shState->input().is_triggered_any(n)? Qtrue : Qfalse;
}

static VALUE input_trigger_double(VALUE self, VALUE number)
{
  int num = getButtonArg(self, number);
  return shState->input().is_triggered_double(num) ? Qtrue : Qfalse;
}

static VALUE input_trigger_exclude(int size, VALUE* buttons, VALUE self)
{
  if (size == 1 && ARRAY_TYPE_P(buttons[0])) {
    size = RARRAY_LEN(buttons[0]);
    buttons = RARRAY_PTR(buttons[0]);
  }
  if (!shState->input().is_triggered_any(2))
    return Qfalse;
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(self, buttons[n]);
    if (shState->input().is_triggered(2, num))
      return Qfalse;
  }
  return Qtrue;
}

static VALUE input_trigger_kind(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  int num = shState->input().triggered_kind(n);
  return INT2FIX(num);
}

static VALUE input_trigger_gp_value(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  int num = shState->input().triggered_js_value(n);
  return INT2FIX(num);
}

static VALUE input_trigger_gp_axis(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  int num = shState->input().triggered_js_axis(n);
  return INT2FIX(num);
}

static VALUE input_trigger_gp_dir(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  int num = shState->input().triggered_js_dir(n);
  return INT2FIX(num);
}

static VALUE input_trigger_gp_clear(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  shState->input().triggered_bind_clear(n);
  return INT2FIX(0);
}

static VALUE input_trigger_last_clear(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  shState->input().triggered_last_clear(n);
  return INT2FIX(0);
}

static VALUE input_trigger_last(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  int num = shState->input().triggered_last(n);
  return INT2FIX(num);
}

static VALUE input_trigger_old(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  int num = shState->input().triggered_old(n);
  return INT2FIX(num);
}

static VALUE input_button_targets(VALUE self)
{
  return rb_iv_get(self, "@button_targets");
}

static VALUE input_button_target_names(VALUE self)
{
  return rb_iv_get(self, "@button_target_names");
}

static VALUE input_button_sequence(VALUE self)
{
  return rb_iv_get(self, "@button_sequence");
}

static VALUE input_button_sequence_names(VALUE self)
{
  return rb_iv_get(self, "@button_sequence_names");
}

static VALUE input_store_sequence_get(VALUE self)
{
  return rb_iv_get(self, "@store_sequence");
}

static VALUE input_sequence_max_get(VALUE self)
{
  return rb_iv_get(self, "@sequence_max");
}

static VALUE input_sequence_timer_get(VALUE self)

{
  return rb_iv_get(self, "@sequence_timer");
}

static VALUE input_start_sequence_timer_get(VALUE self)
{
  return rb_iv_get(self, "@start_sequence_timer");
}

static VALUE input_are_triggered(int size, VALUE* buttons, VALUE self)
{
  if (size < 2)
    return Qfalse;
  VALUE pos = buttons[0];
  int n = FIX2INT(pos);
  if (size == 2 && ARRAY_TYPE_P(buttons[1])) {
    size = RARRAY_LEN(buttons[1]);
    buttons = RARRAY_PTR(buttons[1]);
  }
  for (int n = 1; n < size; n++) {
    int num = getButtonArg(self, buttons[n]);
    if (!shState->input().is_triggered(n, num))
      return Qfalse;
  }
  return Qtrue;
}

static VALUE input_are_pressed(int size, VALUE *buttons, VALUE self)
{
  if (size < 2)
    return Qfalse;
  VALUE pos = buttons[0];
  int n = FIX2INT(pos);
  if (size == 2 && ARRAY_TYPE_P(buttons[1])) {
    size = RARRAY_LEN(buttons[1]);
    buttons = RARRAY_PTR(buttons[1]);
  }
  for (int n = 1; n < size; n++) {
    int num = getButtonArg(self, buttons[n]);
    if (!shState->input().is_pressed(n, num))
      return Qfalse;
  }
  return Qtrue;
}

static VALUE input_press_left_click(VALUE self)
{
  return shState->input().press_left_click() ? Qtrue : Qfalse;
}

static VALUE input_press_right_click(VALUE self)
{
  return shState->input().press_right_click() ? Qtrue : Qfalse;
}

static VALUE input_trigger_up_down(int size, VALUE *buttons, VALUE self)
{
  if (size > 1)
    return Qfalse;
  int n = size == 0 ? 2 : FIX2INT(buttons[0]);
  if (shState->input().is_triggered(n, Input::Up))
    return Qtrue;
  if (shState->input().is_triggered(n, Input::Down))
    return Qtrue;
  return Qfalse;
}

static VALUE input_trigger_left_right(int size, VALUE *buttons, VALUE self)
{
  if (size > 1)
    return Qfalse;
  int n = size == 0 ? 2 : FIX2INT(buttons[0]);
  if (shState->input().is_triggered(n, Input::Left))
    return Qtrue;
  if (shState->input().is_triggered(n, Input::Right))
    return Qtrue;
  return Qfalse;
}

static VALUE input_repeat_left_click(VALUE self)
{
  return shState->input().is_repeated(2, Input::MouseLeft) ? Qtrue : Qfalse;
}

static VALUE input_repeat_right_click(VALUE self)
{
  return shState->input().is_repeated(2, Input::MouseRight) ? Qtrue : Qfalse;
}

static VALUE inputDir4(VALUE self)
{
  return INT2FIX(shState->input().dir4Value());
}

static VALUE inputDir8(VALUE self)
{
  return INT2FIX(shState->input().dir8Value());
}

static VALUE input_is_dir4(VALUE self)
{
  return shState->input().is_dir4() ? Qtrue : Qfalse;
}

static VALUE input_is_dir8(VALUE self)
{
  return shState->input().is_dir8() ? Qtrue : Qfalse;
}

static VALUE input_player_dir4(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  return INT2FIX(shState->input().player_dir4(n));
}

static VALUE input_player_dir8(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  return INT2FIX(shState->input().player_dir8(n));
}

static VALUE input_is_player_dir4(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  return shState->input().is_player_dir4(n) ? Qtrue : Qfalse;
}

static VALUE input_is_player_dir8(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  return shState->input().is_player_dir8(n) ? Qtrue : Qfalse;
}

// Non-standard extensions
static VALUE input_left_click(VALUE self)
{
  return shState->input().is_left_click() ? Qtrue : Qfalse;
}

static VALUE input_middle_click(VALUE self)
{
  return shState->input().is_middle_click() ? Qtrue : Qfalse;
}

static VALUE input_right_click(VALUE self)
{
  return shState->input().is_right_click() ? Qtrue : Qfalse;
}

static VALUE input_double_click(VALUE self, VALUE button)
{
  int btn = FIX2INT(button);
  return shState->input().is_double_click(btn) ? Qtrue : Qfalse; 
}

static VALUE input_double_left_click(VALUE self)
{
  return shState->input().is_double_left_click() ? Qtrue : Qfalse; 
}

static VALUE input_double_right_click(VALUE self)
{
  return shState->input().is_double_right_click() ? Qtrue : Qfalse; 
}

static VALUE input_is_last_key(VALUE self)
{
  return shState->input().is_last_key() ? Qtrue : Qfalse;
}

static VALUE input_last_key(VALUE self)
{
  int key = shState->input().last_key();
  return INT2FIX(key);
}

static VALUE input_last_char(VALUE self)
{
  VALUE n, hash;
  n = input_last_key(self);
  hash = rb_const_get(self, rb_intern("KEY2CHAR"));
  n = rb_hash_aref(hash, n);
  if (n == Qnil)
    return Qnil;
  if (SDL_GetModState() & 0x2000)
    return n;
  else
    return rb_funcall(n, rb_intern("downcase"), 0);
}

static VALUE input_last_key_clear(VALUE self)
{
  shState->input().last_key_clear();
  return INT2FIX(0);
}

static VALUE input_capslock_state(VALUE self)
{
  return SDL_GetModState() & 0x2000 ? Qtrue : Qfalse;
}

static VALUE input_text_input(VALUE self)
{
  return rb_iv_get(self, "@text_input");
}

static VALUE input_text_input_set(VALUE self, VALUE number)
{
  int n = FIX2INT(number);
  shState->input().set_text_input(n);
  return rb_iv_set(self, "@text_input", number);
}

static VALUE input_text_input_clear(VALUE self)
{
  input_last_key_clear(self);
  return input_text_input_set(self, INT2FIX(0));
}

static VALUE input_has_gamepad(VALUE self)
{
  return SDL_NumJoysticks() > 0 ? Qtrue : Qfalse;
}

static VALUE input_total_gamepads(VALUE self)
{
  return INT2FIX(SDL_NumJoysticks());
}

static VALUE input_gamepad_change(VALUE self)
{
  return INT2FIX(shState->rtData().joystick_change);
}

static VALUE input_gamepad_change_now(VALUE self)
{
  shState->rtData().joystick_change = 2;
  return INT2FIX(2);
}

static VALUE input_gamepad_clear_change(VALUE self)
{
  shState->rtData().joystick_change = 0;
  return INT2FIX(0);
}

static VALUE input_gamepad_index(VALUE self)
{
  int n = EventThread::joystick_index();
  return INT2FIX(n);
}

static VALUE input_gamepad_update(VALUE self)
{
  VALUE state = rb_iv_get(self, "@gamepad_updates");
  return rb_ary_pop(state);
}

static VALUE input_gamepad_updates(VALUE self)
{
  return rb_iv_get(self, "@gamepad_updates");
}

static VALUE input_gamepad_close(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  bool result = EventThread::close_joystick(n);
  return result ? Qtrue : Qfalse;
}

static VALUE input_gamepad_open(VALUE self, VALUE pos)
{
  int n = FIX2INT(pos);
  bool result = EventThread::open_joystick(n);
  return result ? Qtrue : Qfalse;
}

static VALUE input_gamepad_reset(VALUE self, VALUE pos)
{
  input_gamepad_close(self, pos);
  return input_gamepad_open(self, pos);
}

static VALUE input_gamepad_basic_values(VALUE self, VALUE pos)
{
  VALUE ary = rb_ary_new();
  if (!SDL_NumJoysticks()) {
    VALUE zero = INT2FIX(0);
    rb_ary_push(ary, rstr("None"));
    rb_ary_push(ary, zero);
    rb_ary_push(ary, zero);
    rb_ary_push(ary, INT2FIX(-1));
    rb_ary_push(ary, Qfalse);
    return ary;
  }
  int n = FIX2INT(pos);
  const char *name = SDL_JoystickName(shState->rtData().joysticks[n]->js);
  int val, rumble;
  rb_ary_push(ary, rstr(name));
  std::vector<int> v = shState->input().joystick_basic_values(n);
  for (int m = 0; m < v.size(); m++) {
    val = v.at(m);
    rb_ary_push(ary, INT2FIX(val));
  }
  rumble = SDL_JoystickHasRumble(shState->rtData().joysticks[n]->js);
  rb_ary_push(ary, rumble ? Qtrue : Qfalse);
  return ary;
}

static VALUE input_trigger_timer(VALUE self)
{
  int n = shState->input().trigger_timer();
  return INT2FIX(n);
}

static VALUE input_default_trigger_timer(VALUE self)
{
  return rb_iv_get(self, "default_trigger_timer");
}

static VALUE input_default_trigger_timer_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = FIX2INT(val);
  if (n < 0)
    return rb_iv_get(self, "default_trigger_timer");
  shState->input().set_trigger_base_timer(n);
  return rb_iv_set(self, "default_trigger_timer", val);
}

void inputBindingInit()
{
  VALUE input, input_meta, btn_seq, zero = RB_INT2FIX(0);
  input = rb_define_module("Input");
  input_meta = rb_singleton_class(input);
  gamepad = rb_define_class_under(input, "Gamepad", rb_cObject);
  rb_const_set(gamepad, rb_intern("DEFAULT_NAME"), rstr("None"));
  rb_const_set(gamepad, rb_intern("DEFAULT_VENDOR"), rstr("None"));
  rb_define_attr(gamepad, "id", 1, 0);
  rb_define_attr(gamepad, "name", 1, 0);
  rb_define_attr(gamepad, "vendor", 1, 0);
  rb_define_attr(gamepad, "type", 1, 0);
  rb_define_attr(gamepad, "type_number", 1, 0);
  rb_define_attr(gamepad, "power", 1, 0);
  rb_define_attr(gamepad, "power_level", 1, 0);
  rb_define_attr(gamepad, "active", 1, 0);
  rb_define_attr(gamepad, "rumble", 1, 0);
  rb_define_attr(gamepad, "last_rumble", 1, 0);
  rb_define_attr(gamepad, "bindings", 1, 0);
  rb_define_method(gamepad, "axes", RMF(input_gamepad_number_axis), 0);
  rb_define_method(gamepad, "hats", RMF(input_gamepad_number_hats), 0);
  rb_define_method(gamepad, "buttons", RMF(input_gamepad_number_buttons), 0);
  rb_define_method(gamepad, "set_rumble", RMF(input_gamepad_set_rumble), 3);
  rb_iv_set(input, "text_input", zero);
  rb_iv_set(input, "default_trigger_timer", INT2FIX(TRIGGER_TIMER));
  module_func(input, "trigger_timer", input_trigger_timer, 0);
  module_func(input, "base_trigger_timer", input_default_trigger_timer, 0);
  module_func(input, "base_trigger_timer=", input_default_trigger_timer_set, 1);
  module_func(input, "default_trigger_timer", input_default_trigger_timer, 0);
  module_func(input, "default_trigger_timer=", input_default_trigger_timer_set, 1);
  module_func(input, "update_internal", input_update_internal, 0);
  module_func(input, "left_click?", input_left_click, 0);
  module_func(input, "middle_click?", input_middle_click, 0);
  module_func(input, "right_click?", input_right_click, 0);
  module_func(input, "double_click?", input_double_click, 1);
  module_func(input, "double_left_click?", input_double_left_click, 0);
  module_func(input, "double_right_click?", input_double_right_click, 0);
  module_func(input, "press?", input_press, -1);
  module_func(input, "press_left_click?", input_press_left_click, 0);
  module_func(input, "press_right_click?", input_press_right_click, 0);
  module_func(input, "press_any?", input_press_any, -1);
  module_func(input, "press_all?", input_are_pressed, -1);
  module_func(input, "trigger?", input_trigger, -1);
  module_func(input, "trigger_any?", input_trigger_any, -1);
  module_func(input, "trigger_double?", input_trigger_double, 1);
  module_func(input, "trigger_buttons?", input_are_triggered, -1);
  module_func(input, "trigger_up_down?", input_trigger_up_down, -1);
  module_func(input, "trigger_left_right?", input_trigger_left_right, -1);
  module_func(input, "trigger_exclude?", input_trigger_exclude, -1);
  module_func(input, "trigger_type", input_trigger_kind, 1);
  module_func(input, "trigger_gp_value", input_trigger_gp_value, 1);
  module_func(input, "trigger_gp_axis", input_trigger_gp_axis, 1);
  module_func(input, "trigger_gp_dir", input_trigger_gp_dir, 1);
  module_func(input, "trigger_gp_clear", input_trigger_gp_clear, 1);
  module_func(input, "trigger_last_clear", input_trigger_last_clear, 1);
  module_func(input, "trigger_last", input_trigger_last, 1);
  module_func(input, "trigger_old", input_trigger_old, 1);
  module_func(input, "repeat?", input_repeat, -1);
  module_func(input, "repeat_left_click?", input_repeat_left_click, 0);
  module_func(input, "repeat_right_click?", input_repeat_right_click, 0);
  module_func(input, "last_key?", input_is_last_key, 0);
  module_func(input, "last_key", input_last_key, 0);
  module_func(input, "last_char", input_last_char, 0);
  module_func(input, "capslock_state", input_capslock_state, 0);
  module_func(input, "text_input", input_text_input, 0);
  module_func(input, "text_input=", input_text_input_set, 1);
  module_func(input, "clear_last_key", input_last_key_clear, 0);
  module_func(input, "clear_text_input", input_text_input_clear, 0);
  module_func(input, "gamepad?", input_has_gamepad, 0);
  module_func(input, "total_gamepads", input_total_gamepads, 0);
  module_func(input, "gamepad_change?", input_gamepad_change, 0);
  module_func(input, "gamepad_change!", input_gamepad_change_now, 0);
  module_func(input, "gamepad_clear_change", input_gamepad_clear_change, 0);
  module_func(input, "gamepad_index", input_gamepad_index, 0);
  module_func(input, "gamepad_update", input_gamepad_update, 0);
  module_func(input, "gamepad_updates", input_gamepad_updates, 0);
  module_func(input, "gamepad_close!", input_gamepad_close, 1);
  module_func(input, "gamepad_open!", input_gamepad_open, 1);
  module_func(input, "gamepad_reset!", input_gamepad_reset, 0);
  module_func(input, "gamepad_basic_values", input_gamepad_basic_values, 1);
  module_func(input, "reset_sdl_bindings", input_reset_sdl_bindings, 1);
  module_func(input, "dir4", inputDir4, 0);
  module_func(input, "dir8", inputDir8, 0);
  module_func(input, "dir4?", input_is_dir4, 0);
  module_func(input, "dir8?", input_is_dir8, 0);
  module_func(input, "player_dir4", input_player_dir4, 1);
  module_func(input, "player_dir8", input_player_dir8, 1);
  module_func(input, "player_dir4?", input_is_player_dir4, 1);
  module_func(input, "player_dir8?", input_is_player_dir8, 1);
  module_pfunc(input_meta, "text_input=", input_text_input_set, 1);
  module_pfunc(input_meta, "gamepad_basic_values", input_gamepad_basic_values, 1);
  module_pfunc(input_meta, "reset_sdl_bindings", input_reset_sdl_bindings, 1);
  VALUE key, val, hash = rb_hash_new();
  const char *tmp;
  for (size_t i = 0; i < button_scancodesN; ++i) {
    key = INT2FIX(button_scancodes[i].button);
    val = INT2FIX(button_scancodes[i].scancode);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("BUTTON_SCANCODES"), hash);
  rb_hash_set_ifnone(hash, zero);
  hash = rb_funcall(hash, rb_intern("invert"), 0);
  rb_const_set(input, rb_intern("SCANCODE_BUTTONS"), hash);
  rb_hash_set_ifnone(hash, zero);
  hash = rb_hash_new();
  /* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
   * to be compatible with the previous convention */
  for (size_t i = 0; i < buttonCodesN; ++i) {
    ID sym = rb_intern(buttonCodes[i].str);
    val = INT2FIX(buttonCodes[i].val);
    rb_const_set(input, sym, val);
    rb_hash_aset(hash, rb_id2sym(sym), val);
  }
  getRbData()->buttoncodeHash = hash;
  rb_const_set(input, rb_intern("BUTTON_CODES"), hash);
  rb_hash_set_ifnone(hash, zero);
  hash = rb_funcall(hash, rb_intern("invert"), 0);
  rb_hash_set_ifnone(hash, zero);
  rb_const_set(input, rb_intern("CODE_BUTTONS"), hash);
  hash = rb_hash_new();
  for (size_t i = 0; i < vendorsN; ++i) {
    key = INT2FIX(vendors[i].id);
    val = rstr(vendors[i].name);
    rb_hash_aset(hash, key, val);
  }
  rb_hash_set_ifnone(hash, rb_const_get(gamepad, rb_intern("DEFAULT_VENDOR")));
  rb_cvar_set(gamepad, rb_intern("@@vendors"), hash);
  hash = rb_hash_new();
  for (size_t i = 0; i < button_stringsN; ++i) {
    key = INT2FIX(button_strings[i].code);
    tmp = button_strings[i].str;
    val = !tmp ? Qnil : rstr(tmp);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("KEY2CHAR"), hash);
  hash = rb_hash_dup(hash);
  for (size_t i = 0; i < other_button_stringsN; ++i) {
    key = INT2FIX(other_button_strings[i].code);
    tmp = other_button_strings[i].str;
    val = !tmp ? rstr("") : rstr(tmp);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("KEY2NAME"), hash);
}

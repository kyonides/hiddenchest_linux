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
#include "input.h"
#include "input_buttons.h"
#include "input_vendors.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "keybindings.h"
#include "exception.h"
#include "binding-util.h"
#include "util.h"
#include "hcextras.h"
#include <SDL_keyboard.h>
#include "debugwriter.h"

#define ONE RB_INT2FIX(1)
#define ZERO RB_INT2FIX(0)
#define SUBZERO RB_INT2FIX(-1)


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
  int v, dir;
  value = rb_iv_get(b, "@value");
  v = RB_FIX2INT(value);
  switch (type)
  {
  case 0:
    break;
  case 1:
    value = rb_iv_get(b, "@scancode");
    v = RB_FIX2INT(value);
    add_keyboard_key(bind, v, target);
    break;
  case 2:
    add_joystick_button(bind, v, target);
    break;
  case 3:
    rdir = rb_iv_get(b, "@dir");
    dir = RB_FIX2INT(rdir);
    add_axis_binding(bind, v, dir, target);
    break;
  case 4:
    rdir = rb_iv_get(b, "@dir");
    dir = RB_FIX2INT(rdir);
    add_hat_binding(bind, v, dir, target);
    break;
  }
}

static void gamepad_set_sdl_bound_values(VALUE list, int total, BDescVec &bind)
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
      kind = RB_FIX2INT(type);
      gamepad_set_sdl_binding(bind, target, kind, b);
    }
  }
}

static int get_target_button_index(int target)
{
  int n = -1;
  for (; n < target_buttonsN; ++n)
    if (target_buttons[n] == target)
      break;
  return n;
}

static void gamepad_set_rb_bound_values(VALUE input, VALUE list,
                                        int total, BDescVec &bind)
{
  BindingDesc b;
  AxisDir ad;
  SDL_Scancode s;
  int type, o, old_target, target = -1, pos = 0;
  VALUE bg, data, kb, cb, sb, sc, kn, key, name, sym, fix, val;
  cb = rb_const_get(input, rb_intern("CODE_BUTTONS"));
  sb = rb_const_get(input, rb_intern("SCANCODE_BUTTONS"));
  kn = rb_const_get(input, rb_intern("KEY2NAME"));
  for (int n = 0; n < total; n++) {
    b = bind.at(n);
    old_target = target;
    target = b.target;
    bg = rb_ary_entry(list, n / 4);
    data = rb_iv_get(bg, "@data");
    pos = (target == old_target)? pos + 1 : 0;
    kb = rb_ary_entry(data, pos);
    type = b.src.type;
    rb_funcall(kb, rb_intern("type="), 1, RB_INT2FIX(type));
    switch (type)
    {
    case Invalid:
      rb_funcall(kb, rb_intern("clear_invalid"), 0);
      continue;
    case Key:
      s = b.src.d.scan;
      sc = RB_INT2FIX(s);
      key = rb_hash_aref(sb, sc);
      name = rb_hash_aref(kn, key);
      sym = rb_funcall(name, rb_intern("sub"), 2, rstr(" "), rstr(""));
      sym = rb_any_to_s(sym);
      val = rb_hash_aref(sb, sc);
      rb_iv_set(kb, "@name", name);
      rb_iv_set(kb, "@symbol", sym);
      rb_iv_set(kb, "@value", val);
      rb_iv_set(kb, "@scancode", sc);
      rb_iv_set(kb, "@dir", ZERO);
      continue;
    case JButton:
      o = b.src.d.jb;
      sc = RB_INT2FIX(o);
      fix = rb_funcall(sc, rb_intern("to_s"), 0);
      name = rstr("JS ");
      name = rb_str_plus(name, fix);
      rb_iv_set(kb, "@name", name);
      rb_iv_set(kb, "@symbol", Qnil);
      rb_iv_set(kb, "@value", sc);
      rb_iv_set(kb, "@scancode", ZERO);
      continue;
    case JAxis:
      ad = b.src.d.ja.dir;
      val = ad == Positive ? ONE : ZERO;
      sc = RB_INT2FIX(b.src.d.ja.axis);
      fix = rb_funcall(sc, rb_intern("to_s"), 0);
      name = rstr("Axis ");
      name = rb_str_plus(name, fix);
      fix = ad == Positive ? rstr("+") : rstr("-");
      name = rb_str_plus(name, fix);
      rb_iv_set(kb, "@name", name);
      rb_iv_set(kb, "@symbol", Qnil);
      rb_iv_set(kb, "@value", sc);
      rb_iv_set(kb, "@dir", val);
      continue;
    case JHat:
      sc = RB_INT2FIX(b.src.d.jh.hat);
      fix = rb_funcall(sc, rb_intern("to_s"), 0);
      name = rstr("Hat ");
      name = rb_str_plus(name, fix);
      name = rb_str_plus(name, rstr(":"));
      o = b.src.d.jh.pos;
      if (o == 1)
        fix = rstr("U");
      else if (o == 2)
        fix = rstr("R");
      else if (o == 4)
        fix = rstr("D");
      else
        fix = rstr("L");
      name = rb_str_plus(name, fix);
      rb_iv_set(kb, "@name", name);
      rb_iv_set(kb, "@symbol", Qnil);
      rb_iv_set(kb, "@value", sc);
      rb_iv_set(kb, "@dir", val);
      continue;
    }
  }
}

static VALUE input_reset_sdl_bindings(VALUE self)
{
  BDescVec bind;
  VALUE list, rgss, kb_gen_code;
  list = rb_iv_get(self, "@bindings");
  list = rb_iv_get(list, "@list");
  int total = RARRAY_LEN(list);
  gamepad_set_sdl_bound_values(list, total, bind);
  shState->rtData().bindingUpdateMsg.post(bind);
  return Qtrue;
}

static VALUE input_reset_rb_bindings(VALUE self)
{
  VALUE list;
  BDescVec bind;
  shState->rtData().bindingUpdateMsg.get(bind);
  int total = bind.size();
  list = rb_iv_get(self, "@bindings");
  list = rb_iv_get(list, "@list");
  gamepad_set_rb_bound_values(self, list, total, bind);
  shState->rtData().modify_settings = false;
  return Qtrue;
}

static VALUE input_gamepad_number_axis(VALUE self)
{
  if (rb_iv_get(self, "@active") == Qtrue) {
    int n = SDL_JoystickNumAxes(shState->rtData().joystick);
    return RB_INT2FIX(n);
  } else
    return ZERO;
}

static VALUE input_gamepad_number_hats(VALUE self)
{
  if (rb_iv_get(self, "@active") == Qtrue) {
    int n = SDL_JoystickNumHats(shState->rtData().joystick);
    return RB_INT2FIX(n);
  } else
    return ZERO;
}

static VALUE input_gamepad_number_buttons(VALUE self)
{
  if (rb_iv_get(self, "@active") == Qtrue) {
    int n = SDL_JoystickNumButtons(shState->rtData().joystick);
    return RB_INT2FIX(n);
  } else
    return ZERO;
}

static VALUE input_gamepad_set_rumble(VALUE self, VALUE lint, VALUE rint, VALUE ms)
{
  if (rb_iv_get(self, "@rumble") == Qfalse)
    return rb_iv_set(self, "@last_rumble", Qfalse);
  int lfreq = RB_FIX2INT(lint);
  int rfreq = RB_FIX2INT(rint);
  int milli = RB_FIX2INT(ms);
  int result = shState->input().joystick_set_rumble(lfreq, rfreq, milli);
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
    return RB_FIX2INT(number);
  if (SYMBOL_P(number)) {
    VALUE sym_hash = rb_const_get(input, rb_intern("BUTTON_CODES"));
    return RB_FIX2INT(rb_hash_aref(sym_hash, number));
  }
  return 0;
}

static VALUE inputPress(VALUE self, VALUE number)
{
  int num = getButtonArg(self, number);
  return shState->input().isPressed(num) ? Qtrue : Qfalse;
}

static VALUE inputTrigger(VALUE self, VALUE number)
{
  int num = getButtonArg(self, number);
  return shState->input().isTriggered(num) ? Qtrue : Qfalse;
}

static VALUE inputRepeat(VALUE self, VALUE number)
{
  int num = getButtonArg(self, number);
  return shState->input().isRepeated(num) ? Qtrue : Qfalse;
}

static VALUE input_press_any(VALUE self)
{
  return shState->input().is_pressed_any()? Qtrue : Qfalse;
}

static VALUE input_trigger_any(VALUE self)
{
  return shState->input().is_triggered_any()? Qtrue : Qfalse;
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
  if (!shState->input().is_triggered_any())
    return Qfalse;
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(self, buttons[n]);
    if (shState->input().isTriggered(num))
      return Qfalse;
  }
  return Qtrue;
}

static VALUE input_trigger_kind(VALUE self)
{
  int num = shState->input().triggered_kind();
  return RB_INT2FIX(num);
}

static VALUE input_trigger_gp_value(VALUE self)
{
  int num = shState->input().triggered_js_value();
  return RB_INT2FIX(num);
}

static VALUE input_trigger_gp_axis(VALUE self)
{
  int num = shState->input().triggered_js_axis();
  return RB_INT2FIX(num);
}

static VALUE input_trigger_gp_dir(VALUE self)
{
  int num = shState->input().triggered_js_dir();
  return RB_INT2FIX(num);
}

static VALUE input_trigger_gp_clear(VALUE self)
{
  shState->input().triggered_bind_clear();
  return ZERO;
}

static VALUE input_trigger_last(VALUE self)
{
  int num = shState->input().triggered_last();
  return RB_INT2FIX(num);
}

static VALUE input_trigger_old(VALUE self)
{
  int num = shState->input().triggered_old();
  return RB_INT2FIX(num);
}

static VALUE input_are_triggered(int size, VALUE* buttons, VALUE self)
{
  if (size == 1 && ARRAY_TYPE_P(buttons[0])) {
    size = RARRAY_LEN(buttons[0]);
    buttons = RARRAY_PTR(buttons[0]);
  }
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(self, buttons[n]);
    if (!shState->input().isTriggered(num))
      return Qfalse;
  }
  return Qtrue;
}

static VALUE input_are_pressed(int size, VALUE *buttons, VALUE self)
{
  if (size == 1 && ARRAY_TYPE_P(buttons[0])) {
    size = RARRAY_LEN(buttons[0]);
    buttons = RARRAY_PTR(buttons[0]);
  }
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(self, buttons[n]);
    if (!shState->input().isPressed(num))
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

static VALUE input_trigger_up_down(VALUE self)
{
  if (shState->input().isTriggered(Input::Up))
    return Qtrue;
  if (shState->input().isTriggered(Input::Down))
    return Qtrue;
  return Qfalse;
}

static VALUE input_trigger_left_right(VALUE self)
{
  if (shState->input().isTriggered(Input::Left))
    return Qtrue;
  if (shState->input().isTriggered(Input::Right))
    return Qtrue;
  return Qfalse;
}

static VALUE input_repeat_left_click(VALUE self)
{
  return shState->input().isRepeated(Input::MouseLeft) ? Qtrue : Qfalse;
}

static VALUE input_repeat_right_click(VALUE self)
{
  return shState->input().isRepeated(Input::MouseRight) ? Qtrue : Qfalse;
}

static VALUE inputDir4(VALUE self)
{
  return RB_INT2FIX(shState->input().dir4Value());
}

static VALUE inputDir8(VALUE self)
{
  return RB_INT2FIX(shState->input().dir8Value());
}

static VALUE input_is_dir4(VALUE self)
{
  return shState->input().is_dir4() ? Qtrue : Qfalse;
}

static VALUE input_is_dir8(VALUE self)
{
  return shState->input().is_dir8() ? Qtrue : Qfalse;
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
  int btn = RB_FIX2INT(button);
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
  return RB_INT2FIX(key);
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
  return ZERO;
}

static VALUE input_capslock_state(VALUE self)
{
  return SDL_GetModState() & 0x2000 ? Qtrue : Qfalse;
}

static VALUE input_text_input(VALUE self)
{
  return rb_iv_get(self, "text_input");
}

static VALUE input_text_input_set(VALUE self, VALUE number)
{
  if (!FIXNUM_P(number))
    return rb_iv_get(self, "text_input");
  int n = RB_FIX2INT(number);
  Debug() << "Text Input Mode:" << n;
  shState->input().set_text_input(n);
  return rb_iv_set(self, "text_input", number);
}

static VALUE input_text_input_clear(VALUE self)
{
  input_last_key_clear(self);
  return input_text_input_set(self, ZERO);
}

static VALUE input_gamepad(VALUE self)
{
  return rb_iv_get(self, "@gamepad");
}

static VALUE input_has_gamepad(VALUE self)
{
  return SDL_NumJoysticks() > 0 ? Qtrue : Qfalse;
}

static VALUE input_total_gamepads(VALUE self)
{
  return RB_INT2FIX(SDL_NumJoysticks());
}

static VALUE input_gamepad_change(VALUE self)
{
  return RB_INT2FIX(shState->rtData().joystick_change);
}

static VALUE input_gamepad_clear_change(VALUE self)
{
  shState->rtData().joystick_change = 0;
  return ZERO;
}

static VALUE input_sdl_bindings_change()
{
  return shState->rtData().modify_settings ? Qtrue : Qfalse;
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

static VALUE input_gamepad_close(VALUE self)
{
  bool result = EventThread::close_joystick();
  return result ? Qtrue : Qfalse;
}

static VALUE input_gamepad_open(VALUE self)
{
  bool result = EventThread::open_joystick();
  return result ? Qtrue : Qfalse;
}

static VALUE input_gamepad_reset(VALUE self)
{
  input_gamepad_close(self);
  return input_gamepad_open(self);
}

static VALUE input_gamepad_basic_values(VALUE self)
{
  VALUE ary = rb_ary_new();
  if (!SDL_NumJoysticks()) {
    rb_ary_push(ary, rstr("None"));
    rb_ary_push(ary, ZERO);
    rb_ary_push(ary, ZERO);
    rb_ary_push(ary, SUBZERO);
    rb_ary_push(ary, Qfalse);
    return ary;
  }
  const char *name = SDL_JoystickName(shState->rtData().joystick);
  int val, rumble;
  rb_ary_push(ary, rstr(name));
  std::vector<int> v = shState->input().joystick_basic_values();
  for (int n = 0; n < v.size(); n++) {
    val = v.at(n);
    rb_ary_push(ary, RB_INT2FIX(val));
  }
  rumble = SDL_JoystickHasRumble(shState->rtData().joystick);
  rb_ary_push(ary, rumble ? Qtrue : Qfalse);
  return ary;
}

static VALUE input_trigger_timer(VALUE self)
{
  int n = shState->input().trigger_timer();
  return RB_INT2FIX(n);
}

static VALUE input_default_trigger_timer(VALUE self)
{
  return rb_iv_get(self, "default_trigger_timer");
}

static VALUE input_default_trigger_timer_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = RB_FIX2INT(val);
  if (n < 0)
    return rb_iv_get(self, "default_trigger_timer");
  shState->input().set_trigger_base_timer(n);
  return rb_iv_set(self, "default_trigger_timer", val);
}

void input_create_gamepad_types(VALUE pad)
{
  VALUE types, levels;
  types = rb_ary_new();
  rb_ary_push(types, rstr("Unknown"));
  rb_ary_push(types, rstr("Controller"));
  rb_ary_push(types, rstr("Wheel"));
  rb_ary_push(types, rstr("Arcade Stick"));
  rb_ary_push(types, rstr("Flight Stick"));
  rb_ary_push(types, rstr("Dance Pad"));
  rb_ary_push(types, rstr("Guitar"));
  rb_ary_push(types, rstr("Drum Kit"));
  rb_ary_push(types, rstr("Arcade Pad"));
  rb_ary_push(types, rstr("Throttle"));
  levels = rb_hash_new();
  rb_hash_aset(levels, SUBZERO, rstr("Unknown"));
  rb_hash_aset(levels, ZERO, rstr("Empty"));
  rb_hash_aset(levels, ONE, rstr("Low"));
  rb_hash_aset(levels, RB_INT2FIX(2), rstr("Medium"));
  rb_hash_aset(levels, RB_INT2FIX(3), rstr("Full"));
  rb_hash_aset(levels, RB_INT2FIX(4), rstr("Wired"));
  rb_hash_aset(levels, RB_INT2FIX(5), rstr("Maximum"));
  rb_cvar_set(pad, rb_intern("@@types"), types);
  rb_cvar_set(pad, rb_intern("@@levels"), levels);
}

void inputBindingInit()
{
  VALUE input;
  input = rb_define_module("Input");
  gamepad = rb_define_class_under(input, "Gamepad", rb_cObject);
  rb_const_set(gamepad, rb_intern("DEFAULT_NAME"), rstr("None"));
  rb_const_set(gamepad, rb_intern("DEFAULT_VENDOR"), rstr("None"));
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
  input_create_gamepad_types(gamepad);
  rb_iv_set(input, "text_input", ZERO);
  rb_iv_set(input, "default_trigger_timer", RB_INT2FIX(TRIGGER_TIMER));
  rb_iv_set(input, "@gamepad_updates", rb_ary_new());
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
  module_func(input, "press?", inputPress, 1);
  module_func(input, "press_left_click?", input_press_left_click, 0);
  module_func(input, "press_right_click?", input_press_right_click, 0);
  module_func(input, "press_any?", input_press_any, 0);
  module_func(input, "press_all?", input_are_pressed, -1);
  module_func(input, "trigger?", inputTrigger, 1);
  module_func(input, "trigger_any?", input_trigger_any, 0);
  module_func(input, "trigger_double?", input_trigger_double, 1);
  module_func(input, "trigger_buttons?", input_are_triggered, -1);
  module_func(input, "trigger_up_down?", input_trigger_up_down, 0);
  module_func(input, "trigger_left_right?", input_trigger_left_right, 0);
  module_func(input, "trigger_exclude?", input_trigger_exclude, -1);
  module_func(input, "trigger_type", input_trigger_kind, 0);
  module_func(input, "trigger_gp_value", input_trigger_gp_value, 0);
  module_func(input, "trigger_gp_axis", input_trigger_gp_axis, 0);
  module_func(input, "trigger_gp_dir", input_trigger_gp_dir, 0);
  module_func(input, "trigger_gp_clear", input_trigger_gp_clear, 0);
  module_func(input, "trigger_last", input_trigger_last, 0);
  module_func(input, "trigger_old", input_trigger_old, 0);
  module_func(input, "repeat?", inputRepeat, 1);
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
  module_func(input, "gamepad", input_gamepad, 0);
  module_func(input, "gamepad?", input_has_gamepad, 0);
  module_func(input, "total_gamepads", input_total_gamepads, 0);
  module_func(input, "gamepad_change?", input_gamepad_change, 0);
  module_func(input, "gamepad_clear_change", input_gamepad_clear_change, 0);
  module_func(input, "gamepad_bindings_change?", input_sdl_bindings_change, 0);
  module_func(input, "gamepad_update", input_gamepad_update, 0);
  module_func(input, "gamepad_updates", input_gamepad_updates, 0);
  module_func(input, "gamepad_close!", input_gamepad_close, 0);
  module_func(input, "gamepad_open!", input_gamepad_open, 0);
  module_func(input, "gamepad_reset!", input_gamepad_reset, 0);
  module_func(input, "gamepad_basic_values", input_gamepad_basic_values, 0);
  module_func(input, "reset_sdl_bindings", input_reset_sdl_bindings, 0);
  module_func(input, "reset_ruby_bindings", input_reset_rb_bindings, 0);
  module_func(input, "dir4", inputDir4, 0);
  module_func(input, "dir8", inputDir8, 0);
  module_func(input, "dir4?", input_is_dir4, 0);
  module_func(input, "dir8?", input_is_dir8, 0);
  VALUE key, val, hash = rb_hash_new();
  const char *tmp;
  for (size_t i = 0; i < button_scancodesN; ++i) {
    key = RB_INT2FIX(button_scancodes[i].button);
    val = RB_INT2FIX(button_scancodes[i].scancode);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("BUTTON_SCANCODES"), hash);
  rb_hash_set_ifnone(hash, ZERO);
  hash = rb_funcall(hash, rb_intern("invert"), 0);
  rb_const_set(input, rb_intern("SCANCODE_BUTTONS"), hash);
  rb_hash_set_ifnone(hash, ZERO);
  hash = rb_hash_new();
  /* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
   * to be compatible with the previous convention */
  for (size_t i = 0; i < buttonCodesN; ++i) {
    ID sym = rb_intern(buttonCodes[i].str);
    val = RB_INT2FIX(buttonCodes[i].val);
    rb_const_set(input, sym, val);
    rb_hash_aset(hash, rb_id2sym(sym), val);
  }
  getRbData()->buttoncodeHash = hash;
  rb_const_set(input, rb_intern("BUTTON_CODES"), hash);
  rb_hash_set_ifnone(hash, ZERO);
  hash = rb_funcall(hash, rb_intern("invert"), 0);
  rb_hash_set_ifnone(hash, ZERO);
  rb_const_set(input, rb_intern("CODE_BUTTONS"), hash);
  hash = rb_hash_new();
  for (size_t i = 0; i < vendorsN; ++i) {
    key = RB_INT2FIX(vendors[i].id);
    val = rstr(vendors[i].name);
    rb_hash_aset(hash, key, val);
  }
  rb_hash_set_ifnone(hash, rb_const_get(gamepad, rb_intern("DEFAULT_VENDOR")));
  rb_cvar_set(gamepad, rb_intern("@@vendors"), hash);
  hash = rb_hash_new();
  for (size_t i = 0; i < button_stringsN; ++i) {
    key = RB_INT2FIX(button_strings[i].code);
    tmp = button_strings[i].str;
    val = !tmp ? Qnil : rstr(tmp);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("KEY2CHAR"), hash);
  hash = rb_hash_dup(hash);
  for (size_t i = 0; i < other_button_stringsN; ++i) {
    key = RB_INT2FIX(other_button_strings[i].code);
    tmp = other_button_strings[i].str;
    val = !tmp ? rstr("") : rstr(tmp);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("KEY2NAME"), hash);
}

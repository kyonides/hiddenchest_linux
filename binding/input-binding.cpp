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
#include "exception.h"
#include "binding-util.h"
#include "util.h"
#include "hcextras.h"
#include <SDL_keyboard.h>
#include "debugwriter.h"

#define ZERO RB_INT2FIX(0)
#define SUBZERO RB_INT2FIX(-1)

static VALUE joystick;

static void input_gamepad_init(VALUE pad, VALUE nm, int vndr, int kind,
                               int lvl, VALUE b1, VALUE b2)
{
  VALUE type, level, vendor, vendor_id, vendors;
  type = rb_cvar_get(joystick, rb_intern("types"));
  type = rb_ary_entry(type, kind);
  level = rb_cvar_get(joystick, rb_intern("levels"));
  level = rb_hash_aref(level, lvl);
  vendor_id = RB_INT2FIX(vndr);
  vendors = rb_cvar_get(joystick, rb_intern("vendors"));
  vendor = rb_hash_aref(vendors, vendor_id);
  rb_iv_set(pad, "@name", nm);
  rb_iv_set(pad, "@vendor", vendor);
  rb_iv_set(pad, "@vendor_id", vendor_id);
  rb_iv_set(pad, "@type", type);
  rb_iv_set(pad, "@type_number", RB_INT2FIX(kind));
  rb_iv_set(pad, "@power", level);
  rb_iv_set(pad, "@power_level", RB_INT2FIX(lvl));
  rb_iv_set(pad, "@rumble", b1);
  rb_iv_set(pad, "@last_rumble", Qnil);
  rb_iv_set(pad, "@active", b2);
}

static VALUE input_gamepad_new(VALUE input)
{
  VALUE gamepad, name;
  gamepad = rb_class_new_instance(0, 0, joystick);
  name = rb_const_get(joystick, rb_intern("DEFAULT_NAME"));
  input_gamepad_init(gamepad, name, 0, 0, -1, Qfalse, Qfalse);
  return rb_iv_set(input, "gamepad", gamepad);
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

static void joystick_state_change(VALUE input)
{
  int state = shState->input().joystick_change();
  if (state == 0)
    return;
  VALUE ary = rb_iv_get(input, "joystick_updates");
  rb_ary_pop(ary);
  if (state == 2) {
    const char *name = shState->input().joystick_name();
    int vendor = shState->input().joystick_vendor();
    int kind = shState->input().joystick_kind();
    int power = shState->input().joystick_power();
    VALUE gamepad, rumble;
    gamepad = rb_iv_get(input, "gamepad");
    rumble = shState->input().joystick_has_rumble() ? Qtrue : Qfalse;
    input_gamepad_init(gamepad, rstr(name), vendor, kind, power, rumble, Qtrue);
    rb_ary_push(ary, hc_sym("add"));
  } else {
    rb_ary_push(ary, hc_sym("remove"));
    ary = input_gamepad_new(input);
  }
  shState->input().reset_joystick_change();
}

static VALUE input_update(VALUE self)
{
  shState->input().update();
  joystick_state_change(self);
  return Qnil;
}

// FIXME: RMXP allows only few more types that don't make sense (symbols in pre 3, floats)
static int getButtonArg(VALUE number)
{
  if (FIXNUM_P(number))
    return RB_FIX2INT(number);
  if (SYMBOL_P(number)) {// && rgssVer == 3
    VALUE sym_hash = getRbData()->buttoncodeHash;
    return RB_FIX2INT(rb_hash_aref(sym_hash, number));
  }
  return 0;
}

static VALUE inputPress(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
  return shState->input().isPressed(num) ? Qtrue : Qfalse;
}

static VALUE inputTrigger(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
  return shState->input().isTriggered(num) ? Qtrue : Qfalse;
}

static VALUE inputRepeat(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
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
  int num = getButtonArg(number);
  return shState->input().is_triggered_double(num) ? Qtrue : Qfalse;
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
  if (ARRAY_TYPE_P(buttons[0])) {
    VALUE rbuttons = buttons[0];
    int size = RARRAY_LEN(rbuttons);
    for (int n = 0; n < size; n++) {
      int num = getButtonArg(rb_ary_entry(rbuttons, n));
      if (shState->input().isTriggered(num))
        return Qtrue;
    }
    return Qfalse;
  }
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(buttons[n]);
    if (shState->input().isTriggered(num))
      return Qtrue;
  }
  return Qfalse;
}

static VALUE input_are_pressed(int size, VALUE* buttons, VALUE self)
{
  if (ARRAY_TYPE_P(buttons[0])) {
    VALUE rbuttons = buttons[0];
    int size = RARRAY_LEN(rbuttons);
    for (int n = 0; n < size; n++) {
      int num = getButtonArg(rb_ary_entry(rbuttons, n));
      if (!shState->input().isPressed(num))
        return Qfalse;
    }
    return Qtrue;
  }
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(buttons[n]);
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

static VALUE input_text_input_set(VALUE self, VALUE state)
{
  shState->input().set_text_input(state == Qtrue);
  return rb_iv_set(self, "text_input", state);
}

static VALUE input_text_input_clear(VALUE self)
{
  input_last_key_clear(self);
  return input_text_input_set(self, Qfalse);
}

static VALUE input_joystick(VALUE self)
{
  return rb_iv_get(self, "gamepad");
}

static VALUE input_has_joystick(VALUE self)
{
  return shState->input().has_joystick() ? Qtrue : Qfalse;
}

static VALUE input_joystick_update(VALUE self)
{
  VALUE state = rb_iv_get(self, "joystick_updates");
  return rb_ary_pop(state);
}

static VALUE input_joystick_updates(VALUE self)
{
  return rb_iv_get(self, "joystick_updates");
}

static VALUE input_joystick_close(VALUE self)
{
  bool result = shState->input().close_joystick();
  if (result)
    joystick_state_change(self);
  return result ? Qtrue : Qfalse;
}

static VALUE input_joystick_open(VALUE self)
{
  bool result = shState->input().open_joystick();
  if (result)
    joystick_state_change(self);
  return result ? Qtrue : Qfalse;
}

static VALUE input_joystick_reset(VALUE self)
{
  input_joystick_close(self);
  return input_joystick_open(self);
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
  rb_hash_aset(levels, RB_INT2FIX(1), rstr("Low"));
  rb_hash_aset(levels, RB_INT2FIX(2), rstr("Medium"));
  rb_hash_aset(levels, RB_INT2FIX(3), rstr("Full"));
  rb_hash_aset(levels, RB_INT2FIX(4), rstr("Wired"));
  rb_hash_aset(levels, RB_INT2FIX(5), rstr("Maximum"));
  rb_cvar_set(pad, rb_intern("types"), types);
  rb_cvar_set(pad, rb_intern("levels"), levels);
}

void inputBindingInit()
{
  VALUE input, gamepad;
  input = rb_define_module("Input");
  joystick = rb_define_class_under(input, "Gamepad", rb_cObject);
  rb_const_set(joystick, rb_intern("DEFAULT_NAME"), rstr("None"));
  rb_const_set(joystick, rb_intern("DEFAULT_VENDOR"), rstr("None"));
  rb_define_attr(joystick, "name", 1, 0);
  rb_define_attr(joystick, "vendor", 1, 0);
  rb_define_attr(joystick, "type", 1, 0);
  rb_define_attr(joystick, "type_number", 1, 0);
  rb_define_attr(joystick, "power", 1, 0);
  rb_define_attr(joystick, "power_level", 1, 0);
  rb_define_attr(joystick, "active", 1, 0);
  rb_define_attr(joystick, "rumble", 1, 0);
  rb_define_attr(joystick, "last_rumble", 1, 0);
  rb_define_method(joystick, "set_rumble", RMF(input_gamepad_set_rumble), 3);
  input_create_gamepad_types(joystick);
  rb_iv_set(input, "text_input", Qfalse);
  rb_iv_set(input, "default_trigger_timer", RB_INT2FIX(TRIGGER_TIMER));
  rb_iv_set(input, "joystick_updates", rb_ary_new());
  module_func(input, "trigger_timer", input_trigger_timer, 0);
  module_func(input, "base_trigger_timer", input_default_trigger_timer, 0);
  module_func(input, "base_trigger_timer=", input_default_trigger_timer_set, 1);
  module_func(input, "default_trigger_timer", input_default_trigger_timer, 0);
  module_func(input, "default_trigger_timer=", input_default_trigger_timer_set, 1);
  module_func(input, "update", input_update, 0);
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
  module_func(input, "gamepad", input_joystick, 0);
  module_func(input, "joystick", input_joystick, 0);
  module_func(input, "gamepad?", input_has_joystick, 0);
  module_func(input, "joystick?", input_has_joystick, 0);
  module_func(input, "gamepad_update", input_joystick_update, 0);
  module_func(input, "joystick_update", input_joystick_update, 0);
  module_func(input, "gamepad_updates", input_joystick_updates, 0);
  module_func(input, "joystick_updates", input_joystick_updates, 0);
  module_func(input, "gamepad_close!", input_joystick_close, 0);
  module_func(input, "joystick_close!", input_joystick_close, 0);
  module_func(input, "gamepad_open!", input_joystick_open, 0);
  module_func(input, "joystick_open!", input_joystick_open, 0);
  module_func(input, "gamepad_reset!", input_joystick_reset, 0);
  module_func(input, "joystick_reset!", input_joystick_reset, 0);
  module_func(input, "dir4", inputDir4, 0);
  module_func(input, "dir8", inputDir8, 0);
  module_func(input, "dir4?", input_is_dir4, 0);
  module_func(input, "dir8?", input_is_dir8, 0);
  VALUE key, val, hash = rb_hash_new();
  const char *tmp;
  /* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
   * to be compatible with the previous convention */
  for (size_t i = 0; i < buttonCodesN; ++i) {
    ID sym = rb_intern(buttonCodes[i].str);
    val = RB_INT2FIX(buttonCodes[i].val);
    rb_const_set(input, sym, val);
    rb_hash_aset(hash, rb_id2sym(sym), val);
  }
  getRbData()->buttoncodeHash = hash;
  rb_hash_set_ifnone(hash, ZERO);
  hash = rb_hash_new();
  for (size_t i = 0; i < vendorsN; ++i) {
    key = RB_INT2FIX(vendors[i].id);
    val = rstr(vendors[i].name);
    rb_hash_aset(hash, key, val);
  }
  rb_hash_set_ifnone(hash, rb_const_get(joystick,rb_intern("DEFAULT_VENDOR")));
  rb_cvar_set(joystick, rb_intern("vendors"), hash);
  hash = rb_hash_new();
  for (size_t i = 0; i < buttonStringsN; ++i) {
    key = RB_INT2FIX(buttonStrings[i].code);
    tmp = buttonStrings[i].str;
    val = !tmp ? Qnil : rstr(tmp);
    rb_hash_aset(hash, key, val);
  }
  rb_const_set(input, rb_intern("KEY2CHAR"), hash);
  gamepad = input_gamepad_new(input);
}

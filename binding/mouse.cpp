/*
** mouse.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2024 Kyonides-Arkanthes
*/

#include "clicks.h"
#include "hcextras.h"
#include "input.h"
#include "sharedstate.h"

#define ZERO RB_INT2FIX(0)

static VALUE mouse_click_timer(VALUE self)
{
  int n = shState->input().click_timer();
  return RB_INT2FIX(n);
}

static VALUE mouse_default_timer(VALUE self)
{
  return rb_iv_get(self, "default_timer");
}

static VALUE mouse_default_timer_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = RB_FIX2INT(val);
  if (n < 0)
    return rb_iv_get(self, "default_timer");
  shState->input().set_click_base_timer(n);
  return rb_iv_set(self, "default_timer", val);
}

static VALUE mouse_scroll_factor(VALUE self)
{
  return rb_iv_get(self, "scroll_factor");
}

static VALUE mouse_scroll_factor_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = RB_FIX2INT(val);
  if (n < 0)
    return rb_iv_get(self, "scroll_factor");
  shState->input().set_scroll_factor(n);
  return rb_iv_set(self, "scroll_factor", val);
}

static VALUE mouse_x(VALUE self)
{
  return RB_INT2FIX(shState->input().mouseX());
}

static VALUE mouse_y(VALUE self)
{
  return RB_INT2FIX(shState->input().mouseY());
}

static VALUE mouse_ox(VALUE self)
{
  return rb_iv_get(self, "ox");
}

static VALUE mouse_oy(VALUE self)
{
  return rb_iv_get(self, "oy");
}

static VALUE mouse_xy_set(VALUE self, VALUE mx, VALUE my)
{
  int x = RB_FIX2INT(mx);
  int y = RB_FIX2INT(my);
  shState->input().mouse_set_xy(x, y);
  return rb_ary_new3(2, mx, my);
}

static VALUE mouse_ox_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = RB_FIX2INT(val);
  shState->input().mouse_set_ox(n);
  return rb_iv_set(self, "ox", val);
}

static VALUE mouse_oy_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = RB_FIX2INT(val);
  shState->input().mouse_set_oy(n);
  return rb_iv_set(self, "oy", val);
}

static VALUE mouse_scroll_x(VALUE self)
{
  int n = shState->input().mouse_scroll_x();
  return RB_INT2FIX(n);
}

static VALUE mouse_scroll_y(VALUE self)
{
  int n = shState->input().mouse_scroll_y();
  return RB_INT2FIX(n);
}

static VALUE mouse_scroll_reset(VALUE self)
{
  shState->input().mouse_scroll_reset();
  return ZERO;
}

static VALUE mouse_scrolled_x(VALUE self, VALUE val)
{
  bool go_up;
  if (hc_sym("UP") == val)
    go_up = true;
  else if (hc_sym("DOWN") == val)
    go_up = false;
  else
    return Qnil;
  return shState->input().is_mouse_scroll_x(go_up) ? Qtrue : Qfalse; 
}

static VALUE mouse_scrolled_y(VALUE self, VALUE val)
{
  bool go_up;
  if (hc_sym("UP") == val)
    go_up = true;
  else if (hc_sym("DOWN") == val)
    go_up = false;
  else
    return Qnil;
  return shState->input().is_mouse_scroll_y(go_up) ? Qtrue : Qfalse; 
}

static VALUE mouse_no_target(VALUE self, VALUE obj)
{
  return rb_iv_get(self, "target") == Qnil ? Qtrue : Qfalse;
}

static VALUE mouse_target(VALUE self, VALUE obj)
{
  VALUE ml = rb_iv_get(self, "target");
  return (ml != Qnil & ml == obj)? Qtrue : Qfalse;
}

static VALUE mouse_target_set(VALUE self, VALUE obj)
{
  return rb_iv_set(self, "target", obj);
}

void init_mouse()
{
  VALUE mouse = rb_define_module("Mouse");
  rb_iv_set(mouse, "ox", RB_INT2FIX(8));
  rb_iv_set(mouse, "oy", RB_INT2FIX(-4));
  rb_iv_set(mouse, "default_timer", RB_INT2FIX(CLICK_TIMER));
  rb_iv_set(mouse, "scroll_factor", RB_INT2FIX(SCROLL_FACTOR));
  rb_iv_set(mouse, "target", Qnil);
  module_func(mouse, "click_timer", mouse_click_timer, 0);
  module_func(mouse, "base_timer", mouse_default_timer, 0);
  module_func(mouse, "base_timer=", mouse_default_timer_set, 1);
  module_func(mouse, "default_timer", mouse_default_timer, 0);
  module_func(mouse, "default_timer=", mouse_default_timer_set, 1);
  module_func(mouse, "scroll_factor", mouse_scroll_factor, 0);
  module_func(mouse, "scroll_factor=", mouse_scroll_factor_set, 1);
  module_func(mouse, "scroll_x?", mouse_scrolled_x, 1);
  module_func(mouse, "scroll_y?", mouse_scrolled_y, 1);
  module_func(mouse, "x", mouse_x, 0);
  module_func(mouse, "y", mouse_y, 0);
  module_func(mouse, "ox", mouse_ox, 0);
  module_func(mouse, "oy", mouse_oy, 0);
  module_func(mouse, "set_xy", mouse_xy_set, 2);
  module_func(mouse, "ox=", mouse_ox_set, 1);
  module_func(mouse, "oy=", mouse_oy_set, 1);
  module_func(mouse, "scroll_x", mouse_scroll_x, 0);
  module_func(mouse, "scroll_y", mouse_scroll_y, 0);
  module_func(mouse, "scroll_reset", mouse_scroll_reset, 0);
  module_func(mouse, "no_target?", mouse_no_target, 0);
  module_func(mouse, "target?", mouse_target, 1);
  module_func(mouse, "target=", mouse_target_set, 1);
}

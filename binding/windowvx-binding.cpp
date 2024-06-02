/*
** window-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
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

#include "windowvx.h"
#include "disposable-binding.h"
#include "viewportelement-binding.h"
#include "binding-util.h"
#include "bitmap.h"
#include "hcextras.h"

DECL_TYPE(Rect);
extern VALUE rect_from_ary(VALUE ary);

DEF_TYPE_CUSTOMNAME(WindowVX, "Window");

void bitmapInitProps(Bitmap *b, VALUE self);

extern VALUE zero;

RB_METHOD(windowVXInitialize)
{
  WindowVX *w;
  if (rgssVer == 3) {
    int x, y, width, height;
    x = y = width = height = 0;
    if (argc == 4)
      rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);
    w = new WindowVX(x, y, width, height);
  } else {
	w = viewportElementInitialize<WindowVX>(argc, argv, self);
  }
  setPrivateData(self, w);
  w->initDynAttribs();
  rb_iv_set(self, "@area", rb_ary_new());
  wrapProperty(self, &w->getCursorRect(), "cursor_rect", RectType);
  if (rgssVer == 3)
    wrapProperty(self, &w->getTone(), "tone", ToneType);
  Bitmap *contents = new Bitmap(1, 1);
  VALUE contentsObj = wrapObject(contents, BitmapType);
  bitmapInitProps(contents, contentsObj);
  rb_iv_set(self, "contents", contentsObj);
  rb_iv_set(self, "x", zero);
  rb_iv_set(self, "y", zero);
  rb_iv_set(self, "width", RB_INT2FIX(1));
  rb_iv_set(self, "height", RB_INT2FIX(1));
  rb_iv_set(self, "pause_x", zero);
  rb_iv_set(self, "pause_y", zero);
  return self;
}

static VALUE windowVXUpdate(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  w->update();
  return Qnil;
}

RB_METHOD(windowVXMove)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  int x, y, width, height;
  rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);
  w->move(x, y, width, height);
  return Qnil;
}

static VALUE windowVXIsOpen(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return Qnil;
  return w->isOpen() ? Qtrue : Qfalse;
}

static VALUE windowVXIsClosed(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return Qnil;
  return w->isClosed() ? Qtrue : Qfalse;
}

static VALUE window_x(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  return !w ? zero : rb_iv_get(self, "x");
}

static VALUE window_y(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  return !w ? zero : rb_iv_get(self, "y");
}

static VALUE window_width(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  return !w ? zero : rb_iv_get(self, "width");
}

static VALUE window_height(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  return !w ? zero : rb_iv_get(self, "height");
}

static VALUE window_pause_x(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  return !w ? zero : rb_iv_get(self, "pause_x");
}

static VALUE window_pause_y(VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  return !w ? zero : rb_iv_get(self, "pause_y");
}

static VALUE window_x_set(VALUE self, VALUE rx)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return zero;
  w->set_x(RB_FIX2INT(rx));
  return rx;
}

static VALUE window_y_set(VALUE self, VALUE ry)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return zero;
  w->set_y(RB_FIX2INT(ry));
  return ry;
}

static VALUE window_set_xy(VALUE self, VALUE rx, VALUE ry)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return rb_ary_new3(2, zero, zero);
  w->set_xy(RB_FIX2INT(rx), RB_FIX2INT(ry));
  return rb_ary_new3(2, rx, ry);
}

static VALUE window_set_xyz(VALUE self, VALUE rx, VALUE ry, VALUE rz)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return rb_ary_new3(3, zero, zero, zero);
  w->set_xy(RB_FIX2INT(rx), RB_FIX2INT(ry));
  w->setZ(RB_FIX2INT(rz));
  return rb_ary_new3(3, rx, ry, rz);
}

static VALUE window_width_set(VALUE self, VALUE rw)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return zero;
  w->set_width(RB_FIX2INT(rw));
  return rw;
}

static VALUE window_height_set(VALUE self, VALUE rh)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return zero;
  w->set_height(RB_FIX2INT(rh));
  return rh;
}

static VALUE window_pause_x_set(VALUE self, VALUE rx)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return zero;
  w->set_pause_x(RB_FIX2INT(rx));
  return rx;
}

static VALUE window_pause_y_set(VALUE self, VALUE ry)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return zero;
  w->set_pause_y(RB_FIX2INT(ry));
  return ry;
}

static VALUE window_pause_set_xy(VALUE self, VALUE rx, VALUE ry)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return rb_ary_new3(2, zero, zero);
  w->set_pause_xy(RB_FIX2INT(rx), RB_FIX2INT(ry));
  return rb_ary_new3(2, rx, ry);
}

static VALUE window_is_mouse_inside(int argc, VALUE *v, VALUE self)
{
  WindowVX *w = getPrivateData<WindowVX>(self);
  if (!w)
    return Qnil;
  if (argc == 0) {
    return w->is_mouse_inside() ? Qtrue : Qfalse;
  }
  int index = RB_FIX2INT(v[0]);
  VALUE area = rb_ary_entry(rb_iv_get(self, "@area"), index);
  if (area == Qnil)
    return Qnil;
  else if (ARRAY_TYPE_P(area))
    area = rect_from_ary(area);
  Rect *r = static_cast<Rect*>(RTYPEDDATA_DATA(area));
  return w->is_mouse_inside(r->x, r->y, r->width, r->height) ? Qtrue : Qfalse;
}

DEF_PROP_OBJ_REF(WindowVX, Bitmap, Windowskin, "windowskin")
DEF_PROP_OBJ_REF(WindowVX, Bitmap, Contents, "contents")
DEF_PROP_OBJ_VAL(WindowVX, Rect, CursorRect, "cursor_rect")
DEF_PROP_OBJ_VAL(WindowVX, Tone, Tone,       "tone")
DEF_PROP_I(WindowVX, OX)
DEF_PROP_I(WindowVX, OY)
DEF_PROP_I(WindowVX, Padding)
DEF_PROP_I(WindowVX, PaddingBottom)
DEF_PROP_I(WindowVX, Opacity)
DEF_PROP_I(WindowVX, BackOpacity)
DEF_PROP_I(WindowVX, ContentsOpacity)
DEF_PROP_I(WindowVX, Openness)
DEF_PROP_B(WindowVX, Active)
DEF_PROP_B(WindowVX, ArrowsVisible)
DEF_PROP_B(WindowVX, Pause)

void windowVXBindingInit()
{
  VALUE klass = rb_define_class("Window", rb_cObject);
  rb_define_alloc_func(klass, classAllocate<&WindowVXType>);
  disposableBindingInit<WindowVX>(klass);
  viewportElementBindingInit<WindowVX>(klass);
  rb_define_attr(klass, "area", 1, 0);
  _rb_define_method(klass, "initialize", windowVXInitialize);
  rb_define_method(klass, "update", RMF(windowVXUpdate), 0);
  INIT_PROP_BIND(WindowVX, Windowskin,      "windowskin"      );
  INIT_PROP_BIND(WindowVX, Contents,        "contents"        );
  INIT_PROP_BIND(WindowVX, CursorRect,      "cursor_rect"     );
  INIT_PROP_BIND(WindowVX, Active,          "active"          );
  INIT_PROP_BIND(WindowVX, Pause,           "pause"           );
  INIT_PROP_BIND(WindowVX, OX,              "ox"              );
  INIT_PROP_BIND(WindowVX, OY,              "oy"              );
  INIT_PROP_BIND(WindowVX, Opacity,         "opacity"         );
  INIT_PROP_BIND(WindowVX, BackOpacity,     "back_opacity"    );
  INIT_PROP_BIND(WindowVX, ContentsOpacity, "contents_opacity");
  INIT_PROP_BIND(WindowVX, Openness,        "openness"        );
  rb_define_method(klass, "x", RMF(window_x), 0);
  rb_define_method(klass, "y", RMF(window_y), 0);
  rb_define_method(klass, "x=", RMF(window_x_set), 1);
  rb_define_method(klass, "y=", RMF(window_y_set), 1);
  rb_define_method(klass, "set_xy", RMF(window_set_xy), 2);
  rb_define_method(klass, "set_xyz", RMF(window_set_xyz), 3);
  rb_define_method(klass, "width", RMF(window_width), 0);
  rb_define_method(klass, "height", RMF(window_height), 0);
  rb_define_method(klass, "width=", RMF(window_width_set), 1);
  rb_define_method(klass, "height=", RMF(window_height_set), 1);
  rb_define_method(klass, "pause_x", RMF(window_pause_x), 0);
  rb_define_method(klass, "pause_y", RMF(window_pause_y), 0);
  rb_define_method(klass, "pause_x=", RMF(window_pause_x_set), 1);
  rb_define_method(klass, "pause_y=", RMF(window_pause_y_set), 1);
  rb_define_method(klass, "pause_xy", RMF(window_pause_set_xy), 2);
  rb_define_method(klass, "mouse_inside?", RMF(window_is_mouse_inside), -1);
  rb_define_method(klass, "mouse_above?", RMF(window_is_mouse_inside), -1);
  _rb_define_method(klass, "move", windowVXMove);
  rb_define_method(klass, "open?", RMF(windowVXIsOpen), 0);
  rb_define_method(klass, "close?", RMF(windowVXIsClosed), 0);
  INIT_PROP_BIND(WindowVX, ArrowsVisible,   "arrows_visible");
  INIT_PROP_BIND(WindowVX, Padding,         "padding"       );
  INIT_PROP_BIND(WindowVX, PaddingBottom,   "padding_bottom");
  INIT_PROP_BIND(WindowVX, Tone,            "tone"          );
}

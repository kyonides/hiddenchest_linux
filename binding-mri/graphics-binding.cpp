/*
** graphics-binding.cpp
**
** This file is part of mkxpplus and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** 2018 Modified by Kyonides-Arkanthes
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

#include "hcextras.h"
#include "graphics.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"
#include "exception.h"

extern VALUE rect_from_ary(VALUE ary);

static VALUE graphicsUpdate(VALUE self)
{
  shState->graphics().update();
  return Qnil;
}

static VALUE graphicsFreeze(VALUE self)
{
  shState->graphics().freeze();
  return Qnil;
}

RB_METHOD(graphicsTransition)
{
  RB_UNUSED_PARAM;
  int duration = 8;
  const char *filename = "";
  int vague = 40;
  if (argc > 0)
    rb_get_args(argc, argv, "|izi", &duration, &filename, &vague RB_ARG_END);
  GUARD_EXC( shState->graphics().transition(duration, filename, vague); )
  return Qnil;
}

static VALUE graphicsFrameReset(VALUE self)
{
  shState->graphics().frameReset();
  return Qnil;
}

static VALUE graphicsWidth(VALUE self)
{
  return RB_INT2FIX(shState->graphics().width());
}

static VALUE graphicsHeight(VALUE self)
{
  return RB_INT2FIX(shState->graphics().height());
}

static VALUE graphics_dimensions(VALUE self)
{
  VALUE width = RB_INT2NUM( shState->graphics().width() );
  VALUE height = RB_INT2NUM( shState->graphics().height() );
  return rb_ary_new3(2, width, height);
}

static VALUE graphics_screen_rect(VALUE self)
{
  VALUE ary, width, height, zero = RB_INT2FIX(0);
  width = RB_INT2NUM( shState->graphics().width() );
  height = RB_INT2NUM( shState->graphics().height() );
  ary = rb_ary_new3(4, zero, zero, width, height);
  return rect_from_ary(ary);
}

RB_METHOD(graphicsWait)
{
  RB_UNUSED_PARAM;
  int duration;
  rb_get_args(argc, argv, "i", &duration RB_ARG_END);
  shState->graphics().wait(duration);
  return Qnil;
}

RB_METHOD(graphicsFadeout)
{
  RB_UNUSED_PARAM;
  int duration;
  rb_get_args(argc, argv, "i", &duration RB_ARG_END);
  shState->graphics().fadeout(duration);
  return Qnil;
}

RB_METHOD(graphicsFadein)
{
  RB_UNUSED_PARAM;
  int duration;
  rb_get_args(argc, argv, "i", &duration RB_ARG_END);
  shState->graphics().fadein(duration);
  return Qnil;
}

void bitmapInitProps(Bitmap *b, VALUE self);

static VALUE graphicsSnapToBitmap(VALUE self)
{
  Bitmap *result = 0;
  GUARD_EXC( result = shState->graphics().snapToBitmap(); );
  VALUE obj = wrapObject(result, BitmapType);
  bitmapInitProps(result, obj);
  return obj;
}

static VALUE graphics_snap2gray_bitmap(VALUE self)
{
  Bitmap *result = 0;
  GUARD_EXC( result = shState->graphics().snap_to_gray_bitmap(); );
  VALUE obj = wrapObject(result, BitmapType);
  bitmapInitProps(result, obj);
  return obj;
}

static VALUE graphics_snap2sepia_bitmap(VALUE self)
{
  Bitmap *result = 0;
  GUARD_EXC( result = shState->graphics().snap_to_sepia_bitmap(); );
  VALUE obj = wrapObject(result, BitmapType);
  bitmapInitProps(result, obj);
  return obj;
}

static VALUE graphics_snap2color_bitmap(VALUE self, VALUE color)
{
  Bitmap *b = 0;
  if (color == hc_sym("red"))
    GUARD_EXC( b = shState->graphics().snap_to_color_bitmap(0); )
  else if (color == hc_sym("green"))
    GUARD_EXC( b = shState->graphics().snap_to_color_bitmap(1); )
  else if (color == hc_sym("blue"))
    GUARD_EXC( b = shState->graphics().snap_to_color_bitmap(2); )
  else if (color == hc_sym("yellow"))
    GUARD_EXC( b = shState->graphics().snap_to_color_bitmap(3); )
  else if (color == hc_sym("sepia"))
    GUARD_EXC( b = shState->graphics().snap_to_sepia_bitmap(); )
  else if (color == hc_sym("gray"))
    GUARD_EXC( b = shState->graphics().snap_to_gray_bitmap(); )
  VALUE obj = wrapObject(b, BitmapType);
  bitmapInitProps(b, obj);
  return obj;
}

static VALUE graphics_save_screenshot(VALUE self)
{
  bool result = false;
  VALUE set = rb_const_get(rb_cObject, rb_intern("Settings"));
  safe_mkdir(rb_iv_get(set, "@snapshot_dir"));
  result = shState->graphics().save_screenshot();
  return result;
}

static VALUE graphicsResizeScreen(VALUE self, VALUE w, VALUE h)
{
  int width = RB_FIX2INT(w), height = RB_FIX2INT(h);
  shState->graphics().resizeScreen(width, height);
  return Qnil;
}

static VALUE graphicsReset(VALUE self)
{
  shState->graphics().reset();
  return Qnil;
}

static VALUE graphicsPlayMovie(VALUE self, VALUE filename)
{
  const char *fn = StringValueCStr(filename);
  shState->graphics().playMovie(fn);
  return Qnil;
}

static VALUE graphicsGetFrameRate(VALUE self)
{
  return rb_fix_new(shState->graphics().getFrameRate());
}

static VALUE graphicsSetFrameRate(VALUE self, VALUE num)
{
  shState->graphics().setFrameRate(RB_FIX2INT(num));
  return rb_fix_new(shState->graphics().getFrameRate());
}

static VALUE graphicsGetFrameCount(VALUE self)
{
  return rb_fix_new(shState->graphics().getFrameCount());
}

static VALUE graphicsSetFrameCount(VALUE self, VALUE num)
{
  shState->graphics().setFrameCount(RB_FIX2INT(num));
  return rb_fix_new(shState->graphics().getFrameCount());
}

static VALUE graphicsGetBrightness(VALUE self)
{
  return rb_fix_new(shState->graphics().getBrightness());
}

static VALUE graphicsSetBrightness(VALUE self, VALUE num)
{
  shState->graphics().setBrightness(RB_FIX2INT(num));
  return rb_fix_new(shState->graphics().getBrightness());
}

static VALUE graphics_get_block_fullscreen(VALUE self)
{
  return rb_iv_get(self, "@block_fullscreen");
}

static VALUE graphics_set_block_fullscreen(VALUE self, VALUE boolean)
{
  shState->graphics().set_block_fullscreen(boolean == Qtrue ? true : false);
  return rb_iv_set(self, "@block_fullscreen", boolean);
}

static VALUE graphics_get_block_ftwelve(VALUE self)
{
  return rb_iv_get(self, "@block_f12");
}

static VALUE graphics_set_block_ftwelve(VALUE self, VALUE boolean)
{
  shState->graphics().set_block_ftwelve(boolean == Qtrue ? true : false);
  return rb_iv_set(self, "@block_f12", boolean);
}

static VALUE graphics_get_block_fone(VALUE self)
{
  return rb_iv_get(self, "@block_f1");
}

static VALUE graphics_set_block_fone(VALUE self, VALUE boolean)
{
  shState->graphics().set_block_fone(boolean == Qtrue ? true : false);
  return rb_iv_set(self, "@block_f1", boolean);
}

static VALUE graphicsGetFullscreen(VALUE self)
{
  return shState->graphics().get_fullscreen() ? Qtrue : Qfalse;
}

static VALUE graphicsSetFullscreen(VALUE self, VALUE boolean)
{
  if (rb_iv_get(self, "@block_fullscreen") == Qtrue) return Qfalse;
  shState->graphics().set_fullscreen(boolean == Qtrue ? true : false);
  return shState->graphics().get_fullscreen() ? Qtrue : Qfalse;
}

static VALUE graphics_get_show_cursor(VALUE self)
{
  return shState->graphics().get_show_cursor() ? Qtrue : Qfalse;
}

static VALUE graphics_set_show_cursor(VALUE self, VALUE boolean)
{
  shState->graphics().set_show_cursor(boolean == Qtrue ? true : false);
  return boolean;
}

static VALUE graphics_get_delta(VALUE self)
{
  return RB_INT2FIX(shState->graphics().getDelta());
}

static VALUE graphics_get_title(VALUE self)
{
  return rstr(shState->get_title());
}

static VALUE graphics_set_title(VALUE self, VALUE title)
{
  shState->set_title(RSTRING_PTR(title));
  return title;
}

void graphicsBindingInit()
{
  VALUE graph = rb_define_module("Graphics");
  rb_iv_set(graph, "@block_fullscreen", Qfalse);
  rb_iv_set(graph, "@block_f12", Qfalse);
  rb_iv_set(graph, "@block_f1", Qfalse);
  rb_define_module_function(graph, "update", RMF(graphicsUpdate), 0);
  rb_define_module_function(graph, "freeze", RMF(graphicsFreeze), 0);
  rb_define_module_function(graph, "transition", RMF(graphicsTransition), -1);
  rb_define_module_function(graph, "frame_reset", RMF(graphicsFrameReset), 0);
  rb_define_module_function(graph, "__reset__", RMF(graphicsReset), 0);
  rb_define_module_function(graph, "frame_rate", RMF(graphicsGetFrameRate), 0);
  rb_define_module_function(graph, "frame_rate=", RMF(graphicsSetFrameRate), 1);
  rb_define_module_function(graph, "frame_count", RMF(graphicsGetFrameCount), 0);
  rb_define_module_function(graph, "frame_count=", RMF(graphicsSetFrameCount), 1);
  rb_define_module_function(graph, "width", RMF(graphicsWidth), 0);
  rb_define_module_function(graph, "height", RMF(graphicsHeight), 0);
  rb_define_module_function(graph, "dimensions", RMF(graphics_dimensions), 0);
  rb_define_module_function(graph, "size", RMF(graphics_dimensions), 0);
  rb_define_module_function(graph, "screen_rect", RMF(graphics_screen_rect), 0);
  rb_define_module_function(graph, "wait", RMF(graphicsWait), -1);
  rb_define_module_function(graph, "fadeout", RMF(graphicsFadeout), -1);
  rb_define_module_function(graph, "fadein", RMF(graphicsFadein), -1);
  rb_define_module_function(graph, "snap_to_bitmap", RMF(graphicsSnapToBitmap), 0);
  rb_define_module_function(graph, "snap_to_gray_bitmap", RMF(graphics_snap2gray_bitmap), 0);
  rb_define_module_function(graph, "snap_to_sepia_bitmap", RMF(graphics_snap2sepia_bitmap), 0);
  rb_define_module_function(graph, "snap_to_color_bitmap", RMF(graphics_snap2color_bitmap), 1);
  rb_define_module_function(graph, "save_screenshot", RMF(graphics_save_screenshot), 0);
  rb_define_module_function(graph, "screenshot", RMF(graphics_save_screenshot), 0);
  rb_define_module_function(graph, "resize_screen", RMF(graphicsResizeScreen), 2);
  rb_define_module_function(graph, "brightness", RMF(graphicsGetBrightness), 0);
  rb_define_module_function(graph, "brightness=", RMF(graphicsSetBrightness), 1);
  rb_define_module_function(graph, "play_movie", RMF(graphicsPlayMovie), 1);
  rb_define_module_function(graph, "block_fullscreen", RMF(graphics_get_block_fullscreen), 0);
  rb_define_module_function(graph, "block_fullscreen=", RMF(graphics_set_block_fullscreen), 1);
  rb_define_module_function(graph, "block_f12", RMF(graphics_get_block_ftwelve), 0);
  rb_define_module_function(graph, "block_f12=", RMF(graphics_set_block_ftwelve), 1);
  rb_define_module_function(graph, "block_f1", RMF(graphics_get_block_fone), 0);
  rb_define_module_function(graph, "block_f1=", RMF(graphics_set_block_fone), 1);
  rb_define_module_function(graph, "fullscreen", RMF(graphicsGetFullscreen), 0);
  rb_define_module_function(graph, "fullscreen=", RMF(graphicsSetFullscreen), 1);
  rb_define_module_function(graph, "show_cursor", RMF(graphics_get_show_cursor), 0);
  rb_define_module_function(graph, "show_cursor=", RMF(graphics_set_show_cursor), 1);
  rb_define_module_function(graph, "delta", RMF(graphics_get_delta), 0);
  rb_define_module_function(graph, "title", RMF(graphics_get_title), 0);
  rb_define_module_function(graph, "title=", RMF(graphics_set_title), 1);
  VALUE sys = rb_define_module("System");
  rb_define_module_function(sys, "delta", RMF(graphics_get_delta), 0);
  rb_define_module_function(sys, "uptime", RMF(graphics_get_delta), 0);
  rb_define_module_function(sys, "set_window_title", RMF(graphics_set_title), 1);
  rb_define_module_function(sys, "window_title", RMF(graphics_get_title), 0);
  rb_define_module_function(sys, "window_title=", RMF(graphics_set_title), 1);
}

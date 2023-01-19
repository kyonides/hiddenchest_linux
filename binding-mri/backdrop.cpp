/*
** backdrop.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2019 Kyonides-Arkanthes
*/

#include "hcextras.h"
#include "bitmap.h"
#include "graphics.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"
#include "disposable-binding.h"

static VALUE backdrop, graphics;

static VALUE backdrop_keep_bitmap(VALUE self)
{
  Bitmap *result = shState->graphics().snapToBitmap();
  VALUE bitmap = wrapObject(result, BitmapType);
  return rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_invert_colors(VALUE self)
{
  Bitmap *result = shState->graphics().snapToBitmap();
  result->invert_colors();
  VALUE bitmap = wrapObject(result, BitmapType);
  return rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_gray_bitmap(VALUE self)
{
  Bitmap *result = shState->graphics().snap_to_gray_bitmap();
  VALUE bitmap = wrapObject(result, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_sepia_bitmap(VALUE self)
{
  Bitmap *result = shState->graphics().snap_to_sepia_bitmap();
  VALUE bitmap = wrapObject(result, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_color_bitmap(VALUE self, VALUE color)
{
  Bitmap *b = 0;
  if (color == hc_sym("red"))
    b = shState->graphics().snap_to_color_bitmap(0);
  else if (color == hc_sym("green"))
    b = shState->graphics().snap_to_color_bitmap(1);
  else if (color == hc_sym("blue"))
    b = shState->graphics().snap_to_color_bitmap(2);
  else if (color == hc_sym("yellow"))
    b = shState->graphics().snap_to_color_bitmap(3);
  else if (color == hc_sym("black"))
    b = shState->graphics().snap_to_color_bitmap(4);
  else if (color == hc_sym("sepia"))
    b = shState->graphics().snap_to_sepia_bitmap();
  else if (color == hc_sym("gray"))
    b = shState->graphics().snap_to_gray_bitmap();
  else
    return Qnil;
  VALUE bitmap = wrapObject(b, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_gradient_bitmap(VALUE self, VALUE c1, VALUE c2, VALUE bln)
{
  Color *clr1 = static_cast<Color*>(Check_TypedStruct(c1, &ColorType));
  Color *clr2 = static_cast<Color*>(Check_TypedStruct(c2, &ColorType));
  int mode;
  if (bln == Qfalse) mode = 0;
  else if (bln == Qtrue) mode = 1;
  else if (bln == Qnil) mode = 2;
  Bitmap *b = shState->graphics().gradient_bitmap(clr1->norm, clr2->norm, mode);
  VALUE bitmap = wrapObject(b, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_pos_gradient_bitmap(VALUE self, VALUE x, VALUE y, VALUE cl1, VALUE cl2)
{
  Vec2i o(RB_FIX2INT(x), RB_FIX2INT(y));
  Color *c1 = static_cast<Color*>(Check_TypedStruct(cl1, &ColorType));
  Color *c2 = static_cast<Color*>(Check_TypedStruct(cl2, &ColorType));
  Bitmap *b = shState->graphics().snap_to_gradient_bitmap(o, c1->norm, c2->norm);
  VALUE bitmap = wrapObject(b, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_map_gradient_bitmap(VALUE self, VALUE op, VALUE x, VALUE y, VALUE cl1, VALUE cl2)
{
  int n = RB_FIX2INT(op);
  Vec2i v(RB_FIX2INT(x), RB_FIX2INT(y));
  Color *c1 = static_cast<Color*>(Check_TypedStruct(cl1, &ColorType));
  Color *c2 = static_cast<Color*>(Check_TypedStruct(cl2, &ColorType));
  Bitmap *b = shState->graphics().snap_to_map_gradient_bitmap(n, v, c1->norm, c2->norm);
  VALUE bitmap = wrapObject(b, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_vignette_bitmap(VALUE self, VALUE color_kind)
{
  Vec4 vec;
  int mode = 0;
  if (color_kind == hc_sym("gray"))
    mode = 1;
  else if (color_kind == hc_sym("sepia"))
    mode = 2;
  else if (color_kind == hc_sym("red"))
    mode = 3;
  else if (color_kind == hc_sym("green"))
    mode = 4;
  else if (color_kind == hc_sym("blue"))
    mode = 5;
  else if (color_kind == hc_sym("yellow"))
    mode = 6;
  else if (color_kind == hc_sym("black"))
    mode = 7;
  Bitmap *b = shState->graphics().snap_to_vignette(mode);
  VALUE bitmap = wrapObject(b, BitmapType);
  return self == graphics ? bitmap : rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_blur_bitmap(VALUE self)
{
  Bitmap *b = shState->graphics().snapToBitmap();
  b->blur();
  VALUE bitmap = wrapObject(b, BitmapType);
  return rb_iv_set(self, "@bitmap", bitmap);
}

static VALUE backdrop_bitmap(VALUE self)
{
  return rb_iv_get(self, "@bitmap");
}

static VALUE backdrop_bitmap_dup(VALUE self)
{
  VALUE img = rb_iv_get(self, "@bitmap");
  return rb_obj_dup(img);
}

static VALUE backdrop_clear_bitmap(VALUE self)
{
  VALUE bitmap = rb_iv_get(self, "@bitmap");
  if (bitmap == Qnil) return Qnil;
  Bitmap *d = getPrivateData<Bitmap>(bitmap);
  if (!d || d->isDisposed()) rb_iv_set(self, "@bitmap", Qnil);
  if (rgssVer == 1) disposableDisposeChildren(bitmap);
  d->dispose();
  return rb_iv_set(self, "@bitmap", Qnil);
}

void init_backdrop()
{
  backdrop = rb_define_module("Backdrop");
  graphics = rb_define_module("Graphics");
  rb_iv_set(backdrop, "@bitmap", Qnil);
  rb_define_module_function(backdrop, "keep_bitmap", RMF(backdrop_keep_bitmap), 0);
  rb_define_module_function(backdrop, "invert_colors", RMF(backdrop_invert_colors), 0);
  rb_define_module_function(backdrop, "gray_bitmap", RMF(backdrop_gray_bitmap), 0);
  rb_define_module_function(backdrop, "sepia_bitmap", RMF(backdrop_sepia_bitmap), 0);
  rb_define_module_function(backdrop, "color_bitmap", RMF(backdrop_color_bitmap), 1);
  rb_define_module_function(backdrop, "gradient_bitmap", RMF(backdrop_gradient_bitmap), 3);
  rb_define_module_function(backdrop, "pos_gradient_bitmap", RMF(backdrop_pos_gradient_bitmap), 4);
  rb_define_module_function(backdrop, "map_gradient_bitmap", RMF(backdrop_map_gradient_bitmap), 5);
  rb_define_module_function(backdrop, "vignette_bitmap", RMF(backdrop_vignette_bitmap), 1);
  rb_define_module_function(backdrop, "blur_bitmap", RMF(backdrop_blur_bitmap), 0);
  rb_define_module_function(backdrop, "bitmap", RMF(backdrop_bitmap), 0);
  rb_define_module_function(backdrop, "bitmap_dup", RMF(backdrop_bitmap_dup), 0);
  rb_define_module_function(backdrop, "clear_bitmap", RMF(backdrop_clear_bitmap), 0);
  rb_define_module_function(graphics, "snap_to_gray_bitmap", RMF(backdrop_gray_bitmap), 0);
  rb_define_module_function(graphics, "snap_to_sepia_bitmap", RMF(backdrop_sepia_bitmap), 0);
  rb_define_module_function(graphics, "snap_to_color_bitmap", RMF(backdrop_color_bitmap), 1);
  rb_define_module_function(graphics, "gradient_bitmap", RMF(backdrop_gradient_bitmap), 3);
  rb_define_module_function(graphics, "pos_gradient_bitmap", RMF(backdrop_pos_gradient_bitmap), 4);
  rb_define_module_function(graphics, "map_gradient_bitmap", RMF(backdrop_map_gradient_bitmap), 5);
  rb_define_module_function(graphics, "vignette_bitmap", RMF(backdrop_vignette_bitmap), 1);
}

/*
** bitmap-binding.cpp
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

#include "hcextras.h"
#include "bitmap.h"
#include "font.h"
#include "exception.h"
#include "sharedstate.h"
#include "disposable-binding.h"
#include "binding-util.h"
#include "binding-types.h"
#include "debugwriter.h"

DEF_TYPE(Bitmap);

static const char *objAsStringPtr(VALUE obj)
{
  VALUE str = rb_obj_as_string(obj);
  return RSTRING_PTR(str);
}

void bitmapInitProps(Bitmap *b, VALUE self)
{ // Wrap properties
  VALUE fontKlass = rb_const_get(rb_cObject, rb_intern("Font"));
  VALUE fontObj = rb_obj_alloc(fontKlass);
  rb_obj_call_init(fontObj, 0, 0);
  Font *font = getPrivateData<Font>(fontObj);
  b->setInitFont(font);
  rb_iv_set(self, "font", fontObj);
}

static VALUE bitmapInitialize(int argc, VALUE* argv, VALUE self)
{
  Bitmap *b = 0;
  if (argc == 0) {
    GUARD_EXC( b = new Bitmap(1); )
  } else if (argc == 1) {
    GUARD_EXC( b = new Bitmap(RSTRING_PTR(argv[0])); )
  } else {
    if (RB_NIL_P(argv[1])) {
      GUARD_EXC( b = new Bitmap(RSTRING_PTR(argv[0]), 0); )
    } else {
      int width = RB_FIX2INT(argv[0]);
      int height = RB_FIX2INT(argv[1]);
      GUARD_EXC( b = new Bitmap(width, height); )
    }
  }
  RTYPEDDATA_DATA(self) = b;
  bitmapInitProps(b, self);
  rb_iv_set(self, "disposed", Qfalse);
  return self;
}

static VALUE bitmapInitializeCopy(int argc, VALUE* argv, VALUE self)
{
  rb_check_argc(argc, 1);
  VALUE origObj = argv[0];
  if (!OBJ_INIT_COPY(self, origObj))
    return self;
  Bitmap *orig = getPrivateData<Bitmap>(origObj);
  Bitmap *b = 0;
  GUARD_EXC( b = new Bitmap(*orig); );
  bitmapInitProps(b, self);
  b->setFont(orig->getFont());
  setPrivateData(self, b);
  rb_iv_set(self, "disposed", Qfalse);
  return self;
}

inline void bitmapDisposeChildren(VALUE disp)
{
  VALUE children = rb_iv_get(disp, "children");
  if ( RB_NIL_P(children) )
    return;
  ID dispFun = rb_intern("_HC_dispose_alias");
  for (long i = 0; i < RARRAY_LEN(children); ++i)
    rb_funcall(rb_ary_entry(children, i), dispFun, 0);
}

static VALUE bitmapDispose(VALUE self)
{
  if (rb_iv_get(self, "disposed") == Qtrue) {
    Debug() << "Already disposed!";
    return Qnil;
  }
  Bitmap *b = getPrivateData<Bitmap>(self);
  if (!b)
    return Qnil;
  if (shState->rgssVersion == 1)
    bitmapDisposeChildren(self);
  b->dispose();
  rb_iv_set(self, "disposed", Qtrue);
  return Qnil;
}

static VALUE bitmap_is_disposed(VALUE self)
{
  return rb_iv_get(self, "disposed");
}

static VALUE bitmapWidth(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  int value = 0;
  GUARD_EXC( value = b->width(); );
  return RB_INT2FIX(value);
}

static VALUE bitmapHeight(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  int value = 0;
  GUARD_EXC( value = b->height(); );
  return RB_INT2FIX(value);
}

static VALUE bitmapRect(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  IntRect rect;
  GUARD_EXC( rect = b->rect(); );
  Rect *r = new Rect(rect);
  return wrapObject(r, RectType);
}

Rect* bitmap_c_rect(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  Rect *r;
  GUARD_EXC( r = new Rect(b->rect()); );
  return r;
}

RB_METHOD(bitmapBlt)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  int x, y;
  VALUE srcObj;
  VALUE srcRectObj;
  int opacity = 255;
  Bitmap *src;
  Rect *srcRect;
  rb_get_args(argc, argv, "iioo|i", &x, &y, &srcObj, &srcRectObj, &opacity RB_ARG_END);
  src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
  srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);
  GUARD_EXC( b->blt(x, y, *src, srcRect->toIntRect(), opacity); );
  return self;
}

RB_METHOD(bitmapStretchBlt)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  VALUE destRectObj;
  VALUE srcObj;
  VALUE srcRectObj;
  int opacity = 255;
  Bitmap *src;
  Rect *destRect, *srcRect;
  rb_get_args(argc, argv, "ooo|i", &destRectObj, &srcObj, &srcRectObj, &opacity RB_ARG_END);
  src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
  destRect = getPrivateDataCheck<Rect>(destRectObj, RectType);
  srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);
  GUARD_EXC( b->stretchBlt(destRect->toIntRect(), *src, srcRect->toIntRect(), opacity); );
  return self;
}

static VALUE bitmap_fill_rect(int argc, VALUE* argv, VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  VALUE rbcolor;
  Color *color;
  if (argc == 2) {
    Rect *rect;
    rbcolor = argv[1];
    VALUE rbrect = argv[0];
    if (rbrect == hc_sym("rect"))
      rect = bitmap_c_rect(self);
    else
      rect = getPrivateDataCheck<Rect>(rbrect, RectType);
    color = getPrivateDataCheck<Color>(rbcolor, ColorType);
    GUARD_EXC( b->fillRect(rect->toIntRect(), color->norm); );
  } else {
    int x, y, width, height;
    rb_get_args(argc, argv, "iiiio", &x, &y, &width, &height, &rbcolor RB_ARG_END);
    color = getPrivateDataCheck<Color>(rbcolor, ColorType);
    GUARD_EXC( b->fillRect(x, y, width, height, color->norm); );
  }
  return self;
}

static VALUE bitmapClear(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  GUARD_EXC( b->clear(); )
  return self;
}

static VALUE bitmap_is_alpha_pixel(VALUE self, VALUE rx, VALUE ry)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  if (!b) return Qnil;
  return b->is_alpha_pixel(RB_FIX2INT(rx), RB_FIX2INT(ry)) ? Qtrue : Qfalse;
}

static VALUE bitmapGetPixel(VALUE self, VALUE rx, VALUE ry)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  int x = RB_FIX2INT(rx), y = RB_FIX2INT(ry);
  Color value;
  GUARD_EXC( value = b->getPixel(x, y); );
  Color *color = new Color(value);
  return wrapObject(color, ColorType);
}

static VALUE bitmapSetPixel(int argc, VALUE* argv, VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  int x, y;
  VALUE colorObj;
  Color *color;
  rb_get_args(argc, argv, "iio", &x, &y, &colorObj RB_ARG_END);
  color = getPrivateDataCheck<Color>(colorObj, ColorType);
  GUARD_EXC( b->setPixel(x, y, *color); );
  return self;
}

static VALUE bitmapHueChange(VALUE self, VALUE hue)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  GUARD_EXC( b->hueChange(NUM2INT(hue)); );
  return self;
}

static VALUE bitmap_gray_out(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  GUARD_EXC( b->gray_out(); );
  return self;
}

static VALUE bitmap_turn_sepia(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  GUARD_EXC( b->turn_sepia(); );
  return self;
}

static VALUE bitmap_invert(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  GUARD_EXC( b->invert_colors(); );
  return self;
}

static VALUE bitmap_draw_text(int argc, VALUE* argv, VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  int align = Bitmap::Left;
  int s_pos = argc < 4 ? 1 : 4;
  VALUE rbstr = rb_funcall(argv[s_pos], rb_intern("to_s"), 0);
  const char *str = RSTRING_PTR(rbstr);
  if (argc == 2 || argc == 3) {
    Rect *rect;
    VALUE rbrect = argv[0];
    if (rbrect == hc_sym("rect"))
      rect = bitmap_c_rect(self);
    else
      rect = getPrivateDataCheck<Rect>(rbrect, RectType);
    if (argc == 3)
      align = RB_FIX2INT(argv[2]);
    GUARD_EXC( b->drawText(rect->toIntRect(), str, align); );
  } else {
    int x, y, width, height;
    x = RB_FIX2INT(argv[0]);
    y = RB_FIX2INT(argv[1]);
    width = RB_FIX2INT(argv[2]);
    height = RB_FIX2INT(argv[3]);
    if (argc == 6)
      align = RB_FIX2INT(argv[5]);
    GUARD_EXC( b->drawText(x, y, width, height, str, align); );
  }
  return self;
}

static VALUE bitmap_text_size(VALUE self, VALUE rbstr)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  rbstr = rb_funcall(rbstr, rb_intern("to_s"), 0);
  const char *str = RSTRING_PTR(rbstr);
  IntRect value;
  GUARD_EXC( value = b->textSize(str); );
  Rect *rect = new Rect(value);
  return wrapObject(rect, RectType);
}

static VALUE bitmap_text_width(VALUE self, VALUE rbstr)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  rbstr = rb_funcall(rbstr, rb_intern("to_s"), 0);
  const char *str = RSTRING_PTR(rbstr);
  int value;
  GUARD_EXC( value = b->textWidth(str); );
  return RB_INT2FIX(value);
}

static VALUE bitmap_text_height(VALUE self, VALUE rbstr)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  rbstr = rb_funcall(rbstr, rb_intern("to_s"), 0);
  const char *str = RSTRING_PTR(rbstr);
  int value;
  GUARD_EXC( value = b->textHeight(str); );
  return RB_INT2FIX(value);
}

static VALUE bitmapGetMaxSize(VALUE self)
{
  return RB_INT2FIX(Bitmap::maxSize());
}

static VALUE bitmapGetFont(VALUE self)
{
  checkDisposed<Bitmap>(self);
  return rb_iv_get(self, "font");
}

static VALUE bitmapSetFont(VALUE self, VALUE sfont)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  Font *font;
  font = getPrivateDataCheck<Font>(sfont, FontType);
  GUARD_EXC( b->setFont(*font); )
  return rb_iv_get(self, "font");
}

RB_METHOD(bitmapGradientFillRect)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  VALUE color1Obj, color2Obj;
  Color *color1, *color2;
  bool vertical = false;
  if (argc == 3 || argc == 4) {
    VALUE rectObj;
    Rect *rect;
    rb_get_args(argc, argv, "ooo|b", &rectObj,
                &color1Obj, &color2Obj, &vertical RB_ARG_END);
    rect = getPrivateDataCheck<Rect>(rectObj, RectType);
    color1 = getPrivateDataCheck<Color>(color1Obj, ColorType);
    color2 = getPrivateDataCheck<Color>(color2Obj, ColorType);
    GUARD_EXC( b->gradientFillRect(rect->toIntRect(), color1->norm, color2->norm, vertical); );
  } else {
    int x, y, width, height;
    rb_get_args(argc, argv, "iiiioo|b", &x, &y, &width, &height,
                &color1Obj, &color2Obj, &vertical RB_ARG_END);
    color1 = getPrivateDataCheck<Color>(color1Obj, ColorType);
    color2 = getPrivateDataCheck<Color>(color2Obj, ColorType);
    GUARD_EXC( b->gradientFillRect(x, y, width, height, color1->norm, color2->norm, vertical); );
  }
  return self;
}

static VALUE bitmapClearRect(int argc, VALUE* argv, VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  if (argc == 1) {
    VALUE rectObj;
    Rect *rect;
    rb_get_args(argc, argv, "o", &rectObj RB_ARG_END);
    rect = getPrivateDataCheck<Rect>(rectObj, RectType);
    GUARD_EXC( b->clearRect(rect->toIntRect()); );
  } else {
    int x, y, width, height;
    rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);
    GUARD_EXC( b->clearRect(x, y, width, height); );
  }
  return self;
}

static VALUE bitmapBlur(VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  b->blur();
  return self;
}

static VALUE bitmapRadialBlur(VALUE self, VALUE angle, VALUE divisions)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  b->radialBlur(NUM2INT(angle), NUM2INT(divisions));
  return Qnil;
}

static VALUE bitmapStormFillRect(VALUE self, VALUE i)
{
  VALUE rcolor1, rcolor2, max = RB_INT2FIX(255), n = RB_INT2FIX(i);
  Color *color1, *color2;
  VALUE RColor = rb_const_get(rb_cObject, rb_intern("Color"));
  rcolor1 = rb_funcall(RColor, rb_intern("new"), 4, max, max, max, max);
  rcolor2 = rb_funcall(RColor, rb_intern("new"), 4, max, max, max, RB_INT2FIX(128));
  color1 = getPrivateDataCheck<Color>(rcolor1, ColorType);
  color2 = getPrivateDataCheck<Color>(rcolor2, ColorType);
  Bitmap *b = getPrivateData<Bitmap>(self);
  try {
    b->fillRect(33-n, n*2, 1, 2, color2->norm);
    b->fillRect(32-n, n*2, 1, 2, color1->norm);
    b->fillRect(31-n, n*2, 1, 2, color2->norm);
  } catch (const Exception &exc) {
    raiseRbExc(exc);
  }
  return self;
}

static VALUE bitmapSnowFillRect(VALUE self)
{
  VALUE rcolor1, rcolor2, max = RB_INT2FIX(255);
  Color *color1, *color2;
  VALUE RColor = rb_const_get(rb_cObject, rb_intern("Color"));
  rcolor1 = rb_funcall(RColor, rb_intern("new"), 4, max, max, max, max);
  rcolor2 = rb_funcall(RColor, rb_intern("new"), 4, max, max, max, RB_INT2FIX(128));
  void *cobj1 = Check_TypedStruct(rcolor1, &ColorType);
  color1 = static_cast<Color*>(cobj1);
  void *cobj2 = Check_TypedStruct(rcolor2, &ColorType);
  color2 = static_cast<Color*>(cobj2);
  Bitmap *b = getPrivateData<Bitmap>(self);
  try {
    b->fillRect(0, 1, 6, 4, color2->norm);
    b->fillRect(1, 0, 4, 6, color2->norm);
    b->fillRect(1, 2, 4, 2, color1->norm);
    b->fillRect(2, 1, 2, 4, color1->norm);
  } catch (const Exception &exc) {
    raiseRbExc(exc);
  }
  return self;
}

static VALUE bitmap_write(int argc, VALUE* argv, VALUE self)
{
  Bitmap *b = getPrivateData<Bitmap>(self);
  if (!b || argc == 0)
    return Qfalse;
  rb_check_type(argv[0], T_STRING);
  const char *ext;
  const char *str = RSTRING_PTR(argv[0]);
  if (argc == 1) {
    VALUE settings = rb_define_module("Setup");
    VALUE fmt = rb_iv_get(settings, "@shot_format");
    ext = RSTRING_PTR(fmt);
  } else if (argc == 2) {
    rb_check_type(argv[1], T_STRING);
    ext = RSTRING_PTR(argv[1]);
  }
  bool result = false;
  try {
    result = b->write(str, ext);
  } catch (const Exception &exc) {
    raiseRbExc(exc);
  }
  return result ? Qtrue : Qfalse;
}

#define RMF(func) ((VALUE (*)(ANYARGS))(func))

void bitmapBindingInit()
{
  VALUE klass = rb_define_class("Bitmap", rb_cObject);
  rb_define_alloc_func(klass, classAllocate<&BitmapType>);
  rb_define_method(klass, "disposed?", RMF(bitmap_is_disposed), 0);
  rb_define_method(klass, "dispose", RMF(bitmapDispose), 0);
  if (rgssVer == 1)
    rb_define_alias(klass, "_HC_dispose_alias", "dispose");
  rb_define_method(klass, "initialize", RMF(bitmapInitialize), -1);
  rb_define_method(klass, "initialize_copy", RMF(bitmapInitializeCopy), -1);
  rb_define_method(klass, "width", RMF(bitmapWidth), 0);
  rb_define_method(klass, "height", RMF(bitmapHeight), 0);
  rb_define_method(klass, "rect", RMF(bitmapRect), 0);
  rb_define_method(klass, "blt", RMF(bitmapBlt), -1);
  rb_define_method(klass, "stretch_blt", RMF(bitmapStretchBlt), -1);
  rb_define_method(klass, "fill_rect", RMF(bitmap_fill_rect), -1);
  rb_define_method(klass, "clear", RMF(bitmapClear), 0);
  rb_define_method(klass, "alpha_pixel?", RMF(bitmap_is_alpha_pixel), 2);
  rb_define_method(klass, "get_pixel", RMF(bitmapGetPixel), 2);
  rb_define_method(klass, "set_pixel", RMF(bitmapSetPixel), -1);
  rb_define_method(klass, "hue_change", RMF(bitmapHueChange), 1);
  rb_define_method(klass, "gray_out", RMF(bitmap_gray_out), 0),
  rb_define_method(klass, "turn_sepia", RMF(bitmap_turn_sepia), 0);
  rb_define_method(klass, "invert!", RMF(bitmap_invert), 0);
  rb_define_method(klass, "draw_text", RMF(bitmap_draw_text), -1);
  rb_define_method(klass, "text_size", RMF(bitmap_text_size), 1);
  rb_define_method(klass, "text_width", RMF(bitmap_text_width), 1);
  rb_define_method(klass, "text_height", RMF(bitmap_text_height), 1);
  rb_define_method(klass, "gradient_fill_rect", RMF(bitmapGradientFillRect), -1);
  rb_define_method(klass, "storm_fill_rect", RMF(bitmapStormFillRect), 1);
  rb_define_method(klass, "snow_fill_rect", RMF(bitmapSnowFillRect), 0);
  rb_define_method(klass, "clear_rect", RMF(bitmapClearRect), -1);
  rb_define_method(klass, "blur", RMF(bitmapBlur), 0);
  rb_define_method(klass, "radial_blur", RMF(bitmapRadialBlur), 2);
  rb_define_method(klass, "font", RMF(bitmapGetFont), 0);
  rb_define_method(klass, "font=", RMF(bitmapSetFont), 1);
  rb_define_method(klass, "write", RMF(bitmap_write), -1);
  rb_define_method(klass, "to_file", RMF(bitmap_write), -1);
  rb_define_singleton_method(klass, "max_size", RMF(bitmapGetMaxSize), 0);
}

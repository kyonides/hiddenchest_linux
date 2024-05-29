/*
** etc-binding.cpp
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
#include "etc.h"
#include "binding-util.h"
#include "serializable-binding.h"
#include "sharedstate.h"

DEF_TYPE(Color);
DEF_TYPE(Tone);
DEF_TYPE(Rect);

#define ATTR_RW(Klass, Attr, arg_type, arg_t_s, value_fun) \
RB_METHOD(Klass##Get##Attr) \
{ \
  RB_UNUSED_PARAM \
  Klass *p = getPrivateData<Klass>(self); \
  return value_fun(p->get##Attr()); \
} \
RB_METHOD(Klass##Set##Attr) \
{ \
  Klass *p = getPrivateData<Klass>(self); \
  arg_type arg; \
  rb_get_args(argc, argv, arg_t_s, &arg RB_ARG_END); \
  p->set##Attr(arg); \
  return *argv; \
}

#define ATTR_DOUBLE_RW(Klass, Attr) ATTR_RW(Klass, Attr, double, "f", rb_float_new)
#define ATTR_INT_RW(Klass, Attr)   ATTR_RW(Klass, Attr, int, "i", rb_fix_new)

ATTR_DOUBLE_RW(Color, Red)
ATTR_DOUBLE_RW(Color, Green)
ATTR_DOUBLE_RW(Color, Blue)
ATTR_DOUBLE_RW(Color, Alpha)
ATTR_DOUBLE_RW(Tone, Red)
ATTR_DOUBLE_RW(Tone, Green)
ATTR_DOUBLE_RW(Tone, Blue)
ATTR_DOUBLE_RW(Tone, Gray)
ATTR_INT_RW(Rect, X)
ATTR_INT_RW(Rect, Y)
ATTR_INT_RW(Rect, Width)
ATTR_INT_RW(Rect, Height)

#define EQUAL_FUN(Klass) \
RB_METHOD(Klass##Equal) \
{ \
  Klass *p = getPrivateData<Klass>(self); \
  VALUE otherObj; \
  Klass *other; \
  rb_get_args(argc, argv, "o", &otherObj RB_ARG_END); \
  if (rgssVer >= 3) \
      if (!rb_typeddata_is_kind_of(otherObj, &Klass##Type)) \
          return Qfalse; \
  other = getPrivateDataCheck<Klass>(otherObj, Klass##Type); \
  return rb_bool_new(*p == *other); \
}

EQUAL_FUN(Color)
EQUAL_FUN(Tone)

#define INIT_FUN(Klass, param_type, param_t_s, last_param_def) \
RB_METHOD(Klass##Initialize) \
{ \
  Klass *k; \
  if (argc == 0) \
  { \
      k = new Klass(); \
  } \
  else \
  { \
      param_type p1, p2, p3, p4 = last_param_def; \
      rb_get_args(argc, argv, param_t_s, &p1, &p2, &p3, &p4 RB_ARG_END); \
      k = new Klass(p1, p2, p3, p4); \
  } \
  setPrivateData(self, k); \
  return self; \
}

INIT_FUN(Color, double, "fff|f", 255)
INIT_FUN(Tone, double, "fff|f", 0)
//INIT_FUN(Rect, int, "iiii", 0)

static VALUE rect_initialize(int argc, VALUE* argv, VALUE self)
{
  Rect *r;
  if (argc == 0) {
    r = new Rect();
  } else if (argc == 1) {
    VALUE ary = argv[0];
    if ( ARRAY_TYPE_P(ary) ) {
      int x, y, w, h;
      x = RB_FIX2INT( rb_ary_entry(ary, 0) );
      y = RB_FIX2INT( rb_ary_entry(ary, 1) );
      w = RB_FIX2INT( rb_ary_entry(ary, 2) );
      h = RB_FIX2INT( rb_ary_entry(ary, 3) );
      r = new Rect(x, y, w, h);
    } else {
      r = new Rect();
    }
  } else {
    int p1, p2, p3, p4 = 0;
    rb_get_args(argc, argv, "iiii", &p1, &p2, &p3, &p4 RB_ARG_END);
    r = new Rect(p1, p2, p3, p4);
  }
  RTYPEDDATA_DATA(self) = r;
  return self;
}

VALUE rect_from_ary(VALUE ary)
{
  VALUE c_ary[1] = { ary };
  VALUE rect = rb_define_class("Rect", rb_cObject);
  rect = rb_obj_alloc(rect);
  return rect_initialize(1, c_ary, rect);
}

static VALUE ary_to_rect(VALUE self)
{
  return rect_from_ary(self);
}

static VALUE rect_equal(VALUE self, VALUE obj)
{
  return (rb_equal(self, obj) == Qtrue);
}

#define SET_FUN(Klass, param_type, param_t_s, last_param_def) \
	RB_METHOD(Klass##Set) \
	{ \
		Klass *k = getPrivateData<Klass>(self); \
		if (argc == 1) \
		{ \
			VALUE otherObj = argv[0]; \
			Klass *other = getPrivateDataCheck<Klass>(otherObj, Klass##Type); \
			*k = *other; \
		} \
		else \
		{ \
			param_type p1, p2, p3, p4 = last_param_def; \
			rb_get_args(argc, argv, param_t_s, &p1, &p2, &p3, &p4 RB_ARG_END); \
			k->set(p1, p2, p3, p4); \
		} \
		return self; \
	}

SET_FUN(Color, double, "fff|f", 255)
SET_FUN(Tone, double, "fff|f", 0)
SET_FUN(Rect, int, "iiii", 0)

RB_METHOD(rectEmpty)
{
  RB_UNUSED_PARAM;
  Rect *r = getPrivateData<Rect>(self);
  r->empty();
  return self;
}

RB_METHOD(ColorStringify)
{
  RB_UNUSED_PARAM;
  Color *c = getPrivateData<Color>(self);
  return rb_sprintf("(%f, %f, %f, %f)",
                    c->red, c->green, c->blue, c->alpha);
}

RB_METHOD(ToneStringify)
{
  RB_UNUSED_PARAM;
  Tone *t = getPrivateData<Tone>(self);
  return rb_sprintf("(%f, %f, %f, %f)",
                    t->red, t->green, t->blue, t->gray);
}

RB_METHOD(RectStringify)
{
  RB_UNUSED_PARAM;
  Rect *r = getPrivateData<Rect>(self);
  return rb_sprintf("(%d, %d, %d, %d)",
                    r->x, r->y, r->width, r->height);
}

MARSH_LOAD_FUN(Color)
MARSH_LOAD_FUN(Tone)
MARSH_LOAD_FUN(Rect)
INITCOPY_FUN(Tone)
INITCOPY_FUN(Color)
INITCOPY_FUN(Rect)

void etcBindingInit()
{
  VALUE color = rb_define_class("Color", rb_cObject);
  rb_define_alloc_func(color, classAllocate<&ColorType>);
  rb_define_singleton_method(color, "_load", RMF(ColorLoad), -1);
  serializableBindingInit<Color>(color);
  rb_define_method(color, "initialize", RMF(ColorInitialize), -1);
  rb_define_method(color, "initialize_copy", RMF(ColorInitializeCopy), -1);
  rb_define_method(color, "set", RMF(ColorSet), -1);
  rb_define_method(color, "==", RMF(ColorEqual), -1);
  rb_define_method(color, "===", RMF(ColorEqual), -1);
  rb_define_method(color, "eql?", RMF(ColorEqual), -1);
  rb_define_method(color, "to_s", RMF(ColorStringify), -1);
  rb_define_method(color, "inspect", RMF(ColorStringify), -1);
  rb_define_method(color, "red", RMF(ColorGetRed), -1);
  rb_define_method(color, "green", RMF(ColorGetGreen), -1);
  rb_define_method(color, "blue", RMF(ColorGetBlue), -1);
  rb_define_method(color, "alpha", RMF(ColorGetAlpha), -1);
  rb_define_method(color, "red=", RMF(ColorSetRed), -1);
  rb_define_method(color, "green=", RMF(ColorSetGreen), -1);
  rb_define_method(color, "blue=", RMF(ColorSetBlue), -1);
  rb_define_method(color, "alpha=", RMF(ColorSetAlpha), -1);
  VALUE tone = rb_define_class("Tone", rb_cObject);
  rb_define_alloc_func(tone, classAllocate<&ToneType>);
  rb_define_singleton_method(tone, "_load", RMF(ToneLoad), -1);
  serializableBindingInit<Tone>(tone);
  rb_define_method(tone, "initialize", RMF(ToneInitialize), -1);
  rb_define_method(tone, "initialize_copy", RMF(ToneInitializeCopy), -1);
  rb_define_method(tone, "set", RMF(ToneSet), -1);
  rb_define_method(tone, "==", RMF(ToneEqual), -1);
  rb_define_method(tone, "===", RMF(ToneEqual), -1);
  rb_define_method(tone, "eql?", RMF(ToneEqual), -1);
  rb_define_method(tone, "to_s", RMF(ToneStringify), -1);
  rb_define_method(tone, "inspect", RMF(ToneStringify), -1);
  rb_define_method(tone, "red", RMF(ToneGetRed), -1);
  rb_define_method(tone, "green", RMF(ToneGetGreen), -1);
  rb_define_method(tone, "blue", RMF(ToneGetBlue), -1);
  rb_define_method(tone, "gray", RMF(ToneGetGray), -1);
  rb_define_method(tone, "red=", RMF(ToneSetRed), -1);
  rb_define_method(tone, "green=", RMF(ToneSetGreen), -1);
  rb_define_method(tone, "blue=", RMF(ToneSetBlue), -1);
  rb_define_method(tone, "gray=", RMF(ToneSetGray), -1);
  VALUE rect = rb_define_class("Rect", rb_cObject);
  rb_define_alloc_func(rect, classAllocate<&RectType>);
  rb_define_singleton_method(rect, "_load", RMF(RectLoad), -1);
  serializableBindingInit<Rect>(rect);
  rb_define_method(rect, "initialize", RMF(rect_initialize), -1);
  rb_define_method(rect, "initialize_copy", RMF(RectInitializeCopy), -1);
  rb_define_method(rect, "set", RMF(RectSet), -1);
  rb_define_method(rect, "==", RMF(rect_equal), -1);
  rb_define_method(rect, "===", RMF(rect_equal), -1);
  rb_define_method(rect, "eql?", RMF(rect_equal), -1);
  rb_define_method(rect, "to_s", RMF(RectStringify), -1);
  rb_define_method(rect, "inspect", RMF(RectStringify), -1);
  rb_define_method(rect, "x", RMF(RectGetX), -1);
  rb_define_method(rect, "y", RMF(RectGetY), -1);
  rb_define_method(rect, "width", RMF(RectGetWidth), -1);
  rb_define_method(rect, "height", RMF(RectGetHeight), -1);
  rb_define_method(rect, "x=", RMF(RectSetX), -1);
  rb_define_method(rect, "y=", RMF(RectSetY), -1);
  rb_define_method(rect, "width=", RMF(RectSetWidth), -1);
  rb_define_method(rect, "height=", RMF(RectSetHeight), -1);
  rb_define_method(rect, "empty", RMF(rectEmpty), -1);
  VALUE ary = rb_define_class("Array", rb_cObject);
  rb_define_method(ary, "to_rect", RMF(ary_to_rect), 0);
}

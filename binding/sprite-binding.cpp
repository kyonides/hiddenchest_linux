/*
** sprite-binding.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** 2019 Extended by Kyonides Arkanthes <kyonides@gmail.com>
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

#include "hcextras.h"
#include "sprite.h"
#include "sharedstate.h"
#include "disposable-binding.h"
#include "flashable-binding.h"
#include "sceneelement-binding.h"
#include "viewportelement-binding.h"
#include "binding-util.h"
#include "binding-types.h"
#include "debugwriter.h"

extern VALUE rect_from_ary(VALUE ary);

rb_data_type_t SpriteType = { "Sprite",
  { 0, freeInstance<Sprite>, 0, 0, { 0 } }, 0, 0, 0 };

static VALUE spriteInitialize(int argc, VALUE* argv, VALUE self)
{
  Sprite *s = viewportElementInitialize<Sprite>(argc, argv, self);
  setPrivateData(self, s);
  s->initDynAttribs();
  wrapProperty(self, &s->getSrcRect(), "src_rect", RectType);
  wrapProperty(self, &s->getColor(), "color", ColorType);
  wrapProperty(self, &s->getTone(), "tone", ToneType);
  rb_iv_set(self, "opacity", RB_INT2FIX(255));
  rb_iv_set(self, "blend_type", RB_INT2FIX(0));
  rb_iv_set(self, "@area", rb_ary_new());
  return self;
}

static VALUE SpriteGetBitmap(VALUE self)
{
  return rb_iv_get(self, "bitmap");
}

static VALUE SpriteSetBitmap(VALUE self, VALUE bit)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  Bitmap *b;
  if ( bit == Qnil ) {
    b = 0;
  } else {
    b = getPrivateDataCheck<Bitmap>(bit, BitmapType);
  }
  GUARD_EXC( s->setBitmap(b); )
  return rb_iv_set(self, "bitmap", bit);
}

static VALUE SpriteGetSrcRect(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "src_rect");
}

static VALUE SpriteSetSrcRect(VALUE self, VALUE rect)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  Rect *r;
  if ( RB_NIL_P(rect) )
    r = 0;
  else
    r = getPrivateDataCheck<Rect>(rect, RectType);
  GUARD_EXC( s->setSrcRect(*r); )
  return rb_iv_set(self, "src_rect", rect);
}

static VALUE sprite_gray_out(VALUE self, VALUE boolean)
{
  checkDisposed<Sprite>(self);
  VALUE bit = rb_iv_get(self, "bitmap");
  if (bit == Qnil)
    return Qnil;
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (boolean == Qtrue) {
    rb_iv_set(self, "before_gray", rb_obj_dup(bit));
    GUARD_EXC( s->gray_out(); )
  } else {
    VALUE gray = rb_iv_get(self, "before_gray");
    if (gray != Qnil) {
      Bitmap* b = getPrivateDataCheck<Bitmap>(gray, BitmapType);
      GUARD_EXC( s->setBitmap(b); )
      rb_iv_set(self, "bitmap", gray);
    }
  }
  return rb_iv_set(self, "@grayed_out", boolean);
}

static VALUE sprite_turn_sepia(VALUE self, VALUE boolean)
{
  checkDisposed<Sprite>(self);
  VALUE bit = rb_iv_get(self, "bitmap");
  if (RB_NIL_P(bit))
    return Qnil;
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (boolean == Qtrue) {
    rb_iv_set(self, "non_sepia", rb_obj_dup(bit));
    GUARD_EXC( s->turn_sepia(); )
  } else {
    VALUE non_sepia = rb_iv_get(self, "non_sepia");
    if (non_sepia != Qnil) {
      Bitmap* bns = getPrivateDataCheck<Bitmap>(non_sepia, BitmapType);
      GUARD_EXC( s->setBitmap(bns); )
      rb_iv_set(self, "bitmap", non_sepia);
      rb_iv_set(self, "non_sepia", Qnil);
    }
  }
  return rb_iv_set(self, "@sepia", boolean);
}

static VALUE sprite_invert_colors(VALUE self, VALUE boolean)
{
  checkDisposed<Sprite>(self);
  VALUE bit = rb_iv_get(self, "bitmap");
  if (RB_NIL_P(bit))
    return Qnil;
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (boolean == Qtrue) {
    rb_iv_set(self, "non_inverted", rb_obj_dup(bit));
    GUARD_EXC( s->invert_colors(); )
  } else {
    VALUE c = rb_iv_get(self, "non_inverted");
    if (c != Qnil) {
      Bitmap* b = getPrivateDataCheck<Bitmap>(c, BitmapType);
      GUARD_EXC( s->setBitmap(b); )
      rb_iv_set(self, "bitmap", c);
      rb_iv_set(self, "non_inverted", Qnil);
    }
  }
  return rb_iv_set(self, "@invert_colors", boolean);
}

static VALUE sprite_is_grayed_out(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "@grayed_out");
}

static VALUE sprite_is_sepia(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "@sepia");
}

static VALUE sprite_are_colors_inverted(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "@invert_colors");
}

static VALUE SpriteGetColor(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "color");
}

static VALUE SpriteSetColor(VALUE self, VALUE color)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  Color *c;
  if ( RB_NIL_P(color) )
    c = 0;
  else
    c = getPrivateDataCheck<Color>(color, ColorType);
  GUARD_EXC( s->setColor(*c); )
  return rb_iv_set(self, "color", color);
}

static VALUE SpriteGetTone(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "tone");
}

static VALUE SpriteSetTone(VALUE self, VALUE tone)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  Tone *t;
  if ( RB_NIL_P(tone) )
    t = 0;
  else
    t = getPrivateDataCheck<Tone>(tone, ToneType);
  GUARD_EXC( s->setTone(*t); )
  return tone;
}

static VALUE sprite_area(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return !s ? Qnil : rb_iv_get(self, "@area");
}

static VALUE SpriteGetX(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return RB_INT2FIX( s->getX() );
}

static VALUE SpriteSetX(VALUE self, VALUE x)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  x = rb_funcall(x, rb_intern("to_i"), 0);
  GUARD_EXC( s->setX( RB_FIX2INT(x) ); )
  return RB_INT2FIX( s->getX() );
}

static VALUE SpriteGetY(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return RB_INT2FIX( s->getY() );
}

static VALUE SpriteSetY(VALUE self, VALUE y)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  y = rb_funcall(y, rb_intern("to_i"), 0);
  GUARD_EXC( s->setY( RB_FIX2INT(y) ); )
  return RB_INT2FIX( s->getY() );
}

static VALUE SpriteGetZ(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return RB_INT2FIX( s->getZ() );
}

static VALUE SpriteSetZ(VALUE self, VALUE z)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  z = rb_funcall(z, rb_intern("to_i"), 0);
  GUARD_EXC( s->setZ( RB_FIX2INT(z) ); )
  return RB_INT2FIX( s->getZ() );
}

static VALUE sprite_set_xy(VALUE self, VALUE x, VALUE y)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  x = rb_funcall(x, rb_intern("to_i"), 0);
  y = rb_funcall(y, rb_intern("to_i"), 0);
  GUARD_EXC( s->set_xy( RB_FIX2INT(x), RB_FIX2INT(y) ); )
  return rb_ary_new3(2, x, y);
}

static VALUE sprite_set_xyz(VALUE self, VALUE x, VALUE y, VALUE z)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  x = rb_funcall(x, rb_intern("to_i"), 0);
  y = rb_funcall(y, rb_intern("to_i"), 0);
  z = rb_funcall(z, rb_intern("to_i"), 0);
  GUARD_EXC( s->set_xy( RB_FIX2INT(x), RB_FIX2INT(y) ); )
  GUARD_EXC( s->setZ( RB_FIX2INT(z) ); )
  return rb_ary_new3(3, x, y, z);
}

static VALUE SpriteGetOX(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return RB_INT2FIX( s->getOX() );
}

static VALUE SpriteSetOX(VALUE self, VALUE ox)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  GUARD_EXC( s->setOX( RB_FIX2INT(ox) ); )
  return RB_INT2FIX( s->getOX() );
}

static VALUE SpriteGetOY(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return RB_INT2FIX( s->getOY() );
}

static VALUE SpriteSetOY(VALUE self, VALUE oy)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  GUARD_EXC( s->setOY( RB_FIX2INT(oy) ); )
  return RB_INT2FIX( s->getOY() );
}

static VALUE SpriteGetBushDepth(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return RB_INT2FIX( s->getBushDepth() );
}

static VALUE SpriteSetBushDepth(VALUE self, VALUE depth)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  GUARD_EXC( s->setBushDepth( RB_FIX2INT(depth) ); )
  return RB_INT2FIX( s->getBushDepth() );
}

static VALUE SpriteGetBushOpacity(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "bush_opacity");
}

static VALUE SpriteSetBushOpacity(VALUE self, VALUE opacity)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  GUARD_EXC( s->setBushOpacity( NUM2DBL(opacity) ); )
  VALUE op = DBL2NUM(s->getBushOpacity());
  op = rb_funcall(op, rb_intern("to_i"), 0);
  return rb_iv_set(self, "bush_opacity", op);
}

static VALUE SpriteGetOpacity(VALUE self)
{ 
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "opacity");
}

static VALUE SpriteSetOpacity(VALUE self, VALUE opacity)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setOpacity( NUM2DBL(opacity) ); )
  VALUE op = DBL2NUM(s->getOpacity());
  op = rb_funcall(op, rb_intern("to_i"), 0);
  return rb_iv_set(self, "opacity", op);
}

static VALUE SpriteGetBlendType(VALUE self)
{
  checkDisposed<Sprite>(self);
  return rb_iv_get(self, "blend_type");
}

static VALUE SpriteSetBlendType(VALUE self, VALUE type)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setBlendType( RB_FIX2INT(type) ); )
  VALUE blend_kind = RB_INT2FIX( s->getBlendType() );
  return rb_iv_set(self, "blend_type", blend_kind);
}

static VALUE SpriteGetWaveRotate(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->getWaveRotate() ? Qtrue : Qfalse;
}

static VALUE SpriteSetWaveRotate(VALUE self, VALUE bln)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  s->setWaveRotate(bln == Qtrue);
  return s->getWaveRotate() ? Qtrue : Qfalse;
}

static VALUE SpriteGetWaveAmp(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return RB_INT2FIX( s->getWaveAmp() );
}

static VALUE SpriteSetWaveAmp(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setWaveAmp( RB_FIX2INT(number) ); )
  return RB_INT2FIX( s->getWaveAmp() );
}

static VALUE SpriteGetWaveLength(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return RB_INT2FIX( s->getWaveLength() );
}

static VALUE SpriteSetWaveLength(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setWaveLength( RB_FIX2INT(number) ); )
  return RB_INT2FIX( s->getWaveLength() );
}

static VALUE SpriteGetWaveSpeed(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return RB_INT2FIX( s->getWaveSpeed() );
}

static VALUE SpriteSetWaveSpeed(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setWaveSpeed( RB_FIX2INT(number) ); )
  return RB_INT2FIX( s->getWaveSpeed() );
}

static VALUE SpriteGetWavePhase(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return rb_float_new( s->getWavePhase() );
}

static VALUE SpriteSetWavePhase(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setWavePhase( NUM2DBL(number) ); )
  return rb_float_new( s->getWavePhase() );
}

static VALUE SpriteGetZoomX(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return rb_float_new( s->getZoomX() );
}

static VALUE SpriteSetZoomX(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setZoomX( NUM2DBL(number) ); )
  return rb_float_new( s->getZoomX() );
}

static VALUE SpriteGetZoomY(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  return rb_float_new( s->getZoomY() );
}

static VALUE SpriteSetZoomY(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( s->setZoomY( NUM2DBL(number) ); )
  return rb_float_new( s->getZoomY() );
}

static VALUE SpriteGetAngle(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return RB_INT2FIX(0);
  return rb_float_new( s->getAngle() );
}

static VALUE SpriteSetAngle(VALUE self, VALUE number)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return RB_INT2FIX(0);
  GUARD_EXC( s->setAngle( NUM2DBL(number) ); )
  return rb_float_new( s->getAngle() );
}

static VALUE SpriteGetMirror(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  return s->getMirror() ? Qtrue : Qfalse;
}

static VALUE SpriteSetMirror(VALUE self, VALUE bln)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  s->setMirror(bln == Qtrue);
  return bln;
}

static VALUE SpriteGetMirrorY(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  return s->getMirrorY() ? Qtrue : Qfalse;
}

static VALUE SpriteSetMirrorY(VALUE self, VALUE bln)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  s->setMirrorY(bln == Qtrue);
  return bln;
}

static VALUE SpriteWidth(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return RB_INT2FIX(0);
  return RB_INT2FIX( s->getWidth() );
}

static VALUE SpriteHeight(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return RB_INT2FIX(0);
  return RB_INT2FIX( s->getHeight() );
}

static VALUE SpriteGetReduceSpeed(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return RB_INT2FIX(0);
  return RB_INT2FIX( s->getReduceSpeed() );
}

static VALUE SpriteSetReduceSpeed(VALUE self, VALUE rspeed)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return RB_INT2FIX(0);
  s->setReduceSpeed(RB_FIX2INT(rspeed));
  return rspeed;
}

static VALUE SpriteIncreaseWidth(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  s->increaseWidth();
  return Qtrue;
}

static VALUE SpriteIncreaseHeight(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  s->increaseHeight();
  return Qtrue;
}

static VALUE SpriteIncreaseWidthHeight(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qfalse;
  s->increaseWidthHeight();
  return Qtrue;
}

static VALUE SpriteReduceWidth(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  s->reduceWidth();
  return Qtrue;
}

static VALUE SpriteReduceHeight(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  s->reduceHeight();
  return Qtrue;
}

static VALUE SpriteReduceWidthHeight(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  s->reduceWidthHeight();
  return Qtrue;
}

static VALUE SpriteIsWidthIncreased(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->isWidthIncreased() ? Qtrue : Qfalse;
}

static VALUE SpriteIsHeightIncreased(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->isHeightIncreased() ? Qtrue : Qfalse;
}

static VALUE SpriteIsWidthReduced(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->isWidthReduced() ? Qtrue : Qfalse;
}

static VALUE SpriteIsHeightReduced(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->isHeightReduced() ? Qtrue : Qfalse;
}

static VALUE sprite_is_mouse_inside(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->mouse_is_inside() ? Qtrue : Qfalse;
}

static VALUE sprite_is_mouse_above_color(VALUE self)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  return s->isMouseAboveColorFound() ? Qtrue : Qfalse;
}

static VALUE check_click_area(VALUE self, VALUE pos, bool state)
{
  Sprite *s = static_cast<Sprite*>(RTYPEDDATA_DATA(self));
  if (!s)
    return Qnil;
  VALUE area = rb_iv_get(self, "@area");
  if (area == Qnil)
    return Qfalse;
  int index = RB_FIX2INT(pos);
  area = rb_ary_entry(area, index);
  if (RB_NIL_P(area))
    return Qfalse;
  else if (ARRAY_TYPE_P(area))
    area = rect_from_ary(area);
  Rect *r = static_cast<Rect*>(RTYPEDDATA_DATA(area));
  return s->mouse_is_inside_area(r, state) ? Qtrue : Qfalse;
}

static VALUE sprite_is_click_area(VALUE self, VALUE pos)
{
  return check_click_area(self, pos, false);
}

static VALUE sprite_is_press_click_area(VALUE self, VALUE pos)
{
  return check_click_area(self, pos, true);
}

template<rb_data_type_t *SpriteType>
static VALUE SpriteAllocate(VALUE klass)
{
  return rb_data_typed_object_wrap(klass, 0, SpriteType);
}

void SpriteBindingInit()
{
  VALUE RSprite = rb_define_class("Sprite", rb_cObject);
  rb_define_alloc_func(RSprite, SpriteAllocate<&SpriteType>);
  disposableBindingInit<Sprite>(RSprite);
  flashableBindingInit<Sprite>(RSprite);
  viewportElementBindingInit<Sprite>(RSprite);
  rb_define_method(RSprite, "initialize", RMF(spriteInitialize), -1);
  rb_define_method(RSprite, "bitmap", RMF(SpriteGetBitmap), 0);
  rb_define_method(RSprite, "bitmap=", RMF(SpriteSetBitmap), 1);
  rb_define_method(RSprite, "src_rect", RMF(SpriteGetSrcRect), 0);
  rb_define_method(RSprite, "src_rect=", RMF(SpriteSetSrcRect), 1);
  rb_define_method(RSprite, "color", RMF(SpriteGetColor), 0);
  rb_define_method(RSprite, "color=", RMF(SpriteSetColor), 1);
  rb_define_method(RSprite, "tone", RMF(SpriteGetTone), 0);
  rb_define_method(RSprite, "tone=", RMF(SpriteSetTone), 1);
  rb_define_method(RSprite, "area", RMF(sprite_area), 0);
  rb_define_method(RSprite, "x", RMF(SpriteGetX), 0);
  rb_define_method(RSprite, "x=", RMF(SpriteSetX), 1);
  rb_define_method(RSprite, "y", RMF(SpriteGetY), 0);
  rb_define_method(RSprite, "y=", RMF(SpriteSetY), 1);
  rb_define_method(RSprite, "z", RMF(SpriteGetZ), 0);
  rb_define_method(RSprite, "z=", RMF(SpriteSetZ), 1);
  rb_define_method(RSprite, "set_xy", RMF(sprite_set_xy), 2);
  rb_define_method(RSprite, "set_xyz", RMF(sprite_set_xyz), 3);
  rb_define_method(RSprite, "ox", RMF(SpriteGetOX), 0);
  rb_define_method(RSprite, "ox=", RMF(SpriteSetOX), 1);
  rb_define_method(RSprite, "oy", RMF(SpriteGetOY), 0);
  rb_define_method(RSprite, "oy=", RMF(SpriteSetOY), 1);
  rb_define_method(RSprite, "zoom_x", RMF(SpriteGetZoomX), 0);
  rb_define_method(RSprite, "zoom_x=", RMF(SpriteSetZoomX), 1);
  rb_define_method(RSprite, "zoom_y", RMF(SpriteGetZoomY), 0);
  rb_define_method(RSprite, "zoom_y=", RMF(SpriteSetZoomY), 1);
  rb_define_method(RSprite, "angle", RMF(SpriteGetAngle), 0);
  rb_define_method(RSprite, "angle=", RMF(SpriteSetAngle), 1);
  rb_define_method(RSprite, "mirror", RMF(SpriteGetMirror), 0);
  rb_define_method(RSprite, "mirror=", RMF(SpriteSetMirror), 1);
  rb_define_method(RSprite, "mirror_y", RMF(SpriteGetMirrorY), 0);
  rb_define_method(RSprite, "mirror_y=", RMF(SpriteSetMirrorY), 1);
  rb_define_method(RSprite, "flip", RMF(SpriteGetMirror), 0);
  rb_define_method(RSprite, "flip=", RMF(SpriteSetMirror), 1);
  rb_define_method(RSprite, "flip_y", RMF(SpriteGetMirrorY), 0);
  rb_define_method(RSprite, "flip_y=", RMF(SpriteSetMirrorY), 1);
  rb_define_method(RSprite, "bush_depth", RMF(SpriteGetBushDepth), 0);
  rb_define_method(RSprite, "bush_depth=", RMF(SpriteSetBushDepth), 1);
  rb_define_method(RSprite, "opacity", RMF(SpriteGetOpacity), 0);
  rb_define_method(RSprite, "opacity=", RMF(SpriteSetOpacity), 1);
  rb_define_method(RSprite, "blend_type", RMF(SpriteGetBlendType), 0);
  rb_define_method(RSprite, "blend_type=", RMF(SpriteSetBlendType), 1);
  rb_define_method(RSprite, "width", RMF(SpriteWidth), 0);
  rb_define_method(RSprite, "height", RMF(SpriteHeight), 0);
  rb_define_method(RSprite, "reduce_speed", RMF(SpriteGetReduceSpeed), 0);
  rb_define_method(RSprite, "reduce_speed=", RMF(SpriteSetReduceSpeed), 1);
  rb_define_method(RSprite, "increase_width!", RMF(SpriteIncreaseWidth), 0);
  rb_define_method(RSprite, "increase_height!", RMF(SpriteIncreaseHeight), 0);
  rb_define_method(RSprite, "increase_width_height!", RMF(SpriteIncreaseWidthHeight), 0);
  rb_define_method(RSprite, "increased_width?", RMF(SpriteIsWidthIncreased), 0);
  rb_define_method(RSprite, "increased_height?", RMF(SpriteIsHeightIncreased), 0);
  rb_define_method(RSprite, "reduce_width!", RMF(SpriteReduceWidth), 0);
  rb_define_method(RSprite, "reduce_height!", RMF(SpriteReduceHeight), 0);
  rb_define_method(RSprite, "reduce_width_height!", RMF(SpriteReduceWidthHeight), 0);
  rb_define_method(RSprite, "reduced_width?", RMF(SpriteIsWidthReduced), 0);
  rb_define_method(RSprite, "reduced_height?", RMF(SpriteIsHeightReduced), 0);
  rb_define_method(RSprite, "mouse_inside?", RMF(sprite_is_mouse_inside), 0);
  rb_define_method(RSprite, "mouse_above?", RMF(sprite_is_mouse_inside), 0);
  rb_define_method(RSprite, "mouse_above_color?", RMF(sprite_is_mouse_above_color), 0);
  rb_define_method(RSprite, "click_area?", RMF(sprite_is_click_area), 1);
  rb_define_method(RSprite, "press_click_area?", RMF(sprite_is_press_click_area), 1);
  rb_define_method(RSprite, "gray_out=", RMF(sprite_gray_out), 1);
  rb_define_method(RSprite, "turn_sepia=", RMF(sprite_turn_sepia), 1);
  rb_define_method(RSprite, "invert_colors=", RMF(sprite_invert_colors), 1);
  rb_define_method(RSprite, "grayed_out?", RMF(sprite_is_grayed_out), 0);
  rb_define_method(RSprite, "sepia?", RMF(sprite_is_sepia), 0);
  rb_define_method(RSprite, "colors_inverted?", RMF(sprite_are_colors_inverted), 0);
  if (rgssVer >= 2) {
    rb_define_method(RSprite, "bush_opacity", RMF(SpriteGetBushOpacity), 0);
    rb_define_method(RSprite, "bush_opacity=", RMF(SpriteSetBushOpacity), 1);
  }
  rb_define_method(RSprite, "wave_rotate", RMF(SpriteGetWaveRotate), 0);
  rb_define_method(RSprite, "wave_rotate=", RMF(SpriteSetWaveRotate), 1);
  rb_define_method(RSprite, "wave_amp", RMF(SpriteGetWaveAmp), 0);
  rb_define_method(RSprite, "wave_amp=", RMF(SpriteSetWaveAmp), 1);
  rb_define_method(RSprite, "wave_length", RMF(SpriteGetWaveLength), 0);
  rb_define_method(RSprite, "wave_length=", RMF(SpriteSetWaveLength), 1);
  rb_define_method(RSprite, "wave_speed", RMF(SpriteGetWaveSpeed), 0);
  rb_define_method(RSprite, "wave_speed=", RMF(SpriteSetWaveSpeed), 1);
  rb_define_method(RSprite, "wave_phase", RMF(SpriteGetWavePhase), 0);
  rb_define_method(RSprite, "wave_phase=", RMF(SpriteSetWavePhase), 1);
}

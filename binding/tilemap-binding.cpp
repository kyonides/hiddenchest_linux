/*
** tilemap-binding.cpp
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
#include "tilemap.h"
#include "viewport.h"
#include "bitmap.h"
#include "table.h"
#include "disposable-binding.h"
#include "binding-util.h"
#include "binding-types.h"

DEF_TYPE_CUSTOMFREE(TilemapAutotiles, RUBY_TYPED_NEVER_FREE);

static VALUE tilemapAutotilesSet(VALUE self, VALUE pos, VALUE obj)
{
  Tilemap::Autotiles *a = getPrivateData<Tilemap::Autotiles>(self);
  int i = RB_FIX2INT(pos);
  Bitmap *bitmap = getPrivateDataCheck<Bitmap>(obj, BitmapType);
  a->set(i, bitmap);
  VALUE ary = rb_iv_get(self, "array");
  rb_ary_store(ary, i, obj);
  return self;
}

static VALUE tilemapAutotilesGet(VALUE self, VALUE pos)
{
  int i = RB_FIX2INT(pos);
  if (i < 0 || i > 6)
    return Qnil;
  VALUE ary = rb_iv_get(self, "array");
  return rb_ary_entry(ary, i);
}

DEF_TYPE(Tilemap);

static VALUE tilemapInitialize(int argc, VALUE *argv, VALUE self)
{
  Tilemap *t;
  Viewport *viewport = 0;
  if (!RB_NIL_P(argv[0]))
    viewport = getPrivateDataCheck<Viewport>(argv[0], ViewportType);
  t = new Tilemap(viewport);
  setPrivateData(self, t);
  rb_iv_set(self, "viewport", argv[0]);
  rb_iv_set(self, "autotiles_speed", RB_INT2FIX(1));
  rb_iv_set(self, "zoom", RB_INT2FIX(100));
  rb_iv_set(self, "z", RB_INT2FIX(0));
  wrapProperty(self, &t->getAutotiles(), "autotiles", TilemapAutotilesType);
  VALUE autotilesObj = rb_iv_get(self, "autotiles");
  VALUE ary = rb_ary_new2(7);
  for (int i = 0; i < 7; ++i) { rb_ary_push(ary, Qnil); }
  rb_iv_set(autotilesObj, "array", ary);
  // Circular reference so both objects are always alive at the same time
  rb_iv_set(autotilesObj, "tilemap", self);
  return self;
}

RB_METHOD(tilemapGetAutotiles)
{
  RB_UNUSED_PARAM;
  checkDisposed<Tilemap>(self);
  return rb_iv_get(self, "autotiles");
}

RB_METHOD(tilemapUpdate)
{
  RB_UNUSED_PARAM;
  Tilemap *t = getPrivateData<Tilemap>(self);
  if (!t)
    return Qfalse;
  t->update();
  return Qnil;
}

static VALUE tilemapGetViewport(VALUE self)
{
  checkDisposed<Tilemap>(self);
  return rb_iv_get(self, "viewport");
}

static VALUE tilemap_set_viewport(VALUE self, VALUE port)
{
  Tilemap *t = getPrivateData<Tilemap>(self);
  if (!t)
    return Qfalse;
  Viewport *vp = getPrivateDataCheck<Viewport>(port, ViewportType);
  t->set_viewport(vp);
  return rb_iv_set(self, "viewport", port);
}

static VALUE tilemap_z_set(VALUE self, VALUE rz)
{
  Tilemap *t = getPrivateData<Tilemap>(self);
  if (!t)
    return RB_INT2FIX(-1);
  int z = RB_FIX2INT(rz);
  t->set_z(z);
  return rb_iv_set(self, "z", rz);
}

static VALUE tilemap_z_get(VALUE self)
{
  return rb_iv_get(self, "z");
}

static VALUE tilemap_at_speed_set(VALUE self, VALUE speed)
{
  VALUE rb_speed = rb_iv_get(self, "speed");
  int next_speed = RB_FIX2INT(speed);
  if (rb_speed == speed)
    return rb_speed;
  Tilemap *t = getPrivateData<Tilemap>(self);
  t->set_autotiles_speed(next_speed);
  next_speed = t->get_autotiles_speed();
  speed = RB_INT2FIX(next_speed);
  return rb_iv_set(self, "autotiles_speed", speed);
}

static VALUE tilemap_at_speed_get(VALUE self)
{
  return rb_iv_get(self, "autotiles_speed");
}

static VALUE tilemap_zoom_set(VALUE self, VALUE zoom)
{
  VALUE rb_zoom = rb_iv_get(self, "zoom");
  int next_zoom = RB_FIX2INT(zoom);
  if (rb_zoom == zoom) {
    return rb_zoom;
  } else if (next_zoom < 100) {
    next_zoom = 100;
    zoom = RB_INT2FIX(next_zoom);
  } else if (next_zoom > 200) {
    next_zoom = 200;
    zoom = RB_INT2FIX(next_zoom);
  }
  Tilemap *t = getPrivateData<Tilemap>(self);
  t->set_tile_zoom(next_zoom);
  return rb_iv_set(self, "zoom", zoom);
}

static VALUE tilemap_zoom_get(VALUE self)
{
  return rb_iv_get(self, "zoom");
}

DEF_PROP_OBJ_REF(Tilemap, Bitmap,   Tileset,    "tileset")
DEF_PROP_OBJ_REF(Tilemap, Table,    MapData,    "map_data")
DEF_PROP_OBJ_REF(Tilemap, Table,    FlashData,  "flash_data")
DEF_PROP_OBJ_REF(Tilemap, Table,    Priorities, "priorities")
DEF_PROP_B(Tilemap, Visible)
DEF_PROP_I(Tilemap, OX)
DEF_PROP_I(Tilemap, OY)

void tilemapBindingInit()
{
  VALUE klass = rb_define_class("TilemapAutotiles", rb_cObject);
  rb_define_alloc_func(klass, classAllocate<&TilemapAutotilesType>);
  rb_define_method(klass, "[]=", RMF(tilemapAutotilesSet), 2);
  rb_define_method(klass, "[]",  RMF(tilemapAutotilesGet), 1);
  klass = rb_define_class("Tilemap", rb_cObject);
  rb_define_alloc_func(klass, classAllocate<&TilemapType>);
  disposableBindingInit<Tilemap>(klass);
  _rb_define_method(klass, "initialize", tilemapInitialize);
  _rb_define_method(klass, "autotiles", tilemapGetAutotiles);
  _rb_define_method(klass, "update", tilemapUpdate);
  rb_define_method(klass, "viewport", tilemapGetViewport, 0);
  rb_define_method(klass, "viewport=", tilemap_set_viewport, 1);
  INIT_PROP_BIND( Tilemap, Tileset,    "tileset"   );
  INIT_PROP_BIND( Tilemap, MapData,    "map_data"  );
  INIT_PROP_BIND( Tilemap, FlashData,  "flash_data");
  INIT_PROP_BIND( Tilemap, Priorities, "priorities");
  INIT_PROP_BIND( Tilemap, Visible,    "visible"   );
  INIT_PROP_BIND( Tilemap, OX,         "ox"        );
  INIT_PROP_BIND( Tilemap, OY,         "oy"        );
  rb_define_method(klass, "z=", RMF(tilemap_z_set), 1);
  rb_define_method(klass, "z",  RMF(tilemap_z_get), 0);
  rb_define_method(klass, "autotiles_speed=", RMF(tilemap_at_speed_set), 1);
  rb_define_method(klass, "autotiles_speed",  RMF(tilemap_at_speed_get), 0);
  rb_define_method(klass, "zoom=", RMF(tilemap_zoom_set), 1);
  rb_define_method(klass, "zoom",  RMF(tilemap_zoom_get), 0);
}

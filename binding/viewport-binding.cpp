/*
** viewport-binding.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** 2019 Extended by Kyonides Arkanthes
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
** along with HiddenChest.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "viewport.h"
#include "sharedstate.h"
#include "disposable-binding.h"
#include "flashable-binding.h"
#include "sceneelement-binding.h"
#include "binding-util.h"
#include "binding-types.h"

DEF_TYPE(Viewport);

RB_METHOD(viewportInitialize)
{
  Viewport *v;
  if (argc == 0 && rgssVer >= 3) {
    v = new Viewport();
  } else if (argc == 1) { // The rect arg is only used to init the viewport,
    //and does NOT replace its 'rect' property
    VALUE rectObj;
    Rect *rect;
    rb_get_args(argc, argv, "o", &rectObj RB_ARG_END);
    rect = getPrivateDataCheck<Rect>(rectObj, RectType);
    v = new Viewport(rect);
  } else if ((rgssVer == 1 || rgssVer == 4) && argc == 4) {
    int x = RB_FIX2INT(argv[0]), y = RB_FIX2INT(argv[1]);
    int width = RB_FIX2INT(argv[2]), height = RB_FIX2INT(argv[3]);
    v = new Viewport(x, y, width, height);
  } else {
    int x, y, width, height;
    rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);
    v = new Viewport(x, y, width, height);
  }
  setPrivateData(self, v);
  v->initDynAttribs(); // Wrap property objects
  wrapProperty(self, &v->getRect(),  "rect",  RectType);
  wrapProperty(self, &v->getColor(), "color", ColorType);
  wrapProperty(self, &v->getTone(),  "tone",  ToneType);
  // 'elements' holds all SceneElements that become children of this
  // viewport, so we can dispose them when the viewport is disposed
  rb_iv_set(self, "elements", rb_ary_new());
  return self;
}

DEF_PROP_OBJ_VAL(Viewport, Rect,  Rect,  "rect")
DEF_PROP_OBJ_VAL(Viewport, Color, Color, "color")
DEF_PROP_OBJ_VAL(Viewport, Tone,  Tone,  "tone")
DEF_PROP_I(Viewport, OX)
DEF_PROP_I(Viewport, OY)
DEF_PROP_I(Viewport, X)
DEF_PROP_I(Viewport, Y)
//DEF_PROP_I(Viewport, Width)
//DEF_PROP_I(Viewport, Height)

static VALUE viewport_get_width(VALUE self)
{
  Viewport *v = static_cast<Viewport*>(RTYPEDDATA_DATA(self));
  int w;
  GUARD_EXC( w = v->getWidth(); )
  return RB_INT2FIX(w);
}

static VALUE viewport_set_width(VALUE self, VALUE width)
{
  Viewport *v = static_cast<Viewport*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( v->setWidth( RB_FIX2INT(width) ); )
  return RB_INT2FIX( v->getWidth() );
}

static VALUE viewport_get_height(VALUE self)
{
  Viewport *v = static_cast<Viewport*>(RTYPEDDATA_DATA(self));
  int h;
  GUARD_EXC( h = v->getHeight(); )
  return RB_INT2FIX(h);
}

static VALUE viewport_set_height(VALUE self, VALUE height)
{
  Viewport *v = static_cast<Viewport*>(RTYPEDDATA_DATA(self));
  GUARD_EXC( v->setHeight( RB_FIX2INT(height) ); )
  return RB_INT2FIX( v->getHeight() );
}

void viewportBindingInit()
{
  VALUE klass = rb_define_class("Viewport", rb_cObject);
  rb_define_alloc_func(klass, classAllocate<&ViewportType>);
  disposableBindingInit  <Viewport>(klass);
  flashableBindingInit   <Viewport>(klass);
  sceneElementBindingInit<Viewport>(klass);
  _rb_define_method(klass, "initialize", viewportInitialize);
  INIT_PROP_BIND( Viewport, Rect,   "rect"  );
  INIT_PROP_BIND( Viewport, OX,     "ox"    );
  INIT_PROP_BIND( Viewport, OY,     "oy"    );
  INIT_PROP_BIND( Viewport, X,      "x"     );
  INIT_PROP_BIND( Viewport, Y,      "y"     );
  //INIT_PROP_BIND( Viewport, Width,  "width" );
  //INIT_PROP_BIND( Viewport, Height, "height");
  INIT_PROP_BIND( Viewport, Color,  "color" );
  INIT_PROP_BIND( Viewport, Tone,   "tone"  );
  rb_define_method(klass, "width", viewport_get_width, 0);
  rb_define_method(klass, "width=", viewport_set_width, 1);
  rb_define_method(klass, "height", viewport_get_height, 0);
  rb_define_method(klass, "height=", viewport_set_height, 1);
}


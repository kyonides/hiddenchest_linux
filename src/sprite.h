/*
** sprite.h
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
** along with HiddenChest.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SPRITE_H
#define SPRITE_H

#include "scene.h"
#include "flashable.h"
#include "disposable.h"
#include "viewport.h"
#include "util.h"
#include "etc-internal.h"

class Bitmap;
struct Color;
struct Tone;
struct Rect;
struct SpritePrivate;

class Sprite : public ViewportElement, public Flashable, public Disposable
{
public:
  Sprite(Viewport *viewport = 0);
  ~Sprite();
  void update();
  DECL_ATTR( BushDepth,   int     )
  DECL_ATTR( WaveAmp,     int     )
  DECL_ATTR( WaveLength,  int     )
  DECL_ATTR( WaveSpeed,   int     )
  DECL_ATTR( WavePhase,   float   )
  bool getWaveRotate() const;
  void setWaveRotate(bool);
  int getX() const;
  int getY() const;
  int getOX() const;
  int getOY() const;
  int getWidth() const;
  int getHeight() const;
  float getZoomX() const;
  float getZoomY() const;
  float getAngle() const;
  bool getMirror() const;
  bool mirror_y() const;
  void setX(int);
  void setY(int);
  void set_xy(int, int);
  void setOX(int);
  void setOY(int);
  void setZoomX(float);
  void setZoomY(float);
  void setAngle(float);
  void setMirror(bool);
  void set_mirror_y(bool);
  Bitmap* getBitmap();
  void setBitmap(Bitmap*);
  void gray_out();
  void turn_sepia();
  void invert_colors();
  void pixelate();
  Rect& getSrcRect() const;
  void setSrcRect(Rect& rect);
  int getBushOpacity() const;
  void setBushOpacity(NormValue opacity);
  int getOpacity() const;
  void setOpacity(NormValue opacity);
  int getBlendType() const;
  void setBlendType(int type);
  Color& getColor() const;
  void setColor(Color& color);
  void setColor(double r, double g, double b, double a);
  Tone& getTone() const;
  void setTone(Tone& tone);
  void set_drag_margin_y(int my);
  int  reduce_speed();
  void set_reduce_speed(int);
  void increase_width();
  void increase_height();
  void increase_width_height();
  void reduce_width();
  void reduce_height();
  void reduce_width_height();
  bool is_width_increased();
  bool is_height_increased();
  bool is_width_reduced();
  bool is_height_reduced();
  bool mouse_is_inside();
  bool mouse_is_inside_area(Rect *rect, bool state);
  bool mouse_is_above_color_found();
  void initDynAttribs();
  void onGeometryChange(const Scene::Geometry &);

private:
  SpritePrivate *p;
  void draw();
  void releaseResources();
  const char *klassName() const { return "sprite"; }
  ABOUT_TO_ACCESS_DISP
};

#endif // SPRITE_H

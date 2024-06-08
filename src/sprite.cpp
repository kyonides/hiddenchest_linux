/*
** sprite.cpp
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

#include "sprite.h"
#include "sharedstate.h"
#include "bitmap.h"
#include "input.h"
#include "etc.h"
#include "util.h"
#include "gl-util.h"
#include "quad.h"
#include "transform.h"
#include "shader.h"
#include "glstate.h"
#include "quadarray.h"
#include <math.h>
#include <SDL_rect.h>
#include <sigc++/connection.h>
#include "debugwriter.h"

#define ROWH 6

struct SpritePrivate
{
  Bitmap *bitmap;
  Quad quad;
  Transform trans;
  Rect *srcRect;
  sigc::connection srcRectCon;
  bool mirrored;
  bool mirrored_y;
  bool increase_width;
  bool increase_height;
  bool reduce_width;
  bool reduce_height;
  int reduced_width;
  int reduced_height;
  int reduce_speed;
  int drag_margin_y;
  int bushDepth;
  float efBushDepth;
  NormValue bushOpacity;
  NormValue opacity;
  BlendType blendType;
  IntRect sceneRect;
  Vec2i sceneOrig;
  bool isVisible;// Would this sprite be visible on the screen if drawn?
  Color *color;
  Tone *tone;
  struct
  {
    bool rotated;
    int amp;
    int length;
    int speed;
    float phase;
    // Wave effect is active (amp != 0)
    bool active;
    // qArray needs updating
    bool dirty;
    SimpleQuadArray qArray;
  } wave;
  EtcTemps tmp;
  sigc::connection prepareCon;
  SpritePrivate()
  : bitmap(0),
    srcRect(&tmp.rect),
    mirrored(false),
    mirrored_y(false),
    bushDepth(0),
    efBushDepth(0),
    bushOpacity(128),
    opacity(255),
    blendType(BlendNormal),
    increase_width(false),
    increase_height(false),
    reduce_width(false),
    reduce_height(false),
    reduced_width(0),
    reduced_height(0),
    reduce_speed(ROWH),
    drag_margin_y(8),
    isVisible(false),
    color(&tmp.color),
    tone(&tmp.tone)
  {
    sceneRect.x = sceneRect.y = 0;
    updateSrcRectCon();
    prepareCon = shState->prepareDraw.connect(sigc::mem_fun(this, &SpritePrivate::prepare));
    wave.rotated = false;
    wave.amp = 0;
    wave.length = 180;
    wave.speed = 360;
    wave.phase = 0.0f;
    wave.dirty = false;
  }

  ~SpritePrivate()
  {
    srcRectCon.disconnect();
    prepareCon.disconnect();
  }

  void update_reduce_width()
  {
    if (increase_width) {
      int w = bitmap->width();
      reduced_width -= reduce_speed;
      reduced_width = clamp<int>(reduced_width, 0, w);
      onSrcRectChange();
      increase_width = reduced_width > 0;
      return;
    }
    if (reduce_width) {
      int w = bitmap->width();
      reduced_width += reduce_speed;
      reduced_width = clamp<int>(reduced_width, 0, w);
      onSrcRectChange();
      reduce_width = w > reduced_width;
    }
  }

  void update_reduce_height()
  {
    if (increase_height) {
      int h = bitmap->height();
      reduced_height -= reduce_speed;
      reduced_height = clamp<int>(reduced_height, 0, h);
      onSrcRectChange();
      increase_height = reduced_height > 0;
      return;
    }
    if (reduce_height) {
      int h = bitmap->height();
      reduced_height += reduce_speed;
      reduced_height = clamp<int>(reduced_height, 0, h);
      onSrcRectChange();
      reduce_height = h > reduced_height;
    }
  }

  void recomputeBushDepth()
  {
    if (nullOrDisposed(bitmap))
      return;
    // Calculate effective (normalized) bush depth
    float texBushDepth = (bushDepth / trans.getScale().y) -
                         (srcRect->y + srcRect->height) +
                         bitmap->height();
    efBushDepth = 1.0f - texBushDepth / bitmap->height();
  }

  void onSrcRectChange()
  {
    FloatRect rect = srcRect->toFloatRect();
    Vec2i bmSize;
    if (!nullOrDisposed(bitmap))
      bmSize = Vec2i(bitmap->width() - reduced_width, bitmap->height() - reduced_height);
    // Clamp the rectangle so it doesn't reach outside the bitmap bounds
    rect.w = clamp<int>(rect.w, 0, bmSize.x-rect.x);
    rect.h = clamp<int>(rect.h, 0, bmSize.y-rect.y);
    quad.setTexRect(mirrored ? rect.hFlipped() : rect);
    quad.setTexRect(mirrored_y ? rect.wFlipped() : rect);
    quad.setPosRect(FloatRect(0, 0, rect.w, rect.h));
    recomputeBushDepth();
    wave.dirty = true;
  }

  void updateSrcRectCon()
  { // Cut old connection and Create new one
    srcRectCon.disconnect();
    srcRectCon = srcRect->valueChanged.connect
                (sigc::mem_fun(this, &SpritePrivate::onSrcRectChange));
  }

  void updateVisibility()
  {
    isVisible = false;
    if (nullOrDisposed(bitmap) || !opacity)
      return;
    if (wave.active) {
      isVisible = true;
      return;// Don't do expensive wave bounding box calculations
    }
    // Compare sprite bounding box against the scene
    // If sprite is zoomed/rotated, just opt out for now for simplicity's sake
    const Vec2 &scale = trans.getScale();
    if (scale.x != 1 || scale.y != 1 || trans.getRotation() != 0) {
      isVisible = true;
      return;
    }
    IntRect self;
    self.setPos(trans.getPositionI() - (trans.getOriginI() + sceneOrig));
    self.w = bitmap->width();
    self.h = bitmap->height();
    isVisible = SDL_HasIntersection(&self, &sceneRect);
  }

  void emitWaveChunk(SVertex *&vert, float phase, int width, float zoomY, int chunkY, int chunkLength)
  {
    float wavePos = phase + (chunkY / (float) wave.length) * (float) (M_PI * 2);
    float chunkX = sin(wavePos) * wave.amp;
    FloatRect tex(0, chunkY / zoomY, width, chunkLength / zoomY);
    FloatRect pos = tex;
    pos.x = chunkX;
    Quad::setTexPosRect(vert, tex, pos);
    vert += 4;
  }

  void emitWaveChunk2(SVertex *&vert, float phase, int height, float zoomX, int chunkX, int chunkLength)
  {
    float wavePos = phase + (chunkX / (float) wave.length) * (float) (M_PI * 2);
    float chunkY = sin(wavePos) * wave.amp;
    FloatRect tex(chunkX / zoomX, 0, chunkLength / zoomX, height);
    FloatRect pos = tex;
    pos.y = chunkY;
    Quad::setTexPosRect(vert, tex, pos);
    vert += 4;
  }

  void updateWave()
  {
    if (nullOrDisposed(bitmap))
      return;
    if (wave.amp == 0) {
      wave.active = false;
      return;
    }
    wave.active = true;
    int width = srcRect->width;
    int height = srcRect->height;
    float zoomX = trans.getScale().x;
    float zoomY = trans.getScale().y;
    if (!wave.rotated) {
      if (wave.amp < -(width / 2)) {
        wave.qArray.resize(0);
        wave.qArray.commit();
        return;
      }
    } else {
      if (wave.amp < -(height / 2)) {
        wave.qArray.resize(0);
        wave.qArray.commit();
        return;
      }
    }
    // RMVX does this, and I have no *** clue why
    if (wave.amp < 0) {
      wave.qArray.resize(1);
      int x, y, w, h;
      if (!wave.rotated) {
        x = -wave.amp;
        y = srcRect->y;
        w = width + wave.amp * 2;
        h = srcRect->height;
      } else {
        x = srcRect->x;
        y = -wave.amp;
        w = srcRect->width;
        h = height + wave.amp * 2;
      }
      FloatRect tex(x, y, w, h);
      Quad::setTexPosRect(&wave.qArray.vertices[0], tex, tex);
      wave.qArray.commit();
      return;
    }
    int visibleLength, firstLength;
    if (!wave.rotated) {
      // The length of the sprite as it appears on screen
      visibleLength = height * zoomY;
      // First chunk length (aligned to 8 pixel boundary
      firstLength = ((int) trans.getPosition().y) % 8;
    } else {
      visibleLength = width * zoomX;
      firstLength = ((int) trans.getPosition().x) % 8;
    }
    // Amount of full 8 pixel chunks in the middle
    int chunks = (visibleLength - firstLength) / 8;
    // Final chunk length
    int lastLength = (visibleLength - firstLength) % 8;
    wave.qArray.resize(!!firstLength + chunks + !!lastLength);
    SVertex *vert = &wave.qArray.vertices[0];
    float phase = (wave.phase * (float) M_PI) / 180.0f;
    if (!wave.rotated) {
      if (firstLength > 0)
        emitWaveChunk(vert, phase, width, zoomY, 0, firstLength);
      for (int i = 0; i < chunks; ++i)
        emitWaveChunk(vert, phase, width, zoomY, firstLength + i * 8, 8);
      if (lastLength > 0)
        emitWaveChunk(vert, phase, width, zoomY, firstLength + chunks * 8, lastLength);
    } else {
      if (firstLength > 0)
        emitWaveChunk2(vert, phase, height, zoomY, 0, firstLength);
      for (int i = 0; i < chunks; ++i)
        emitWaveChunk2(vert, phase, height, zoomX, firstLength + i * 8, 8);
      if (lastLength > 0)
        emitWaveChunk2(vert, phase, height, zoomX, firstLength + chunks * 8, lastLength);
    }
    wave.qArray.commit();
  }

  void prepare() {
    if (wave.dirty) {
      updateWave();
      wave.dirty = false;
    }
    updateVisibility();
  }
};

Sprite::Sprite(Viewport *viewport) : ViewportElement(viewport)
{
  p = new SpritePrivate;
  onGeometryChange(scene->getGeometry());
}

Sprite::~Sprite() { dispose(); }

Bitmap* Sprite::getBitmap()
{
  return p->bitmap;
}

int Sprite::getX() const
{
  return p->trans.getPosition().x;
}

int Sprite::getY() const
{
  return p->trans.getPosition().y;
}

int Sprite::getOX() const
{
  return p->trans.getOrigin().x;
}

int Sprite::getOY() const
{
  return p->trans.getOrigin().y;
}

float Sprite::getZoomX() const
{
  return p->trans.getScale().x;
}

float Sprite::getZoomY() const
{
  return p->trans.getScale().y;
}

float Sprite::getAngle() const
{
  return p->trans.getRotation();
}

bool Sprite::getMirror() const
{
  return p->mirrored;
}

bool Sprite::mirror_y() const
{
  return p->mirrored_y;
}

int Sprite::getBlendType() const
{
  return p->blendType;
}

int Sprite::getWidth() const
{
  return p->srcRect->width;
}

int Sprite::getHeight() const
{
  return p->srcRect->height;
}

DEF_ATTR_RD_SIMPLE(Sprite, BushDepth,  int,     p->bushDepth)
DEF_ATTR_RD_SIMPLE(Sprite, WaveAmp,    int,     p->wave.amp)
DEF_ATTR_RD_SIMPLE(Sprite, WaveLength, int,     p->wave.length)
DEF_ATTR_RD_SIMPLE(Sprite, WaveSpeed,  int,     p->wave.speed)
DEF_ATTR_RD_SIMPLE(Sprite, WavePhase,  float,   p->wave.phase)

bool Sprite::getWaveRotate() const
{
  return p->wave.rotated;
}

void Sprite::setWaveRotate(bool state)
{
  p->wave.rotated = state;
}

int Sprite::getBushOpacity() const
{
  guardDisposed();
  return p->bushOpacity.unNorm;
}

void Sprite::setBushOpacity(NormValue opacity)
{
  guardDisposed();
  p->bushOpacity = opacity;
}

int Sprite::getOpacity() const
{
  guardDisposed();
  return p->opacity.unNorm;
}

void Sprite::setOpacity(NormValue opacity)
{
  guardDisposed();
  p->opacity = opacity;
}

Rect& Sprite::getSrcRect() const
{
  guardDisposed();
  return *p->srcRect;
}

void Sprite::setSrcRect(Rect &rect)
{
  guardDisposed();
  *p->srcRect = rect;
}

Color& Sprite::getColor() const
{
  guardDisposed();
  return *p->color;
}

void Sprite::setColor(Color &color)
{
  guardDisposed();
  *p->color = color;
}

void Sprite::setColor(double r, double g, double b, double a)
{
  guardDisposed();
  p->color->set(r, g, b, a);
}

Tone& Sprite::getTone() const
{
  guardDisposed();
  return *p->tone;
}

void Sprite::setTone(Tone &tone)
{
  guardDisposed();
  *p->tone = tone;
}

void Sprite::setBitmap(Bitmap *bitmap)
{
  guardDisposed();
  if (p->bitmap == bitmap)
    return;
  p->bitmap = bitmap;
  if (nullOrDisposed(bitmap))
    return;
  bitmap->ensureNonMega();
  *p->srcRect = bitmap->rect();
  p->onSrcRectChange();
  p->quad.setPosRect(p->srcRect->toFloatRect());
  if (p->wave.active)
    p->wave.dirty = true;
}

void Sprite::gray_out()
{
  p->bitmap->gray_out();
}

void Sprite::turn_sepia()
{
  p->bitmap->turn_sepia();
}

void Sprite::invert_colors()
{
  p->bitmap->invert_colors();
}

void Sprite::setX(int nx)
{
  guardDisposed();
  if (p->trans.getPosition().x == nx)
    return;
  p->trans.setPosition(Vec2(nx, getY()));
}

void Sprite::setY(int ny)
{
  guardDisposed();
  if (p->trans.getPosition().y == ny)
    return;
  p->trans.setPosition(Vec2(getX(), ny));
  if (!p->wave.active)
    return;
  p->wave.dirty = true;
  setSpriteY(ny);
}

void Sprite::set_xy(int nx, int ny)
{
  guardDisposed();
  if (p->trans.getPosition().x != nx || p->trans.getPosition().y != ny)
    p->trans.setPosition(Vec2(nx, ny));
  if (!p->wave.active)
    return;
  p->wave.dirty = true;
  setSpriteY(ny);
}

void Sprite::setOX(int value)
{
  guardDisposed();
  if (p->trans.getOrigin().x == value)
    return;
  p->trans.setOrigin(Vec2(value, getOY()));
}

void Sprite::setOY(int value)
{
  guardDisposed();
  if (p->trans.getOrigin().y == value)
    return;
  p->trans.setOrigin(Vec2(getOX(), value));
}

void Sprite::setZoomX(float value)
{
  guardDisposed();
  if (p->trans.getScale().x == value)
    return;
  p->trans.setScale(Vec2(value, getZoomY()));
}

void Sprite::setZoomY(float value)
{
  guardDisposed();
  if (p->trans.getScale().y == value)
    return;
  p->trans.setScale(Vec2(getZoomX(), value));
  p->recomputeBushDepth();
  p->wave.dirty = true;
}

void Sprite::setAngle(float value)
{
  guardDisposed();
  if (p->trans.getRotation() == value)
    return;
  p->trans.setRotation(value);
}

void Sprite::setMirror(bool mirrored)
{
  guardDisposed();
  if (p->mirrored == mirrored)
    return;
  p->mirrored = mirrored;
  p->onSrcRectChange();
}

void Sprite::set_mirror_y(bool mirrored)
{
  guardDisposed();
  if (p->mirrored_y == mirrored)
    return;
  p->mirrored_y = mirrored;
  p->onSrcRectChange();
}

void Sprite::setBushDepth(int value)
{
  guardDisposed();
  if (p->bushDepth == value)
    return;
  p->bushDepth = value;
  p->recomputeBushDepth();
}

void Sprite::setBlendType(int type)
{
  guardDisposed();
  switch (type) {
  default :
  case BlendNormal :
    p->blendType = BlendNormal;
    return;
  case BlendAddition :
    p->blendType = BlendAddition;
    return;
  case BlendSubstraction :
    p->blendType = BlendSubstraction;
    return;
  }
}

void Sprite::set_drag_margin_y(int my)
{
  p->drag_margin_y = my;
}

int Sprite::reduce_speed()
{
  guardDisposed();
  return p->reduce_speed;
}

void Sprite::set_reduce_speed(int speed)
{
  guardDisposed();
  p->reduce_speed = speed;
}

void Sprite::increase_width()
{
  guardDisposed();
  p->reduced_width = p->bitmap->width();
  p->increase_width = true;
  p->update_reduce_width();
}

void Sprite::increase_height()
{
  guardDisposed();
  p->reduced_height = p->bitmap->height();
  p->increase_height = true;
  p->update_reduce_height();
}

void Sprite::increase_width_height()
{
  guardDisposed();
  p->reduced_width = p->bitmap->width();
  p->reduced_height = p->bitmap->height();
  p->increase_width = true;
  p->increase_height = true;
  p->update_reduce_width();
  p->update_reduce_height();
}

void Sprite::reduce_width()
{
  guardDisposed();
  p->reduce_width = true;
}

void Sprite::reduce_height()
{
  guardDisposed();
  p->reduce_height = true;
}

void Sprite::reduce_width_height()
{
  guardDisposed();
  p->reduce_width = true;
  p->reduce_height = true;
}

bool Sprite::is_width_increased()
{
  guardDisposed();
  return p->reduced_width == 0;
}

bool Sprite::is_height_increased()
{
  guardDisposed();
  return p->reduced_height == 0;
}

bool Sprite::is_width_reduced()
{
  guardDisposed();
  return p->reduced_width == p->bitmap->width();
}

bool Sprite::is_height_reduced()
{
  guardDisposed();
  return p->reduced_height == p->bitmap->height();
}

bool Sprite::mouse_is_inside()
{
  guardDisposed();
  if (!p->isVisible)
    return false;
  int mp = shState->input().mouseX();
  int x = p->trans.getPosition().x;
  if (mp < x || mp > x + p->srcRect->width)
    return false;
  mp = shState->input().mouseY();
  int y = p->trans.getPosition().y;
  if (mp < y)
    return false;
  return mp <= y + p->srcRect->height;
}

bool Sprite::mouse_is_inside_area(Rect *rect, bool state)
{
  guardDisposed();
  if (!p->isVisible)
    return false;
  int mp = shState->input().mouseX();
  int sp = p->trans.getPosition().x;
  if (mp < sp + rect->x || mp > sp + rect->x + rect->width)
    return false;
  int mpoy = state ? p->drag_margin_y : 0;
  mp = shState->input().mouseY();
  sp = p->trans.getPosition().y - mpoy;
  if (mp < sp + rect->y)
    return false;
  return mp <= sp + rect->y + rect->height + mpoy;
}

bool Sprite::mouse_is_above_color_found()
{
  guardDisposed();
  if (!p->isVisible)
    return false;
  int mx = shState->input().mouseX();
  int x = p->trans.getPosition().x;
  if (mx < x)
    return false;
  if (mx > x + p->srcRect->width)
    return false;
  int my = shState->input().mouseY();
  int y = p->trans.getPosition().y;
  if (my < y)
    return false;
  if (my > y + p->srcRect->height)
    return false;
  int ax = mx - x, ay = my - y;
  return !p->bitmap->is_alpha_pixel(ax, ay);
}

#define DEF_WAVE_SETTER(Name, name, type) \
void Sprite::setWave##Name(type value) \
{ \
  guardDisposed(); \
  if (p->wave.name == value) return; \
  p->wave.name = value; \
  p->wave.dirty = true; \
}

DEF_WAVE_SETTER(Amp,    amp,    int)
DEF_WAVE_SETTER(Length, length, int)
DEF_WAVE_SETTER(Speed,  speed,  int)
DEF_WAVE_SETTER(Phase,  phase,  float)

#undef DEF_WAVE_SETTER

void Sprite::initDynAttribs()
{
  p->srcRect = new Rect;
  p->color = new Color;
  p->tone = new Tone;
  p->updateSrcRectCon();
}
// Flashable
void Sprite::update()
{
  guardDisposed();
  Flashable::update();
  p->update_reduce_width();
  p->update_reduce_height();
  if (!p->wave.active)
    return;
  p->wave.phase += p->wave.speed / 180;
  p->wave.dirty = true;
}
// SceneElement
void Sprite::draw()
{
  if (!p->isVisible || emptyFlashFlag)
    return;
  ShaderBase *base;
  bool renderEffect = p->color->hasEffect() ||
                      p->tone->hasEffect()  ||
                      flashing              ||
                      p->bushDepth != 0;
  if (renderEffect) {
    SpriteShader &shader = shState->shaders().sprite;
    shader.bind();
    shader.applyViewportProj();
    shader.setSpriteMat(p->trans.getMatrix());
    shader.setTone(p->tone->norm);
    shader.setOpacity(p->opacity.norm);
    shader.setBushDepth(p->efBushDepth);
    shader.setBushOpacity(p->bushOpacity.norm);
    /* When both flashing and effective color are set,
     * the one with higher alpha will be blended */
    const Vec4 *blend = (flashing && flashColor.w > p->color->norm.w) ?
                             &flashColor : &p->color->norm;
    shader.setColor(*blend);
    base = &shader;
  } else if (p->opacity != 255) {
    AlphaSpriteShader &shader = shState->shaders().alphaSprite;
    shader.bind();
    shader.setSpriteMat(p->trans.getMatrix());
    shader.setAlpha(p->opacity.norm);
    shader.applyViewportProj();
    base = &shader;
  } else {
    SimpleSpriteShader &shader = shState->shaders().simpleSprite;
    shader.bind();
    shader.setSpriteMat(p->trans.getMatrix());
    shader.applyViewportProj();
    base = &shader;
  }
  glState.blendMode.pushSet(p->blendType);
  p->bitmap->bindTex(*base);
  if (p->wave.active)
    p->wave.qArray.draw();
  else
    p->quad.draw();
  glState.blendMode.pop();
}

void Sprite::onGeometryChange(const Scene::Geometry &geo)
{// Offset at which the sprite will be drawn relative to screen origin
  p->trans.setGlobalOffset(geo.offset());
  p->sceneRect.setSize(geo.rect.size());
  p->sceneOrig = geo.orig;
}

void Sprite::releaseResources() {
  unlink();
  delete p;
}

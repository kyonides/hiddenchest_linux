/*
** window.cpp
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

#include "input.h"
#include "window.h"
#include "viewport.h"
#include "sharedstate.h"
#include "bitmap.h"
#include "etc.h"
#include "etc-internal.h"
#include "tilequad.h"
#include "gl-util.h"
#include "quad.h"
#include "quadarray.h"
#include "texpool.h"
#include "glstate.h"
#include <sigc++/connection.h>

#define MINOPENH 32

template<typename T>
struct Sides
{
  T l, r, t, b;
};

template<typename T>
struct Corners
{
  T tl, tr, bl, br;
};

static const IntRect backgroundSrc(0, 0, 128, 128);

static const IntRect cursorSrc(128, 64, 32, 32);

static const IntRect pauseAniSrc[] =
{
  IntRect(160, 64, 16, 16),
  IntRect(176, 64, 16, 16),
  IntRect(160, 80, 16, 16),
  IntRect(176, 80, 16, 16)
};

static const Sides<IntRect> bordersSrc =
{
  IntRect(128, 16, 16, 32),
  IntRect(176, 16, 16, 32),
  IntRect(144,  0, 32, 16),
  IntRect(144, 48, 32, 16)
};

static const Corners<IntRect> cornersSrc =
{
  IntRect(128,  0, 16, 16),
  IntRect(176,  0, 16, 16),
  IntRect(128, 48, 16, 16),
  IntRect(176, 48, 16, 16)
};

static const Sides<IntRect> scrollArrowSrc =
{
  IntRect(144, 24,  8, 16),
  IntRect(168, 24,  8, 16),
  IntRect(152, 16, 16,  8),
  IntRect(152, 40, 16,  8)
};

/* Cycling */
static const uint8_t cursorAniAlpha[] =
{
  /* Fade out */
  0xFF, 0xF7, 0xEF, 0xE7, 0xDF, 0xD7, 0xCF, 0xC7,
  0xBF, 0xB7, 0xAF, 0xA7, 0x9F, 0x97, 0x8F, 0x87,
  /* Fade in */
  0x7F, 0x87, 0x8F, 0x97, 0x9F, 0xA7, 0xAF, 0xB7,
  0xBF, 0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7
};

static elementsN(cursorAniAlpha);

/* Cycling */
static const uint8_t pauseAniQuad[] =
{
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3
};

static elementsN(pauseAniQuad);

/* No cycle */
static const uint8_t pauseAniAlpha[] =
{
  0x00, 0x20, 0x40, 0x60,
  0x80, 0xA0, 0xC0, 0xE0,
  0xFF
};

static elementsN(pauseAniAlpha);

/* Points to an array of quads which it doesn't own.
 * Useful for setting alpha of quads stored inside
 * bigger arrays */
struct QuadChunk
{
  Vertex *vert;
  int count; /* In quads */

  QuadChunk() : vert(0), count(0) {}

  void setAlpha(float value)
  {
    for (int i = 0; i < count*4; ++i)
      vert[i].color.w = value;
  }
};

/* Vocabulary:
 *
 * Base: 'Base' layer of window; includes background and borders.
 *   Drawn at z+0.
 *
 * Controls: 'Controls' layer of window; includes scroll arrows,
 *   pause animation, cursor rectangle and contents bitmap.
 *   Drawn at z+2.
 *
 * Scroll arrows: Arrows that appear automatically when a part of
 *   the contents bitmap is not visible in either upper, lower, left
 *   or right direction.
 *
 * Pause: Animation that displays an animating icon in the bottom
 *   center of the window, usually indicating user input is awaited,
 *   such as when text is displayed.
 *
 * Cursor: Blinking rectangle that usually displays a selection to
 *   the user.
 *
 * Contents: User settable bitmap that is drawn inside the window,
 *   clipped to a 16 pixel smaller rectangle. Position is adjusted
 *   with OX/OY.
 *
 * BaseTex: If the window has an opacity <255, we have to prerender
 *   the base to a texture and draw that. Otherwise, we can draw the
 *   quad array directly to the screen.
 */

struct WindowPrivate
{
  Bitmap *windowskin;
  Bitmap *contents;
  bool bgStretch;
  Rect *cursorRect;
  bool active;
  bool pause;
  sigc::connection cursorRectCon;
  Vec2i sceneOffset;
  Vec2i position;
  Vec2i size;
  Vec2i contentsOffset;
  NormValue opacity;
  NormValue backOpacity;
  NormValue contentsOpacity;
  bool baseVertDirty;
  bool opacityDirty;
  bool baseTexDirty;
  bool needOpenness;
  int openness;
  int tempHeight;
  int openMode;
  int padding;
  ColorQuadArray baseQuadArray;
  // Used when opacity < 255
  TEXFBO baseTex;
  bool useBaseTex;
  QuadChunk backgroundVert;
  Quad baseTexQuad;
  struct WindowControls : public ViewportElement
  {
    WindowPrivate *p;

    WindowControls(WindowPrivate *p, Viewport *viewport = 0)
    : ViewportElement(viewport), p(p)
    {
      setZ(2);
    }

    void draw()
    {
      p->drawControls();
    }

    void release()
    {
      unlink();
    }

    ABOUT_TO_ACCESS_NOOP
  };

  WindowControls controlsElement;
  ColorQuadArray controlsQuadArray;
  int controlsQuadCount, pause_x, pause_y;
  Quad contentsQuad;
  QuadChunk pauseAniVert;
  QuadChunk cursorVert;
  uint8_t cursorAniAlphaIdx;
  uint8_t pauseAniAlphaIdx;
  uint8_t pauseAniQuadIdx;
  bool controlsVertDirty;
  EtcTemps tmp;
  sigc::connection prepareCon;

  WindowPrivate(Viewport *viewport = 0, int mode = 0)
  : windowskin(0),
    contents(0),
    bgStretch(true),
    cursorRect(&tmp.rect),
    active(true),
    pause(false),
    opacity(255),
    backOpacity(255),
    contentsOpacity(255),
    openness(100),
    openMode(mode),
    tempHeight(0),
    padding(16),
    pause_x(0),
    pause_y(0),
    baseVertDirty(true),
    opacityDirty(true),
    baseTexDirty(true),
    needOpenness(true),
    controlsElement(this, viewport),
    cursorAniAlphaIdx(0),
    pauseAniAlphaIdx(0),
    pauseAniQuadIdx(0),
    controlsVertDirty(true)
  {
    refreshCursorRectCon();
    controlsQuadArray.resize(14);
    cursorVert.count = 9;
    pauseAniVert.count = 1;
    processOpenMode();
    if (openMode > 0)
      openness = 0;
    prepareCon = shState->prepareDraw.connect(sigc::mem_fun(this, &WindowPrivate::prepare));
    prepare();
  }

  ~WindowPrivate()
  {
    shState->texPool().release(baseTex);
    cursorRectCon.disconnect();
    prepareCon.disconnect();
  }

  void processOpenMode()
  {
    int offset = size.y;
    if (baseTex.height > offset)
      offset = baseTex.height;
    if (tempHeight > offset)
      offset = tempHeight;
    if (openMode == 3)
      sceneOffset.y = offset - MINOPENH;
    if (openMode == 2)
      sceneOffset.y = offset / 2 - MINOPENH / 2;
    if (openMode < 2)
      sceneOffset.y = 0;
    needOpenness = openMode > 0;
  }

  void markControlVertDirty()
  {
    controlsVertDirty = true;
  }

  void refreshCursorRectCon()
  {
    cursorRectCon.disconnect();
    cursorRectCon = cursorRect->valueChanged.connect
        (sigc::mem_fun(this, &WindowPrivate::markControlVertDirty));
  }

  void buildBaseVert()
  {
    int w = size.x, h = size.y;
    if (needOpenness) {
      h = size.y * openness / 100;
      if (h < MINOPENH) h = MINOPENH;
      tempHeight = h;
      processOpenMode();
    }
    IntRect bgRect(2, 2, w - 4, h - 4);
    Sides<IntRect> borderRects;
    borderRects.l = IntRect(0, 8, padding, h-padding);
    borderRects.r = IntRect(w-padding, 8, padding, h-padding);
    borderRects.t = IntRect(8, 0, w-padding, padding);
    borderRects.b = IntRect(8, h-padding, w-padding, padding);
    Corners<IntRect> cornerRects;
    cornerRects.tl = IntRect(0, 0, padding, padding);
    cornerRects.tr = IntRect(w-padding, 0, padding, padding);
    cornerRects.bl = IntRect(0, h-padding, padding, padding);
    cornerRects.br = IntRect(w-padding, h-padding, padding, padding);
    int count = 0; // Required quad count
    if (bgStretch) // Background
      backgroundVert.count = 1;
    else
      backgroundVert.count =
        TileQuads::twoDimCount(128, 128, bgRect.w, bgRect.h);
    count += backgroundVert.count;
    // Borders (sides)
    count += TileQuads::oneDimCount(padding*2, w-padding) * 2;
    count += TileQuads::oneDimCount(padding*2, h-padding) * 2;
    count += 4; // Corners
    baseQuadArray.resize(count); // Our vertex array
    Vertex *vert = baseQuadArray.vertices.data();
    int i = 0;
    backgroundVert.vert = &vert[i];
    if (bgStretch) { // Background
      Quad::setTexRect(&vert[i*4], backgroundSrc);
      Quad::setPosRect(&vert[i*4], bgRect);
      i += 1;
    } else {
      i += TileQuads::build(backgroundSrc, bgRect, &vert[i*4]);
    }
    // Borders
    i += TileQuads::buildH(bordersSrc.t, w-padding, 8, 0, &vert[i*4]);
    i += TileQuads::buildH(bordersSrc.b, w-padding, 8, h-padding, &vert[i*4]);
    i += TileQuads::buildV(bordersSrc.l, h-padding, 0, 8, &vert[i*4]);
    i += TileQuads::buildV(bordersSrc.r, h-padding, w-padding, 8, &vert[i*4]);
    // Corners
    i += Quad::setTexPosRect(&vert[i*4], cornersSrc.tl, cornerRects.tl);
    i += Quad::setTexPosRect(&vert[i*4], cornersSrc.tr, cornerRects.tr);
    i += Quad::setTexPosRect(&vert[i*4], cornersSrc.bl, cornerRects.bl);
    i += Quad::setTexPosRect(&vert[i*4], cornersSrc.br, cornerRects.br);
    for (int j = 0; j < count*4; ++j)
      vert[j].color = Vec4(1, 1, 1, 1);
    FloatRect texRect = FloatRect(0, 0, size.x, h);
    baseTexQuad.setTexPosRect(texRect, texRect);
    opacityDirty = true;
    baseTexDirty = true;
  }

  void updateBaseAlpha()
  { // This is always applied unconditionally
    backgroundVert.setAlpha(backOpacity.norm);
    baseTexQuad.setColor(Vec4(1, 1, 1, opacity.norm));
    baseTexDirty = true;
  }

  void ensureBaseTexReady()
  { // Make sure texture is big enough
    int newW = baseTex.width;
    int newH = baseTex.height;
    bool resizeNeeded = false;
    if (size.x > baseTex.width) {
      newW = findNextPow2(size.x);
      resizeNeeded = true;
    }
    if (size.y > baseTex.height) {
      newH = findNextPow2(size.y);
      resizeNeeded = true;
    }
    if (!resizeNeeded) return;
    shState->texPool().release(baseTex);
    baseTex = shState->texPool().request(newW, newH);
    baseTexDirty = true;
  }

  void forceBaseTexRecreate()
  {
    shState->texPool().release(baseTex);
    baseTex = shState->texPool().request(baseTex.width, tempHeight);
  }

  void redrawBaseTex()
  { // Discard old buffer
    if (needOpenness) {
      if (openMode == 1)
        sceneOffset.y = 0;
      if (openMode == 2)
        sceneOffset.y = size.y / 2 - baseTex.height / 2;
      if (openMode == 3)
        sceneOffset.y = size.y - baseTex.height;
    }
    TEX::bind(baseTex.tex);
    TEX::allocEmpty(baseTex.width, baseTex.height);
    TEX::unbind();
    FBO::bind(baseTex.fbo);
    glState.viewport.pushSet(IntRect(0, 0, baseTex.width, baseTex.height));
    glState.clearColor.pushSet(Vec4());
    SimpleAlphaShader &shader = shState->shaders().simpleAlpha;
    shader.bind();
    shader.applyViewportProj();
    shader.setTranslation(Vec2i());
    FBO::clear(); // Clear texture
    windowskin->bindTex(shader); // Repaint base
    TEX::setSmooth(true);
    /* We need to blit the background without blending,
     * because we want to retain its correct alpha value.
     * Otherwise it would be mutliplied by the backgrounds 0 alpha */
    glState.blend.pushSet(false);
    baseQuadArray.draw(0, backgroundVert.count);
    /* Now draw the rest (ie. the frame) with blending */
    glState.blend.pop();
    glState.blendMode.pushSet(BlendNormal);
    baseQuadArray.draw(backgroundVert.count, baseQuadArray.count()-backgroundVert.count);
    glState.clearColor.pop();
    glState.blendMode.pop();
    glState.viewport.pop();
    TEX::setSmooth(false);
  }

  void buildControlsVert()
  {
    int i = 0;
    Vertex *vert = controlsQuadArray.vertices.data();
    if (!cursorRect->isEmpty()) {
      // Cursor: Effective cursor rect has 16 xy offset to window
      IntRect effectRect(cursorRect->x+padding, cursorRect->y+padding,
                         cursorRect->width, cursorRect->height);
      cursorVert.vert = &vert[i*4];
      TileQuads::buildFrameSource(cursorSrc, cursorVert.vert);
      i += TileQuads::buildFrame(effectRect, cursorVert.vert);
    } // Scroll arrow position: Top Bottom X, Left Right Y
    const Vec2i scroll = (size - Vec2i(16)) / 2;
    Sides<IntRect> scrollArrows;
    scrollArrows.l = IntRect(4, scroll.y, 8, 16);
    scrollArrows.r = IntRect(size.x - 12, scroll.y, 8, 16);
    scrollArrows.t = IntRect(scroll.x, 4, 16, 8);
    scrollArrows.b = IntRect(scroll.x, size.y - 12, 16, 8);
    if (contents) {
      if (contentsOffset.x > 0)
        i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.l, scrollArrows.l);
      if (contentsOffset.y > 0)
        i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.t, scrollArrows.t);
      if ((size.x - 32) < (contents->width() - contentsOffset.x))
        i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.r, scrollArrows.r);
      if ((size.y - 32) < (contents->height() - contentsOffset.y))
        i += Quad::setTexPosRect(&vert[i*4], scrollArrowSrc.b, scrollArrows.b);
    } // Pause animation
    if (pause) {
      pauseAniVert.vert = &vert[i*4];
      FloatRect rect = FloatRect((size.x - 16) / 2, size.y - 16, 16, 16);
      if (pause_x != 0)
        rect.x = pause_x;
      if (pause_y != 0)
        rect.y = pause_y;
      i += Quad::setTexPosRect(&vert[i*4], pauseAniSrc[pauseAniQuad[pauseAniQuadIdx]], rect);
    }
    controlsQuadArray.commit();
    controlsQuadCount = i;
  }

  void prepare()
  {
    if (size.x <= 0 || size.y <= 0)
      return;
    bool updateBaseQuadArray = false;
    if (baseVertDirty) {
      buildBaseVert();
      baseVertDirty = false;
      updateBaseQuadArray = true;
    }
    if (opacityDirty) {
      updateBaseAlpha();
      opacityDirty = false;
      updateBaseQuadArray = true;
    }
    if (updateBaseQuadArray)
      baseQuadArray.commit();
    /* If opacity has effect, we must prerender to a texture
     * and then draw this texture instead of the quad array */
    useBaseTex = opacity < 255;
    if (!useBaseTex)
      return;
    ensureBaseTexReady();
    if (!baseTexDirty)
      return;
    redrawBaseTex();
    baseTexDirty = false;
  }

  void drawBase()
  {
    if (nullOrDisposed(windowskin))
      return;
    if (size == Vec2i(0, 0))
      return;
    SimpleAlphaShader &shader = shState->shaders().simpleAlpha;
    shader.bind();
    shader.applyViewportProj();
    shader.setTranslation(position + sceneOffset);
    if (useBaseTex) {
      shader.setTexSize(Vec2i(baseTex.width, baseTex.height));
      TEX::bind(baseTex.tex);
      baseTexQuad.draw();
    } else {
      windowskin->bindTex(shader);
      TEX::setSmooth(true);
      baseQuadArray.draw();
      TEX::setSmooth(false);
    }
  }

  void drawControls()
  {
    if (nullOrDisposed(windowskin) && nullOrDisposed(contents))
      return;
    if (openness < 100 || size == Vec2i(0, 0))
      return;
    if (controlsVertDirty) {
      buildControlsVert();
      updateControls();
      controlsVertDirty = false;
    } // Effective on screen coordinates
    const Vec2i efPos = position + sceneOffset;
    const IntRect windowRect(efPos, size);
    const IntRect contentsRect(efPos + Vec2i(16), size - Vec2i(32));
    glState.scissorTest.pushSet(true);
    glState.scissorBox.push();
    glState.scissorBox.setIntersect(windowRect);
    SimpleAlphaShader &shader = shState->shaders().simpleAlpha;
    shader.bind();
    shader.applyViewportProj();
    if (!nullOrDisposed(windowskin)) {
      shader.setTranslation(efPos);
      // Draw arrows or cursors
      windowskin->bindTex(shader);
      TEX::setSmooth(true);
      controlsQuadArray.draw(0, controlsQuadCount);
      TEX::setSmooth(false);
    }
    if (!nullOrDisposed(contents)) { // Draw contents bitmap
      glState.scissorBox.setIntersect(contentsRect);
      shader.setTranslation(efPos + (Vec2i(16) - contentsOffset));
      contents->bindTex(shader);
      contentsQuad.draw();
    }
    glState.scissorBox.pop();
    glState.scissorTest.pop();
  }

  void updateControls()
  {
    if (needOpenness)
      return;
    bool updateArray = false;
    if (active && cursorVert.vert) {
      float alpha = cursorAniAlpha[cursorAniAlphaIdx] / 255.0f;
      cursorVert.setAlpha(alpha);
      updateArray = true;
    }
    if (pause && pauseAniVert.vert) {
      float alpha = pauseAniAlpha[pauseAniAlphaIdx] / 255.0f;
      FloatRect frameRect = pauseAniSrc[pauseAniQuad[pauseAniQuadIdx]];
      pauseAniVert.setAlpha(alpha);
      Quad::setTexRect(pauseAniVert.vert, frameRect);
      updateArray = true;
    }
    if (updateArray)
      controlsQuadArray.commit();
  }

  void stepAnimations()
  {
    if (++cursorAniAlphaIdx == cursorAniAlphaN)
      cursorAniAlphaIdx = 0;
    if (pauseAniAlphaIdx < pauseAniAlphaN-1)
      ++pauseAniAlphaIdx;
    if (++pauseAniQuadIdx == pauseAniQuadN)
      pauseAniQuadIdx = 0;
  }

  void updateOpenness()
  {
    if (!needOpenness)
      return;
    buildBaseVert();
    updateBaseAlpha();
    baseQuadArray.commit();
    forceBaseTexRecreate();
    redrawBaseTex();
    needOpenness = openness < 100;
    SimpleAlphaShader &shader = shState->shaders().simpleAlpha;
    shader.bind();
    shader.applyViewportProj();
    shader.setTranslation(position + sceneOffset);
    windowskin->bindTex(shader);
    TEX::setSmooth(true);
    baseQuadArray.draw();
    TEX::setSmooth(false);
    prepare();
  }
};

Window::Window(Viewport *viewport, int mode) : ViewportElement(viewport)
{
  p = new WindowPrivate(viewport, mode);
  if (p->openMode < 2) p->sceneOffset = scene->getGeometry().offset();
}

Window::~Window()
{
  dispose();
}

void Window::update()
{
  if (isDisposed())
    return;
  p->updateOpenness();
  p->updateControls();
  p->stepAnimations();
}

DEF_ATTR_RD_SIMPLE(Window, Windowskin,      Bitmap*, p->windowskin)
DEF_ATTR_RD_SIMPLE(Window, Contents,        Bitmap*, p->contents)
DEF_ATTR_RD_SIMPLE(Window, Stretch,         bool,    p->bgStretch)
DEF_ATTR_RD_SIMPLE(Window, Active,          bool,    p->active)
DEF_ATTR_RD_SIMPLE(Window, Width,           int,     p->size.x)
DEF_ATTR_RD_SIMPLE(Window, Height,          int,     p->size.y)
DEF_ATTR_RD_SIMPLE(Window, OX,              int,     p->contentsOffset.x)
DEF_ATTR_RD_SIMPLE(Window, OY,              int,     p->contentsOffset.y)
DEF_ATTR_RD_SIMPLE(Window, Opacity,         int,     p->opacity)
DEF_ATTR_RD_SIMPLE(Window, BackOpacity,     int,     p->backOpacity)
DEF_ATTR_RD_SIMPLE(Window, ContentsOpacity, int,     p->contentsOpacity)

int Window::get_x() const
{
  guardDisposed();
  return p->position.x;
}

int Window::get_y() const
{
  guardDisposed();
  return p->position.y;
}

bool Window::get_pause() const
{
  guardDisposed();
  return p->pause;
}

void Window::set_x(int value)
{
  guardDisposed();
  p->position.x = value;
}

void Window::set_y(int value)
{
  guardDisposed();
  p->position.y = value;
  p->needOpenness = p->openMode > 0;
  p->processOpenMode();
}

void Window::set_xy(int val, int num)
{
  guardDisposed();
  p->position.x = val;
  p->position.y = num;
  p->needOpenness = p->openMode > 0;
  p->processOpenMode();
}

void Window::set_pause_x(int x)
{
  guardDisposed();
  p->pause_x = x;
}

void Window::set_pause_y(int y)
{
  guardDisposed();
  p->pause_y = y;
}

void Window::set_pause_xy(int x, int y)
{
  guardDisposed();
  p->pause_x = x;
  p->pause_y = y;
}

void Window::setWidth(int value)
{
  guardDisposed();
  if (p->size.x == value)
    return;
  p->size.x = value;
  p->baseVertDirty = true;
}

void Window::setHeight(int value)
{
  guardDisposed();
  if (p->size.y == value)
    return;
  p->size.y = value;
  p->baseVertDirty = true;
  p->needOpenness = p->openMode > 0;
  p->processOpenMode();
}

void Window::setOX(int value)
{
  guardDisposed();
  if (p->contentsOffset.x == value)
    return;
  p->contentsOffset.x = value;
  p->controlsVertDirty = true;
}

void Window::setOY(int value)
{
  guardDisposed();
  if (p->contentsOffset.y == value)
    return;
  p->contentsOffset.y = value;
  p->controlsVertDirty = true;
}

Rect& Window::getCursorRect() const
{
  guardDisposed();
  return *p->cursorRect;
}

void Window::setCursorRect(Rect& rect)
{
  guardDisposed();
  *p->cursorRect = rect;
}

void Window::setWindowskin(Bitmap *value)
{
  guardDisposed();
  p->windowskin = value;
  if (nullOrDisposed(value))
    value->ensureNonMega();
}

void Window::setContents(Bitmap *value)
{
  guardDisposed();
  if (p->contents == value)
    return;
  p->contents = value;
  p->controlsVertDirty = p->openness == 100;
  if (nullOrDisposed(value))
    return;
  value->ensureNonMega();
  p->contentsQuad.setTexPosRect(value->rect(), value->rect());
}

void Window::setStretch(bool value)
{
  guardDisposed();
  if (value == p->bgStretch)
    return;
  p->bgStretch = value;
  p->baseVertDirty = true;
}

void Window::setActive(bool value)
{
  guardDisposed();
  if (p->active == value)
    return;
  p->active = value;
  p->cursorAniAlphaIdx = 0;
}

void Window::set_pause(bool value)
{
  guardDisposed();
  if (p->pause == value)
    return;
  p->pause = value;
  p->pauseAniAlphaIdx = 0;
  p->pauseAniQuadIdx = 0;
  p->controlsVertDirty = true;
}

int Window::getOpenMode() const
{
  guardDisposed();
  return p->openMode;
}

void Window::setOpenMode(int val)
{
  guardDisposed();
  p->openMode = (val < 0 && val > 3)? 0 : val;
  p->openness = p->openMode == 0 ? 100 : 0;
  p->processOpenMode();
  //setVisible(!(p->openness == 0 && p->openMode == 0));
  //p->updateOpenness();
}

int Window::getOpenness() const
{
  guardDisposed();
  return p->openness;
}

void Window::setOpenness(int val)
{
  guardDisposed();
  if (p->openness == val)
    return;
  p->openness = val < 0 ? 0 : val > 100 ? 100 : val;
  p->needOpenness = true;
  p->processOpenMode();
  setVisible(!(p->openness == 0 && p->openMode == 0));
  p->updateOpenness();
}

bool Window::isOpen() const
{
  guardDisposed();
  return (p->openMode == 0 || p->openness == 100);
}

bool Window::isClosed() const
{
  guardDisposed();
  return (p->openMode > 0 && p->openness == 0);
}

bool Window::is_mouse_inside() const
{
  guardDisposed();
  int mp = shState->input().mouseX() - p->position.x - p->padding / 2;
  if (mp < 4 || mp > p->size.x - 4)
    return false;
  mp = shState->input().mouseY() - p->position.y - p->padding / 2;
  return (mp >= 4 && mp <= p->size.y - 4);
}

bool Window::is_mouse_inside(int x, int y, int w, int h) const
{
  guardDisposed();
  int mp = shState->input().mouseX() - p->position.x - p->padding;
  if (mp < x || mp > x + w)
    return false;
  mp = shState->input().mouseY() - p->position.y - p->padding;
  return (mp >= y && mp <= y + h);
}

void Window::setOpacity(int value)
{
  guardDisposed();
  if (p->opacity == value)
    return;
  p->opacity = value;
  p->opacityDirty = true;
}

void Window::setBackOpacity(int value)
{
  guardDisposed();
  if (p->backOpacity == value)
    return;
  p->backOpacity = value;
  p->opacityDirty = true;
}

void Window::setContentsOpacity(int value)
{
  guardDisposed();
  if (p->contentsOpacity == value)
    return;
  p->contentsOpacity = value;
  p->contentsQuad.setColor(Vec4(1, 1, 1, p->contentsOpacity.norm));
}

void Window::initDynAttribs()
{
  p->cursorRect = new Rect;
  p->refreshCursorRectCon();
}

void Window::draw()
{
  p->drawBase();
}

void Window::onGeometryChange(const Scene::Geometry &geo)
{
  p->sceneOffset = geo.offset();
}

void Window::setZ(int value)
{
  ViewportElement::setZ(value);
  p->controlsElement.setZ(value + 2);
}

void Window::setVisible(bool value)
{
  ViewportElement::setVisible(value);
  p->controlsElement.setVisible(value);
}

void Window::onViewportChange()
{
  p->controlsElement.setScene(*this->scene);
}

void Window::releaseResources()
{
  p->controlsElement.release();
  unlink();
  delete p;
}

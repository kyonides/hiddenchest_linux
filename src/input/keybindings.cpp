/*
** keybindings.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
** Copyright (C) 2018-2026 Kyonides Arkanthes <kyonides@gmail.com>
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

#include "keybindings.h"
#include "config.h"
#include "util.h"
#include <stdio.h>
#include <iostream>
#include <regex>
#include "../binding/input_vendors.h"
#include "debugwriter.h"

struct KbBindingData
{
  SDL_Scancode source;
  Input::ButtonCode target;

  void add(BDescVec &d) const
  {
    SourceDesc src;
    src.type = Key;
    src.d.scan = source;
    BindingDesc desc;
    desc.src = src;
    desc.target = target;
    d.push_back(desc);
  }
};

struct JsBindingData
{
  int source;
  Input::ButtonCode target;

  void add(BDescVec &d) const
  {
    SourceDesc src;
    src.type = JButton;
    src.d.jb = source;
    BindingDesc desc;
    desc.src = src;
    desc.target = target;
    d.push_back(desc);
  }
};

// Common
static const KbBindingData defaultKbBindings[] =
{
  { SDL_SCANCODE_LEFT,     Input::Left  },
  { SDL_SCANCODE_RIGHT,    Input::Right },
  { SDL_SCANCODE_UP,       Input::Up    },
  { SDL_SCANCODE_DOWN,     Input::Down  },
  { SDL_SCANCODE_SPACE,    Input::C     },
  { SDL_SCANCODE_RETURN,   Input::C     },
  { SDL_SCANCODE_ESCAPE,   Input::B     },
  { SDL_SCANCODE_KP_0,     Input::B     },
  { SDL_SCANCODE_LSHIFT,   Input::A     },
  { SDL_SCANCODE_X,        Input::B     },
  { SDL_SCANCODE_D,        Input::Z     },
  { SDL_SCANCODE_Q,        Input::L     },
  { SDL_SCANCODE_W,        Input::R     },
  { SDL_SCANCODE_A,        Input::X     },
  { SDL_SCANCODE_S,        Input::Y     }
};

// RGSS1
static const KbBindingData defaultKbBindings1[] =
{
  { SDL_SCANCODE_Z,  Input::A },
  { SDL_SCANCODE_C,  Input::C },
};

// RGSS2 and higher
static const KbBindingData defaultKbBindings2[] =
{
  { SDL_SCANCODE_Z,  Input::C }
};

static const KbBindingData kb_enter_binding[] =
{
  { SDL_SCANCODE_KP_ENTER, Input::C },
};

static elementsN(defaultKbBindings);
static elementsN(defaultKbBindings1);
static elementsN(defaultKbBindings2);

static const JsBindingData defaultJsBindings[] =
{
  { 0, Input::A },
  { 1, Input::B },
  { 2, Input::C },
  { 3, Input::X },
  { 4, Input::Y },
  { 5, Input::Z },
  { 6, Input::L },
  { 7, Input::R }
};

static const JsBindingData ps_button_bindings[] =
{
  {  0, Input::C },
  {  1, Input::B },
  {  2, Input::X },
  {  3, Input::A },
  {  4, Input::Y },
  {  5, Input::Z },
  {  9, Input::L },
  { 10, Input::R },
  { 11, Input::Up    },
  { 12, Input::Down  },
  { 13, Input::Left  },
  { 14, Input::Right }
};

static elementsN(defaultJsBindings);

static void addAxisBinding(BDescVec &d, uint8_t axis, AxisDir dir, Input::ButtonCode target)
{
  SourceDesc src;
  src.type = JAxis;
  src.d.ja.axis = axis;
  src.d.ja.dir = dir;
  BindingDesc desc;
  desc.src = src;
  desc.target = target;
  d.push_back(desc);
}

static void addHatBinding(BDescVec &d, uint8_t hat, uint8_t pos, Input::ButtonCode target)
{
  SourceDesc src;
  src.type = JHat;
  src.d.jh.hat = hat;
  src.d.jh.pos = pos;
  BindingDesc desc;
  desc.src = src;
  desc.target = target;
  d.push_back(desc);
}

static void set_axis_buttons(BDescVec &d)
{
  addAxisBinding(d, 0, Negative, Input::Left );
  addAxisBinding(d, 0, Positive, Input::Right);
  addAxisBinding(d, 1, Negative, Input::Up   );
  addAxisBinding(d, 1, Positive, Input::Down );
}

static void set_ps_axis_buttons(BDescVec &d)
{
  addAxisBinding(d, 4, Negative, Input::L2);
  addAxisBinding(d, 4, Positive, Input::L2);
  addAxisBinding(d, 5, Negative, Input::R2);
  addAxisBinding(d, 5, Positive, Input::R2);
}

static void set_hat_buttons(BDescVec &d)
{
  addHatBinding(d, 0, SDL_HAT_LEFT,  Input::Left );
  addHatBinding(d, 0, SDL_HAT_RIGHT, Input::Right);
  addHatBinding(d, 0, SDL_HAT_UP,    Input::Up   );
  addHatBinding(d, 0, SDL_HAT_DOWN,  Input::Down );
}

/*
** input.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
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

#include "clicks.h"
#include "input.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "keybindings.h"
#include "exception.h"
#include "util.h"
#include <SDL_events.h>
#include <SDL_scancode.h>
#include <SDL_mouse.h>
#include <SDL_joystick.h>
#include <utility>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "debugwriter.h"

struct ButtonState
{
  bool pressed, triggered, repeated;
  ButtonState()
  : pressed(false),
    triggered(false),
    repeated(false)
  {}
};

struct KbBindingData
{
  SDL_Scancode source;
  Input::ButtonCode target;
};

struct Binding
{
  Binding(Input::ButtonCode target = Input::None, uint16_t source = 0)
  : target(target),
    source(source),
    index(0),
    type(0),
    extra(0)
  {}
  virtual bool sourceActive() const = 0;
  virtual bool sourceRepeatable() const = 0;

  Input::ButtonCode target;
  unsigned int type;
  unsigned int extra;
  unsigned int index;
  uint16_t source;
};

// Keyboard binding
struct KbBinding : public Binding
{
  KbBinding() {}
  KbBinding(const KbBindingData &data)
  : Binding(data.target, data.source)
  {
    type = 1;
  }

  bool sourceActive() const
  { // Special case aliases
    if (source == SDL_SCANCODE_LSHIFT)
      return EventThread::keyStates[source]
             || EventThread::keyStates[SDL_SCANCODE_RSHIFT];
    if (source == SDL_SCANCODE_RETURN)
      return EventThread::keyStates[source] ||
             EventThread::keyStates[SDL_SCANCODE_KP_ENTER];
    return EventThread::keyStates[source];
  }

  bool sourceRepeatable() const
  {
    return true;
  }
};
// Joystick button binding
struct JsButtonBinding : public Binding
{
  JsButtonBinding()
  {
    type = 2;
  }

  bool sourceActive() const
  {
    return EventThread::joyStates[index].buttons[source];
  }

  bool sourceRepeatable() const
  {
    return true;
  }
};

// Joystick axis binding
struct JsAxisBinding : public Binding
{
  JsAxisBinding() {}
  JsAxisBinding(uint16_t source,
                AxisDir dir,
                Input::ButtonCode target)
  : Binding(target, source),
    dir(dir)
  {
    type = 3;
    extra = dir;
  }

  bool sourceActive() const
  {
    int val = EventThread::joyStates[index].axes[source];
    return (dir == Negative) ? val < -JAXIS_THRESHOLD : val > JAXIS_THRESHOLD;
  }

  bool sourceRepeatable() const
  {
    return true;
  }
  AxisDir dir;
};

// Joystick hat binding
struct JsHatBinding : public Binding
{
  JsHatBinding() {}
  JsHatBinding(uint16_t source,
               uint8_t pos,
               Input::ButtonCode target)
  : Binding(target, source),
    pos(pos)
  {
    type = 4;
    extra = pos;
  }

  bool sourceActive() const
  { // For a diagonal input accept it as an input for both the axes
    return (pos & EventThread::joyStates[index].hats[source]) != 0;
  }

  bool sourceRepeatable() const
  {
    return true;
  }
  uint8_t pos;
};

// Mouse button binding
struct MsBinding : public Binding
{
  MsBinding() {}
  MsBinding(int buttonIndex, Input::ButtonCode target)
      : Binding(target), index(buttonIndex)
  {}

  bool sourceActive() const
  {
    return EventThread::mouseState.buttons[index];
  }
  bool sourceRepeatable() const
  {
    return true;
  }

  int index;
};

static const KbBindingData kb_dir4_bindings[] =
{
  { SDL_SCANCODE_DOWN,           Input::Down  },
  { SDL_SCANCODE_LEFT,           Input::Left  },
  { SDL_SCANCODE_RIGHT,          Input::Right },
  { SDL_SCANCODE_UP,             Input::Up    }
};

// Not rebindable
static const KbBindingData staticKbBindings[] =
{
  //{ SDL_SCANCODE_LSHIFT,       Input::Shift },
  //{ SDL_SCANCODE_RSHIFT,       Input::Shift },
  //{ SDL_SCANCODE_LCTRL,        Input::Ctrl  },
  //{ SDL_SCANCODE_RCTRL,        Input::Ctrl  },
  { SDL_SCANCODE_A,              Input::KeyA },
  { SDL_SCANCODE_B,              Input::KeyB },
  { SDL_SCANCODE_C,              Input::KeyC },
  { SDL_SCANCODE_D,              Input::KeyD },
  { SDL_SCANCODE_E,              Input::KeyE },
  { SDL_SCANCODE_F,              Input::KeyF },
  { SDL_SCANCODE_G,              Input::KeyG },
  { SDL_SCANCODE_H,              Input::KeyH },
  { SDL_SCANCODE_I,              Input::KeyI },
  { SDL_SCANCODE_J,              Input::KeyJ },
  { SDL_SCANCODE_K,              Input::KeyK },
  { SDL_SCANCODE_L,              Input::KeyL },
  { SDL_SCANCODE_M,              Input::KeyM },
  { SDL_SCANCODE_N,              Input::KeyN },
  { SDL_SCANCODE_O,              Input::KeyO },
  { SDL_SCANCODE_P,              Input::KeyP },
  { SDL_SCANCODE_Q,              Input::KeyQ },
  { SDL_SCANCODE_R,              Input::KeyR },
  { SDL_SCANCODE_S,              Input::KeyS },
  { SDL_SCANCODE_T,              Input::KeyT },
  { SDL_SCANCODE_U,              Input::KeyU },
  { SDL_SCANCODE_V,              Input::KeyV },
  { SDL_SCANCODE_W,              Input::KeyW },
  { SDL_SCANCODE_X,              Input::KeyX },
  { SDL_SCANCODE_Y,              Input::KeyY },
  { SDL_SCANCODE_Z,              Input::KeyZ },
  { SDL_SCANCODE_F1,             Input::F1   },
  { SDL_SCANCODE_F2,             Input::F2   },
  { SDL_SCANCODE_F3,             Input::F3   },
  { SDL_SCANCODE_F4,             Input::F4   },
  { SDL_SCANCODE_F5,             Input::F5   },
  { SDL_SCANCODE_F6,             Input::F6   },
  { SDL_SCANCODE_F7,             Input::F7   },
  { SDL_SCANCODE_F8,             Input::F8   },
  { SDL_SCANCODE_F9,             Input::F9   },
  { SDL_SCANCODE_F10,            Input::F10  },
  { SDL_SCANCODE_F11,            Input::F11  },
  { SDL_SCANCODE_F12,            Input::F12  },
  { SDL_SCANCODE_1,              Input::N1   },
  { SDL_SCANCODE_2,              Input::N2   },
  { SDL_SCANCODE_3,              Input::N3   },
  { SDL_SCANCODE_4,              Input::N4   },
  { SDL_SCANCODE_5,              Input::N5   },
  { SDL_SCANCODE_6,              Input::N6   },
  { SDL_SCANCODE_7,              Input::N7   },
  { SDL_SCANCODE_8,              Input::N8   },
  { SDL_SCANCODE_9,              Input::N9   },
  { SDL_SCANCODE_0,              Input::N0   },
  { SDL_SCANCODE_RETURN,         Input::Return         },
  { SDL_SCANCODE_ESCAPE,         Input::Escape         },
  { SDL_SCANCODE_BACKSPACE,      Input::Backspace      },
  { SDL_SCANCODE_SPACE,          Input::Space          },
  { SDL_SCANCODE_TAB,            Input::Tab            },
  { SDL_SCANCODE_MINUS,          Input::Minus          },
  { SDL_SCANCODE_EQUALS,         Input::Equals         },
  { SDL_SCANCODE_LEFTBRACKET,    Input::LeftBracket    },
  { SDL_SCANCODE_RIGHTBRACKET,   Input::RightBracket   },
  { SDL_SCANCODE_BACKSLASH,      Input::BackSlash      },
  { SDL_SCANCODE_SEMICOLON,      Input::Semicolon      },
  { SDL_SCANCODE_APOSTROPHE,     Input::Apostrophe     },
  { SDL_SCANCODE_GRAVE,          Input::Grave          },
  { SDL_SCANCODE_COMMA,          Input::Comma          },
  { SDL_SCANCODE_PERIOD,         Input::Period         },
  { SDL_SCANCODE_SLASH,          Input::Slash          },
  { SDL_SCANCODE_CAPSLOCK,       Input::CapsLock       },
  { SDL_SCANCODE_SCROLLLOCK,     Input::ScrollLock     },
  { SDL_SCANCODE_PAUSE,          Input::Pause          },
  { SDL_SCANCODE_INSERT,         Input::Insert         },
  { SDL_SCANCODE_HOME,           Input::Home           },
  { SDL_SCANCODE_PAGEUP,         Input::PageUp         },
  { SDL_SCANCODE_DELETE,         Input::Delete         },
  { SDL_SCANCODE_END,            Input::End            },
  { SDL_SCANCODE_PAGEDOWN,       Input::PageDown       },
  { SDL_SCANCODE_KP_DIVIDE,      Input::NumPadDivide   },
  { SDL_SCANCODE_KP_MULTIPLY,    Input::NumPadMultiply },
  { SDL_SCANCODE_KP_MINUS,       Input::NumPadMinus    },
  { SDL_SCANCODE_KP_PLUS,        Input::NumPadPlus     },
  { SDL_SCANCODE_KP_ENTER,       Input::Enter          },
  { SDL_SCANCODE_KP_1,           Input::NumPad1        },
  { SDL_SCANCODE_KP_2,           Input::NumPad2        },
  { SDL_SCANCODE_KP_3,           Input::NumPad3        },
  { SDL_SCANCODE_KP_4,           Input::NumPad4        },
  { SDL_SCANCODE_KP_5,           Input::NumPad5        },
  { SDL_SCANCODE_KP_6,           Input::NumPad6        },
  { SDL_SCANCODE_KP_7,           Input::NumPad7        },
  { SDL_SCANCODE_KP_8,           Input::NumPad8        },
  { SDL_SCANCODE_KP_9,           Input::NumPad9        },
  { SDL_SCANCODE_KP_0,           Input::NumPad0        },
  { SDL_SCANCODE_KP_PERIOD,      Input::NumPadDot      },
  { SDL_SCANCODE_NONUSBACKSLASH, Input::LessOrGreater  },
  { SDL_SCANCODE_APPLICATION,    Input::APP            },
  { SDL_SCANCODE_KP_EQUALS,      Input::NumPadEquals   },
  { SDL_SCANCODE_LCTRL,          Input::LeftCtrl       },
  { SDL_SCANCODE_LSHIFT,         Input::LeftShift      },
  { SDL_SCANCODE_LALT,           Input::LeftAlt        },
  { SDL_SCANCODE_LGUI,           Input::LeftMeta       },
  { SDL_SCANCODE_RCTRL,          Input::RightCtrl      },
  { SDL_SCANCODE_RSHIFT,         Input::RightShift     },
  { SDL_SCANCODE_RALT,           Input::RightAlt       },
  { SDL_SCANCODE_RGUI,           Input::RightMeta      },
  { SDL_SCANCODE_WWW,            Input::Web            },
  { SDL_SCANCODE_MAIL,           Input::Mail           },
  { SDL_SCANCODE_CALCULATOR,     Input::Calculator     },
  { SDL_SCANCODE_COMPUTER,       Input::Computer       },
  { SDL_SCANCODE_APP1,           Input::APP1           },
  { SDL_SCANCODE_APP2,           Input::APP2           }
};

static elementsN(staticKbBindings);

// Keys for Text Input - Not rebindable
static const KbBindingData char_kb_bindings[] =
{
	{ SDL_SCANCODE_LEFT,           Input::Left  },
	{ SDL_SCANCODE_RIGHT,          Input::Right },
  { SDL_SCANCODE_LSHIFT,         Input::Shift },
  { SDL_SCANCODE_RSHIFT,         Input::Shift },
  { SDL_SCANCODE_LCTRL,          Input::Ctrl  },
  { SDL_SCANCODE_RCTRL,          Input::Ctrl  },
  { SDL_SCANCODE_A,              Input::KeyA  },
  { SDL_SCANCODE_B,              Input::KeyB  },
  { SDL_SCANCODE_C,              Input::KeyC  },
  { SDL_SCANCODE_D,              Input::KeyD  },
  { SDL_SCANCODE_E,              Input::KeyE  },
  { SDL_SCANCODE_F,              Input::KeyF  },
  { SDL_SCANCODE_G,              Input::KeyG  },
  { SDL_SCANCODE_H,              Input::KeyH  },
  { SDL_SCANCODE_I,              Input::KeyI  },
  { SDL_SCANCODE_J,              Input::KeyJ  },
  { SDL_SCANCODE_K,              Input::KeyK  },
  { SDL_SCANCODE_L,              Input::KeyL  },
  { SDL_SCANCODE_M,              Input::KeyM  },
  { SDL_SCANCODE_N,              Input::KeyN  },
  { SDL_SCANCODE_O,              Input::KeyO  },
  { SDL_SCANCODE_P,              Input::KeyP  },
  { SDL_SCANCODE_Q,              Input::KeyQ  },
  { SDL_SCANCODE_R,              Input::KeyR  },
  { SDL_SCANCODE_S,              Input::KeyS  },
  { SDL_SCANCODE_T,              Input::KeyT  },
  { SDL_SCANCODE_U,              Input::KeyU  },
  { SDL_SCANCODE_V,              Input::KeyV  },
  { SDL_SCANCODE_W,              Input::KeyW  },
  { SDL_SCANCODE_X,              Input::KeyX  },
  { SDL_SCANCODE_Y,              Input::KeyY  },
  { SDL_SCANCODE_Z,              Input::KeyZ  },
  { SDL_SCANCODE_1,              Input::N1    },
  { SDL_SCANCODE_2,              Input::N2    },
  { SDL_SCANCODE_3,              Input::N3    },
  { SDL_SCANCODE_4,              Input::N4    },
  { SDL_SCANCODE_5,              Input::N5    },
  { SDL_SCANCODE_6,              Input::N6    },
  { SDL_SCANCODE_7,              Input::N7    },
  { SDL_SCANCODE_8,              Input::N8    },
  { SDL_SCANCODE_9,              Input::N9    },
  { SDL_SCANCODE_0,              Input::N0    },
  { SDL_SCANCODE_RETURN,         Input::Return         },
  { SDL_SCANCODE_ESCAPE,         Input::Escape         },
  { SDL_SCANCODE_BACKSPACE,      Input::Backspace      },
  { SDL_SCANCODE_SPACE,          Input::Space          },
  { SDL_SCANCODE_MINUS,          Input::Minus          },
  { SDL_SCANCODE_EQUALS,         Input::Equals         },
  { SDL_SCANCODE_LEFTBRACKET,    Input::LeftBracket    },
  { SDL_SCANCODE_RIGHTBRACKET,   Input::RightBracket   },
  { SDL_SCANCODE_BACKSLASH,      Input::BackSlash      },
  { SDL_SCANCODE_SEMICOLON,      Input::Semicolon      },
  { SDL_SCANCODE_APOSTROPHE,     Input::Apostrophe     },
  { SDL_SCANCODE_GRAVE,          Input::Grave          },
  { SDL_SCANCODE_COMMA,          Input::Comma          },
  { SDL_SCANCODE_PERIOD,         Input::Period         },
  { SDL_SCANCODE_SLASH,          Input::Slash          },
  { SDL_SCANCODE_CAPSLOCK,       Input::CapsLock       },
  { SDL_SCANCODE_PAUSE,          Input::Pause          },
  { SDL_SCANCODE_INSERT,         Input::Insert         },
  { SDL_SCANCODE_DELETE,         Input::Delete         },
  { SDL_SCANCODE_KP_MINUS,       Input::NumPadMinus    },
  { SDL_SCANCODE_KP_PLUS,        Input::NumPadPlus     },
  { SDL_SCANCODE_KP_ENTER,       Input::Enter          },
  { SDL_SCANCODE_KP_1,           Input::NumPad1        },
  { SDL_SCANCODE_KP_2,           Input::NumPad2        },
  { SDL_SCANCODE_KP_3,           Input::NumPad3        },
  { SDL_SCANCODE_KP_4,           Input::NumPad4        },
  { SDL_SCANCODE_KP_5,           Input::NumPad5        },
  { SDL_SCANCODE_KP_6,           Input::NumPad6        },
  { SDL_SCANCODE_KP_7,           Input::NumPad7        },
  { SDL_SCANCODE_KP_8,           Input::NumPad8        },
  { SDL_SCANCODE_KP_9,           Input::NumPad9        },
  { SDL_SCANCODE_KP_0,           Input::NumPad0        },
  { SDL_SCANCODE_KP_PERIOD,      Input::NumPadDot      },
  { SDL_SCANCODE_NONUSBACKSLASH, Input::LessOrGreater  },
  { SDL_SCANCODE_KP_EQUALS,      Input::NumPadEquals   },
};

static elementsN(char_kb_bindings);

static const Input::ButtonCode dirs[] =
{ Input::Down, Input::Left, Input::Right, Input::Up };

static const int dirFlags[] =
{
  1 << Input::Down,
  1 << Input::Left,
  1 << Input::Right,
  1 << Input::Up
};

/* Dir4 is always zero on these combinations */
static const int deadDirFlags[] =
{
  dirFlags[0] | dirFlags[3],
  dirFlags[1] | dirFlags[2]
};

static const Input::ButtonCode otherDirs[4][3] =
{
  { Input::Left, Input::Right, Input::Up    }, /* Down  */
  { Input::Down, Input::Up,    Input::Right }, /* Left  */
  { Input::Down, Input::Up,    Input::Left  }, /* Right */
  { Input::Left, Input::Right, Input::Up    }  /* Up    */
};

struct InputPrivate
{
  std::vector<KbBinding> kbStatBindings;
  std::vector<KbBinding> kbBindings;
  std::vector<KbBinding> min_kb_bindings;
  std::vector<KbBinding> dir4_kb_bindings;
  std::vector<JsAxisBinding> jsABindings;
  std::vector<JsHatBinding> jsHBindings;
  std::vector<JsButtonBinding> jsBBindings;
  std::vector<MsBinding> msBindings;
  // Collective binding array
  std::vector<Binding*> bindings;
  std::vector<Binding*> text_bindings;
  std::vector<Binding*> bind_bindings;
  ButtonState stateArray[520 * 2];
  ButtonState *states;
  ButtonState *statesOld;
  ButtonState *text_states;
  ButtonState *text_states_old;
  ButtonState *bind_states;
  ButtonState *bind_states_old;
  Input::ButtonCode btn;
  Input::ButtonCode repeating;
  Input::ButtonCode trigger_old;
  Input::ButtonCode trigger_new;
  Input *input;
  unsigned int repeatCount;
  unsigned int trigger_timer;
  unsigned int trigger_base_timer;
  unsigned int click_timer;
  unsigned int click_base_timer;
  unsigned int clicks;
  unsigned int double_target;
  unsigned int index;
  int ox;
  int oy;
  int last_mx;
  int last_my;
  int scroll_x;
  int scroll_y;
  int old_scroll_x;
  int old_scroll_y;
  int scroll_factor;
  int scroll_remainder;
  int trigger_now;
  int last_input;
  unsigned int trigger_kind;
  unsigned int trigger_js_value;
  unsigned int trigger_js_axis;
  unsigned int trigger_js_dir;
  bool same_mouse_pos;
  bool press_any = false;
  bool trigger_any = false;

  struct
  {
    int active;
    Input::ButtonCode previous;
  } dir4Data;

  struct
  {
    int active;
  } dir8Data;

  InputPrivate(const RGSSThreadData &rtData, int pos, Input *parent)
  {
    index = pos;
    input = parent;
    last_input = 0;
    same_mouse_pos = false;
    init_char_kb_bindings();
    initStaticKbBindings();
    initMsBindings();
    states    = stateArray;
    statesOld = stateArray + 520; // + BUTTON_CODE_COUNT;
    text_states     = stateArray;
    text_states_old = stateArray + 520;
    bind_states = stateArray;
    bind_states_old = stateArray + 520;
    scroll_factor = SCROLL_FACTOR;
    // Clear buffers
    clearBuffer();
    swapBuffers();
    clearBuffer();
    clear_text_buffer();
    swap_text_buffers();
    clear_text_buffer();
    clear_bind_buffer();
    swap_bind_buffers();
    clear_bind_buffer();
    trigger_now = 0;
    trigger_old = Input::None;
    trigger_new = Input::None;
    trigger_kind = 0;
    trigger_js_value = 0;
    trigger_js_axis = 0;
    trigger_js_dir = 0;
    repeating = Input::None;
    repeatCount = 0;
    ox = 8;
    oy = -4;
    dir4Data.active = 0;
    dir4Data.previous = Input::None;
    dir8Data.active = 0;
  }

  inline ButtonState &getStateCheck(int code)
  {
    return (code < 0 ? states[0] : states[code]);
  }

  inline ButtonState &getState(Input::ButtonCode code)
  {
    return (code < 0 ? states[0] : states[code]);
  }

  inline ButtonState &getOldState(Input::ButtonCode code)
  {
    return (code < 0 ? states[0] : statesOld[code]);
  }
  
  void swapBuffers()
  {
    ButtonState *tmp = states;
    states = statesOld;
    statesOld = tmp;
  }

  void clearBuffer()
  {
    memset(states, 0, 300 * 2);
    press_any = false;
    trigger_any = false;
  }

  inline ButtonState &get_text_state_check(int code)
  {
    return (code < 0 ? text_states[0] : text_states[code]);
  }

  inline ButtonState &get_text_state(Input::ButtonCode code)
  {
    return (code < 0 ? text_states[0] : text_states[code]);
  }

  inline ButtonState &get_text_old_state(Input::ButtonCode code)
  {
    return (code < 0 ? text_states[0] : text_states_old[code]);
  }
  
  void swap_text_buffers()
  {
    ButtonState *tmp = text_states;
    text_states = text_states_old;
    text_states_old = tmp;
  }

  void clear_text_buffer()
  {
    memset(text_states, 0, 300 * 2);
    press_any = false;
    trigger_any = false;
  }

  inline ButtonState &get_bind_state_check(int code)
  {
    return (code < 0 ? bind_states[0] : bind_states[code]);
  }

  inline ButtonState &get_bind_state(Input::ButtonCode code)
  {
    return (code < 0 ? bind_states[0] : bind_states[code]);
  }

  inline ButtonState &get_bind_old_state(Input::ButtonCode code)
  {
    return (code < 0 ? bind_states[0] : bind_states_old[code]);
  }
  
  void swap_bind_buffers()
  {
    ButtonState *tmp = bind_states;
    bind_states = bind_states_old;
    bind_states_old = tmp;
  }

  void clear_bind_buffer()
  {
    memset(bind_states, 0, 300 * 2);
    press_any = false;
    trigger_any = false;
  }

  inline ButtonState &get_this_state_check(int code)
  {
    switch (Input::text_input)
    {
    case 0:
      return getStateCheck(code);
    case 1:
      return get_text_state_check(code);
    default:
      return get_bind_state_check(code);
    }
  }

  void checkBindingChange(RGSSThreadData &rtData)
  {
    BDescVec d;
    if (!rtData.poll_bindings(index, d))
      return;
    Debug() << "Gamepad #" << index << "Bindings";
    Debug() << "Change Type:" << rtData.joystick_change;
    applyBindingDesc(d);
    rtData.joystick_change = 0;
  }

  void check_text_input_state(const RGSSThreadData &rtData)
  {
    BDescVec d;
    if (Input::text_input == last_input)
      return;
    rtData.bindingUpdateMsg.poll(d);
    apply_key_input_binding(d);
    last_input = Input::text_input;
  }

  void update_bindings_index()
  {
    for (size_t i = 0; i < bindings.size(); i++)
      bindings[i]->index = index;
  }

  template<class B>
  void appendBindings(std::vector<B> &bind)
  {
    for (size_t i = 0; i < bind.size(); i++)
      bindings.push_back(&bind[i]);
  }

  template<class B>
  void append_text_bindings(std::vector<B> &bind)
  {
    for (size_t i = 0; i < bind.size(); i++)
      text_bindings.push_back(&bind[i]);
  }

  template<class B>
  void append_bind_bindings(std::vector<B> &bind)
  {
    for (size_t i = 0; i < bind.size(); i++)
      bind_bindings.push_back(&bind[i]);
  }

  void applyBindingDesc(const BDescVec &d)
  {
    kbBindings.clear();
    jsABindings.clear();
    jsHBindings.clear();
    jsBBindings.clear();
    for (size_t i = 0; i < d.size(); ++i) {
      const BindingDesc &desc = d[i];
      const SourceDesc &src = desc.src;
      if (desc.target == Input::None)
        continue;
      switch (desc.src.type)
      {
      case Invalid :
        break;
      case Key :
      {
        KbBinding bind;
        bind.source = src.d.scan;
        bind.target = desc.target;
        kbBindings.push_back(bind);
        break;
      }
      case JAxis :
      {
        JsAxisBinding bind;
        bind.index = index;
        bind.type = 3;
        bind.source = src.d.ja.axis;
        bind.dir = src.d.ja.dir;
        bind.extra = src.d.ja.dir;
        bind.target = desc.target;
        jsABindings.push_back(bind);
        break;
      }
      case JHat :
      {
        JsHatBinding bind;
        bind.index = index;
        bind.type = 4;
        bind.source = src.d.jh.hat;
        bind.pos = src.d.jh.pos;
        bind.extra = src.d.jh.pos;
        bind.target = desc.target;
        jsHBindings.push_back(bind);
        break;
      }
      case JButton :
      {
        JsButtonBinding bind;
        bind.index = index;
        bind.source = src.d.jb;
        bind.target = desc.target;
        jsBBindings.push_back(bind);
        break;
      }
      default :
        assert(!"unreachable");
      }
    }
    bindings.clear();
    appendBindings(kbStatBindings);
    appendBindings(msBindings);
    appendBindings(kbBindings);
    appendBindings(jsABindings);
    appendBindings(jsHBindings);
    appendBindings(jsBBindings);
    text_bindings.clear();
    append_text_bindings(min_kb_bindings);
    append_text_bindings(msBindings);
  }

  void apply_key_input_binding(const BDescVec &d)
  {
    bind_bindings.clear();
    append_bind_bindings(dir4_kb_bindings);
    append_bind_bindings(kbStatBindings);
    append_bind_bindings(msBindings);
    append_bind_bindings(jsABindings);
    append_bind_bindings(jsHBindings);
    append_bind_bindings(jsBBindings);
  }

  void init_char_kb_bindings()
  {
    trigger_base_timer = TRIGGER_TIMER;
    trigger_timer = 0;
    min_kb_bindings.clear();
    dir4_kb_bindings.clear();
    for (size_t i = 0; i < char_kb_bindingsN; ++i)
      min_kb_bindings.push_back(KbBinding(char_kb_bindings[i]));
    for (size_t i = 0; i < 4; ++i)
      dir4_kb_bindings.push_back(KbBinding(kb_dir4_bindings[i]));
  }

  void initStaticKbBindings()
  {
    trigger_base_timer = TRIGGER_TIMER;
    trigger_timer = 0;
    kbStatBindings.clear();
    for (size_t i = 0; i < staticKbBindingsN; ++i)
      kbStatBindings.push_back(KbBinding(staticKbBindings[i]));
  }

  void initMsBindings()
  {
    click_base_timer = CLICK_TIMER;
    click_timer = 0;
    clicks = 0;
    last_mx = 0;
    last_my = 0;
    reset_scroll_xy();
    msBindings.resize(3);
    size_t i = 0;
    msBindings[i++] = MsBinding(SDL_BUTTON_LEFT,   Input::MouseLeft);
    msBindings[i++] = MsBinding(SDL_BUTTON_MIDDLE, Input::MouseMiddle);
    msBindings[i++] = MsBinding(SDL_BUTTON_RIGHT,  Input::MouseRight);
  }

  void reset_scroll_xy()
  {
    old_scroll_x = scroll_x = 0;
    old_scroll_y = scroll_y = 0;
  }

  void update_timers()
  {
    if (trigger_timer > 0)
      trigger_timer--;
    else
      trigger_old = Input::None;
    if (EventThread::mouseState.scrolled_x) {
      scroll_remainder = scroll_x - old_scroll_x;
      if (scroll_remainder == scroll_factor || scroll_remainder == -scroll_factor)
        old_scroll_x = scroll_x;
      scroll_x -= EventThread::mouseState.scroll_x;
    }
    if (EventThread::mouseState.scrolled_y) {
      scroll_remainder = scroll_y - old_scroll_y;
      if (scroll_remainder == scroll_factor || scroll_remainder == -scroll_factor)
        old_scroll_y = scroll_y;
      scroll_y -= EventThread::mouseState.scroll_y;
    }
    EventThread::mouseState.scroll_x = 0;
    EventThread::mouseState.scroll_y = 0;
    if (click_timer > 0)
      click_timer--;
    else
      double_target = clicks = 0;
  }

  void pollBindings(Input::ButtonCode &repeat_btn)
  {
    for (size_t i = 0; i < bindings.size(); ++i)
      pollBindingPriv(*bindings[i], repeat_btn);
    poll_alt_ctrl_shift_main();
    updateDir4();
    updateDir8();
  }

  void pollBindingPriv(const Binding &b, Input::ButtonCode &repeat_btn)
  {
    if (!b.sourceActive())
      return;
    btn = b.target;
    if (btn == Input::None)
      return;
    ButtonState &state = getState(btn);
    ButtonState &oldState = getOldState(btn);
    state.pressed = true;
    press_any = true;
    // Must have been released before to trigger it
    if (!oldState.pressed) {
      state.triggered = true;
      trigger_any = true;
      trigger_old = trigger_new;
      trigger_new = btn;
      if (trigger_old == btn && trigger_timer == 0)
        trigger_old = Input::None;
      if (trigger_old != btn)
        trigger_timer = trigger_base_timer;
      // Double Click Check
      if (btn == Input::MouseLeft || btn == Input::MouseRight) {
        clicks = (btn == double_target)? 2 : 1;
        if (clicks == 1) {
          set_click(btn);
        } else if (clicks == 2) {
          click_timer = 0;
          same_mouse_pos = (input->mouseX() == last_mx && input->mouseY() == last_my);
        }
      } else {
        clear_unused_clicks();
      }
    }
    if (repeating != btn && !oldState.pressed)
      repeat_btn = btn;
  }

  void poll_bindings4text(Input::ButtonCode &repeat_btn)
  {
    for (size_t i = 0; i < text_bindings.size(); ++i)
      poll_binding_text_priv(*text_bindings[i], repeat_btn);
    update_dir4();
  }

  void poll_binding_text_priv(const Binding &b, Input::ButtonCode &repeat_btn)
  {
    if (!b.sourceActive())
      return;
    btn = b.target;
    if (btn == Input::None)
      return;
    ButtonState &state = get_text_state(btn);
    ButtonState &state_old = get_text_old_state(btn);
    state.pressed = true;
    press_any = true;
    // Must have been released before to trigger it
    if (!state_old.pressed) {
      state.triggered = true;
      trigger_any = true;
      trigger_old = trigger_new;
      trigger_new = btn;
      trigger_now = btn;
      if (trigger_old == btn && trigger_timer == 0)
        trigger_old = Input::None;
      if (trigger_old != btn)
        trigger_timer = trigger_base_timer;
      // Double Click Check
      if (btn == Input::MouseLeft || btn == Input::MouseRight) {
        clicks = (btn == double_target)? 2 : 1;
        if (clicks == 1) {
          set_click(btn);
        } else if (clicks == 2) {
          click_timer = 0;
          same_mouse_pos = (input->mouseX() == last_mx && input->mouseY() == last_my);
        }
      } else {
        clear_unused_clicks();
      }
    }
    if (repeating != btn && !state_old.pressed)
      repeat_btn = btn;
  }

  void poll_bindings4bind()
  {
    for (size_t i = 0; i < bind_bindings.size(); ++i)
      poll_binding_bind_priv(*bind_bindings[i]);
  }

  void poll_binding_bind_priv(const Binding &b)
  {
    if (!b.sourceActive())
      return;
    btn = b.target;
    if (btn == Input::None)
      return;
    ButtonState &state = get_bind_state(btn);
    ButtonState &state_old = get_bind_old_state(btn);
    state.pressed = true;
    press_any = true;
    if (state_old.pressed)
      return;
    state.triggered = true;
    trigger_any = true;
    trigger_old = trigger_new;
    trigger_new = btn;
    trigger_now = btn;
    trigger_kind = b.type;
    if (trigger_kind > 1)
      trigger_js_value = b.source;
    if (trigger_kind > 2)
      trigger_js_dir = b.extra;
  }

  void set_click(Input::ButtonCode button)
  {
    click_timer = click_base_timer;
    double_target = button;
    last_mx = input->mouseX();
    last_my = input->mouseY();
  }

  void clear_unused_clicks()
  {
    click_timer = clicks = 0;
    last_mx = last_my = 0;
    double_target = 0;
    same_mouse_pos = false;
  }

  void updateDir4()
  {
    int dirFlag = 0;
    for (size_t i = 0; i < 4; ++i)
      dirFlag |= (getState(dirs[i]).pressed ? dirFlags[i] : 0);
    if (dirFlag == deadDirFlags[0] || dirFlag == deadDirFlags[1]) {
      dir4Data.active = Input::None;
      return;
    }
    if (dir4Data.previous != Input::None) {// Check if prev still pressed
      if (getState(dir4Data.previous).pressed) {
        for (size_t i = 0; i < 3; ++i) {
          Input::ButtonCode other = otherDirs[(dir4Data.previous/2)-1][i];
          if (!getState(other).pressed)
            continue;
          dir4Data.active = other;
          return;
        }
      }
    }
    for (size_t i = 0; i < 4; ++i) {
      if (!getState(dirs[i]).pressed)
        continue;
      dir4Data.active = dirs[i];
      dir4Data.previous = dirs[i];
      return;
    }
    dir4Data.active   = Input::None;
    dir4Data.previous = Input::None;
  }

  void updateDir8()
  {
    static const int combos[4][4] =
    {
      { 2, 1, 3, 0 },
      { 1, 4, 0, 7 },
      { 3, 0, 6, 9 },
      { 0, 7, 9, 8 }
    };
    dir8Data.active = 0;
    for (size_t i = 0; i < 4; ++i) {
      Input::ButtonCode one = dirs[i];
      if (!getState(one).pressed)
        continue;
      for (int j = 0; j < 3; ++j) {
        Input::ButtonCode other = otherDirs[i][j];
        if (!getState(other).pressed)
          continue;
        dir8Data.active = combos[(one/2)-1][(other/2)-1];
        return;
      }
      dir8Data.active = one;
      return;
    }
  }

  void update_dir4()
  {
    int dirFlag = 0;
    for (size_t i = 0; i < 4; ++i)
      dirFlag |= (get_text_state(dirs[i]).pressed ? dirFlags[i] : 0);
    if (dirFlag == deadDirFlags[0] || dirFlag == deadDirFlags[1]) {
      dir4Data.active = Input::None;
      return;
    }
    if (dir4Data.previous != Input::None) {
      if (get_text_state(dir4Data.previous).pressed) {
        for (size_t i = 0; i < 3; ++i) {
          Input::ButtonCode other = otherDirs[(dir4Data.previous/2)-1][i];
          if (!get_text_state(other).pressed)
            continue;
          dir4Data.active = other;
          return;
        }
      }
    }
    for (size_t i = 0; i < 4; ++i) {
      if (!get_text_state(dirs[i]).pressed)
        continue;
      dir4Data.active = dirs[i];
      dir4Data.previous = dirs[i];
      return;
    }
    dir4Data.active   = Input::None;
    dir4Data.previous = Input::None;
  }

  void poll_alt_ctrl_shift_main()
  {
    getState(Input::Alt).pressed = getState(Input::LeftAlt).pressed ||
      getState(Input::RightAlt).pressed;
    getState(Input::Alt).triggered = getState(Input::LeftAlt).triggered ||
      getState(Input::RightAlt).triggered;
    getState(Input::Alt).repeated = getState(Input::LeftAlt).repeated ||
      getState(Input::RightAlt).repeated;
    getState(Input::Ctrl).pressed = getState(Input::LeftCtrl).pressed ||
      getState(Input::RightCtrl).pressed;
    getState(Input::Ctrl).triggered = getState(Input::LeftCtrl).triggered ||
      getState(Input::RightCtrl).triggered;
    getState(Input::Ctrl).repeated = getState(Input::LeftCtrl).repeated ||
      getState(Input::RightCtrl).repeated;
    getState(Input::Shift).pressed = getState(Input::LeftShift).pressed ||
      getState(Input::RightShift).pressed;
    getState(Input::Shift).triggered = getState(Input::LeftShift).triggered ||
      getState(Input::RightShift).triggered;
    getState(Input::Shift).repeated = getState(Input::LeftShift).repeated ||
      getState(Input::RightShift).repeated;
  }

  bool is_same_trigger(int button)
  {
    if (!get_this_state_check(button).triggered || trigger_timer == 0)
      return false;
    return trigger_old == trigger_new;
  }

  void check_repeating_key(Input::ButtonCode &repeat_btn)
  {
    if (Input::None != repeat_btn && repeat_btn != repeating) {
      repeating = repeat_btn;
      repeatCount = 0;
      getState(repeat_btn).repeated = true;
      return;
    }
    if (getState(repeating).pressed) {
      repeatCount++;
      bool repeated;
      repeated = repeatCount >= 23 && ((repeatCount+1) % 6) == 0;
      getState(repeating).repeated |= repeated;
      return;
    }
    repeating = Input::None;
  }

  void check_repeating_btn(Input::ButtonCode &repeat_btn)
  {
    if (Input::None != repeat_btn && repeat_btn != repeating) {
      repeating = repeat_btn;
      repeatCount = 0;
      get_text_state(repeat_btn).repeated = true;
      return;
    }
    if (get_text_state(repeating).pressed) {
      repeatCount++;
      bool repeated;
      repeated = repeatCount >= 23 && ((repeatCount+1) % 6) == 0;
      get_text_state(repeating).repeated |= repeated;
      return;
    }
    repeating = Input::None;
  }
};

Input::Input(const RGSSThreadData &rtData)
{
  p1 = new InputPrivate(rtData, 0, this);
  p2 = new InputPrivate(rtData, 1, this);
  text_input = 0;
}

void Input::update()
{
  shState->checkShutdown();
  switch (text_input) {
  case 0:
    main_update();
    return;
  case 1:
    text_update();
    return;
  default:
    bind_update();
    return;
  }
}

void Input::main_update()
{
  p1->checkBindingChange(shState->rtData());
  p1->swapBuffers();
  p1->clearBuffer();
  p1->update_timers();
  p2->checkBindingChange(shState->rtData());
  p2->swapBuffers();
  p2->clearBuffer();
  p2->update_timers();
  ButtonCode repeat_btn1 = None;
  ButtonCode repeat_btn2 = None;
  // Poll all bindings
  p1->pollBindings(repeat_btn1);
  p1->check_repeating_key(repeat_btn1);
  p2->pollBindings(repeat_btn2);
  p2->check_repeating_key(repeat_btn2);
}

void Input::text_update()
{
  p1->checkBindingChange(shState->rtData());
  p1->swap_text_buffers();
  p1->clear_text_buffer();
  p1->update_timers();
  p2->checkBindingChange(shState->rtData());
  p2->swap_text_buffers();
  p2->clear_text_buffer();
  p2->update_timers();
  ButtonCode repeat_btn1 = None;
  ButtonCode repeat_btn2 = None;
  // Poll all bindings
  p1->poll_bindings4text(repeat_btn1);
  p1->check_repeating_btn(repeat_btn1);
  p2->poll_bindings4text(repeat_btn2);
  p2->check_repeating_btn(repeat_btn2);
}

void Input::bind_update()
{
  p1->check_text_input_state(shState->rtData());
  p1->swap_bind_buffers();
  p1->clear_bind_buffer();
  p1->update_timers();
  p1->poll_bindings4bind();
  p2->check_text_input_state(shState->rtData());
  p2->swap_bind_buffers();
  p2->clear_bind_buffer();
  p2->update_timers();
  p2->poll_bindings4bind();
}

int Input::trigger_timer() const
{
  return p1->trigger_timer;
}

int Input::trigger_base_timer() const
{
  return p1->trigger_base_timer;
}

void Input::set_trigger_base_timer(int timer)
{
  p1->trigger_base_timer = timer;
}

int Input::click_timer() const
{
  return p1->click_timer;
}

int Input::click_base_timer() const
{
  return p1->click_base_timer;
}

void Input::set_click_base_timer(int timer)
{
  p1->click_base_timer = timer;
}

int Input::scroll_factor() const
{
  return p1->scroll_factor;
}

void Input::set_scroll_factor(int value)
{
  p1->scroll_factor = clamp(1, value, 20);
}

void Input::mouse_scroll_reset()
{
  EventThread::mouseState.scroll_x = 0;
  EventThread::mouseState.scroll_y = 0;
  p1->reset_scroll_xy();
}

void Input::clear_clicks()
{
  mouse_scroll_reset();
  p1->clear_unused_clicks();
}

bool Input::is_left_click()
{
  if (!p1->get_this_state_check(MouseLeft).triggered)
    return false;
  return p1->clicks == 1 && !shState->rtData().mouse_moved;
}

bool Input::is_middle_click()
{
  if (!p1->get_this_state_check(MouseMiddle).triggered)
    return false;
  return p1->clicks == 1 && !shState->rtData().mouse_moved;
}

bool Input::is_right_click()
{
  if (!p1->get_this_state_check(MouseRight).triggered)
    return false;
  return p1->clicks == 1 && !shState->rtData().mouse_moved;
}

bool Input::is_double_left_click()
{
  if (p1->double_target != MouseLeft)
    return false;
  if (!p1->get_this_state_check(MouseLeft).triggered)
    return false;
  return (p1->clicks == 2 && p1->same_mouse_pos && !shState->rtData().mouse_moved);
}

bool Input::is_double_right_click()
{
  if (p1->double_target != MouseRight)
    return false;
  if (!p1->get_this_state_check(MouseRight).triggered)
    return false;
  return (p1->clicks == 2 && p1->same_mouse_pos && !shState->rtData().mouse_moved);
}

bool Input::is_double_click(int btn)
{
  if (btn != MouseLeft || btn != MouseRight || p1->double_target != btn)
    return false;
  if (!p1->get_this_state_check(btn).triggered)
    return false;
  return (p1->clicks == 2 && p1->same_mouse_pos && !shState->rtData().mouse_moved);
}

bool Input::press_left_click()
{
  if (p1->get_this_state_check(MouseLeft).triggered)
    return false;
  return p1->get_this_state_check(MouseLeft).pressed;
}

bool Input::press_right_click()
{
  if (p1->get_this_state_check(MouseRight).triggered)
    return false;
  return p1->get_this_state_check(MouseRight).pressed;
}

bool Input::is_mouse_scroll_x(bool go_up)
{
  if (shState->rtData().mouse_moved)
    return false;
  else if (!EventThread::mouseState.scrolled_x)
    return false;
  else if (go_up)
    return p1->scroll_x - p1->old_scroll_x == -p1->scroll_factor;
  else
    return p1->scroll_x - p1->old_scroll_x == p1->scroll_factor;
}

bool Input::is_mouse_scroll_y(bool go_up)
{
  if (shState->rtData().mouse_moved)
    return false;
  else if (!EventThread::mouseState.scrolled_y)
    return false;
  else if (go_up)
    return p1->scroll_y - p1->old_scroll_y == -p1->scroll_factor;
  else
    return p1->scroll_y - p1->old_scroll_y == p1->scroll_factor;
}

bool Input::is_pressed(int pos, int button)
{
  ButtonState state1 = p1->get_this_state_check(button);
  ButtonState state2 = p2->get_this_state_check(button);
  if (button == MouseLeft || button == MouseRight)
    if (state1.triggered)
      return false;
  if (pos == 2)
    return state1.pressed || state2.pressed;
  else if (pos == 1)
    return state2.pressed;
  else
    return state1.pressed;
}

bool Input::is_triggered(int pos, int button)
{
  ButtonState state1 = p1->get_this_state_check(button);
  ButtonState state2 = p2->get_this_state_check(button);
  if (button == MouseLeft || button == MouseRight) {
    bool trig = state1.triggered;
    state1.triggered = false;
    state2.triggered = false;
    return trig;
  }
  if (pos == 2)
    return state1.triggered || state2.triggered;
  else if (pos == 1)
    return state2.triggered;
  else
    return state1.triggered;
}

bool Input::is_repeated(int pos, int button)
{
  if (pos == 2) {
    if (button == Ctrl)
      return p1->get_this_state_check(LeftCtrl).repeated ||
             p1->get_this_state_check(RightCtrl).repeated ||
             p2->get_this_state_check(LeftCtrl).repeated ||
             p2->get_this_state_check(RightCtrl).repeated;
    else if (button == Shift)
      return p1->get_this_state_check(LeftShift).repeated ||
             p1->get_this_state_check(RightShift).repeated ||
             p2->get_this_state_check(LeftShift).repeated ||
             p2->get_this_state_check(RightShift).repeated;
    else if (button == Alt)
      return p1->get_this_state_check(LeftAlt).repeated ||
             p1->get_this_state_check(RightAlt).repeated ||
             p2->get_this_state_check(LeftAlt).repeated ||
             p2->get_this_state_check(RightAlt).repeated;
    return p1->get_this_state_check(button).repeated ||
           p2->get_this_state_check(button).repeated;
  } else if (pos == 1) {
    if (button == Ctrl)
      return p2->get_this_state_check(LeftCtrl).repeated ||
             p2->get_this_state_check(RightCtrl).repeated;
    if (button == Shift)
      return p2->get_this_state_check(LeftShift).repeated ||
             p2->get_this_state_check(RightShift).repeated;
    if (button == Alt)
      return p2->get_this_state_check(LeftAlt).repeated ||
             p2->get_this_state_check(RightAlt).repeated;
    return p2->get_this_state_check(button).repeated;
  } else {
    if (button == Ctrl)
      return p1->get_this_state_check(LeftCtrl).repeated ||
             p1->get_this_state_check(RightCtrl).repeated;
    if (button == Shift)
      return p1->get_this_state_check(LeftShift).repeated ||
             p1->get_this_state_check(RightShift).repeated;
    if (button == Alt)
      return p1->get_this_state_check(LeftAlt).repeated ||
             p1->get_this_state_check(RightAlt).repeated;
    return p1->get_this_state_check(button).repeated;
  }
}

bool Input::is_pressed_any(int pos)
{
  if (pos == 2)
    return p1->press_any || p2->press_any;
  else if (pos == 1)
    return p2->press_any;
  else
    return p1->press_any;
}

bool Input::is_triggered_any(int pos)
{
  if (pos == 2)
    return p1->trigger_any || p2->trigger_any;
  else if (pos == 1)
    return p2->trigger_any;
  else
    return p1->trigger_any;
}

bool Input::is_triggered_double(int button)
{
  return p1->is_same_trigger(button) || p2->is_same_trigger(button);
}

bool Input::is_last_key()
{
  int k = p1->trigger_now;
  if (!k)
    return false;
  return k != Backspace && k != Delete && k != Enter && k != Return && k != Shift;
}

void Input::set_text_input(int value, int dev_index)
{
  int old_ti = text_input;
  text_input = clamp(0, value, 2);
  if (dev_index == 1) {
    p2->clear_unused_clicks();
    if (!text_input && old_ti != 1) {
      p2->checkBindingChange(shState->rtData());
      return;
    }
    if (text_input == 2)
      p2->check_text_input_state(shState->rtData());
    return;
  } else {
    p1->clear_unused_clicks();
    if (!text_input && old_ti != 1) {
      p1->checkBindingChange(shState->rtData());
      return;
    }
    if (text_input == 2)
      p1->check_text_input_state(shState->rtData());
  }
}

int Input::last_key()
{
  return p1->trigger_now;
}

void Input::last_key_clear()
{
  p1->trigger_now = 0;
}

void Input::triggered_bind_clear(int n)
{
  if (n == 1) {
    p2->trigger_kind = 0;
    p2->trigger_js_value = 0;
    p2->trigger_js_dir = 0;
  } else {
    p1->trigger_kind = 0;
    p1->trigger_js_value = 0;
    p1->trigger_js_dir = 0;
  }
}

int Input::triggered_kind(int n)
{
  if (n == 1)
    return p2->trigger_kind;
  else
    return p1->trigger_kind;
}

int Input::triggered_js_value(int n)
{
  if (n == 1)
    return p2->trigger_js_value;
  else
    return p1->trigger_js_value;
}

int Input::triggered_js_axis(int n)
{
  if (n == 1)
    return p2->trigger_js_axis;
  else
    return p1->trigger_js_axis;
}

int Input::triggered_js_dir(int n)
{
  if (n == 1)
    return p2->trigger_js_dir;
  else
    return p1->trigger_js_dir;
}

int Input::triggered_last(int n)
{
  if (n == 1)
    return p2->trigger_new;
  else
    return p1->trigger_new;
}

int Input::triggered_old(int n)
{
  if (n == 1)
    return p2->trigger_old;
  else
    return p1->trigger_old;
}

void Input::triggered_last_clear(int n)
{
  if (n == 2) {
    p2->trigger_new = Input::None;
    p1->trigger_new = Input::None;
  } else if (n == 1) {
    p2->trigger_new = Input::None;
  } else {
    p1->trigger_new = Input::None;
  }
}

int Input::dir4Value()
{
  return p1->dir4Data.active;
}

int Input::dir8Value()
{
  return p1->dir8Data.active;
}

bool Input::is_dir4()
{
  return p1->dir4Data.active > 0;
}

bool Input::is_dir8()
{
  return p1->dir8Data.active > 0;
}

int Input::player_dir4(int n)
{
  if (n == 0)
    return p1->dir4Data.active;
  else
    return p2->dir4Data.active;
}

int Input::player_dir8(int n)
{
  if (n == 0)
    return p1->dir8Data.active;
  else
    return p2->dir8Data.active;
}

bool Input::is_player_dir4(int n)
{
  if (n == 0)
    return p1->dir4Data.active > 0;
  else
    return p2->dir4Data.active > 0;
}

bool Input::is_player_dir8(int n)
{
  if (n == 0)
    return p1->dir8Data.active > 0;
  else
    return p2->dir8Data.active > 0;
}

int Input::mouseX()
{
  RGSSThreadData &rtData = shState->rtData();
  int mx = EventThread::mouseState.x;
  return (mx + p1->ox - rtData.screenOffset.x) * rtData.sizeResoRatio.x;
}

int Input::mouseY()
{
  RGSSThreadData &rtData = shState->rtData();
  int my = EventThread::mouseState.y;
  return (my + p1->oy - rtData.screenOffset.y) * rtData.sizeResoRatio.y;
}

int Input::mouse_ox() const
{
  return p1->ox;
}

int Input::mouse_oy() const
{
  return p1->oy;
}

void Input::mouse_set_xy(int x, int y)
{
  EventThread::mouseState.x = x;
  EventThread::mouseState.y = y;
  SDL_WarpMouseInWindow(shState->sdlWindow(), x, y);
}

void Input::mouse_set_ox(int n)
{
  p1->ox = n;
}

void Input::mouse_set_oy(int n)
{
  p1->oy = n;
}

int Input::mouse_scroll_x() const
{
  return p1->scroll_x;
}

int Input::mouse_scroll_y() const
{
  return p1->scroll_y;
}

bool Input::mouse_is_inside(int index, Rect *rect)
{
  int mx = mouseX();
  int my = mouseY();
  if (index == 0) {
    if (rect->x > mx || rect->width < mx)
      return false;
    return (rect->y <= my && rect->height >= my);
  } else {
    double center_x = rect->width / 2.0;
    double center_y = rect->height / 2.0;
    double dx = mx - center_x;
    double dy = my - center_y;
    double dr = dx * dx + dy * dy;
    return (sqrt(dr) <= center_x);
  }
}

void Input::switch_joysticks()
{
  std::swap(p1, p2);
  p1->index = 0;
  p2->index = 1;
  p1->update_bindings_index();
  p2->update_bindings_index();
}

bool Input::has_joystick()
{
  return SDL_NumJoysticks() > 0;
}

const char* Input::joystick_name(int n)
{
  return SDL_JoystickName(shState->rtData().joysticks[n]->js);
}

int Input::joystick_vendor(int n)
{
  return SDL_JoystickGetVendor(shState->rtData().joysticks[n]->js);
}

int Input::joystick_kind(int n)
{
  return SDL_JoystickGetType(shState->rtData().joysticks[n]->js);
}

int Input::joystick_power(int n)
{
  return SDL_JoystickCurrentPowerLevel(shState->rtData().joysticks[n]->js);
}

std::vector<int> Input::joystick_basic_values(int n)
{
  std::vector<int> values;
  values.push_back(joystick_vendor(n));
  values.push_back(joystick_kind(n));
  values.push_back(joystick_power(n));
  return values;
}

int Input::joystick_axis_number(int n)
{
  return SDL_JoystickNumAxes(shState->rtData().joysticks[n]->js);
}

int Input::joystick_hat_number(int n)
{
  return SDL_JoystickNumHats(shState->rtData().joysticks[n]->js);
}

int Input::joystick_button_number(int n)
{
  return SDL_JoystickNumButtons(shState->rtData().joysticks[n]->js);
}

bool Input::joystick_has_rumble(int n)
{
  return SDL_JoystickHasRumble(shState->rtData().joysticks[n]->js);
}

int Input::joystick_set_rumble(int n, int lfr, int rfr, int ms)
{
  return SDL_JoystickRumble(shState->rtData().joysticks[n]->js, lfr, rfr, ms);
}

Input::~Input()
{
  delete p1;
  delete p2;
}

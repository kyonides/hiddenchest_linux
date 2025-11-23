/*
** input-binding.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** 2018-2025 Extended by Kyonides-Arkanthes <kyonides@gmail.com>
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
#include "exception.h"
#include "binding-util.h"
#include "util.h"
#include "etc.h"
#include "hcextras.h"

#define ZERO RB_INT2FIX(0)

static void joystick_state_change(VALUE input)
{
  int state = shState->input().joystick_change();
  if (state == 0)
    return;
  VALUE ary = rb_iv_get(input, "joystick_updates");
  rb_ary_pop(ary);
  if (state == 2)
    rb_ary_push(ary, hc_sym("add"));
  else
    rb_ary_push(ary, hc_sym("remove"));
  shState->input().reset_joystick_change();
}

static VALUE inputUpdate(VALUE self)
{
  shState->input().update();
  joystick_state_change(self);
  return Qnil;
}

// FIXME: RMXP allows only few more types that don't make sense (symbols in pre 3, floats)
static int getButtonArg(VALUE number)
{
  if (FIXNUM_P(number))
    return RB_FIX2INT(number);
  if (SYMBOL_P(number)) {// && rgssVer == 3
    VALUE sym_hash = getRbData()->buttoncodeHash;
    return RB_FIX2INT(rb_hash_aref(sym_hash, number));
  }
  return 0;
}

static VALUE inputPress(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
  return shState->input().isPressed(num) ? Qtrue : Qfalse;
}

static VALUE inputTrigger(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
  return shState->input().isTriggered(num) ? Qtrue : Qfalse;
}

static VALUE inputRepeat(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
  return shState->input().isRepeated(num) ? Qtrue : Qfalse;
}

static VALUE input_press_any(VALUE self)
{
  return shState->input().is_pressed_any()? Qtrue : Qfalse;
}

static VALUE input_trigger_any(VALUE self)
{
  return shState->input().is_triggered_any()? Qtrue : Qfalse;
}

static VALUE input_trigger_double(VALUE self, VALUE number)
{
  int num = getButtonArg(number);
  return shState->input().is_triggered_double(num) ? Qtrue : Qfalse;
}

static VALUE input_trigger_last(VALUE self)
{
  int num = shState->input().is_triggered_last();
  return RB_INT2FIX(num);
}

static VALUE input_trigger_old(VALUE self)
{
  int num = shState->input().is_triggered_old();
  return RB_INT2FIX(num);
}

static VALUE input_are_triggered(int size, VALUE* buttons, VALUE self)
{
  if (ARRAY_TYPE_P(buttons[0])) {
    VALUE rbuttons = buttons[0];
    int size = RARRAY_LEN(rbuttons);
    for (int n = 0; n < size; n++) {
      int num = getButtonArg(rb_ary_entry(rbuttons, n));
      if (shState->input().isTriggered(num))
        return Qtrue;
    }
    return Qfalse;
  }
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(buttons[n]);
    if (shState->input().isTriggered(num))
      return Qtrue;
  }
  return Qfalse;
}

static VALUE input_are_pressed(int size, VALUE* buttons, VALUE self)
{
  if (ARRAY_TYPE_P(buttons[0])) {
    VALUE rbuttons = buttons[0];
    int size = RARRAY_LEN(rbuttons);
    for (int n = 0; n < size; n++) {
      int num = getButtonArg(rb_ary_entry(rbuttons, n));
      if (!shState->input().isPressed(num))
        return Qfalse;
    }
    return Qtrue;
  }
  for (int n = 0; n < size; n++) {
    int num = getButtonArg(buttons[n]);
    if (!shState->input().isPressed(num))
      return Qfalse;
  }
  return Qtrue;
}

static VALUE input_press_left_click(VALUE self)
{
  return shState->input().press_left_click() ? Qtrue : Qfalse;
}

static VALUE input_press_right_click(VALUE self)
{
  return shState->input().press_right_click() ? Qtrue : Qfalse;
}

static VALUE input_trigger_up_down(VALUE self)
{
  if (shState->input().isTriggered(Input::Up))
    return Qtrue;
  if (shState->input().isTriggered(Input::Down))
    return Qtrue;
  return Qfalse;
}

static VALUE input_trigger_left_right(VALUE self)
{
  if (shState->input().isTriggered(Input::Left))
    return Qtrue;
  if (shState->input().isTriggered(Input::Right))
    return Qtrue;
  return Qfalse;
}

static VALUE input_repeat_left_click(VALUE self)
{
  return shState->input().isRepeated(Input::MouseLeft) ? Qtrue : Qfalse;
}

static VALUE input_repeat_right_click(VALUE self)
{
  return shState->input().isRepeated(Input::MouseRight) ? Qtrue : Qfalse;
}

static VALUE inputDir4(VALUE self)
{
  return RB_INT2FIX(shState->input().dir4Value());
}

static VALUE inputDir8(VALUE self)
{
  return RB_INT2FIX(shState->input().dir8Value());
}

static VALUE input_is_dir4(VALUE self)
{
  return shState->input().is_dir4() ? Qtrue : Qfalse;
}

static VALUE input_is_dir8(VALUE self)
{
  return shState->input().is_dir8() ? Qtrue : Qfalse;
}

// Non-standard extensions
static VALUE input_left_click(VALUE self)
{
  return shState->input().is_left_click() ? Qtrue : Qfalse;
}

static VALUE input_middle_click(VALUE self)
{
  return shState->input().is_middle_click() ? Qtrue : Qfalse;
}

static VALUE input_right_click(VALUE self)
{
  return shState->input().is_right_click() ? Qtrue : Qfalse;
}

static VALUE input_double_click(VALUE self, VALUE button)
{
  int btn = RB_FIX2INT(button);
  return shState->input().is_double_click(btn) ? Qtrue : Qfalse; 
}

static VALUE input_double_left_click(VALUE self)
{
  return shState->input().is_double_left_click() ? Qtrue : Qfalse; 
}

static VALUE input_double_right_click(VALUE self)
{
  return shState->input().is_double_right_click() ? Qtrue : Qfalse; 
}

static VALUE input_has_joystick(VALUE self)
{
  return shState->input().has_joystick() ? Qtrue : Qfalse;
}

static VALUE input_joystick_update(VALUE self)
{
  VALUE state = rb_iv_get(self, "joystick_updates");
  return rb_ary_pop(state);
}

static VALUE input_joystick_updates(VALUE self)
{
  return rb_iv_get(self, "joystick_updates");
}

static VALUE input_is_any_char(VALUE self)
{
  return shState->input().is_any_char() ? Qtrue : Qfalse;
}

static VALUE input_string(VALUE self)
{
  VALUE str = rb_utf8_str_new_cstr(shState->input().string());
  return rb_funcall(str, rb_intern("strip"), 0);
}//return rb_funcall(str, rb_intern("gsub"), 2, rb_str_new_cstr("\x00"), rb_str_new_cstr(""));

static VALUE input_enable_edit(VALUE self, VALUE boolean)
{
  shState->input().enableMode(boolean == Qtrue);
  return boolean;
}

static VALUE input_trigger_timer(VALUE self)
{
  int n = shState->input().trigger_timer();
  return RB_INT2FIX(n);
}

static VALUE input_default_trigger_timer(VALUE self)
{
  return rb_iv_get(self, "default_trigger_timer");
}

static VALUE input_default_trigger_timer_set(VALUE self, VALUE val)
{
  val = rb_funcall(val, rb_intern("to_i"), 0);
  int n = RB_FIX2INT(val);
  if (n < 0)
    return rb_iv_get(self, "default_trigger_timer");
  shState->input().set_trigger_base_timer(n);
  return rb_iv_set(self, "default_trigger_timer", val);
}

struct
{
  const char *str;
  Input::ButtonCode val;
}
static buttonCodes[] =
{
  { "DOWN",           Input::Down  },
  { "LEFT",           Input::Left  },
  { "RIGHT",          Input::Right },
  { "UP",             Input::Up    },
  { "Down",           Input::Down  },
  { "Left",           Input::Left  },
  { "Right",          Input::Right },
  { "Up",             Input::Up    },
  { "A",              Input::A     },
  { "B",              Input::B     },
  { "C",              Input::C     },
  { "X",              Input::X     },
  { "Y",              Input::Y     },
  { "Z",              Input::Z     },
  { "L",              Input::L     },
  { "R",              Input::R     },
  { "SHIFT",          Input::Shift },
  { "CTRL",           Input::Ctrl  },
  { "ALT",            Input::Alt   },
  { "KeyA",           Input::KeyA  },
  { "KeyB",           Input::KeyB  },
  { "KeyC",           Input::KeyC  },
  { "KeyD",           Input::KeyD  },
  { "KeyE",           Input::KeyE  },
  { "KeyF",           Input::KeyF  },
  { "KeyG",           Input::KeyG  },
  { "KeyH",           Input::KeyH  },
  { "KeyI",           Input::KeyI  },
  { "KeyJ",           Input::KeyJ  },
  { "KeyK",           Input::KeyK  },
  { "KeyL",           Input::KeyL  },
  { "KeyM",           Input::KeyM  },
  { "KeyN",           Input::KeyN  },
  { "KeyO",           Input::KeyO  },
  { "KeyP",           Input::KeyP  },
  { "KeyQ",           Input::KeyQ  },
  { "KeyR",           Input::KeyR  },
  { "KeyS",           Input::KeyS  },
  { "KeyT",           Input::KeyT  },
  { "KeyU",           Input::KeyU  },
  { "KeyV",           Input::KeyV  },
  { "KeyW",           Input::KeyW  },
  { "KeyX",           Input::KeyX  },
  { "KeyY",           Input::KeyY  },
  { "KeyZ",           Input::KeyZ  },
  { "N1",             Input::N1    },
  { "N2",             Input::N2    },
  { "N3",             Input::N3    },
  { "N4",             Input::N4    },
  { "N5",             Input::N5    },
  { "N6",             Input::N6    },
  { "N7",             Input::N7    },
  { "N8",             Input::N8    },
  { "N9",             Input::N9    },
  { "N0",             Input::N0    },
  { "Return",         Input::Return         },
  { "Escape",         Input::Escape         },
  { "Backspace",      Input::Backspace      },
  { "Space",          Input::Space          },
  { "Minus",          Input::Minus          },
  { "Equals",         Input::Equals         },
  { "LeftBracket",    Input::LeftBracket    },
  { "RightBracket",   Input::RightBracket   },
  { "BackSlash",      Input::BackSlash      },
  { "Semicolon",      Input::Semicolon      },
  { "Apostrophe",     Input::Apostrophe     },
  { "Grave",          Input::Grave          },
  { "Comma",          Input::Comma          },
  { "Period",         Input::Period         },
  { "Slash",          Input::Slash          },
  { "Tab",            Input::Tab            },
  { "F1",             Input::F1             },
  { "F2",             Input::F2             },
  { "F3",             Input::F3             },
  { "F4",             Input::F4             },
  { "F5",             Input::F5             },
  { "F6",             Input::F6             },
  { "F7",             Input::F7             },
  { "F8",             Input::F8             },
  { "F9",             Input::F9             },
  { "F10",            Input::F10            },
  { "F11",            Input::F11            },
  { "F12",            Input::F12            },
  { "PrintScreen",    Input::PrintScreen    },
  { "ScrollLock",     Input::ScrollLock     },
  { "CapsLock",       Input::CapsLock       },
  { "Pause",          Input::Pause          },
  { "Insert",         Input::Insert         },
  { "Home",           Input::Home           },
  { "PageUp",         Input::PageUp         },
  { "Delete",         Input::Delete         },
  { "End",            Input::End            },
  { "PageDown",       Input::PageDown       },
  { "NumPadDivide",   Input::NumPadDivide   },
  { "NumPadMultiply", Input::NumPadMultiply },
  { "NumPadMinus",    Input::NumPadMinus    },
  { "NumPadPlus",     Input::NumPadPlus     },
  { "Enter",          Input::Enter          },
  { "NumPad1",        Input::NumPad1        },
  { "NumPad2",        Input::NumPad2        },
  { "NumPad3",        Input::NumPad3        },
  { "NumPad4",        Input::NumPad4        },
  { "NumPad5",        Input::NumPad5        },
  { "NumPad6",        Input::NumPad6        },
  { "NumPad7",        Input::NumPad7        },
  { "NumPad8",        Input::NumPad8        },
  { "NumPad9",        Input::NumPad9        },
  { "NumPad0",        Input::NumPad0        },
  { "NumPadDot",      Input::NumPadDot      },
  { "LessOrGreater",  Input::LessOrGreater  },
  { "Input::APP",     Input::APP            },
  { "NumPadEquals",   Input::NumPadEquals   },
  { "LeftCtrl",       Input::LeftCtrl       },
  { "LeftShift",      Input::LeftShift      },
  { "LeftAlt",        Input::LeftAlt        },
  { "LeftMeta",       Input::LeftMeta       },
  { "RightCtrl",      Input::RightCtrl      },
  { "RightShift",     Input::RightShift     },
  { "RightAlt",       Input::RightAlt       },
  { "RightMeta",      Input::RightMeta      },
  { "Web",            Input::Web            },
  { "Mail",           Input::Mail           },
  { "Calculator",     Input::Calculator     },
  { "Computer",       Input::Computer       },
  { "APP1",           Input::APP1           },
  { "APP2",           Input::APP2           },
  { "MOUSELEFT",      Input::MouseLeft      },
  { "MOUSEMIDDLE",    Input::MouseMiddle    },
  { "MOUSERIGHT",     Input::MouseRight     },
  { "MouseLeft",      Input::MouseLeft      },
  { "MouseMiddle",    Input::MouseMiddle    },
  { "MouseRight",     Input::MouseRight     }
};

static elementsN(buttonCodes);

void inputBindingInit()
{
  VALUE input = rb_define_module("Input");
  rb_iv_set(input, "default_trigger_timer", RB_INT2FIX(TRIGGER_TIMER));
  rb_iv_set(input, "joystick_updates", rb_ary_new());
  module_func(input, "trigger_timer", input_trigger_timer, 0);
  module_func(input, "base_trigger_timer", input_default_trigger_timer, 0);
  module_func(input, "base_trigger_timer=", input_default_trigger_timer_set, 1);
  module_func(input, "default_trigger_timer", input_default_trigger_timer, 0);
  module_func(input, "default_trigger_timer=", input_default_trigger_timer_set, 1);
  module_func(input, "update", inputUpdate, 0);
  module_func(input, "left_click?", input_left_click, 0);
  module_func(input, "middle_click?", input_middle_click, 0);
  module_func(input, "right_click?", input_right_click, 0);
  module_func(input, "double_click?", input_double_click, 1);
  module_func(input, "double_left_click?", input_double_left_click, 0);
  module_func(input, "double_right_click?", input_double_right_click, 0);
  module_func(input, "press?", inputPress, 1);
  module_func(input, "press_left_click?", input_press_left_click, 0);
  module_func(input, "press_right_click?", input_press_right_click, 0);
  module_func(input, "press_any?", input_press_any, 0);
  module_func(input, "press_all?", input_are_pressed, -1);
  module_func(input, "trigger?", inputTrigger, 1);
  module_func(input, "trigger_any?", input_trigger_any, 0);
  module_func(input, "trigger_double?", input_trigger_double, 1);
  module_func(input, "trigger_buttons?", input_are_triggered, -1);
  module_func(input, "trigger_up_down?", input_trigger_up_down, 0);
  module_func(input, "trigger_left_right?", input_trigger_left_right, 0);
  module_func(input, "trigger_last", input_trigger_last, 0);
  module_func(input, "trigger_old", input_trigger_old, 0);
  module_func(input, "repeat?", inputRepeat, 1);
  module_func(input, "repeat_left_click?", input_repeat_left_click, 0);
  module_func(input, "repeat_right_click?", input_repeat_right_click, 0);
  module_func(input, "gamepad?", input_has_joystick, 0);
  module_func(input, "joystick?", input_has_joystick, 0);
  module_func(input, "gamepad_update", input_joystick_update, 0);
  module_func(input, "joystick_update", input_joystick_update, 0);
  module_func(input, "gamepad_updates", input_joystick_updates, 0);
  module_func(input, "joystick_updates", input_joystick_updates, 0);
  module_func(input, "dir4", inputDir4, 0);
  module_func(input, "dir8", inputDir8, 0);
  module_func(input, "dir4?", input_is_dir4, 0);
  module_func(input, "dir8?", input_is_dir8, 0);
  module_func(input, "any_char?", input_is_any_char, 0);
  module_func(input, "string", input_string, 0);
  module_func(input, "enable_edit=", input_enable_edit, 1);
  VALUE sym_hash = rb_hash_new();
  rb_hash_set_ifnone(sym_hash, ZERO);
  /* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
   * to be compatible with the previous convention */
  for (size_t i = 0; i < buttonCodesN; ++i) {
    ID sym = rb_intern(buttonCodes[i].str);
    VALUE val = RB_INT2FIX(buttonCodes[i].val);
    rb_const_set(input, sym, val);
    rb_hash_aset(sym_hash, rb_id2sym(sym), val);
  }
  getRbData()->buttoncodeHash = sym_hash;
}

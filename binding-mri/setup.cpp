/*
** setup.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2024 Kyonides-Arkanthes
*/

#include "hcextras.h"
#include "graphics.h"
#include "sharedmidistate.h"
#include "sharedstate.h"
#include "exception.h"
#include "debugwriter.h"

static VALUE set_shot_fmt_set(VALUE self, VALUE fmt)
{
  char *ext = StringValueCStr(fmt);
  shState->graphics().set_screenshot_format(ext);
  return rb_iv_set(self, "@shot_format", fmt);
}

static VALUE set_shot_dir_set(VALUE self, VALUE sdir)
{
  char* dir = StringValueCStr(sdir);
  shState->graphics().set_screenshot_dir(dir);
  return rb_iv_set(self, "@shot_dir", sdir);
}

static VALUE set_shot_filename_set(VALUE self, VALUE name)
{
  char* fn = StringValueCStr(name);
  shState->graphics().set_screenshot_fn(fn);
  return rb_iv_set(self, "@shot_filename", name);
}

static VALUE set_sound_font_set(VALUE self, VALUE fn)
{
  fn = rb_obj_as_string(fn);
  VALUE sfont = rb_iv_get(self, "@soundfont");
  if (rb_str_equal(sfont, fn) == Qtrue)
    return sfont;
  const char *sf = RSTRING_PTR(fn);
  shState->midiState().set_soundfont(sf);
  return rb_iv_set(self, "@soundfont", fn);
}

VALUE set_sound_font_init(VALUE self, int n)
{
  VALUE sfonts = rb_iv_get(self, "@soundfonts");
  VALUE sfont = rb_ary_entry(sfonts, n);
  if (sfont == Qnil) {
    return rb_iv_get(self, "@soundfont");
  } else {
    return set_sound_font_set(self, sfont);
  }
}

static VALUE set_sound_font_by_pos(VALUE self, VALUE n)
{
  return set_sound_font_init(self, RB_FIX2INT(n));
}

void init_setup()
{
  VALUE set = rb_define_module("Setup");
  VALUE sfont = rb_iv_get(set, "@soundfont");
  module_func(set, "shot_format=", set_shot_fmt_set, 1);
  module_func(set, "shot_dir=", set_shot_dir_set, 1);
  module_func(set, "shot_filename=", set_shot_filename_set, 1);
  module_func(set, "soundfont=", set_sound_font_set, 1);
  module_func(set, "choose_soundfont", set_sound_font_by_pos, 1);
  rb_define_alias(set, "change_soundfont", "choose_soundfont");
  if (RSTRING_LEN(sfont) > 4) {
    const char *sf = RSTRING_PTR(sfont);
    shState->midiState().set_default_soundfont(sf);
  }
}

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

static VALUE set_shot_fmts_get(VALUE self)
{
  return rb_iv_get(self, "@shot_formats");
}

static VALUE set_shot_fmt_get(VALUE self)
{
  return rb_iv_get(self, "@shot_format");
}

static VALUE set_shot_fmt_set(VALUE self, VALUE fmt)
{
  char *ext = StringValueCStr(fmt);
  shState->graphics().set_screenshot_format(ext);
  return rb_iv_set(self, "@shot_format", fmt);
}

static VALUE set_shot_dir_get(VALUE self)
{
  return rb_iv_get(self, "@snapshot_dir");
}

static VALUE set_shot_dir_set(VALUE self, VALUE sdir)
{
  char* dir = StringValueCStr(sdir);
  shState->graphics().set_screenshot_dir(dir);
  return rb_iv_set(self, "@snapshot_dir", sdir);
}

static VALUE set_shot_filename_get(VALUE self)
{
  return rb_iv_get(self, "@snapshot_filename");
}

static VALUE set_shot_filename_set(VALUE self, VALUE name)
{
  char* fn = StringValueCStr(name);
  shState->graphics().set_screenshot_fn(fn);
  return rb_iv_set(self, "@snapshot_filename", name);
}

static VALUE set_save_dir_get(VALUE self)
{
  return rb_iv_get(self, "@save_dir");
}

static VALUE set_save_dir_set(VALUE self, VALUE dir)
{
  return rb_iv_set(self, "@save_dir", dir);
}

static VALUE set_save_filename_get(VALUE self)
{
  return rb_iv_get(self, "@save_filename");
}

static VALUE set_save_filename_set(VALUE self, VALUE dir)
{
  return rb_iv_set(self, "@save_filename", dir);
}

static VALUE set_auto_create_dirs(VALUE self)
{
  safe_mkdir(rb_iv_get(self, "@snapshot_dir"));
  safe_mkdir(rb_iv_get(self, "@save_dir"));
  return Qtrue;
}

static VALUE set_click_timer_get(VALUE self)
{
  return rb_iv_get(self, "@click_timer");
}

static VALUE set_click_timer_set(VALUE self, VALUE frames)
{
  return rb_iv_set(self, "@click_timer", frames);
}

static VALUE set_sound_font_get(VALUE self)
{
  return rb_iv_get(self, "@soundfont");
}

static VALUE set_sound_font_set(VALUE self, VALUE fn)
{
  char *sf = RSTRING_PTR(fn);
  shState->midiState().set_soundfont(sf);
  return rb_iv_set(self, "@soundfont", fn);
}

static VALUE set_sound_font_index(VALUE self)
{
  VALUE elem;
  VALUE sf2 = rb_iv_get(self, "@soundfont");
  VALUE ary = rb_iv_get(self, "@soundfonts");
  int max = RARRAY_LEN(ary); 
  for (int n = 0; n < max; n++) {
    elem = rb_ary_entry(ary, n);
    if (rb_equal(elem, sf2))
      return RB_INT2FIX(n);
  }
  return Qnil;
}

static VALUE set_sound_fonts_get(VALUE self)
{
  return rb_iv_get(self, "@soundfonts");
}

static VALUE set_sound_font_init(VALUE self, int n)
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

void find_soundfonts(VALUE self)
{
  ID glob = rb_intern("glob");
  VALUE root = rb_funcall(rb_cDir, rb_intern("pwd"), 0);
  root = rb_str_plus(root, rstr("/"));
  VALUE fn;
  VALUE sys = rb_define_module("System");
  VALUE dsfont = rb_const_get(sys, rb_intern("SOUNDFONT"));
  VALUE temp_fonts = rb_ary_new();
  if (RSTRING_LEN(dsfont) > 0)
    rb_ary_push(temp_fonts, dsfont);
  VALUE names = rb_funcall(rb_cDir, glob, 1, rstr("Audio/SF2/*"));
  int total = RARRAY_LEN(names);
  for (int n = 0; n < total; n++) {
    fn = rb_ary_entry(names, n);
    rb_ary_push(temp_fonts, rb_str_plus(root, fn));
  }
  rb_iv_set(self, "@soundfonts", rb_ary_sort(temp_fonts));
  if (total > 0)
    set_sound_font_init(self, 0);
}

void init_setup()
{
  VALUE set = rb_define_module("Setup");
  VALUE formats = rb_ary_new3(2, rstr("jpg"), rstr("png"));
  rb_iv_set(set, "@shot_formats", formats);
  rb_iv_set(set, "@save_dir", rstr("Saves"));
  rb_iv_set(set, "@save_filename", rstr("Save"));
  rb_iv_set(set, "@click_timer", RB_INT2FIX(10));
  rb_iv_set(set, "@soundfont", rstr(""));
  find_soundfonts(set);
  set_shot_fmt_set(set, rstr("jpg"));
  set_shot_dir_set(set, rstr("Screenshots"));
  set_shot_filename_set(set, rb_str_new_cstr("screenshot"));
  module_func(set, "shot_formats", set_shot_fmts_get, 0);
  module_func(set, "shot_format", set_shot_fmt_get, 0);
  module_func(set, "shot_format=", set_shot_fmt_set, 1);
  module_func(set, "shot_dir", set_shot_dir_get, 0);
  module_func(set, "shot_dir=", set_shot_dir_set, 1);
  module_func(set, "shot_filename", set_shot_filename_get, 0);
  module_func(set, "shot_filename=", set_shot_filename_set, 1);
  module_func(set, "save_dir", set_save_dir_get, 0);
  module_func(set, "save_dir=", set_save_dir_set, 1);
  module_func(set, "save_filename", set_save_filename_get, 0);
  module_func(set, "save_filename=", set_save_filename_set, 1);
  module_func(set, "auto_create_dirs", set_auto_create_dirs, 0);
  module_func(set, "click_timer", set_click_timer_get, 0);
  module_func(set, "click_timer=", set_click_timer_set, 1);
  module_func(set, "soundfont", set_sound_font_get, 0);
  module_func(set, "soundfont=", set_sound_font_set, 1);
  module_func(set, "soundfont_index", set_sound_font_index, 0);
  module_func(set, "soundfonts", set_sound_fonts_get, 0);
  module_func(set, "choose_soundfont", set_sound_font_by_pos, 1);
}

/*
** settings.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2019 Kyonides-Arkanthes
*/

#include "hcextras.h"
#include "graphics.h"
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

void init_settings()
{
  VALUE set = rb_define_module("Settings");
  VALUE formats = rb_ary_new3(2, rstr("jpg"), rstr("png"));
  rb_iv_set(set, "@shot_formats", formats);
  rb_iv_set(set, "@save_dir", rb_str_new_cstr("Saves"));
  rb_iv_set(set, "@save_filename", rb_str_new_cstr("Save"));
  rb_iv_set(set, "@click_timer", RB_INT2FIX(10));
  set_shot_fmt_set(set, rstr("jpg"));
  set_shot_dir_set(set, rb_str_new_cstr("Screenshots"));
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
}

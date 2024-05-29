/*
** game.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2024 Kyonides-Arkanthes
*/

#include "resolution.h"
#include "hcextras.h"
#include "scripts.h"
#include "graphics.h"
#include "binding.h"
#include "binding-util.h"
#include "sharedmidistate.h"
#include "sharedstate.h"
#include <string>
#include "exception.h"
#include "debugwriter.h"
#include <zlib.h>

extern const char module_rpg1[];
extern const char module_rpg2[];
extern const char module_rpg3[];
extern const char win32api_fake[];

void windowBindingInit();
void tilemapBindingInit();
void windowVXBindingInit();
void tilemapVXBindingInit();
void hc_rb_splash(VALUE exception);

static VALUE hc_data_dir(VALUE self)
{
  const std::string &path = shState->config().customDataPath;
  const char *s = path.empty() ? "." : path.c_str();
  return rstr(s);
}

static VALUE game_set_internal_values(VALUE self)
{
  VALUE rversion, width, height, ttl, ver, scr, enc, path, rtp_ary, icon, sfont;
  rversion = rb_const_get(self, rb_intern("RGSS_VERSION"));
  width = rb_const_get(self, rb_intern("WIDTH"));
  height = rb_const_get(self, rb_intern("HEIGHT"));
  ttl = rb_const_get(self, rb_intern("TITLE"));
  ver = rb_const_get(self, rb_intern("VERSION"));
  scr = rb_const_get(self, rb_intern("SCRIPTS"));
  enc = rb_const_get(self, rb_intern("ENCRYPTED_NAME"));
  rtp_ary = rb_const_get(self, rb_intern("RTP"));
  icon = rb_const_get(self, rb_intern("ICON"));
  sfont = rb_iv_get(self, "@soundfont");
  int rgss, w, h, rtp_len;
  rgss = RB_FIX2INT(rversion);
  w = RB_FIX2INT(width);
  h = RB_FIX2INT(height);
  rtp_len = RARRAY_LEN(rtp_ary);
  const char *title = RSTRING_PTR(ttl);
  const char *version = RSTRING_PTR(ver);
  const char *scripts = RSTRING_PTR(scr);
  const char *enc_name = RSTRING_PTR(enc);
  const char *sf = RSTRING_PTR(sfont);
  std::vector<std::string> c_rtp;
  std::string str;
  print_out(1, "RTP List:");
  for (int n = 0; n < rtp_len; n++) {
    path = rb_ary_entry(rtp_ary, n);
    rb_print(1, path);
    str = RSTRING_PTR(path);
    c_rtp.push_back(str);
  }
  if (START_WIDTH != w || START_HEIGHT != h)
    shState->init_size(w, h);
  shState->set_title(title);
  shState->reset_config(rgss, version, scripts, c_rtp);
  shState->check_encrypted_game_file(enc_name);
  if (RSTRING_LEN(icon) > 4) {
    const char *icon_path = RSTRING_PTR(icon);
    shState->set_icon(icon_path);
  }
  if (RSTRING_LEN(sfont) > 4)
    shState->midiState().set_default_soundfont(sf);
  return Qnil;
}

static VALUE game_shot_fmt_set(VALUE self, VALUE fmt)
{
  char *ext = StringValueCStr(fmt);
  shState->graphics().set_screenshot_format(ext);
  return rb_iv_set(self, "@shot_format", fmt);
}

static VALUE game_shot_dir_set(VALUE self, VALUE sdir)
{
  char* dir = StringValueCStr(sdir);
  shState->graphics().set_screenshot_dir(dir);
  return rb_iv_set(self, "@shot_dir", sdir);
}

static VALUE game_shot_filename_set(VALUE self, VALUE name)
{
  char* fn = StringValueCStr(name);
  shState->graphics().set_screenshot_fn(fn);
  return rb_iv_set(self, "@shot_filename", name);
}

static VALUE game_sound_font_set(VALUE self, VALUE fn)
{
  fn = rb_obj_as_string(fn);
  VALUE sfont = rb_iv_get(self, "@soundfont");
  if (rb_str_equal(sfont, fn) == Qtrue)
    return sfont;
  const char *sf = RSTRING_PTR(fn);
  shState->midiState().set_soundfont(sf);
  return rb_iv_set(self, "@soundfont", fn);
}

VALUE game_sound_font_init(VALUE self, int n)
{
  VALUE sfonts = rb_iv_get(self, "@soundfonts");
  VALUE sfont = rb_ary_entry(sfonts, n);
  if (sfont == Qnil) {
    return rb_iv_get(self, "@soundfont");
  } else {
    return game_sound_font_set(self, sfont);
  }
}

static VALUE game_sound_font_by_pos(VALUE self, VALUE n)
{
  return game_sound_font_init(self, RB_FIX2INT(n));
}

void init_game()
{
  VALUE game = rb_define_module("Game");
  rb_const_set(game, rb_intern("START_WIDTH"), RB_INT2FIX(START_WIDTH));
  rb_const_set(game, rb_intern("START_HEIGHT"), RB_INT2FIX(START_HEIGHT));
  module_func(game, "set_internal_values", game_set_internal_values, 0);
  module_func(game, "shot_format=", game_shot_fmt_set, 1);
  module_func(game, "shot_dir=", game_shot_dir_set, 1);
  module_func(game, "shot_filename=", game_shot_filename_set, 1);
  module_func(game, "soundfont=", game_sound_font_set, 1);
  module_func(game, "choose_soundfont", game_sound_font_by_pos, 1);
  rb_define_alias(game, "change_soundfont", "choose_soundfont");
}

/*
** game.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2024 Kyonides-Arkanthes
*/

#include "hcextras.h"
#include "resolution.h"
#include "sharedstate.h"
#include "debugwriter.h"

static VALUE game_set_internal_values(VALUE self)
{
  VALUE rgss_version, width, height, ttl, ver, scr, enc, entry, rtp_ary;
  rgss_version = rb_const_get(self, rb_intern("RGSS_VERSION"));
  width = rb_const_get(self, rb_intern("WIDTH"));
  height = rb_const_get(self, rb_intern("HEIGHT"));
  ttl = rb_const_get(self, rb_intern("TITLE"));
  ver = rb_const_get(self, rb_intern("VERSION"));
  scr = rb_const_get(self, rb_intern("SCRIPTS"));
  enc = rb_const_get(self, rb_intern("ENCRYPTED_NAME"));
  rtp_ary = rb_const_get(self, rb_intern("RTP"));
  int rgss, w, h, rtp_len;
  rgss = RB_FIX2INT(rgss_version);
  w = RB_FIX2INT(width);
  h = RB_FIX2INT(height);
  rtp_len = RARRAY_LEN(rtp_ary);
  const char *title = RSTRING_PTR(ttl);
  const char *version = RSTRING_PTR(ver);
  const char *scripts = RSTRING_PTR(scr);
  const char *enc_name = RSTRING_PTR(enc);
  std::vector<std::string> c_rtp;
  std::string str;
  print_out(1, "RTP List:");
  for (int n = 0; n < rtp_len; n++) {
    entry = rb_ary_entry(rtp_ary, n);
    rb_print(1, entry);
    str = RSTRING_PTR(entry);
    c_rtp.push_back(str);
  }
  if (START_WIDTH != w || START_HEIGHT != h)
    shState->init_size(w, h);
  shState->set_title(title);
  shState->reset_config(rgss, version, scripts, c_rtp);
  shState->check_encrypted_game_file(enc_name);
  return Qnil;
}

void init_game()
{
  VALUE game = rb_define_module("Game");
  module_func(game, "set_internal_values", game_set_internal_values, 0);
  rb_const_set(game, rb_intern("START_WIDTH"), RB_INT2FIX(START_WIDTH));
  rb_const_set(game, rb_intern("START_HEIGHT"), RB_INT2FIX(START_HEIGHT));
}

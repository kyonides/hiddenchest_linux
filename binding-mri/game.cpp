/*
** game.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2024 Kyonides-Arkanthes
*/

#include "hcextras.h"
#include "sharedstate.h"

static VALUE game_set_internal_values(VALUE self)
{
  VALUE rgss_version = rb_const_get(self, rb_intern("RGSS_VERSION"));
  int rgss = RB_FIX2INT(rgss_version);
  const char *title = RSTRING_PTR(rb_const_get(self, rb_intern("TITLE")));
  const char *version = RSTRING_PTR(rb_const_get(self, rb_intern("VERSION")));
  const char *scripts = RSTRING_PTR(rb_const_get(self, rb_intern("SCRIPTS")));
  const char *enc_name = RSTRING_PTR(rb_const_get(self, rb_intern("ENCRYPTED_NAME")));
  shState->set_title(title);
  shState->reset_config(rgss, version, scripts);
  shState->check_encrypted_game_file(enc_name);
  return Qnil;
}

void init_game()
{
  VALUE game = rb_define_module("Game");
  module_func(game, "set_internal_values", game_set_internal_values, 0);
}

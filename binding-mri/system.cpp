/*
** system.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2023 Kyonides-Arkanthes
*/

#include "author.h"
#include "hcextras.h"
#include "graphics.h"
#include "sharedstate.h"
#include "SDL_locale.h"

const char* sys_kind = SYSTEM_STRING;
SDL_Locale *locales;

static VALUE system_platform_get(VALUE self)
{
  return rb_const_get(self, rb_intern("NAME"));
}

static VALUE system_is_linux(VALUE self)
{
  return rb_str_equal(system_platform_get(self), rstr("linux"));
}

static VALUE system_user_lang_get(VALUE self)
{
  return rb_iv_get(self, "user_language");
}

bool system_is_really_windows()
{
  return !strcmp(SYSTEM_STRING, "windows");
}

bool system_is_really_linux()
{
  return !strcmp(SYSTEM_STRING, "linux");
}

void init_system()
{
  locales = SDL_GetPreferredLocales();
  VALUE sys, hidden;
  sys = rb_define_module("System");
  rb_iv_set(sys, "user_language", rstr(locales[0].language));
  rb_define_const(sys, "NAME", rstr(SYSTEM_REAL_STRING));
  rb_define_const(sys, "FAMILY_NAME", rstr(SYSTEM_STRING));
  rb_define_module_function(sys, "platform", RMF(system_platform_get), 0);
  rb_define_module_function(sys, "linux?", RMF(system_is_linux), 0);
  rb_define_module_function(sys, "user_language", RMF(system_user_lang_get), 0);
  hidden = rb_define_module("HiddenChest");
  rb_define_const(hidden, "LOGO", rstr("app_logo"));
  rb_define_const(hidden, "AUTHOR", rstr(HIDDENAUTHOR));
  rb_define_const(hidden, "VERSION", rstr(HIDDENVERSION));
  rb_define_const(hidden, "RELEASE_DATE", rstr(HIDDENDATE));
  rb_define_const(sys, "CODENAME", rstr(CODENAME));
  rb_define_const(hidden, "DESCRIPTION",
    rstr("An RGSS based engine derived from mkxp developed by Ancurio"));
}

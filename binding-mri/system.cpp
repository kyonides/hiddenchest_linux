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

static VALUE system_user_lang_get(VALUE self)
{
  return rb_iv_get(self, "user_language");
}

void init_system()
{
  locales = SDL_GetPreferredLocales();
  VALUE sys = rb_define_module("System");
  rb_iv_set(sys, "user_language", rstr(locales[0].language));
  rb_define_const(sys, "NAME", rstr(SYSTEM_REAL_STRING));
  rb_define_const(sys, "FAMILY_NAME", rstr(SYSTEM_STRING));
  rb_define_module_function(sys, "platform", RMF(system_platform_get), 0);
  rb_define_module_function(sys, "user_language", RMF(system_user_lang_get), 0);
  VALUE mod = rb_define_module("HIDDENCHEST");
  rb_define_const(mod, "LOGO", rstr("app_logo"));
  rb_define_const(mod, "AUTHOR", rstr(HIDDENAUTHOR));
  rb_define_const(mod, "VERSION", rstr(HIDDENVERSION));
  rb_define_const(mod, "RELEASE_DATE", rstr(HIDDENDATE));
  rb_define_const(mod, "DESCRIPTION",
    rstr("An RGSS based engine derived from mkxp developed by Ancurio"));
}

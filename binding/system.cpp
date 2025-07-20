/*
** system.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2025 Kyonides-Arkanthes
*/

#include "author.h"
#include "hcextras.h"
#include "graphics.h"
#include "SDL_locale.h"

const char* sys_kind = SYSTEM_STRING;
SDL_Locale *locales;

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
  VALUE sys = rb_define_module("System");
  rb_iv_set(sys, "@user_language", rstr(locales[0].language));
  rb_define_const(sys, "NAME", rstr(SYSTEM_REAL_STRING));
  rb_define_const(sys, "FAMILY_NAME", rstr(SYSTEM_STRING));
  rb_define_const(sys, "CODENAME", rstr(CODENAME));
}

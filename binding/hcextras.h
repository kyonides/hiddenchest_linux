/*
** hcextras.h
**
** This file is part of HiddenChest.
**
** Copyright (C) 2019 Kyonides-Arkanthes <kyonides@gmail.com>
**
** HiddenChest is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** HiddenChest is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with HiddenChest.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HCEXTRAS_H
#define HCEXTRAS_H

#include <stdarg.h>
#include "binding-util.h"

#define RMF(func) ((VALUE (*)(ANYARGS))(func))
#define rstr(x) rb_str_new_cstr(x)
#define c_rb_ary(a) RARRAY(a)->as.heap.ptr

#define module_func(mod, name, func, args) \
  rb_define_module_function(mod, name, RMF(func), args)
  
#define module_pfunc(mod_meta, name, func, args) \
  rb_define_private_method(mod_meta, name, RMF(func), args)

#define ARRAY_TYPE_P(obj) (!RB_SPECIAL_CONST_P(obj) && \
  RB_BUILTIN_TYPE(obj) == RUBY_T_ARRAY)

#define SYMBOL_TYPE_P(obj) RB_DYNAMIC_SYM_P(obj)

static VALUE hc_sym(const char* str)
{
  return rb_id2sym(rb_intern(str));
}

static VALUE hc_sym2(VALUE str)
{
  return rb_id2sym(rb_intern_str(str));
}

static VALUE print_out(int total, ...)
{
  VALUE line;
  va_list list;
  va_start(list, total);
  try {
    for (int n = 0; n < total; n++) {
      const char* cstr = va_arg(list, const char*);
      line = rb_str_new_cstr(cstr);
      rb_io_write(rb_stdout, line);
      rb_io_write(rb_stdout, rb_default_rs);
    }
  } catch (Exception &e) {
    raiseRbExc(e);
  }
  va_end(list);
  return Qnil;
}

static void rb_print(int total, ...)
{
  VALUE line;
  va_list list;
  va_start(list, total);
  try {
    for (int n = 0; n < total; n++) {
      line = va_arg(list, VALUE);
      rb_io_write(rb_stdout, line);
      rb_io_write(rb_stdout, rb_default_rs);
    }
  } catch (Exception &e) {
    raiseRbExc(e);
  }
  va_end(list);
}

void safe_mkdir(VALUE dir);

#endif

/*
** output.h
**
** This file is part of HiddenChest.
**
** Copyright (C) 2019-2024 Kyonides-Arkanthes <kyonides@gmail.com>
*/

#ifndef PRINT_OUT_H
#define PRINT_OUT_H

#include <stdarg.h>
#include <ruby.h>

static VALUE print_out(int total, ...)
{
  VALUE line;
  va_list list;
  va_start(list, total);
  for (int n = 0; n < total; n++) {
    line = rb_str_new_cstr(va_arg(list, const char*));
    rb_io_write(line, rb_default_rs);
  }
  va_end(list);
  return Qnil;
}

#endif
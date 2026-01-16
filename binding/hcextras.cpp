/*
** hcextras.cpp
**
** This file is part of HiddenChest.
**
** Copyright (C) 2019-26 Kyonides <kyonides@gmail.com>
**
*/

#include "hcextras.h"

VALUE print_out(int total, ...)
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

void rb_print(int total, ...)
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

void print_file(int size, VALUE error)
{
  VALUE filename = rb_ary_shift(error);
  const char *fn = RSTRING_PTR(filename);
  VALUE file = rb_file_open(fn, "a");
  size--; 
  VALUE btr[size];
  for (int n = 0; n < size; n++)
    btr[n] = rb_ary_entry(error, n);
  rb_io_puts(size, btr, file);
  rb_io_close(file);
  rb_ary_clear(error);
}
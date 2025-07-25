/*
** scripts.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2024 Kyonides-Arkanthes
*/

#include "binding-util.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "hcextras.h"
#include "scripts.h"
#include "debugwriter.h"

#define CRMF(func) ((int (*)(ANYARGS))(func))

static VALUE scripts;

static int get_keys(VALUE key, VALUE value, VALUE ary)
{
  rb_ary_push(ary, key);
  return 0;
}

static VALUE rb_hash_keys(VALUE hash)
{
  VALUE keys = rb_ary_new();
  rb_hash_foreach(hash, CRMF(get_keys), keys);
  return keys;
}

static VALUE scripts_sections(VALUE self)
{
  return rb_iv_get(self, "@sections");
}

static VALUE scripts_list(VALUE self)
{
  return rb_iv_get(self, "@pack");
}

static VALUE scripts_names(VALUE self)
{
  return rb_hash_keys(rb_iv_get(self, "@pack"));
}

static VALUE scripts_dependencies(VALUE self)
{
  VALUE pack, keys, result;
  pack = rb_iv_get(self, "@pack");
  keys = rb_hash_keys(pack);
  result = rb_ary_new();
  int max = RARRAY_LEN(keys);
  for (int i = 0; i < max; i++)
    rb_ary_push(result, rb_hash_aref(pack, rb_ary_entry(keys, i)));
  return result;
}

static VALUE scripts_is_included(VALUE self, VALUE name)
{
  VALUE keys = rb_hash_keys(rb_iv_get(self, "@pack"));
  return rb_ary_includes(keys, name);
}

static VALUE scripts_get(VALUE self, VALUE name)
{
  return rb_hash_aref(rb_iv_get(self, "@pack"), name);
}

static VALUE scripts_set(VALUE self, VALUE name, VALUE dependencies)
{
  return rb_hash_aset(rb_iv_get(self, "@pack"), name, dependencies);
}

static VALUE scripts_scene_get(VALUE self)
{
  return rb_iv_get(self, "@scene");
}

static VALUE scripts_scene_set(VALUE self, VALUE name)
{
  return rb_iv_set(self, "@scene", name);
}

static VALUE scripts_main_index_get(VALUE self)
{
  return rb_iv_get(self, "@main_index");
}

static VALUE scripts_main_name_get(VALUE self)
{
  return rb_iv_get(self, "@main_name");
}

static VALUE scripts_main_name_set(VALUE self, VALUE name)
{
  return rb_iv_set(self, "@main_name", name);
}

void scripts_open_log(VALUE mod, VALUE klass, VALUE msg, VALUE bt)
{
  rb_iv_set(mod, "@show_backdrop", Qtrue);
  rb_iv_set(mod, "@error_type", klass);
  rb_iv_set(mod, "@error_msg", msg);
  VALUE names, fn, file, writable, hidden;
  names = rb_gv_get("$RGSS_SCRIPTS");
  try {
    file = rb_file_open("error.log", "w");
  } catch (...) {
    Debug() << "Unable to open file error.log";
    Debug() << "Is the filesystem in read-only mode?";
    return;
  }
  //if ( RARRAY_LEN(bt) > 0 ) rb_ary_pop(bt);
  int max = RARRAY_LEN(bt);
  hidden = rb_define_module("HiddenChest");
  fn = rb_iv_get(hidden, "@filename");
  if (fn != Qnil)
    Debug() << "Unable to open file " << RSTRING_PTR(fn) << ".";
  Debug() << "Backtrace Lines:" << max;
  VALUE btr[max + 2];
  btr[0] = rb_str_plus(rstr("Error Type: "), klass);
  btr[1] = rb_str_plus(msg, rstr("\n"));
  rb_io_write(file, btr[0]);
  rb_io_write(file, rb_default_rs);
  rb_io_write(file, btr[1]);
  if (bt == Qnil) {
    if (names != Qnil) {
      VALUE name = rb_iv_get(hidden, "@script_name");
      Debug() << RSTRING_PTR(name);
      rb_io_write(file, rstr("Script "));
      rb_io_write(file, name);
      rb_io_write(file, rb_default_rs);
    }
    rb_io_close(file);
    rb_io_puts(2, btr, rb_stdout);
    return;
  }
  //printf("No. of lines: %i\n", max);
  VALUE ary, pos, line;
  const char* str = "\\d+";
  const char* eval_str = "eval:";
  if (max < 2) {
    rb_io_write(file, rstr("Backtrace was empty!"));
  } else if (names == Qnil) {
    VALUE name = rb_iv_get(hidden, "@script_name");
    printf("%s\n", StringValueCStr(name));
    for (int n = 0; n < max; n++) {
      line = rb_ary_entry(bt, n);
      btr[n + 2] = line;
      //rb_io_write(file, rstr("Script "));
      //rb_io_write(file, name);
      rb_io_write(file, rstr("in "));
      rb_io_write(file, line);
      rb_io_write(file, rb_default_rs);
    }
  } else {
    VALUE regex_digit, regex_eval, test_regex;
    regex_digit = rb_reg_new(str, strlen(str), 0);
    regex_eval = rb_reg_new(eval_str, strlen(eval_str), 0);
    for (int n = 0; n < max; n++) {
      line = rb_ary_entry(bt, n);
      btr[n + 2] = line;
      ary = rb_funcall(line, rb_intern("scan"), 1, regex_digit);
      ary = rb_ary_entry(ary, 0);
      pos = rb_funcall(ary, rb_intern("to_i"), 0);
      test_regex = rb_funcall(line, rb_intern("scan"), 1, regex_eval);
      if ( rb_ary_entry(test_regex, 0) == Qnil ) {
        ary = rb_ary_entry(names, RB_FIX2INT(pos));
      } else {
        ary = rb_ary_new3(2, rstr(""), rstr("eval"));
      }
      //rb_io_write(file, rstr("Script "));
      //rb_io_write(file, rb_ary_entry(ary, 1));
      rb_io_write(file, rstr("in "));
      rb_io_write(file, line);
      rb_io_write(file, rb_default_rs);
    }
  }
  rb_io_close(file);
  rb_io_puts(max, btr, rb_stdout);
}

void scripts_error_handling()
{
  VALUE rb_error = rb_gv_get("$!");
  if (rb_obj_class(rb_error) == getRbData()->exc[Reset])
    shState->rtData().rqReset.clear();
}

static VALUE scripts_error_handling_rb(VALUE self)
{
  VALUE rb_error = rb_gv_get("$!");
  if (rb_obj_class(rb_error) == getRbData()->exc[Reset])
    shState->rtData().rqReset.clear();
  return Qnil;
}

void init_scripts()
{
  scripts = rb_define_module("Scripts");
  rb_iv_set(scripts, "@sections", rb_ary_new());
  rb_iv_set(scripts, "@list", rb_ary_new());
  rb_iv_set(scripts, "@pack", rb_hash_new());
  rb_iv_set(scripts, "@main_name", rstr("Main"));
  rb_iv_set(scripts, "@main_index", RB_INT2FIX(0));
  module_func(scripts, "sections", scripts_sections, 0);
  module_func(scripts, "list", scripts_list, 0);
  module_func(scripts, "names", scripts_names, 0);
  module_func(scripts, "dependencies", scripts_dependencies, 0);
  module_func(scripts, "include?", scripts_is_included, 1);
  module_func(scripts, "[]", scripts_get, 1);
  module_func(scripts, "[]=", scripts_set, 2);
  module_func(scripts, "main_index", scripts_main_index_get, 0);
  module_func(scripts, "main_index=", scripts_main_index_set, 1);
  module_func(scripts, "main_name", scripts_main_name_get, 0);
  module_func(scripts, "main_name=", scripts_main_name_set, 1);
  module_func(scripts, "scene", scripts_scene_get, 0);
  module_func(scripts, "scene=", scripts_scene_set, 1);
  module_func(scripts, "error_handling", scripts_error_handling_rb, 0);
}

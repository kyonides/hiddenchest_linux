 /*
** system.cpp
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2025 Kyonides-Arkanthes
*/

#include "author.h"
#include "hcextras.h"

extern const char scene_hc[];

static VALUE hidden_fn_get(VALUE self)
{
  return rb_iv_get(self, "@filename");
}

static VALUE hidden_fn_set(VALUE self, VALUE str)
{
  return rb_iv_set(self, "@filename", str);
}

static VALUE hidden_show_backdrop_get(VALUE self)
{
  return rb_iv_get(self, "@show_backdrop");
}

static VALUE hidden_error_type_get(VALUE self)
{
  return rb_iv_get(self, "@error_type");
}

static VALUE hidden_error_msg_get(VALUE self)
{
  return rb_iv_get(self, "@error_msg");
}

static VALUE hidden_script_name_get(VALUE self)
{
  return rb_iv_get(self, "@script_name");
}

static VALUE hidden_show_backdrop_set(VALUE self, VALUE boolean)
{
  return rb_iv_set(self, "@show_backdrop", boolean);
}

static VALUE hidden_error_type_set(VALUE self, VALUE boolean)
{
  return rb_iv_set(self, "@error_type", boolean);
}

static VALUE hidden_error_msg_set(VALUE self, VALUE boolean)
{
  return rb_iv_set(self, "@error_msg", boolean);
}

static VALUE hidden_script_name_set(VALUE self, VALUE str)
{
  return rb_iv_set(self, "@script_name", str);
}

static VALUE hidden_open_error_scene(VALUE self)
{
  int state;
  rb_eval_string_protect(scene_hc, &state);
  if (state) {
    rb_p(rb_errinfo());
    return Qfalse;
  }
  return Qtrue;
}

void init_hiddenchest()
{
  VALUE hidden = rb_define_module("HiddenChest");
  rb_define_const(hidden, "AUTHOR", rstr(HIDDENAUTHOR));
  rb_define_const(hidden, "VERSION", rstr(HIDDENVERSION));
  rb_define_const(hidden, "RELEASE_DATE", rstr(HIDDENDATE));
  rb_define_const(hidden, "CODENAME", rstr(CODENAME));
  rb_iv_set(hidden, "@error_type", rstr(""));
  rb_iv_set(hidden, "@error_msg", rstr(""));
  module_func(hidden, "filename", hidden_fn_get, 0);
  module_func(hidden, "filename=", hidden_fn_set, 1);
  module_func(hidden, "show_backdrop", hidden_show_backdrop_get, 0);
  module_func(hidden, "error_type", hidden_error_type_get, 0);
  module_func(hidden, "error_msg", hidden_error_msg_get, 0);
  module_func(hidden, "script_name", hidden_script_name_get, 0);
  module_func(hidden, "show_backdrop=", hidden_show_backdrop_set, 1);
  module_func(hidden, "error_type=", hidden_error_type_set, 1);
  module_func(hidden, "error_msg=", hidden_error_msg_set, 1);
  module_func(hidden, "script_name=", hidden_script_name_set, 1);
  module_func(hidden, "open_error_scene", hidden_open_error_scene, 0);
}
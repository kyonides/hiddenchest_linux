/*
** filesystem-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "binding-util.h"
#include "sharedstate.h"
#include "filesystem.h"
#include "util.h"
#include <ruby/encoding.h>
#include <ruby/intern.h>
#include "hcextras.h"

extern void hc_rb_splash(VALUE exception);
static VALUE eFileError;

void safe_mkdir(VALUE dir)
{
  if (!rb_funcall(rb_cDir, rb_intern("exist?"), 1, dir))
    rb_funcall(rb_cDir, rb_intern("mkdir"), 1, dir);
}

static void fileIntFreeInstance(void *inst)
{
  SDL_RWops *ops = static_cast<SDL_RWops*>(inst);
  SDL_RWclose(ops);
  SDL_FreeRW(ops);
}

rb_data_type_t FileIntType = { "FileIntr",
  { 0, fileIntFreeInstance, 0, 0, { 0 } }, 0, 0, 0 };

static VALUE fileIntForPath(const char *path, bool rubyExc)
{
  SDL_RWops *ops = SDL_AllocRW();
  try {
    shState->fileSystem().openReadRaw(*ops, path);
  } catch (const Exception &e) {
    SDL_FreeRW(ops);
    if (rubyExc)
      raiseRbExc(e);
    else
      throw e;
  }
  VALUE klass = rb_const_get(rb_cObject, rb_intern("FileInt"));
  VALUE obj = rb_obj_alloc(klass);
  RTYPEDDATA_DATA(obj) = ops;
  return obj;
}

RB_METHOD(fileIntRead)
{
  int length = -1;
  rb_get_args(argc, argv, "i", &length RB_ARG_END);
  SDL_RWops *ops = getPrivateData<SDL_RWops>(self);
  if (length == -1) {
    Sint64 cur = SDL_RWtell(ops);
    Sint64 end = SDL_RWseek(ops, 0, SEEK_END);
    length = end - cur;
    SDL_RWseek(ops, cur, SEEK_SET);
  }
  if (length == 0) return Qnil;
  VALUE data = rb_str_new(0, length);
  SDL_RWread(ops, RSTRING_PTR(data), 1, length);
  return data;
}

RB_METHOD(fileIntClose)
{
  RB_UNUSED_PARAM;
  SDL_RWops *ops = getPrivateData<SDL_RWops>(self);
  SDL_RWclose(ops);
  return Qnil;
}

RB_METHOD(fileIntGetByte)
{
  RB_UNUSED_PARAM;
  SDL_RWops *ops = getPrivateData<SDL_RWops>(self);
  unsigned char byte;
  size_t result = SDL_RWread(ops, &byte, 1, 1);
  return (result == 1) ? rb_fix_new(byte) : Qnil;
}

RB_METHOD(fileIntBinmode)
{
  RB_UNUSED_PARAM;
  return Qnil;
}

static VALUE fileInt_exist(VALUE self, VALUE name)
{
  const char* fn = StringValueCStr(name);
  return shState->fileSystem().exists_ext(fn) ? Qtrue : Qfalse;
}

static bool file_do_exist(const char *fname)
{
  return shState->fileSystem().exists_ext(fname);
}

VALUE kernelLoadDataInt(const char *fname, bool rubyExc)
{
  rb_gc_start();
  if ( !file_do_exist(fname) ) {
    const char *cause = "No such file or directory.";
    rb_raise(rb_eIOError, "Unable to open file '%s'.\n%s", fname, cause);
    return Qnil;
  }
  VALUE port = fileIntForPath(fname, rubyExc);
  if (RB_NIL_P(port)) return Qnil;
  VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));
  // FIXME need to catch exceptions here with begin rescue
  VALUE result;
  result = rb_funcall2(marsh, rb_intern("load"), 1, &port);
  rb_funcall2(port, rb_intern("close"), 0, NULL);
  return result;
}

static VALUE kernelLoadData(VALUE self, VALUE filename)
{
  return kernelLoadDataInt(StringValueCStr(filename), true);
}

static VALUE kernelSaveData(VALUE self, VALUE obj, VALUE filename)
{
  VALUE file = rb_file_open_str(filename, "wb");
  rb_marshal_dump(obj, file);
  rb_io_close(file);
  return Qnil;
}

static VALUE stringForceUTF8(VALUE arg)
{
  if (RB_TYPE_P(arg, RUBY_T_STRING) && ENCODING_IS_ASCII8BIT(arg))
    rb_enc_associate_index(arg, rb_utf8_encindex());
  return arg;
}

static VALUE customProc(VALUE arg, VALUE proc)
{
  VALUE obj = stringForceUTF8(arg);
  obj = rb_funcall2(proc, rb_intern("call"), 1, &obj);
  return obj;
}

RB_METHOD(_marshalLoad)
{
  RB_UNUSED_PARAM;
  VALUE port, proc = Qnil;
  rb_get_args(argc, argv, "o|o", &port, &proc RB_ARG_END);
  VALUE utf8Proc;
  if (RB_NIL_P(proc))
    utf8Proc = rb_proc_new(RUBY_METHOD_FUNC(stringForceUTF8), Qnil);
  else
    utf8Proc = rb_proc_new(RUBY_METHOD_FUNC(customProc), proc);
  VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));
  VALUE v[] = { port, utf8Proc };
  return rb_funcall2(marsh, rb_intern("_HC_load_alias"), ARRAY_SIZE(v), v);
}

void fileIntBindingInit()
{
  VALUE klass = rb_define_class("FileInt", rb_cIO);
  rb_define_alloc_func(klass, classAllocate<&FileIntType>);
  _rb_define_method(klass, "read", fileIntRead);
  _rb_define_method(klass, "getbyte", fileIntGetByte);
  _rb_define_method(klass, "binmode", fileIntBinmode);
  _rb_define_method(klass, "close", fileIntClose);
  rb_define_singleton_method(rb_cFile, "exist_compressed?", RMF(fileInt_exist), 1);
  rb_define_singleton_method(klass, "exist?", RMF(fileInt_exist), 1);
  rb_define_module_function(rb_mKernel, "load_data", RMF(kernelLoadData), 1);
  rb_define_module_function(rb_mKernel, "save_data", RMF(kernelSaveData), 2);
  /* We overload the built-in 'Marshal::load()' function to silently
   * insert our utf8proc that ensures all read strings will be UTF-8 encoded */
  VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));
  rb_define_alias(rb_singleton_class(marsh), "_HC_load_alias", "load");
  _rb_define_module_function(marsh, "load", _marshalLoad);
}

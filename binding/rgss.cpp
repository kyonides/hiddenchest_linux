/*
** rgss.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** Copyright (C) 2019-2024 Extended by Kyonides Arkanthes <kyonides@gmail.com>
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

#include "binding.h"
#include "binding-util.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "filesystem.h"
#include "util.h"
#include "sdl-util.h"
#include "debugwriter.h"
#include "graphics.h"
#include "audio.h"
#include "boost-hash.h"
#include "hcextras.h"
#include <ruby/encoding.h>
#include <ruby/io.h>
#include <assert.h>
#include <string>
#include <zlib.h>
#include <SDL_video.h>
#include <SDL_filesystem.h>
#include "scripts.h"

#define ch const char*

extern const char module_rpg_audio[];
extern const char module_rpg1[];
extern const char module_rpg2[];
extern const char module_rpg3[];
extern const char win32api_fake[];
extern const char module_hc[];
extern const char game_ini[];
static void mriBindingExecute();
static void mriBindingTerminate();
static void mriBindingReset();

extern "C" {
void init_zlib();
}
void init_mouse();
void init_game(ch raw_exe_name);
void init_scripts();
void init_system();
void init_backdrop();
int system_is_really_linux();
const char *scripts;
static VALUE hidden, script_ary;

ScriptBinding scriptBindingImpl =
{
  mriBindingExecute,
  mriBindingTerminate,
  mriBindingReset
};

ScriptBinding *scriptBinding = &scriptBindingImpl;

void tableBindingInit();
void etcBindingInit();
void init_font_binding();
void bitmapBindingInit();
void SpriteBindingInit();
void sprite_setup_bush_opacity();
void MsgBoxSpriteBindingInit();
void viewportBindingInit();
void planeBindingInit();
void windowBindingInit();
void tilemapBindingInit();
void windowVXBindingInit();
void tilemapVXBindingInit();
void inputBindingInit();
void audioBindingInit();
void graphicsBindingInit();
void fileIntBindingInit();
void set_rgss_default_names();

RB_METHOD(mriPrint);
RB_METHOD(mriP);
RB_METHOD(HCPuts);
RB_METHOD(HCRawKeyStates);
static VALUE HCMouseInWindow(VALUE self);
RB_METHOD(mriRgssMain);
RB_METHOD(mriRgssStop);
RB_METHOD(_kernelCaller);

static VALUE hc_data_dir(VALUE self)
{
  const std::string &path = shState->config().customDataPath;
  const char *s = path.empty() ? "." : path.c_str();
  return rstr(s);
}

void hc_c_splash(ch error, int code, ch type)
{
  rb_iv_set(hidden, "@error_type", rstr(type));
  rb_iv_set(hidden, "@error_msg", rstr(error));
  int state;
  if (code == 1) {
    rb_eval_string_protect(module_hc, &state);
  } else {
    Debug() << "Processing game_ini";
    VALUE game = rb_define_module("Game");
    if (rb_iv_get(game, "@setup_screen") == Qnil) {
      rb_eval_string_protect(game_ini, &state);
      if (state) {
        rb_p(rb_errinfo());
        state = 0;
      }
    }
    rb_eval_string_protect(scene_hc, &state);
  }
  if (state) {
    rb_p(rb_errinfo());
    Debug() << "C Splash - Error Type:" << code << "- State:" << state;
  }
  scripts_error_handling();
}

void hc_rb_splash(VALUE exception)
{
  VALUE klass, message, backtrace;
  klass = rb_obj_as_string(rb_obj_class(exception));
  message = rb_funcall(exception, rb_intern("message"), 0);
  backtrace = rb_funcall(exception, rb_intern("backtrace"), 0);
  scripts_open_log(hidden, klass, message, backtrace);
  int state;
  rb_eval_string_protect(scene_hc, &state);
  scripts_error_handling();
}

static void mriBindingInit()
{
  if (rgssVer == 1) {
    windowBindingInit();
    tilemapBindingInit();
  } else if (rgssVer == 4) {
    windowVXBindingInit();
    tilemapBindingInit();
  } else {
    windowVXBindingInit();
    tilemapVXBindingInit();
  }
  module_func(rb_mKernel, "msgbox", mriPrint, -1);
  module_func(rb_mKernel, "msgbox_p", mriP, -1);
  module_func(rb_mKernel, "print", mriPrint, -1);
  module_func(rb_mKernel, "p", mriP, -1);
  if (rgssVer == 3) {
    module_func(rb_mKernel, "rgss_main", mriRgssMain, -1);
    module_func(rb_mKernel, "rgss_stop", mriRgssStop, -1);
  } else {
    rb_define_alias(rb_singleton_class(rb_mKernel), "_HC_kernel_caller_alias", "caller");
    module_func(rb_mKernel, "caller", _kernelCaller, -1);
  }
  hidden = rb_define_module("HiddenChest");
  VALUE sys = rb_define_module("System");
  module_func(hidden, "puts", HCPuts, -1);
  module_func(hidden, "raw_key_states", HCRawKeyStates, -1);
  module_func(hidden, "mouse_in_window", HCMouseInWindow, 0);
  module_func(sys, "data_dir", hc_data_dir, 0);
  VALUE game, debug, fullscreen;
  game = rb_define_module("Game");
  debug = rb_const_get(game, rb_intern("DEBUG"));
  fullscreen = rb_const_get(game, rb_intern("FULLSCREEN"));
  shState->graphics().set_fullscreen(fullscreen == Qtrue);
  int state;
  if (rgssVer == 1 || rgssVer == 4) {
    rb_gv_set("$DEBUG", debug);
    rb_eval_string_protect(module_rpg1, &state);
  } else {
    rb_gv_set("$TEST", debug);
    if (rgssVer == 2)
      rb_eval_string_protect(module_rpg2, &state);
    else if (rgssVer == 3)
      rb_eval_string_protect(module_rpg3, &state);
  }
  if (state) {
    Debug() << "C Splash - Error State:" << state;
    scripts_error_handling();
    return;
  }
  Debug() << "Loaded RGSS Module";
  // Load global constants
  rb_gv_set("HiddenChest", Qtrue);
  rb_gv_set("$BTEST", shState->config().editor.battleTest ? Qtrue : Qfalse);
  Debug() << "Loading Fake Win32API...";
  rb_eval_string(win32api_fake);
}

static void showMsg(const std::string &msg)
{
  shState->eThread().showMessageBox(msg.c_str());
}

static void printP(int argc, VALUE *argv, ch convMethod, ch sep)
{
  VALUE dispString = rb_str_buf_new(128);
  ID conv = rb_intern(convMethod);
  for (int i = 0; i < argc; ++i) {
    VALUE str = rb_funcall2(argv[i], conv, 0, NULL);
    rb_str_buf_append(dispString, str);
    if (i < argc) rb_str_buf_cat2(dispString, sep);
  }
  ch msg = RSTRING_PTR(dispString);
  Debug() << msg;
  VALUE error = rb_errinfo();
  if (error != Qnil)
    hc_rb_splash( error );
  if (rb_iv_get(hidden, "@error_found") == Qtrue)
    hc_rb_splash( rb_iv_get(hidden, "@exception") );
  //if (rb_iv_get(hidden, "@error_found") == Qtrue)
    //hc_c_splash("No such file or directory", 2, "Errno::ENOENT");
  showMsg(msg);
}

static VALUE mriPrint(int argc, VALUE* argv, VALUE self)
{
  printP(argc, argv, "to_s", "");
  return Qnil;
}

static VALUE mriP(int argc, VALUE* argv, VALUE self)
{
  printP(argc, argv, "inspect", "\n");
  return Qnil;
}

static VALUE HCPuts(int argc, VALUE* argv, VALUE self)
{
  RB_UNUSED_PARAM;
  const char *str;
  rb_get_args(argc, argv, "z", &str RB_ARG_END);
  Debug() << str;
  return Qnil;
}

static VALUE HCRawKeyStates(int argc, VALUE* argv, VALUE self)
{
  VALUE str = rb_str_new(0, sizeof(EventThread::keyStates));
  memcpy(RSTRING_PTR(str), EventThread::keyStates, sizeof(EventThread::keyStates));
  return str;
}

static VALUE HCMouseInWindow(VALUE self)
{
  return EventThread::mouseState.inWindow ? Qtrue : Qfalse;
}

static VALUE rgssMainCb(VALUE block)
{
  rb_funcall2(block, rb_intern("call"), 0, 0);
  return Qnil;
}

static VALUE rgssMainRescue(VALUE arg, VALUE exc)
{
  VALUE *excRet = (VALUE*) arg;
  *excRet = exc;
  return Qnil;
}

static VALUE newStringUTF8(ch string, long length)
{
  return rb_enc_str_new(string, length, rb_utf8_encoding());
}

struct evalArg
{
  VALUE string;
  VALUE filename;
};

static VALUE evalHelper(evalArg *arg)
{
  VALUE argv[] = { arg->string, Qnil, arg->filename };
  return rb_funcall2(Qnil, rb_intern("eval"), ARRAY_SIZE(argv), argv);
}

static VALUE evalString(VALUE string, VALUE filename, int *state)
{
  evalArg arg = { string, filename };
  return rb_protect((VALUE (*)(VALUE))evalHelper, (VALUE)&arg, state);
}

void process_main_script_reset()
{// Find Main Script Index and Execute Main
  shState->rtData().rqReset.clear();
  VALUE mod, script_section, fname, script, string;
  mod = rb_const_get(rb_cObject, rb_intern("Scripts"));
  int state, index = RB_FIX2INT(rb_iv_get(mod, "@main_index"));
  Debug() << "Reloading Main Script";
  script_section = rb_ary_entry(script_ary, index);
  fname = rb_ary_entry(script_section, 1);
  script = rb_ary_entry(script_section, 3);
  shState->graphics().repaintWait(shState->rtData().rqResetFinish, false);
  string = newStringUTF8(RSTRING_PTR(script), RSTRING_LEN(script));
  evalString(string, fname, &state);
}

RB_METHOD(mriRgssMain)
{
  RB_UNUSED_PARAM;
  while (true) {
    VALUE exc = Qnil;
    rb_rescue2((VALUE(*)(ANYARGS)) rgssMainCb, rb_block_proc(),
               (VALUE(*)(ANYARGS)) rgssMainRescue, (VALUE) &exc,
               rb_eException, (VALUE) 0);
    if (exc == Qnil) break;
    if (rb_obj_class(exc) == getRbData()->exc[Reset])
      process_main_script_reset();
    else
      rb_exc_raise(exc);
  }
  return Qnil;
}

RB_METHOD(mriRgssStop)
{
  RB_UNUSED_PARAM;
  while (true)
    shState->graphics().update();
  return Qnil;
}

RB_METHOD(_kernelCaller)
{
  RB_UNUSED_PARAM;
  VALUE trace = rb_funcall2(rb_mKernel, rb_intern("_HC_kernel_caller_alias"), 0, 0);
  if (!RB_TYPE_P(trace, RUBY_T_ARRAY))
    return trace;
  long len = RARRAY_LEN(trace);
  if (len < 2)
    return trace;
  /* Remove useless "ruby:1:in 'eval'" */
  rb_ary_pop(trace);
  // Also remove trace of this helper function
  rb_ary_shift(trace);
  len -= 2;
  if (len == 0)
    return trace;
  // RMXP does this, not sure if specific or 1.8 related
  VALUE args[] = { rstr(":in `<main>'"), rstr("") };
  rb_funcall2(rb_ary_entry(trace, len-1), rb_intern("gsub!"), 2, args);
  return trace;
}

static bool file_exist(VALUE name, const char* ext)
{
  name = rb_str_plus(name, rstr(ext));
  const char* fn = StringValueCStr(name);
  return shState->fileSystem().exists(fn);
}

static int rb_check_rgss_version()
{
  int state;
  rb_eval_string_protect(game_ini, &state);
  return state;
}

static void runCustomScript(const std::string &filename)
{
  Debug() << "Running Custom Scripts";
  std::string scriptData;
  if (!readFileSDL(filename.c_str(), scriptData)) {
    std::string error = "Unable to open '" + filename + "'";
    hc_c_splash(error.c_str(), 2, "Errno::ENOENT");
    return;
  }
  evalString(newStringUTF8(scriptData.c_str(), scriptData.size()),
             newStringUTF8(filename.c_str(), filename.size()), NULL);
}

VALUE kernelLoadDataInt(const char *filename, bool rubyExc);

struct BacktraceData
{ // Maps: Ruby visible filename, To: Actual script name
  BoostHash<std::string, std::string> scriptNames;
};

#define SCRIPT_SECTION_FMT (rgssVer == 3 ? "Section%04ld" : "Section%03ld")

static void runRGSSscripts(BacktraceData &btData)
{
  if (rgssVer == 0) {
    hc_c_splash("No game file was found!", 1, "LoadError");
    return;
  }
  const Config &conf = shState->rtData().config;
  VALUE game = rb_define_module("Game");
  VALUE rb_path = rb_const_get(game, rb_intern("SCRIPTS"));
  if (!RSTRING_LEN(rb_path)) {
    ch error = "No game scripts specified (missing main INI file?)";
    Debug() << error;
    showMsg(error);
    return;
  }
  const char *scripts = RSTRING_PTR(rb_path);
  if (!shState->fileSystem().exists(scripts)) {
    VALUE rb_error = rstr("Unable to open '");
    rb_error = rb_str_plus(rb_error, rb_path);
    rb_error = rb_str_plus(rb_error, rstr("'."));
    ch error = RSTRING_PTR(rb_error);
    Debug() << "Errno::ENOENT" << error;
    hc_c_splash(error, 2, "Errno::ENOENT");
    return;
  }
  // We checked if Scripts.rxdata exists, but something might still go wrong
  try {
    script_ary = kernelLoadDataInt(scripts, false);
  } catch (const Exception &e) {
    std::string error = "Failed to read script data: " + e.msg;
    hc_c_splash(error.c_str(), 2, "IOError");
    return;
  }
  if (!RB_TYPE_P(script_ary, RUBY_T_ARRAY)) {
    ch error = "Wrong File Format.\nFailed to read script data.";
    hc_c_splash(error, 2, "TypeError");
    return;
  }
  rb_gv_set("$RGSS_SCRIPTS", script_ary);
  VALUE scripts_mod = rb_define_module("Scripts");
  VALUE names = rb_iv_get(scripts_mod, "@sections");
  if (names == Qnil) {
    names = rb_ary_new();
    rb_iv_set(scripts_mod, "@sections", names);
  }
  Debug() << "Searching for the Main Index...";
  scripts_main_index_set(scripts_mod, find_main_script_index(scripts_mod));
  Debug() << "Loading Scripts now";
  long scriptCount = RARRAY_LEN(script_ary);
  std::string decodeBuffer;
  decodeBuffer.resize(0x128000);
  for (long i = 0; i < scriptCount; ++i) {
    VALUE script = rb_ary_entry(script_ary, i);
    if (!RB_TYPE_P(script, RUBY_T_ARRAY))
      continue;
    VALUE scriptName   = rb_ary_entry(script, 1);
    VALUE scriptString = rb_ary_entry(script, 2);
    rb_ary_push(names, scriptName);
    int result = Z_OK;
    unsigned long bufferLen;
    while (true) {
      unsigned char *bufferPtr =
              reinterpret_cast<unsigned char*>(const_cast<char*>(decodeBuffer.c_str()));
      const unsigned char *sourcePtr =
              reinterpret_cast<const unsigned char*>(RSTRING_PTR(scriptString));
      bufferLen = decodeBuffer.length();
      result = uncompress(bufferPtr, &bufferLen, sourcePtr, RSTRING_LEN(scriptString));
      bufferPtr[bufferLen] = '\0';
      if (result != Z_BUF_ERROR)
        break;
      decodeBuffer.resize(decodeBuffer.size()*2);
    }
    if (result != Z_OK) {
      static char buffer[256];
      snprintf(buffer, sizeof(buffer), "Error decoding script %ld: '%s'",
                i, RSTRING_PTR(scriptName));
      hc_c_splash(buffer, 2, "DecodeError");
      break;
    }
    rb_ary_store(script, 3, rstr(decodeBuffer.c_str()));
  } // Execute preloaded scripts
  for (std::set<std::string>::iterator i = conf.preloadScripts.begin();
      i != conf.preloadScripts.end(); ++i)
    runCustomScript(*i);
  VALUE exc = rb_gv_get("$!");
  if (exc != Qnil)
    return;
  int script_pos = 3, name_pos = 1;
  VALUE section, script, string, fname;
  for (long i = 0; i < scriptCount; ++i) {
    section = rb_ary_entry(script_ary, i);
    script = rb_ary_entry(section, script_pos);
    string = newStringUTF8(RSTRING_PTR(script), RSTRING_LEN(script));
    ch scriptName = RSTRING_PTR(rb_ary_entry(section, name_pos));
    char buf[1024];
    int len;
    len = snprintf(buf, sizeof(buf), "Section%04ld:%s", i, scriptName);
    fname = newStringUTF8(buf, len);
    btData.scriptNames.insert(buf, scriptName);
    int state;
    evalString(string, fname, &state);
    if (state)
      break;
    if (rb_iv_get(hidden, "@not_found") == Qtrue)
      break;
  }
  if (rb_obj_class(rb_gv_get("$!")) != getRbData()->exc[Reset])
    return;
  while (true) {
    if (rb_gv_get("$scene") == Qnil)
      break;
    if (rb_obj_class(rb_gv_get("$!")) != getRbData()->exc[Reset])
      break;
    if (rb_iv_get(hidden, "@not_found") == Qtrue)
      break;
    process_main_script_reset();
  }
  shState->rtData().rqReset.clear();
}

static void showExc(VALUE exc, const BacktraceData &btData)
{
  VALUE bt, msg, bt0, name, ds;
  bt = rb_funcall2(exc, rb_intern("backtrace"), 0, NULL);
  msg = rb_funcall2(exc, rb_intern("message"), 0, NULL);
  bt0 = rb_ary_entry(bt, 0);
  name = rb_class_path(rb_obj_class(exc));
  ds = rb_sprintf("%" PRIsVALUE ": %" PRIsVALUE " (%" PRIsVALUE ")",
                        bt0, exc, name);
  /* omit "useless" last entry (from ruby:1:in `eval') */
  for (long i = 1, btlen = RARRAY_LEN(bt) - 1; i < btlen; ++i)
    rb_str_catf(ds, "\n\tfrom %" PRIsVALUE, rb_ary_entry(bt, i));
  Debug() << StringValueCStr(ds);
  if (bt0 == Qnil) {
    char *message = StringValueCStr(msg);
    std::string ms(640, '\0');
    snprintf(&ms[0], ms.size(), "HiddenChestError: %s", message);
    showMsg(ms);
    return;
  }
  char *s = StringValueCStr(bt0);
  char line[16];
  std::string file(512, '\0');
  char *p = s + strlen(s);
  char *e;
  while (p != s)
    if (*--p == ':')
      break;
  e = p;
  while (p != s)
    if (*--p == ':')
      break;
  /* s         p  e
   * SectionXXX:YY: in 'blabla' */
  *e = '\0';
  strncpy(line, *p ? p+1 : p, sizeof(line));
  line[sizeof(line)-1] = '\0';
  *e = ':';
  e = p;
  /* s         e
   * SectionXXX:YY: in 'blabla' */
  *e = '\0';
  strncpy(&file[0], s, file.size());
  *e = ':';
  /* Shrink to fit */
  file.resize(strlen(file.c_str()));
  file = btData.scriptNames.value(file, file);
  std::string ms(640, '\0');
  snprintf(&ms[0], ms.size(), "Script '%s' line %s: %s occured.\n\n%s",
           file.c_str(), line, RSTRING_PTR(name), RSTRING_PTR(msg));
  showMsg(ms);
}

static void mriBindingExecute()
{/* Normally only a ruby executable would do a sysinit,
 * but not doing it will lead to crashes due to closed
 * stdio streams on some platforms (eg. Windows) */
  int argc = 0;
  char **argv = 0;
  ruby_sysinit(&argc, &argv);
  ruby_init();
  rb_enc_set_default_external(rb_enc_from_encoding(rb_utf8_encoding()));
  Config &conf = shState->rtData().config;
  if (!conf.rubyLoadpaths.empty()) {
    // Setup custom load paths
    VALUE lpaths = rb_gv_get(":");
    for (size_t i = 0; i < conf.rubyLoadpaths.size(); ++i) {
      std::string &path = conf.rubyLoadpaths[i];
      VALUE pathv = rb_str_new(path.c_str(), path.size());
      rb_ary_push(lpaths, pathv);
    }
  }
  RbData rbData;
  shState->setBindingData(&rbData);
  init_zlib();
  fileIntBindingInit();
  tableBindingInit();
  etcBindingInit();
  init_font_binding();
  bitmapBindingInit();
  SpriteBindingInit();
  MsgBoxSpriteBindingInit();
  viewportBindingInit();
  planeBindingInit();
  audioBindingInit();
  graphicsBindingInit();
  inputBindingInit();
  init_mouse();
  init_backdrop();
  init_scripts();
  init_system();
  init_game(shState->rtData().argv0);
  rb_eval_string(module_rpg_audio);
  int state = rb_check_rgss_version();
  if (state) {
    rb_p(rb_errinfo());
    ruby_cleanup(0);
    shState->rtData().rqTermAck.set();
    return;
  }
  set_rgss_default_names();
  sprite_setup_bush_opacity();
  BacktraceData btData;
  mriBindingInit();
  std::string &customScript = conf.customScript;
  if (!customScript.empty())
    runCustomScript(customScript);
  else
    runRGSSscripts(btData);
  VALUE exc = rb_errinfo();
  if (rb_obj_class(exc) == getRbData()->exc[Reset]) {
    exc = Qnil;
    Debug() << "Game Reset";
  }
  if (exc != Qnil && !rb_obj_is_kind_of(exc, rb_eSystemExit))
    hc_rb_splash(exc); //showExc(exc, btData);
  ruby_cleanup(0);
  shState->rtData().rqTermAck.set();
}

static void mriBindingTerminate()
{
  rb_raise(rb_eSystemExit, " ");
}

static void mriBindingReset()
{
  rb_raise(getRbData()->exc[Reset], " ");
}

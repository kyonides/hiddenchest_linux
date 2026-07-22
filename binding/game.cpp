/*
** game.cpp
**
** This file is part of HiddenChest.
**
** Copyright (C) 2018-2026 Kyonides Arkanthes <kyonides@gmail.com>
*/

#include "resolution.h"
#include "hcextras.h"
#include "scripts.h"
#include "graphics.h"
#include "binding.h"
#include "binding-util.h"
#include <SDL_image.h>
#include "sharedstate.h"
#include "sharedmidistate.h"
#include <string>
#include "exception.h"
#include "debugwriter.h"
#include <zlib.h>

#ifdef __WINDOWS__
#include <SDL2/SDL_syswm.h>
#endif

struct SDL_Window;
VALUE zero = RB_INT2FIX(0);

extern const char win32api_fake[];

void windowBindingInit();
void tilemapBindingInit();
void windowVXBindingInit();
void tilemapVXBindingInit();
void hc_rb_splash(VALUE exception);

static VALUE hc_data_dir(VALUE self)
{
  const std::string &path = shState->config().customDataPath;
  const char *s = path.empty() ? "." : path.c_str();
  return rstr(s);
}

static VALUE game_get_title(VALUE self)
{
  return rb_iv_get(self, "title");
}

static VALUE game_set_title(VALUE self, VALUE title)
{
  const char *new_title = RSTRING_PTR(title);
  shState->set_title(new_title);
  return rb_iv_set(self, "title", title);
}

static VALUE game_window_set_icon(VALUE self, VALUE icon_name)
{
  icon_name = rb_funcall(icon_name, rb_intern("to_s"), 0);
  const char *icon = RSTRING_PTR(icon_name);
  SDL_RWops *icon_src = SDL_RWFromFile(icon, "rb");
  SDL_Surface *icon_img = IMG_Load_RW(icon_src, SDL_TRUE);
  if (!icon_img)
    return Qnil;
  SDL_SetWindowIcon(shState->sdlWindow(), icon_img);
  SDL_FreeSurface(icon_img);
  return Qtrue;
}

static VALUE game_set_internal_values(VALUE self)
{
  VALUE win, sys, sfonts, rversion, width, height, ver, user_path;
  VALUE scr, path, enc_ary, rtp_ary, icon, sfont, subimg;
  win = rb_define_module_under(self, "Window");
  sys = rb_define_module("System");
  sfonts = rb_const_get(sys, rb_intern("SOUNDFONT_DIR"));
  rversion = rb_const_get(self, rb_intern("RGSS_VERSION"));
  width = rb_const_get(self, rb_intern("WIDTH"));
  height = rb_const_get(self, rb_intern("HEIGHT"));
  ver = rb_const_get(self, rb_intern("VERSION"));
  scr = rb_const_get(self, rb_intern("SCRIPTS"));
  enc_ary = rb_const_get(self, rb_intern("ENCRYPTED_NAMES"));
  rtp_ary = rb_const_get(self, rb_intern("RTP"));
  subimg = rb_const_get(self, rb_intern("SUBIMAGEFIX"));
  icon = rb_const_get(self, rb_intern("ICON"));
  game_window_set_icon(win, icon);
  sfont = rb_iv_get(self, "@soundfont");
  user_path = rb_iv_get(self, "@user_path");
  user_path = rb_str_plus(user_path, rstr("/"));
  int rgss, w, h, rtp_len, enc_len;
  rgss = RB_FIX2INT(rversion);
  w = RB_FIX2INT(width);
  h = RB_FIX2INT(height);
  rtp_len = RARRAY_LEN(rtp_ary);
  enc_len = RARRAY_LEN(enc_ary);
  const char *sf_dir = RSTRING_PTR(sfonts);
  const char *version = RSTRING_PTR(ver);
  const char *scripts = RSTRING_PTR(scr);
  const char *sf = RSTRING_PTR(sfont);
  std::vector<std::string> c_rtp;
  std::vector<std::string> enc_names;
  std::string str;
  print_out(1, "Encrypted Game Files:");
  for (int n = 0; n < enc_len; n++) {
    path = rb_ary_entry(enc_ary, n);
    rb_print(1, path);
    str = RSTRING_PTR(path);
    enc_names.push_back(str);
  }
  print_out(1, "RTP List:");
  for (int n = 0; n < rtp_len; n++) {
    path = rb_ary_entry(rtp_ary, n);
    rb_print(1, path);
    str = RSTRING_PTR(path);
    c_rtp.push_back(str);
  }
  shState->init_size(w, h);
  shState->reset_config(rgss, version, scripts, c_rtp);
  shState->check_encrypted_game_files(enc_names);
  shState->check_soundfont_dir(sf_dir);
  shState->config().subImageFix = subimg == Qtrue;
  shState->config().customDataPath = RSTRING_PTR(user_path);
  if (RSTRING_LEN(sfont) > 4)
    shState->midiState().set_default_soundfont(sf);
  return Qnil;
}

static VALUE game_shot_fmt_set(VALUE self, VALUE fmt)
{
  char *ext = StringValueCStr(fmt);
  shState->graphics().set_screenshot_format(ext);
  return rb_iv_set(self, "@shot_format", fmt);
}

static VALUE game_shot_dir_set(VALUE self, VALUE sdir)
{
  VALUE path = rb_iv_get(self, "@shot_path");
  path = rb_str_plus(path, rstr("/"));
  path = rb_str_plus(path, sdir);
  char* dir = StringValueCStr(path);
  shState->graphics().set_screenshot_dir(dir);
  return rb_iv_set(self, "@shot_dir", sdir);
}

static VALUE game_shot_filename_set(VALUE self, VALUE name)
{
  char* fn = StringValueCStr(name);
  shState->graphics().set_screenshot_fn(fn);
  return rb_iv_set(self, "@shot_filename", name);
}

static VALUE game_sound_font_set(VALUE self, VALUE fn)
{
  fn = rb_obj_as_string(fn);
  VALUE sfont = rb_iv_get(self, "@soundfont");
  if (rb_str_equal(sfont, fn) == Qtrue)
    return sfont;
  const char *sf = RSTRING_PTR(fn);
  shState->midiState().set_soundfont(sf);
  return rb_iv_set(self, "@soundfont", fn);
}

VALUE game_sound_font_init(VALUE self, int n)
{
  VALUE sfonts = rb_iv_get(self, "@soundfonts");
  VALUE sfont = rb_ary_entry(sfonts, n);
  if (sfont == Qnil) {
    return rb_iv_get(self, "@soundfont");
  } else {
    return game_sound_font_set(self, sfont);
  }
}

static VALUE game_sound_font_by_pos(VALUE self, VALUE n)
{
  return game_sound_font_init(self, RB_FIX2INT(n));
}

static VALUE game_screensaver_enable(VALUE self)
{
  return rb_iv_get(self, "screensaver_enable");
}

static VALUE game_screensaver_enable_set(VALUE self, VALUE state)
{
  if (state == Qtrue)
    SDL_EnableScreenSaver();
  else
    SDL_DisableScreenSaver();
  return rb_iv_set(self, "screensaver_enable", state == Qtrue ? Qtrue : Qfalse);
}

static VALUE game_window_resizable(VALUE self)
{
  return rb_iv_get(self, "resizable");
}

static VALUE game_window_borders(VALUE self)
{
  return rb_iv_get(self, "borders");
}

static VALUE game_window_brightness(VALUE self)
{
  double n = SDL_GetWindowBrightness(shState->sdlWindow());
  int b = static_cast<int>(n * 100);
  return RB_INT2FIX(b);
}

static VALUE game_window_resizable_set(VALUE self, VALUE state)
{
  SDL_bool result = (SDL_bool)(state == Qtrue);
  SDL_SetWindowResizable(shState->sdlWindow(), result);
  return rb_iv_set(self, "resizable", state);
}

static VALUE game_window_borders_set(VALUE self, VALUE state)
{
  SDL_bool result = (SDL_bool)(state == Qtrue);
  SDL_SetWindowBordered(shState->sdlWindow(), result);
  return rb_iv_set(self, "borders", state);
}

static VALUE game_window_brightness_set(VALUE self, VALUE value)
{
  value = rb_funcall(value, rb_intern("to_i"), 0);
  int n = clamp(10, RB_FIX2INT(value), 100);
  double dbl = n / 100.0;
  SDL_SetWindowBrightness(shState->sdlWindow(), dbl);
  return RB_INT2FIX(n);
}

static VALUE game_window_block_close(VALUE self)
{
  return rb_iv_get(self, "block_close");
}

static VALUE game_window_block_close_set(VALUE self, VALUE rstate)
{
  if (rb_iv_get(self, "block_close") == rstate)
    return rstate;
  shState->set_block_close(rstate == Qtrue);
#ifdef __WINDOWS__
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  if (SDL_GetWindowWMInfo(shState->sdlWindow(), &wmInfo)) {
    if (wmInfo.subsystem == SDL_SYSWM_WINDOWS) {
      HWND hwnd = wmInfo.info.win.window;
      HMENU hmenu = GetSystemMenu(hwnd, FALSE);
      if (hmenu != 0) {
        const UINT state = rstate == Qtrue ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
        EnableMenuItem(hmenu, SC_CLOSE, MF_BYCOMMAND | state);
        DrawMenuBar(hwnd);
      }
    }
  }
#endif
  return rb_iv_set(self, "block_close", rstate);
}

static VALUE game_window_has_focus(VALUE self)
{
  return shState->window_has_focus() ? Qtrue : Qfalse;
}

static VALUE game_window_changed_focus(VALUE self)
{
  VALUE focus = shState->window_has_focus() ? Qtrue : Qfalse;
  VALUE last = rb_iv_get(self, "focused");
  rb_iv_set(self, "focused", focus);
  return last == focus;
}

void init_game(const char *raw_exe_name)
{
  VALUE game = rb_define_module("Game");
  VALUE win = rb_define_module_under(game, "Window");
  rb_iv_set(game, "screensaver_enable", Qfalse);
  rb_iv_set(win, "resizable", Qtrue);
  rb_iv_set(win, "borders", Qtrue);
  rb_iv_set(win, "focused", Qtrue);
  rb_iv_set(win, "block_close", Qfalse);
  rb_const_set(game, rb_intern("RAW_EXE_NAME"), rstr(raw_exe_name));
  rb_const_set(game, rb_intern("START_WIDTH"), RB_INT2FIX(START_WIDTH));
  rb_const_set(game, rb_intern("START_HEIGHT"), RB_INT2FIX(START_HEIGHT));
  module_func(game, "title=", game_set_title, 1);
  module_func(game, "title", game_get_title, 0);
  module_func(game, "set_internal_values", game_set_internal_values, 0);
  module_func(game, "shot_format=", game_shot_fmt_set, 1);
  module_func(game, "shot_dir=", game_shot_dir_set, 1);
  module_func(game, "shot_filename=", game_shot_filename_set, 1);
  module_func(game, "soundfont=", game_sound_font_set, 1);
  module_func(game, "choose_soundfont", game_sound_font_by_pos, 1);
  module_func(game, "change_soundfont", game_sound_font_by_pos, 1);
  module_func(game, "enable_screensaver?", game_screensaver_enable, 0);
  module_func(game, "enable_screensaver=", game_screensaver_enable_set, 1);
  module_func(win, "icon=", game_window_set_icon, 1);
  module_func(win, "resizable?", game_window_resizable, 0);
  module_func(win, "show_borders?", game_window_borders, 0);
  module_func(win, "brightness", game_window_brightness, 0);
  module_func(win, "resizable=", game_window_resizable_set, 1);
  module_func(win, "show_borders=", game_window_borders_set, 1);
  module_func(win, "brightness=", game_window_brightness_set, 1);
  module_func(win, "block_close", game_window_block_close, 0);
  module_func(win, "block_close=", game_window_block_close_set, 1);
  module_func(win, "has_focus?", game_window_has_focus, 0);
  module_func(win, "change_focus?", game_window_changed_focus, 0);
}

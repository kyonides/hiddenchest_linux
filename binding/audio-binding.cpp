/*
** audio-binding.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** Modified  (C) 2018-2024 Kyonides <kyonides@gmail.com>
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

#include "audio.h"
#include "sharedstate.h"
#include "exception.h"
#include "binding-util.h"
#include "filesystem.h"
#include "hcextras.h"

static VALUE audio_default_bgm_loop(VALUE self)
{
  return rb_iv_get(self, "@default_bgm_loop");
}

static VALUE audio_default_bgm_loop_set(VALUE self, VALUE state)
{
  return rb_iv_set(self, "@default_bgm_loop", state);
}

static VALUE audio_default_bgs_loop(VALUE self)
{
  return rb_iv_get(self, "@default_bgs_loop");
}

static VALUE audio_default_bgs_loop_set(VALUE self, VALUE state)
{
  return rb_iv_set(self, "@default_bgs_loop", state);
}

static VALUE audio_bgmPlay(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int volume = 100;
  int pitch = 100;
  double pos = 0.0;
  rb_iv_set(self, "@old_bgm_name", argv[0]);
  rb_get_args(argc, argv, "z|iif", &filename, &volume, &pitch, &pos RB_ARG_END);
  GUARD_EXC( shState->audio().bgmPlay(filename, volume, pitch, pos); )
  return Qnil;
}

static VALUE audio_bgmStop(VALUE self)
{
  shState->audio().bgmStop();
  return Qnil;
}

static VALUE audio_bgm_pause(VALUE self)
{
  shState->audio().bgm_pause();
  return Qnil;
}

static VALUE audio_bgm_resume(VALUE self)
{
  shState->audio().bgm_resume();
  return Qnil;
}

static VALUE audio_bgm_playing(VALUE self)
{
  return shState->audio().bgm_playing() ? Qtrue : Qfalse;
}

static VALUE audio_bgm_stopped(VALUE self)
{
  return shState->audio().bgm_stopped() ? Qtrue : Qfalse;
}

static VALUE audio_bgm_paused(VALUE self)
{
  return shState->audio().bgm_paused() ? Qtrue : Qfalse;
}

static VALUE audio_bgm_looping(VALUE self)
{
  return shState->audio().bgm_looping() ? Qtrue : Qfalse;
}

static VALUE audio_bgm_loop_set(VALUE self, VALUE state)
{
  if (state == Qtrue)
    shState->audio().bgm_loop();
  else
    shState->audio().bgm_no_loop();
  return audio_bgm_looping(self);
}

static VALUE audio_bgmPos(VALUE self)
{
  VALUE pos = rb_float_new(shState->audio().bgmPos());
  return rb_iv_set(self, "@bgm_pos", pos);
}

static VALUE audio_bgsPlay(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int volume = 100;
  int pitch = 100;
  double pos = 0.0;
  rb_get_args(argc, argv, "z|iif", &filename, &volume, &pitch, &pos RB_ARG_END);
  GUARD_EXC( shState->audio().bgsPlay(filename, volume, pitch, pos); )
  return Qnil;
}

static VALUE audio_bgsStop(VALUE self)
{
  shState->audio().bgsStop();
  return Qnil;
}

static VALUE audio_bgs_pause(VALUE self)
{
  shState->audio().bgs_pause();
  return Qnil;
}

static VALUE audio_bgs_resume(VALUE self)
{
  shState->audio().bgm_resume();
  return Qnil;
}
static VALUE audio_bgs_playing(VALUE self)
{
  return shState->audio().bgs_playing() ? Qtrue : Qfalse;
}

static VALUE audio_bgs_stopped(VALUE self)
{
  return shState->audio().bgs_stopped() ? Qtrue : Qfalse;
}

static VALUE audio_bgs_paused(VALUE self)
{
  return shState->audio().bgs_paused() ? Qtrue : Qfalse;
}

static VALUE audio_bgs_looping(VALUE self)
{
  return shState->audio().bgs_looping() ? Qtrue : Qfalse;
}

static VALUE audio_bgs_loop_set(VALUE self, VALUE state)
{
  if (state == Qtrue)
    shState->audio().bgs_loop();
  else
    shState->audio().bgs_no_loop();
  return audio_bgs_looping(self);
}

static VALUE audio_bgm_volume_get(VALUE self)
{
  return rb_iv_get(self, "@bgm_volume");
}

static VALUE audio_bgs_volume_get(VALUE self)
{
  return rb_iv_get(self, "@bgs_volume");
}

static VALUE audio_se_volume_get(VALUE self)
{
  return rb_iv_get(self, "@se_volume");
}

static VALUE audio_me_volume_get(VALUE self)
{
  return rb_iv_get(self, "@me_volume");
}

static VALUE audio_bgm_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@bgm_volume", volume);
}

static VALUE audio_bgs_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@bgs_volume", volume);
}

static VALUE audio_se_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@se_volume", volume);
}

static VALUE audio_me_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@me_volume", volume);
}

static VALUE audio_bgsPos(VALUE self)
{
  VALUE pos = rb_float_new(shState->audio().bgsPos());
  return rb_iv_set(self, "@bgs_pos", pos);
}

static VALUE audio_old_bgm_pos_get(VALUE self)
{
  return rb_iv_get(self, "@old_bgm_pos");
}

static VALUE audio_old_bgs_pos_get(VALUE self)
{
  return rb_iv_get(self, "@old_bgs_pos");
}

static VALUE audio_old_bgm_name(VALUE self)
{
  return rb_iv_get(self, "@old_bgm_name");
}

static VALUE audio_old_bgs_name(VALUE self)
{
  return rb_iv_get(self, "@old_bgs_name");
}

static VALUE audio_save_bgm_data(VALUE self)
{
  rb_iv_set(self, "@old_bgm_name", audio_bgmPos(self));
  rb_iv_set(self, "@old_bgm_pos", audio_bgmPos(self));
  return true;
}

static VALUE audio_save_bgs_data(VALUE self)
{
  rb_iv_set(self, "@old_bgs_pos", audio_bgsPos(self));
  return true;
}

void raiseRbExc(const Exception &exc);

static VALUE audio_play_se(int argc, VALUE* argv, VALUE self)
{
  VALUE se = rb_iv_get(self, "@se"), name, volume, pitch;
  rb_scan_args(argc, argv, "12", &name, &volume, &pitch);
  if (RB_NIL_P(name) || RSTRING_LEN(name) == 0)
    return Qnil;
  name = rb_str_plus(rb_str_new_cstr("Audio/SE/"), name);
  const char *fn = StringValueCStr(name);
  int vol = RB_NIL_P(volume) ? RB_FIX2INT(rb_iv_get(self, "@se_volume")) :
            RB_FIX2INT(volume);
  int pit = RB_NIL_P(pitch) ? 100 : RB_FIX2INT(pitch);
  try {
    shState->audio().sePlay(fn, vol, pit);
  } catch (const Exception &e) { raiseRbExc(e); }
  return Qnil;
}

static VALUE audio_sePlay(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int volume = 100;
  int pitch = 100;
  rb_get_args(argc, argv, "z|ii", &filename, &volume, &pitch RB_ARG_END);
  GUARD_EXC( shState->audio().sePlay(filename, volume, pitch); )
  return Qnil;
}

static VALUE audio_seStop(VALUE self)
{
  shState->audio().seStop();
  return Qnil;
}

static VALUE audio_mePlay(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int volume = 100;
  int pitch = 100;
  rb_get_args(argc, argv, "z|ii", &filename, &volume, &pitch RB_ARG_END);
  GUARD_EXC( shState->audio().mePlay(filename, volume, pitch); )
  return Qnil;
}

static VALUE audio_meStop(VALUE self)
{
  shState->audio().meStop();
  return Qnil;
}

static VALUE audio_bgmFade(int argc, VALUE* argv, VALUE self)
{
  int time;
  rb_get_args(argc, argv, "i", &time RB_ARG_END);
  shState->audio().bgmFade(time);
  return Qnil;
}

static VALUE audio_bgsFade(int argc, VALUE* argv, VALUE self)
{
  int time;
  rb_get_args(argc, argv, "i", &time RB_ARG_END);
  shState->audio().bgsFade(time);
  return Qnil;
}

static VALUE audio_meFade(int argc, VALUE* argv, VALUE self)
{
  int time;
  rb_get_args(argc, argv, "i", &time RB_ARG_END);
  shState->audio().meFade(time);
  return Qnil;
}

static VALUE audioSetupMidi(VALUE self)
{
  shState->audio().setupMidi();
  return Qnil;
}

static VALUE audio_reset(VALUE self)
{
  shState->audio().reset();
  return Qnil;
}

void audioBindingInit()
{
  VALUE md = rb_define_module("Audio");
  rb_iv_set(md, "@bgm_pos", rb_float_new(0.0));
  rb_iv_set(md, "@bgs_pos", rb_float_new(0.0));
  rb_iv_set(md, "@old_bgm_pos", rb_float_new(0.0));
  rb_iv_set(md, "@old_bgs_pos", rb_float_new(0.0));
  rb_iv_set(md, "@old_bgm_name", rb_str_new_cstr(""));
  rb_iv_set(md, "@old_bgs_name", rb_str_new_cstr(""));
  rb_iv_set(md, "@se", rb_str_new_cstr("Audio/SE/"));
  rb_iv_set(md, "@bgm_volume", RB_INT2FIX(80));
  rb_iv_set(md, "@bgs_volume", RB_INT2FIX(80));
  rb_iv_set(md, "@se_volume", RB_INT2FIX(80));
  rb_iv_set(md, "@me_volume", RB_INT2FIX(80));
  rb_iv_set(md, "@default_bgm_loop", Qtrue);
  rb_iv_set(md, "@default_bgs_loop", Qtrue);
  module_func(md, "default_bgm_loop", audio_default_bgm_loop, 0);
  module_func(md, "default_bgm_loop=", audio_default_bgm_loop_set, 1);
  module_func(md, "default_bgs_loop", audio_default_bgs_loop, 0);
  module_func(md, "default_bgs_loop=", audio_default_bgs_loop_set, 1);
  module_func(md, "bgm_play", audio_bgmPlay, -1);
  module_func(md, "bgm_stop", audio_bgmStop, 0);
  module_func(md, "bgm_pause", audio_bgm_pause, 0);
  module_func(md, "bgm_resume", audio_bgm_resume, 0);
  module_func(md, "bgm_playing?", audio_bgm_playing, 0);
  module_func(md, "bgm_stopped?", audio_bgm_stopped, 0);
  module_func(md, "bgm_paused?", audio_bgm_paused, 0);
  module_func(md, "bgm_loop?", audio_bgm_looping, 0);
  module_func(md, "bgm_loop=", audio_bgm_loop_set, 1);
  module_func(md, "bgm_fade", audio_bgmFade, -1);
  module_func(md, "bgs_play", audio_bgsPlay, -1);
  module_func(md, "bgs_stop", audio_bgsStop, 0);
  module_func(md, "bgs_pause", audio_bgm_pause, 0);
  module_func(md, "bgs_resume", audio_bgm_resume, 0);
  module_func(md, "bgs_playing?", audio_bgs_playing, 0);
  module_func(md, "bgs_stopped?", audio_bgs_stopped, 0);
  module_func(md, "bgs_paused?", audio_bgs_paused, 0);
  module_func(md, "bgs_loop?", audio_bgs_looping, 0);
  module_func(md, "bgs_loop=", audio_bgs_loop_set, 1);
  module_func(md, "bgs_fade", audio_bgsFade, -1);
  module_func(md, "me_play", audio_mePlay, -1);
  module_func(md, "me_stop", audio_meStop, 0);
  module_func(md, "me_fade", audio_meFade, -1);
  module_func(md, "bgm_volume", audio_bgm_volume_get, 0);
  module_func(md, "bgs_volume", audio_bgs_volume_get, 0);
  module_func(md, "se_volume", audio_se_volume_get, 0);
  module_func(md, "me_volume", audio_me_volume_get, 0);
  module_func(md, "bgm_volume=", audio_bgm_volume_set, 1);
  module_func(md, "bgs_volume=", audio_bgs_volume_set, 1);
  module_func(md, "se_volume=", audio_se_volume_set, 1);
  module_func(md, "me_volume=", audio_me_volume_set, 1);
  module_func(md, "bgm_pos", audio_bgmPos, 0);
  module_func(md, "bgs_pos", audio_bgsPos, 0);
  module_func(md, "old_bgm_pos", audio_old_bgm_pos_get, 0);
  module_func(md, "old_bgs_pos", audio_old_bgs_pos_get, 0);
  module_func(md, "old_bgm_name", audio_old_bgm_name, 0);
  module_func(md, "old_bgs_name", audio_old_bgs_name, 0);
  module_func(md, "save_bgm_data", audio_save_bgm_data, 0);
  module_func(md, "save_bgs_data", audio_save_bgs_data, 0);
  if (rgssVer >= 3)
    module_func(md, "setup_midi", audioSetupMidi, 0);
  module_func(md, "se_play", audio_sePlay, -1);
  module_func(md, "se_stop", audio_seStop, 0);
  module_func(md, "reset", audio_reset, 0);
}

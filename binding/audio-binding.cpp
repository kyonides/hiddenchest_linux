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
#include "debugwriter.h"
// BGM Channels
static VALUE audio_bgms_play(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int volume = 100;
  int pitch = 100;
  double pos = 0.0;
  int n = 0;
  VALUE hash = rb_iv_get(self, "@bgms_names");
  rb_hash_aset(hash, argv[0], argv[1]);
  rb_get_args(argc, argv, "iz|iif", &n, &filename, &volume, &pitch, &pos RB_ARG_END);
  n--;
  GUARD_EXC( shState->audio().bgms_play(n, filename, volume, pitch, pos); )
  return Qnil;
}

static VALUE audio_bgms_stop_all(VALUE self)
{
  for (int n = 0; n < BGM_CHANNELS; n++)
    shState->audio().bgms_stop(n);
  return Qnil;
}

static VALUE audio_bgms_stop(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgms_stop(n);
  return Qnil;
}

static VALUE audio_bgms_close(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgms_close(n);
  return Qnil;
}

static VALUE audio_bgms_pause(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgms_pause(n);
  return Qnil;
}

static VALUE audio_bgms_resume(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgms_resume(n);
  return Qnil;
}

static VALUE audio_bgms_playing(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgms_playing(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgms_stopped(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgms_stopped(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgms_closed(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgms_closed(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgms_paused(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgms_paused(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgms_looping(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgms_loop");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgms_loop_set(VALUE self, VALUE pos, VALUE state)
{
  int n = RB_FIX2INT(pos) - 1;
  bool result = state == Qtrue;
  shState->audio().set_bgms_loop(n, result);
  VALUE hash = rb_iv_get(self, "@bgms_loop");
  return rb_hash_aset(hash, pos, result ? Qtrue : Qfalse);
}

static VALUE audio_bgms_loop_all(VALUE self)
{
  return rb_iv_get(self, "@bgms_loop");
}

static VALUE audio_bgms_loop_all_set(VALUE self, VALUE state)
{
  bool result = state == Qtrue;
  state = result ? Qtrue : Qfalse;
  VALUE pos, hash = rb_iv_get(self, "@bgms_loop");
  for (int n = 0; n < BGM_CHANNELS; n++) {
    shState->audio().set_bgms_loop(n, result);
    pos = RB_INT2FIX(n + 1);
    rb_hash_aset(hash, pos, state);
  }
  return hash;
}

static VALUE audio_bgms_sample_rate(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  int rate = shState->audio().bgms_sample_rate(n);
  return RB_INT2FIX(rate);
}

static VALUE audio_bgms_pos(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  pos = rb_float_new(shState->audio().bgms_pos(n));
  return rb_iv_set(self, "@bgms_pos", pos);
}

static VALUE audio_bgms_volume_get(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgms_volume");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgms_volume_set(VALUE self, VALUE pos, VALUE volume)
{
  VALUE hash = rb_iv_get(self, "@bgms_volume");
  return rb_hash_aset(hash, pos, volume);
}

static VALUE audio_old_bgms_pos(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgms_pos");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgms_names(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgms_names");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgms_fade(VALUE self, VALUE pos, VALUE fade_time)
{
  int n = RB_FIX2INT(pos) - 1;
  int time = RB_FIX2INT(fade_time);
  shState->audio().bgms_fade(n, time);
  return Qnil;
}
// End of BGM2
// BGM
static VALUE audio_bgmPlay(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int volume = 100;
  int pitch = 100;
  double pos = 0.0;
  rb_iv_set(self, "@bgm_names", argv[0]);
  rb_get_args(argc, argv, "z|iif", &filename, &volume, &pitch, &pos RB_ARG_END);
  GUARD_EXC( shState->audio().bgmPlay(filename, volume, pitch, pos); )
  return Qnil;
}

static VALUE audio_bgmStop(VALUE self)
{
  shState->audio().bgmStop();
  return Qnil;
}

static VALUE audio_bgm_close(VALUE self)
{
  shState->audio().bgm_close();
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

static VALUE audio_bgm_closed(VALUE self)
{
  return shState->audio().bgm_closed() ? Qtrue : Qfalse;
}

static VALUE audio_bgm_paused(VALUE self)
{
  return shState->audio().bgm_paused() ? Qtrue : Qfalse;
}

static VALUE audio_bgm_looping(VALUE self)
{
  return rb_iv_get(self, "@bgm_loop");
}

static VALUE audio_bgm_loop_set(VALUE self, VALUE state)
{
  bool result = state == Qtrue;
  shState->audio().set_bgm_loop(result);
  return rb_iv_set(self, "@bgm_loop", result ? Qtrue : Qfalse);
}

static VALUE audio_bgm_sample_rate(VALUE self)
{
  int rate = shState->audio().bgm_sample_rate();
  return RB_INT2FIX(rate);
}

static VALUE audio_bgmPos(VALUE self)
{
  VALUE pos = rb_float_new(shState->audio().bgmPos());
  return rb_iv_set(self, "@bgm_pos", pos);
}

static VALUE audio_bgm_volume_get(VALUE self)
{
  return rb_iv_get(self, "@bgm_volume");
}

static VALUE audio_bgm_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@bgm_volume", volume);
}

static VALUE audio_old_bgm_pos_get(VALUE self)
{
  return rb_iv_get(self, "@old_bgm_pos");
}

static VALUE audio_bgm_name(VALUE self)
{
  VALUE hash = rb_iv_get(self, "@bgm_names");
  return rb_hash_aref(hash, RB_INT2FIX(1));
}

static VALUE audio_bgmFade(int argc, VALUE* argv, VALUE self)
{
  int time;
  rb_get_args(argc, argv, "i", &time RB_ARG_END);
  shState->audio().bgmFade(time);
  return Qnil;
}
// BGS Channels
static VALUE audio_bgss_play(int argc, VALUE* argv, VALUE self)
{
  const char *filename;
  int n = 0;
  int volume = 100;
  int pitch = 100;
  double pos = 0.0;
  VALUE hash = rb_iv_get(self, "@bgss_names");
  rb_hash_aset(hash, argv[0], argv[1]);
  rb_get_args(argc, argv, "iz|iif", &n, &filename, &volume, &pitch, &pos RB_ARG_END);
  n--;
  GUARD_EXC( shState->audio().bgss_play(n, filename, volume, pitch, pos); )
  return Qnil;
}

static VALUE audio_bgss_stop_all(VALUE self)
{
  for (int n = 0; n < BGS_CHANNELS; n++)
    shState->audio().bgss_stop(n);
  return Qnil;
}

static VALUE audio_bgss_stop(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgss_stop(n);
  return Qnil;
}

static VALUE audio_bgss_close(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgss_close(n);
  return Qnil;
}

static VALUE audio_bgss_pause(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgss_pause(n);
  return Qnil;
}

static VALUE audio_bgss_resume(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().bgss_resume(n);
  return Qnil;
}

static VALUE audio_bgss_playing(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgss_playing(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgss_stopped(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgss_stopped(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgss_closed(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgss_closed(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgss_paused(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  return shState->audio().bgss_paused(n) ? Qtrue : Qfalse;
}

static VALUE audio_bgss_looping(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgss_loop");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgss_loop_set(VALUE self, VALUE pos, VALUE state)
{
  int n = RB_FIX2INT(pos) - 1;
  shState->audio().set_bgss_loop(n, state == Qtrue);
  VALUE hash = rb_iv_get(self, "@bgss_loop");
  return rb_hash_aset(hash, pos, state == Qtrue ? Qtrue : Qfalse);
}

static VALUE audio_bgss_loop_all(VALUE self)
{
  return rb_iv_get(self, "@bgss_loop");
}

static VALUE audio_bgss_loop_all_set(VALUE self, VALUE state)
{
  bool result = state == Qtrue;
  state = result ? Qtrue : Qfalse;
  VALUE pos, hash = rb_iv_get(self, "@bgss_loop");
  for (int n = 0; n < BGS_CHANNELS; n++) {
    shState->audio().set_bgss_loop(n, result);
    pos = RB_INT2FIX(n + 1);
    rb_hash_aset(hash, pos, state);
  }
  return hash;
}

static VALUE audio_bgss_volume_get(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgss_volume");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgss_pos(VALUE self, VALUE pos)
{
  int n = RB_FIX2INT(pos) - 1;
  float fpos = shState->audio().bgss_pos(n);
  VALUE pos_now = rb_float_new(fpos);
  VALUE hash = rb_iv_get(self, "@bgss_pos");
  return rb_hash_aset(hash, pos, pos_now);
}

static VALUE audio_bgss_volume_set(VALUE self, VALUE pos, VALUE volume)
{
  VALUE hash = rb_iv_get(self, "@bgss_volume");
  return rb_hash_aset(hash, pos, volume);
}

static VALUE audio_old_bgss_pos_get(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@old_bgss_pos");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgss_names(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@bgss_names");
  return rb_hash_aref(hash, pos);
}

static VALUE audio_bgss_fade(int argc, VALUE* argv, VALUE self)
{
  int n, time;
  rb_get_args(argc, argv, "ii", &n, &time RB_ARG_END);
  shState->audio().bgss_fade(n, time);
  return Qnil;
}
// End of BGS Channels
// BGS
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

static VALUE audio_bgs_close(VALUE self)
{
  shState->audio().bgs_close();
  return Qnil;
}

static VALUE audio_bgs_pause(VALUE self)
{
  shState->audio().bgs_pause();
  return Qnil;
}

static VALUE audio_bgs_resume(VALUE self)
{
  shState->audio().bgs_resume();
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

static VALUE audio_bgs_closed(VALUE self)
{
  return shState->audio().bgs_closed() ? Qtrue : Qfalse;
}

static VALUE audio_bgs_paused(VALUE self)
{
  return shState->audio().bgs_paused() ? Qtrue : Qfalse;
}

static VALUE audio_bgs_looping(VALUE self)
{
  return rb_iv_get(self, "@bgs_loop");
}

static VALUE audio_bgs_loop_set(VALUE self, VALUE state)
{
  shState->audio().set_bgs_loop(state == Qtrue);
  return rb_iv_set(self, "@bgs_loop", state == Qtrue ? Qtrue : Qfalse);
}

static VALUE audio_bgs_volume_get(VALUE self)
{
  return rb_iv_get(self, "@bgs_volume");
}

static VALUE audio_bgsPos(VALUE self)
{
  VALUE pos = rb_float_new(shState->audio().bgsPos());
  return rb_iv_set(self, "@bgs_pos", pos);
}

static VALUE audio_bgs_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@bgs_volume", volume);
}

static VALUE audio_old_bgs_pos(VALUE self, VALUE pos)
{
  VALUE hash = rb_iv_get(self, "@old_bgs_pos");
  return rb_hash_aref(hash, RB_INT2FIX(1));
}

static VALUE audio_bgs_name(VALUE self)
{
  VALUE hash = rb_iv_get(self, "@bgs_names");
  return rb_hash_aref(hash, RB_INT2FIX(1));
}

static VALUE audio_bgsFade(int argc, VALUE* argv, VALUE self)
{
  int time;
  rb_get_args(argc, argv, "i", &time RB_ARG_END);
  shState->audio().bgsFade(time);
  return Qnil;
}
// End of BGS
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
  } catch (const Exception &e) {
    raiseRbExc(e);
  }
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

static VALUE audio_meFade(int argc, VALUE* argv, VALUE self)
{
  int time;
  rb_get_args(argc, argv, "i", &time RB_ARG_END);
  shState->audio().meFade(time);
  return Qnil;
}

static VALUE audio_se_volume_get(VALUE self)
{
  return rb_iv_get(self, "@se_volume");
}

static VALUE audio_me_volume_get(VALUE self)
{
  return rb_iv_get(self, "@me_volume");
}

static VALUE audio_se_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@se_volume", volume);
}

static VALUE audio_me_volume_set(VALUE self, VALUE volume)
{
  return rb_iv_set(self, "@me_volume", volume);
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
  VALUE md, zero_float, pos, new_pos, old_pos, name, volume, looped, number;
  md = rb_define_module("Audio");
  zero_float = rb_float_new(0.0);
  pos = rb_ary_new();
  for (int n = 0; n < BGM_CHANNELS; n++)
    rb_ary_push(pos, RB_INT2FIX(n + 1));
// BGM Channels
  new_pos = rb_iv_set(md, "@bgms_pos", rb_hash_new());
  old_pos = rb_iv_set(md, "@old_bgms_pos", rb_hash_new());
  name = rb_iv_set(md, "@bgms_names", rb_hash_new());
  volume = rb_iv_set(md, "@bgms_volume", rb_hash_new());
  looped = rb_iv_set(md, "@bgms_loop", rb_hash_new());
  for (int n = 0; n < BGM_CHANNELS; n++) {
    number = rb_ary_entry(pos, n);
    rb_hash_aset(new_pos, number, zero_float);
    rb_hash_aset(old_pos, number, zero_float);
    rb_hash_aset(name, number, rb_str_new_cstr(""));
    rb_hash_aset(volume, number, RB_INT2FIX(80));
    rb_hash_aset(looped, number, Qtrue);
  }
// BGS Channels
  new_pos = rb_iv_set(md, "@bgss_pos", rb_hash_new());
  old_pos = rb_iv_set(md, "@old_bgss_pos", rb_hash_new());
  name = rb_iv_set(md, "@bgss_names", rb_hash_new());
  volume = rb_iv_set(md, "@bgss_volume", rb_hash_new());
  looped = rb_iv_set(md, "@bgss_loop", rb_hash_new());
  for (int n = 0; n < BGS_CHANNELS; n++) {
    number = rb_ary_entry(pos, n);
    rb_hash_aset(new_pos, number, zero_float);
    rb_hash_aset(old_pos, number, zero_float);
    rb_hash_aset(name, number, rb_str_new_cstr(""));
    rb_hash_aset(volume, number, RB_INT2FIX(80));
    rb_hash_aset(looped, number, Qtrue);
  }
// End of BGM & BGS Channels
  rb_iv_set(md, "@me_volume", RB_INT2FIX(80));
  rb_iv_set(md, "@se", rb_str_new_cstr("Audio/SE/"));
  rb_iv_set(md, "@se_volume", RB_INT2FIX(80));
  module_func(md, "bgm_play", audio_bgmPlay, -1);
  module_func(md, "bgm_stop", audio_bgmStop, 0);
  module_func(md, "bgm_close", audio_bgm_close, 0);
  module_func(md, "bgm_pause", audio_bgm_pause, 0);
  module_func(md, "bgm_resume", audio_bgm_resume, 0);
  module_func(md, "bgm_playing?", audio_bgm_playing, 0);
  module_func(md, "bgm_stopped?", audio_bgm_stopped, 0);
  module_func(md, "bgm_closed?", audio_bgm_closed, 0);
  module_func(md, "bgm_paused?", audio_bgm_paused, 0);
  module_func(md, "bgm_loop", audio_bgm_looping, 0);
  module_func(md, "bgm_loop=", audio_bgm_loop_set, 1);
  module_func(md, "bgm_fade", audio_bgmFade, -1);
  module_func(md, "bgm_sample_rate", audio_bgm_sample_rate, 0);
  module_func(md, "bgm_pos", audio_bgmPos, 0);
  module_func(md, "bgm_volume", audio_bgm_volume_get, 0);
  module_func(md, "bgm_volume=", audio_bgm_volume_set, 1);
  module_func(md, "old_bgm_pos", audio_old_bgm_pos_get, 0);
  module_func(md, "bgm_name", audio_bgm_name, 0);
// BGM Channels
  module_func(md, "bgms_play", audio_bgms_play, -1);
  module_func(md, "bgms_stop_all", audio_bgms_stop_all, 0);
  module_func(md, "bgms_stop", audio_bgms_stop, 1);
  module_func(md, "bgms_close", audio_bgms_close, 1);
  module_func(md, "bgms_pause", audio_bgms_pause, 1);
  module_func(md, "bgms_resume", audio_bgms_resume, 1);
  module_func(md, "bgms_playing?", audio_bgms_playing, 1);
  module_func(md, "bgms_stopped?", audio_bgms_stopped, 1);
  module_func(md, "bgms_closed?", audio_bgms_closed, 1);
  module_func(md, "bgms_paused?", audio_bgms_paused, 1);
  module_func(md, "bgms_loop", audio_bgms_looping, 1);
  module_func(md, "bgms_loop_set", audio_bgms_loop_set, 2);
  module_func(md, "bgms_loop_all", audio_bgms_loop_all, 0);
  module_func(md, "bgms_loop_all=", audio_bgms_loop_all_set, 1);
  module_func(md, "bgms_fade", audio_bgms_fade, 2);
  module_func(md, "bgms_sample_rate", audio_bgms_sample_rate, 1);
  module_func(md, "bgms_pos", audio_bgms_pos, 1);
  module_func(md, "bgms_volume", audio_bgms_volume_get, 1);
  module_func(md, "bgms_volume_set", audio_bgms_volume_set, 2);
  module_func(md, "old_bgms_pos", audio_old_bgms_pos, 1);
  module_func(md, "bgms_names", audio_bgms_names, 1);
// End of BGM Channels
  module_func(md, "bgs_play", audio_bgsPlay, -1);
  module_func(md, "bgs_stop", audio_bgsStop, 0);
  module_func(md, "bgs_close", audio_bgs_close, 0);
  module_func(md, "bgs_pause", audio_bgm_pause, 0);
  module_func(md, "bgs_resume", audio_bgm_resume, 0);
  module_func(md, "bgs_playing?", audio_bgs_playing, 0);
  module_func(md, "bgs_stopped?", audio_bgs_stopped, 0);
  module_func(md, "bgs_closed?", audio_bgs_closed, 0);
  module_func(md, "bgs_paused?", audio_bgs_paused, 0);
  module_func(md, "bgs_loop", audio_bgs_looping, 0);
  module_func(md, "bgs_loop=", audio_bgs_loop_set, 1);
  module_func(md, "bgs_fade", audio_bgsFade, -1);
  module_func(md, "bgs_volume", audio_bgs_volume_get, 0);
  module_func(md, "bgs_volume=", audio_bgs_volume_set, 1);
  module_func(md, "old_bgs_pos", audio_old_bgs_pos, 0);
  module_func(md, "bgs_name", audio_bgs_name, 0);
  module_func(md, "bgs_pos", audio_bgsPos, 0);
// BGS Channels
  module_func(md, "bgss_play", audio_bgss_play, -1);
  module_func(md, "bgss_stop_all", audio_bgss_stop_all, 0);
  module_func(md, "bgss_stop", audio_bgss_stop, 1);
  module_func(md, "bgss_close", audio_bgss_close, 1);
  module_func(md, "bgss_pause", audio_bgss_pause, 1);
  module_func(md, "bgss_resume", audio_bgss_resume, 1);
  module_func(md, "bgss_playing?", audio_bgss_playing, 1);
  module_func(md, "bgss_stopped?", audio_bgss_stopped, 1);
  module_func(md, "bgss_closed?", audio_bgss_closed, 1);
  module_func(md, "bgss_paused?", audio_bgss_paused, 1);
  module_func(md, "bgss_loop", audio_bgss_looping, 1);
  module_func(md, "bgss_loop_set", audio_bgss_loop_set, 2);
  module_func(md, "bgss_loop_all", audio_bgss_loop_all, 0);
  module_func(md, "bgss_loop_all=", audio_bgss_loop_all_set, 1);
  module_func(md, "bgss_fade", audio_bgss_fade, 2);
  module_func(md, "bgss_pos", audio_bgss_pos, 1);
  module_func(md, "bgss_volume", audio_bgss_volume_get, 1);
  module_func(md, "bgss_volume_set", audio_bgss_volume_set, 2);
  module_func(md, "old_bgss_pos", audio_old_bgss_pos_get, 1);
  module_func(md, "bgss_names", audio_bgss_names, 1);
// End of BGS Channels
  module_func(md, "me_play", audio_mePlay, -1);
  module_func(md, "me_stop", audio_meStop, 0);
  module_func(md, "me_fade", audio_meFade, -1);
  module_func(md, "me_volume", audio_me_volume_get, 0);
  module_func(md, "me_volume=", audio_me_volume_set, 1);
  module_func(md, "se_volume", audio_se_volume_get, 0);
  module_func(md, "se_volume=", audio_se_volume_set, 1);
  if (rgssVer == 3)
    module_func(md, "setup_midi", audioSetupMidi, 0);
  module_func(md, "se_play", audio_sePlay, -1);
  module_func(md, "se_stop", audio_seStop, 0);
  module_func(md, "reset", audio_reset, 0);
}

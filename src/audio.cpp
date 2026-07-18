/*
** audio.cpp
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
#include "audio_data.h"
#include "audiostream.h"
#include "soundemitter.h"
#include "sharedstate.h"
#include "sharedmidistate.h"
#include "eventthread.h"
#include "sdl-util.h"
#include <SDL_thread.h>
#include <SDL_timer.h>
//#include "ok_ogg.xxd"#include "wrong_ogg.xxd"

struct AudioPrivate
{
  AudioStream temp;
  AudioStream bgm1;
  AudioStream bgm2;
  AudioStream bgm3;
  AudioStream bgs1;
  AudioStream bgs2;
  AudioStream bgs3;
  AudioStream me;
  SoundEmitter se;
  SyncPoint &syncPoint;
  /* The 'MeWatch' is responsible for detecting a playing ME, quickly fading out
   * the BGM and keeping it paused/stopped while the ME plays, and unpausing /
   * fading the BGM back in again afterwards */
  enum MeWatchState
  {
    MeNotPlaying,
    BgmFadingOut,
    MePlaying,
    BgmFadingIn
  };

  struct
  {
    SDL_Thread *thread;
    AtomicFlag termReq;
    MeWatchState state;
  } meWatch;

  AudioPrivate(RGSSThreadData &rtData)
  : temp(ALStream::NotLooped, "temp"),
    bgm1(ALStream::Looped, "bgm"),
    bgm2(ALStream::Looped, "bgm"),
    bgm3(ALStream::Looped, "bgm"),
    bgs1(ALStream::Looped, "bgs"),
    bgs2(ALStream::Looped, "bgs"),
    bgs3(ALStream::Looped, "bgs"),
    me(ALStream::NotLooped, "me"),
    se(rtData.config),
    syncPoint(rtData.syncPoint)
  {
    meWatch.state = MeNotPlaying;
    meWatch.thread = createSDLThread
      <AudioPrivate, &AudioPrivate::meWatchFun>(this, "audio_mewatch");
  }

  ~AudioPrivate()
  {
    meWatch.termReq.set();
    SDL_WaitThread(meWatch.thread, 0);
  }

  AudioStream* get_bgm(int n)
  {
    switch (n) {
    case 0:
      return &bgm1;
    case 1:
      return &bgm2;
    case 2:
      return &bgm3;
    }
  }

  AudioStream* get_bgs(int n)
  {
    switch (n) {
    case 0:
      return &bgs1;
    case 1:
      return &bgs2;
    case 2:
      return &bgs3;
    }
  }

  void meWatchFun()
  {
    const float fadeOutStep = 1.f / (200  / AUDIO_SLEEP);
    const float fadeInStep  = 1.f / (1000 / AUDIO_SLEEP);
    while (true) {
      syncPoint.passSecondarySync();
      if (meWatch.termReq)
        return;
      switch (meWatch.state) {
      case MeNotPlaying:
      {
        me.lockStream();
        if (me.stream.queryState() == ALStream::Playing) {
          /* ME playing detected. -> FadeOutBGM */
          bgm1.extPaused = true;
          bgm2.extPaused = true;
          bgm3.extPaused = true;
          meWatch.state = BgmFadingOut;
        }
        me.unlockStream();
        break;
      }
      case BgmFadingOut :
      {
        me.lockStream();
        if (me.stream.queryState() != ALStream::Playing) {
          /* ME has ended while fading OUT BGM. -> FadeInBGM */
          me.unlockStream();
          meWatch.state = BgmFadingIn;
          break;
        }
        bgm1.lockStream();
        bgm2.lockStream();
        bgm3.lockStream();
        float vol = bgm1.getVolume(AudioStream::External);
        vol -= fadeOutStep;
        if (vol < 0 || bgm1.stream.queryState() != ALStream::Playing) {
          /* Either BGM has fully faded out, or stopped midway. -> MePlaying */
          bgm1.setVolume(AudioStream::External, 0);
          bgm2.setVolume(AudioStream::External, 0);
          bgm3.setVolume(AudioStream::External, 0);
          bgm1.stream.pause();
          bgm2.stream.pause();
          bgm3.stream.pause();
          bgm1.unlockStream();
          bgm2.unlockStream();
          bgm3.unlockStream();
          meWatch.state = MePlaying;
          me.unlockStream();
          break;
        }
        bgm1.setVolume(AudioStream::External, vol);
        bgm2.setVolume(AudioStream::External, vol);
        bgm3.setVolume(AudioStream::External, vol);
        bgm1.unlockStream();
        bgm2.unlockStream();
        bgm3.unlockStream();
        me.unlockStream();
        break;
      }
      case MePlaying :
      {
        me.lockStream();
        if (me.stream.queryState() != ALStream::Playing) {
          /* ME has ended */
          bgm1.lockStream();
          bgm2.lockStream();
          bgm3.lockStream();
          bgm1.extPaused = false;
          bgm2.extPaused = false;
          bgm3.extPaused = false;
          ALStream::State sState = bgm1.stream.queryState();
          if (sState == ALStream::Paused) {
            /* BGM is paused. -> FadeInBGM */
            bgm1.stream.play();
            bgm2.stream.play();
            bgm3.stream.play();
            meWatch.state = BgmFadingIn;
          } else {
            /* BGM is stopped. -> MeNotPlaying */
            bgm1.setVolume(AudioStream::External, 1.0f);
            bgm2.setVolume(AudioStream::External, 1.0f);
            bgm3.setVolume(AudioStream::External, 1.0f);
            if (!bgm1.noResumeStop)
              bgm1.stream.play();
            if (!bgm2.noResumeStop)
              bgm2.stream.play();
            if (!bgm3.noResumeStop)
              bgm3.stream.play();
            meWatch.state = MeNotPlaying;
          }
          bgm1.unlockStream();
          bgm2.unlockStream();
          bgm3.unlockStream();
        }
        me.unlockStream();
        break;
      }
      case BgmFadingIn :
      {
        bool need_break = false;
        bgm1.lockStream();
        if (bgm1.stream.queryState() == ALStream::Stopped) {
          /* BGM stopped midway fade in. -> MeNotPlaying */
          bgm1.setVolume(AudioStream::External, 1.0f);
          meWatch.state = MeNotPlaying;
          bgm1.unlockStream();
          need_break = true;
        }
        bgm2.lockStream();
        if (bgm2.stream.queryState() == ALStream::Stopped) {
          /* BGM stopped midway fade in. -> MeNotPlaying */
          bgm2.setVolume(AudioStream::External, 1.0f);
          meWatch.state = MeNotPlaying;
          bgm2.unlockStream();
          need_break = true;
        }
        bgm3.lockStream();
        if (bgm3.stream.queryState() == ALStream::Stopped) {
          /* BGM stopped midway fade in. -> MeNotPlaying */
          bgm3.setVolume(AudioStream::External, 1.0f);
          meWatch.state = MeNotPlaying;
          bgm3.unlockStream();
          need_break = true;
        }
        if (need_break)
          break;
        me.lockStream();
        if (me.stream.queryState() == ALStream::Playing) {
          /* ME started playing midway BGM fade in. -> FadeOutBGM */
          bgm1.extPaused = true;
          bgm2.extPaused = true;
          bgm3.extPaused = true;
          bgm1.unlockStream();
          bgm2.unlockStream();
          bgm3.unlockStream();
          meWatch.state = BgmFadingOut;
          me.unlockStream();
          break;
        }
        float vol1 = bgm1.getVolume(AudioStream::External);
        float vol2 = bgm2.getVolume(AudioStream::External);
        float vol3 = bgm3.getVolume(AudioStream::External);
        vol1 += fadeInStep;
        vol2 += fadeInStep;
        vol3 += fadeInStep;
        if (vol1 >= 1 || vol2 >= 1 || vol3 >= 1) {
          // BGM fully faded in. -> MeNotPlaying
          vol1 = 1.0f;
          vol2 = 1.0f;
          vol3 = 1.0f;
          meWatch.state = MeNotPlaying;
        }
        bgm1.setVolume(AudioStream::External, vol1);
        bgm2.setVolume(AudioStream::External, vol2);
        bgm3.setVolume(AudioStream::External, vol3);
        bgm1.unlockStream();
        bgm2.unlockStream();
        bgm3.unlockStream();
        me.unlockStream();
        break;
      }
    }
    SDL_Delay(AUDIO_SLEEP);
    }
  }
};

Audio::Audio(RGSSThreadData &rtData)
: p(new AudioPrivate(rtData))
{}
// Quick Read Audio File
AudioData Audio::read(const char *filename)
{
  AudioData ad;
  p->temp.read(filename, ad);
  return ad;
}

// BGM Channels
void Audio::bgms_play(int n, const char *filename, int volume, int pitch, float pos, int channels)
{
  p->get_bgm(n)->play(filename, volume, pitch, pos, channels);
}

void Audio::bgms_stop(int n)
{
  p->get_bgm(n)->stop();
}

void Audio::bgms_fade(int n, int time)
{
  p->get_bgm(n)->fadeOut(time);
}

void Audio::bgms_close(int n)
{
  p->get_bgm(n)->close();
}

void Audio::bgms_pause(int n)
{
  p->get_bgm(n)->pause();
}

void Audio::bgms_resume(int n)
{
  p->get_bgm(n)->resume();
}

bool Audio::bgms_playing(int n)
{
  return p->get_bgm(n)->playing();
}

bool Audio::bgms_stopped(int n)
{
  return p->get_bgm(n)->stopped();
}

bool Audio::bgms_closed(int n)
{
  return p->get_bgm(n)->closed();
}

bool Audio::bgms_paused(int n)
{
  return p->get_bgm(n)->paused();
}

bool Audio::bgms_looping(int n)
{
  return p->get_bgm(n)->looping();
}

void Audio::set_bgms_loop(int n, bool state)
{
  p->get_bgm(n)->set_loop(state);
}

void Audio::set_bgms_volume(int n, int vol)
{
  p->get_bgm(n)->setVolume(0, vol / 100.0);
}

int Audio::bgms_sample_rate(int n)
{
  return p->get_bgm(n)->sample_rate();
}

int Audio::bgms_samples(int n)
{
  return p->get_bgm(n)->samples();
}

double Audio::bgms_seconds(int n)
{
  return p->get_bgm(n)->seconds();
}

float Audio::bgms_pos(int n)
{
  return p->get_bgm(n)->playingOffset();
}
// BGM
void Audio::bgmPlay(const char *filename, int volume, int pitch, float pos, int channels)
{
  p->bgm1.play(filename, volume, pitch, pos, channels);
}

void Audio::bgmStop()
{
  p->bgm1.stop();
}

void Audio::bgmFade(int time)
{
  p->bgm1.fadeOut(time);
}

void Audio::bgm_close()
{
  p->bgm1.close();
}

void Audio::bgm_pause()
{
  p->bgm1.pause();
}

void Audio::bgm_resume()
{
  p->bgm1.resume();
}

bool Audio::bgm_playing()
{
  return p->bgm1.playing();
}

bool Audio::bgm_stopped()
{
  return p->bgm1.stopped();
}

bool Audio::bgm_closed()
{
  return p->bgm1.closed();
}

bool Audio::bgm_paused()
{
  return p->bgm1.paused();
}

bool Audio::bgm_looping()
{
  return p->bgm1.looping();
}

void Audio::set_bgm_loop(bool state)
{
  p->bgm1.set_loop(state);
}

void Audio::set_bgm_volume(int vol)
{
  p->bgm1.setVolume(0, vol / 100.0);
}

int Audio::bgm_sample_rate()
{
  return p->bgm1.sample_rate();
}

int Audio::bgm_samples()
{
  return p->bgm1.samples();
}

double Audio::bgm_seconds()
{
  return p->bgm1.seconds();
}

float Audio::bgmPos()
{
  return p->bgm1.playingOffset();
}
// End of BGM
// BGS Channels
void Audio::bgss_play(int n, const char *filename, int volume, int pitch, float pos, int channels)
{
  p->get_bgs(n)->play(filename, volume, pitch, pos, channels);
}

void Audio::bgss_stop(int n)
{
  p->get_bgs(n)->stop();
}

void Audio::bgss_fade(int n, int time)
{
  p->get_bgs(n)->fadeOut(time);
}

void Audio::bgss_close(int n)
{
  p->get_bgs(n)->close();
}

void Audio::bgss_pause(int n)
{
  p->get_bgs(n)->pause();
}

void Audio::bgss_resume(int n)
{
  p->get_bgs(n)->resume();
}

bool Audio::bgss_playing(int n)
{
  return p->get_bgs(n)->playing();
}

bool Audio::bgss_stopped(int n)
{
  return p->get_bgs(n)->stopped();
}

bool Audio::bgss_closed(int n)
{
  return p->get_bgs(n)->closed();
}

bool Audio::bgss_paused(int n)
{
  return p->get_bgs(n)->paused();
}

bool Audio::bgss_looping(int n)
{
  return p->get_bgs(n)->looping();
}

void Audio::set_bgss_loop(int n, bool state)
{
  p->get_bgs(n)->set_loop(state);
}

void Audio::set_bgss_volume(int n, int vol)
{
  p->get_bgs(n)->setVolume(0, vol / 100.0);
}

float Audio::bgss_pos(int n)
{
  return p->get_bgs(n)->playingOffset();
}
// BGS
void Audio::bgsPlay(const char *filename, int volume, int pitch, float pos, int channels)
{
  p->bgs1.play(filename, volume, pitch, pos, channels);
}

void Audio::bgsStop()
{
  p->bgs1.stop();
}

void Audio::bgs_close()
{
  p->bgs1.close();
}

void Audio::bgs_pause()
{
  p->bgs1.pause();
}

void Audio::bgs_resume()
{
  p->bgs1.resume();
}

bool Audio::bgs_playing()
{
  return p->bgs1.playing();
}

bool Audio::bgs_stopped()
{
  return p->bgs1.stopped();
}

bool Audio::bgs_closed()
{
  return p->bgs1.closed();
}

bool Audio::bgs_paused()
{
  return p->bgs1.paused();
}

bool Audio::bgs_looping()
{
  return p->bgs1.looping();
}

void Audio::set_bgs_loop(bool state)
{
  p->bgs1.set_loop(state);
}

void Audio::set_bgs_volume(int vol)
{
  p->bgs1.setVolume(0, vol / 100.0);
}

void Audio::bgsFade(int time)
{
  p->bgs1.fadeOut(time);
}

float Audio::bgsPos()
{
  return p->bgs1.playingOffset();
}
// End of BGS
void Audio::mePlay(const char *filename, int volume, int pitch)
{
  p->me.play(filename, volume, pitch);
}

void Audio::meStop()
{
  p->me.stop();
}

void Audio::meFade(int time)
{
  p->me.fadeOut(time);
}

void Audio::sePlay(const char *filename, int volume, int pitch)
{
  p->se.play(filename, volume, pitch);
}

void Audio::seStop()
{
  p->se.stop();
}

void Audio::setupMidi()
{
  shState->midiState().initIfNeeded(shState->config());
}

void Audio::reset()
{
  p->bgm1.stop();
  p->bgm2.stop();
  p->bgm3.stop();
  p->bgs1.stop();
  p->bgs2.stop();
  p->bgs3.stop();
  p->me.stop();
  p->se.stop();
}

Audio::~Audio() { delete p; }

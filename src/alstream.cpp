/*
** alstream.cpp
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "audio_data.h"
#include "alstream.h"
#include "sharedstate.h"
#include "sharedmidistate.h"
#include "eventthread.h"
#include "filesystem.h"
#include "exception.h"
#include "aldatasource.h"
#include "fluid-fun.h"
#include "sdl-util.h"
#include "debugwriter.h"
#include <SDL_mutex.h>
#include <SDL_thread.h>
#include <SDL_timer.h>

ALStream::ALStream(LoopMode loopMode, const std::string &threadId)
: looped(loopMode == Looped),
  state(Closed),
  source(0),
  thread(0),
  preemptPause(false),
  volume(0),
  pitch(1.0f)
{
  alSrc = AL::Source::gen();
  AL::Source::setVolume(alSrc, 1.0f);
  AL::Source::setPitch(alSrc, 1.0f);
  AL::Source::detachBuffer(alSrc);
  volume = AL::Source::get_volume(alSrc);
  for (int i = 0; i < STREAM_BUFS; ++i)
    alBuf[i] = AL::Buffer::gen();
  pauseMut = SDL_CreateMutex();
  threadName = std::string("al_stream (") + threadId + ")";
}

ALStream::~ALStream()
{
  close();
  AL::Source::clearQueue(alSrc);
  AL::Source::del(alSrc);
  for (int i = 0; i < STREAM_BUFS; ++i)
    AL::Buffer::del(alBuf[i]);
  SDL_DestroyMutex(pauseMut);
}

void ALStream::close()
{
  checkStopped();
  switch (state) {
  case Closed:
    return;
  case Playing:
  case Paused:
    stopStream();
  case Stopped:
    closeSource();
    state = Closed;
  }
}

void ALStream::open(const std::string &filename, int channels)
{
  checkStopped();
  switch (state) {
  case Playing:
  case Paused:
    stopStream();
  case Stopped:
    closeSource();
  case Closed:
    openSource(filename, channels);
  }
  state = Stopped;
}

void ALStream::read(const std::string &filename, AudioData &ad)
{
  checkStopped();
  if (state == Closed) {
    read_source(filename, ad);
    delete source;
  } else {
    ad.type = "fail";
    ad.ext = "";
  }
}

void ALStream::stop()
{
  checkStopped();
  switch (state) {
  case Closed:
  case Stopped:
    return;
  case Playing:
  case Paused:
    stopStream();
  }
  state = Stopped;
}

void ALStream::play(float offset)
{
  if (!source)
    return;
  checkStopped();
  switch (state) {
  case Closed:
  case Playing:
    return;
  case Stopped:
    startStream(offset);
    break;
  case Paused :
    resumeStream();
  }
  state = Playing;
}

void ALStream::pause()
{
  checkStopped();
  switch (state) {
  case Closed:
  case Stopped:
  case Paused:
    return;
  case Playing:
    pauseStream();
  }
  state = Paused;
}

void ALStream::setVolume(float value)
{
  AL::Source::setVolume(alSrc, value);
  volume = AL::Source::get_volume(alSrc);
}

void ALStream::setPitch(float value)
{
  AL::Source::setPitch(alSrc, value);
}

void ALStream::set_loop(bool state)
{
  if (looped == state)
    return;
  AL::Source::stop(alSrc);
  looped = state;
}

void ALStream::loop_set(int start, int len)
{
  if (!source || state == Closed || state == Stopped)
    return;
  switch (state) {
  case Paused:
  case Playing:
    source->loop_set(start, len);
  }
}

int ALStream::sample_rate()
{
  return !source ? 0 : source->sampleRate();
}

int ALStream::samples()
{
  return !source ? 0 : source->samples();
}

double ALStream::seconds()
{
  return !source ? 0 : source->seconds();
}

ALStream::State ALStream::queryState()
{
  checkStopped();
  return state;
}

float ALStream::queryOffset()
{
  if (state == Closed)
    return 0;
  float procOffset = static_cast<float>(procFrames) / source->sampleRate();
  return procOffset + AL::Source::getSecOffset(alSrc);
}

bool ALStream::is_playing()
{
  return AL::Source::getState(alSrc) == AL_PLAYING && state == Playing;
}

bool ALStream::has_stopped()
{
  return AL::Source::getState(alSrc) == AL_STOPPED || state == Stopped;
}

bool ALStream::is_closed()
{
  return state == Closed;
}

void ALStream::closeSource()
{
  delete source;
}

struct ALStreamOpenHandler : FileSystem::OpenHandler
{
  SDL_RWops *srcOps;
  bool looped;
  int channels;
  ALDataSource *source;
  std::string errorMsg;
  std::string filetype;
  std::string file_ext;

  ALStreamOpenHandler(SDL_RWops &srcOps, bool looped, int channels=2)
  : srcOps(&srcOps),
    looped(looped),
    channels(channels),
    source(0)
  {}

  bool tryRead(SDL_RWops &ops, const char *ext)
  {/* Copy this because we need to keep it around,
    * as we will continue reading data from it later */
    *srcOps = ops;
    // Try to read ogg file signature
    char sig[14] = { 0 };
    SDL_RWread(srcOps, sig, 1, 14);
    SDL_RWseek(srcOps, 0, RW_SEEK_SET);
    file_ext = ext;
    try {
      if (!strncmp(sig, "OggS", 4)) {
        filetype = "ogg";
        source = createVorbisSource(*srcOps, looped);
        return true;
      }
      if (!strncmp(sig, "MThd", 4)) {
        filetype = "midi";
        shState->midiState().initIfNeeded(shState->config());
        if (HAVE_FLUID) {
          source = createMidiSource(*srcOps, looped, channels);
          return true;
        }
      }
      if (!strncmp(sig, "ID3", 3))
        filetype = "mp3";
      char *tag2 = sig + 8;
      if (!strncmp(tag2, "WAVE", 4))
        filetype = "wave";
      if (!filetype.size())
        filetype = "unknown";
      source = createSDLSource(*srcOps, ext, STREAM_BUF_SIZE, looped);
    } catch (const Exception &e) {
// All source constructors will close the passed ops before throwing errors
      errorMsg = e.msg;
      filetype = "fail";
      return false;
    }
    return true;
  }
};

void ALStream::read_source(const std::string &filename, AudioData &ad)
{
  ALStreamOpenHandler handler(srcOps, looped, 2);
  needsRewind.clear();
  shState->fileSystem().openRead(handler, filename.c_str());
  ad.type = handler.filetype;
  ad.ext = handler.file_ext;
  ad.sample_rate = handler.source->sampleRate();
  ad.samples = handler.source->samples();
  ad.seconds = handler.source->seconds();
}

void ALStream::openSource(const std::string &filename, int channels)
{
  ALStreamOpenHandler handler(srcOps, looped, channels);
  shState->fileSystem().openRead(handler, filename.c_str());
  source = handler.source;
  needsRewind.clear();
  if (!source) {
    Debug() << "Unable to decode audio stream:" << filename << handler.errorMsg;
  }
}

void ALStream::stopStream()
{
  threadTermReq.set();
  if (thread) {
    SDL_WaitThread(thread, 0);
    thread = 0;
  }
  /* Need to stop the source _after_ the thread has terminated,
   * because it might have accidentally started it again before
   * seeing the term request */// else {//needsRewind.set();}
  AL::Source::stop(alSrc);
  AL::Source::rewind(alSrc);
  source->seekToOffset(startOffset);
  //for (int i = 0; i < STREAM_BUFS; i++) {
  //  AL::Buffer::del(alBuf[i]);
  //  alBuf[i] = AL::Buffer::gen(); }
  procFrames = 0;
  startOffset = 0;
  setVolume(0);
}

void ALStream::startStream(float offset)
{
  int buffers_left = AL::Source::queued_buffers(alSrc);
  AL::Source::clearQueue(alSrc);
  preemptPause = false;
  streamInited.clear();
  sourceExhausted.clear();
  threadTermReq.clear();
  startOffset = offset;
  procFrames = offset * source->sampleRate();
  thread = createSDLThread
    <ALStream, &ALStream::streamData>(this, threadName);
}

void ALStream::pauseStream()
{
  SDL_LockMutex(pauseMut);
  if (AL::Source::getState(alSrc) != AL_PLAYING)
    preemptPause = true;
  else
    AL::Source::pause(alSrc);
  SDL_UnlockMutex(pauseMut);
}

void ALStream::resumeStream()
{
  SDL_LockMutex(pauseMut);
  if (preemptPause)
    preemptPause = false;
  else
    AL::Source::play(alSrc);
  SDL_UnlockMutex(pauseMut);
}

void ALStream::checkStopped()
{/* This only concerns the scenario where
   * state is still 'Playing', but the stream
   * has already ended on its own (EOF, Error) */
  if (state == Closed)
    return;
  if (state != Playing)
    return;
  /* If streaming thread hasn't queued up
   * buffers yet there's not point in querying
   * the AL source */
  if (!streamInited)
    return;
  /* If alSrc isn't playing, but we haven't
   * exhausted the data source yet, we're just
   * having a buffer underrun */
  if (!sourceExhausted)
    return;
  if (AL::Source::getState(alSrc) == AL_PLAYING)
    return;
  stopStream();
  state = Stopped;
}

 void ALStream::queue_first_buffers()
{
  ALDataSource::Status status;
  AL::Buffer::ID buffer = AL::Buffer::ID(0);
  status = source->fillBuffer(buffer);
  AL::Source::queueBuffer(alSrc, buffer);
  for (int i = 0; i < STREAM_BUFS; ++i) {
    if (threadTermReq)
      return;
    AL::Buffer::ID buf = alBuf[i];
    status = source->fillBuffer(buf);
    if (status == ALDataSource::Error)
      return;
    AL::Source::queueBuffer(alSrc, buf);
    if (threadTermReq)
      return;
    if (status == ALDataSource::EndOfStream) {
      sourceExhausted.set();
      return;
    }
  }
  resumeStream();
  streamInited.set();
}
// thread func
void ALStream::streamData()
{// Fill up queue
  int error = 0;
  bool skip_buffer = false;
  ALDataSource::Status status;
  if (threadTermReq)
    return;
  if (needsRewind)
    source->seekToOffset(startOffset);
  float old_volume = AL::Source::get_volume(alSrc);
  queue_first_buffers();
  // Wait for buffers to be consumed, then refill and queue them up again
  while (true) {
    shState->rtData().syncPoint.passSecondarySync();
    ALint procBufs = AL::Source::getProcBufferCount(alSrc);
    if (!procBufs)
      procBufs += 3;
    error = (int)alGetError();
    while (procBufs--) {
      if (threadTermReq)
        break;
      AL::Buffer::ID buf = AL::Source::unqueueBuffer(alSrc);
      // If something went wrong, try again later
      if (buf == AL::Buffer::ID(0))
        break;
      if (buf == lastBuf) {
// Reset processed sample count so querying playback offset returns 0.0 again
        procFrames = looped ? source->loopStartFrames() : 0;
        lastBuf = AL::Buffer::ID(0);
      } else {
      // Add the frame count contained in this buffer to the total count
        ALint bits = AL::Buffer::getBits(buf);
        ALint size = AL::Buffer::getSize(buf);
        ALint chan = AL::Buffer::getChannels(buf);
        if (bits != 0 && chan != 0)
          procFrames += ((size / (bits / 8)) / chan);
      }
      if (sourceExhausted)
        continue;
      status = source->fillBuffer(buf);
      if (status == ALDataSource::EndOfStream) {
        sourceExhausted.set();
        if (!looped) {
          AL::Source::unqueueBuffer2(alSrc, buf);
          lastBuf = AL::Buffer::ID(0);
          procBufs = 0;
          skip_buffer = true;
          break;
        }
      }
      if (status == ALDataSource::Error) {
        sourceExhausted.set();
        return;
      }
      AL::Source::queueBuffer(alSrc, buf);
      // In case of buffer underrun, start playing again
      if (AL::Source::getState(alSrc) == AL_STOPPED) {
        Debug() << "Audio Stream Buffer Underrun";
        AL::Source::play(alSrc);
      }
      /* If this was the last buffer before the data
       * source loop wrapped around again, mark it as
       * such so we can catch it and reset the processed
       * sample count once it gets unqueued */
      if (status == ALDataSource::WrapAround)
        lastBuf = buf;
    }
    if (threadTermReq || skip_buffer)
      break;
    SDL_Delay(AUDIO_SLEEP);
  }
}
// soundemitter.cpp

#include "soundemitter.h"
#include "sharedstate.h"
#include "filesystem.h"
#include "exception.h"
#include "config.h"
#include "util.h"
#include "debugwriter.h"
#include <SDL_sound.h>

struct SoundBuffer
{
  AL::Buffer::ID alBuffer;
  SoundBuffer()
  {
    alBuffer = AL::Buffer::gen();
  }

  ~SoundBuffer()
  {
    AL::Buffer::del(alBuffer);
  }
};

struct SoundOpenHandler : FileSystem::OpenHandler
{
  SoundBuffer *buffer;

  SoundOpenHandler()
  : buffer(0)
  {}

  bool tryRead(SDL_RWops &ops, const char *ext)
  {
    ALenum error;
    Sound_Sample *sample = Sound_NewSample(&ops, ext, 0, STREAM_BUF_SIZE);
    if (!sample) {
      SDL_RWclose(&ops);
      void *ops;
      return false;
    }
    /* Do all of the decoding in the handler so we don't have
     * to keep the source ops around */
    uint32_t decBytes = Sound_DecodeAll(sample);
    uint8_t sampleSize = formatSampleSize(sample->actual.format);
    if (!decBytes || !sampleSize) {
      Sound_FreeSample(sample);
      sample = 0;
      return false;
    }
    uint32_t sampleCount = decBytes / sampleSize;
    buffer = new SoundBuffer;
    uint32_t bytes = sampleSize * sampleCount;
    ALenum alFormat = chooseALFormat(sampleSize, sample->actual.channels);
    if (!AL::Buffer::is_buffer(buffer->alBuffer)) {
      Debug() << "Failed to use old buffer";
      buffer->alBuffer = AL::Buffer::gen();
    }
    AL::Buffer::uploadData(buffer->alBuffer, alFormat, sample->buffer,
                           bytes, sample->actual.rate);
    Sound_FreeSample(sample);
    sample = 0;
    return true;
  }
};

SoundEmitter::SoundEmitter(const Config &conf)
: srcCount(conf.SE.sourceCount),
  alSrcs(srcCount),
  atchBufs(srcCount)
{
  for (size_t i = 0; i < srcCount; i++) {
    alSrcs[i] = AL::Source::gen();
    if (AL::Source::is_source(alSrcs[i]) != 1)
      Debug() << "Buffer #" << i << "failed.";
    atchBufs[i] = 0;
  }
}

SoundEmitter::~SoundEmitter()
{
  for (size_t i = 0; i < srcCount; ++i) {
    AL::Source::stop(alSrcs[i]);
    AL::Source::del(alSrcs[i]);
    if (atchBufs[i])
      delete atchBufs[i];
  }
}

void SoundEmitter::play(const std::string &filename,
                        int volume,
                        int pitch)
{
  float _volume = clamp<int>(volume, 0, 100) / 100.0f;
  float _pitch  = clamp<int>(pitch, 50, 150) / 100.0f;
  SoundOpenHandler handler;
  shState->fileSystem().openRead(handler, filename.c_str());
  SoundBuffer *buffer = handler.buffer;
  if (!buffer) {
    Debug() << "Unable to decode sound" << filename << Sound_GetError();
    return;
  }
  size_t target = srcCount;
  // Try to find first free source
  for (size_t n = 0; n < srcCount; n++) {
    if (AL::Source::getState(alSrcs[n]) != AL_PLAYING) {
      target = n;
      break;
    }
  }
  // Use first source as fallback
  if (target == srcCount)
    target = 0;
  AL::Source::ID src = alSrcs[target];
  AL::Source::stop(src);
  if (atchBufs[target])
    delete atchBufs[target];
  atchBufs[target] = buffer;
  AL::Source::attachBuffer(src, buffer->alBuffer);
  AL::Source::setVolume(src, _volume * GLOBAL_VOLUME);
  AL::Source::setPitch(src, _pitch);
  AL::Source::play(src);
}

void SoundEmitter::stop()
{
  for (size_t i = 0; i < srcCount; i++)
    AL::Source::stop(alSrcs[i]);
}

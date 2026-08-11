// soundemitter.cpp

#include <vorbis/vorbisfile.h>
#include "soundemitter.h"
#include "sharedstate.h"
#include "filesystem.h"
#include "exception.h"
#include "config.h"
#include "util.h"
#include "wavfile.h"

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

static size_t se_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  SDL_RWops *ops = static_cast<SDL_RWops *>(datasource);
  return SDL_RWread(ops, ptr, size, nmemb);
}

static int se_seek(void *datasource, ogg_int64_t offset, int whence)
{
  SDL_RWops *ops = static_cast<SDL_RWops *>(datasource);
  Sint64 result = SDL_RWseek(ops, offset, whence);
  return result < 0 ? -1 : 0;
}

static int se_close(void *datasource)
{
  return 0;
}

static long se_tell(void *datasource)
{
  SDL_RWops *ops = static_cast<SDL_RWops *>(datasource);
  Sint64 pos = SDL_RWtell(ops);
  return pos < 0 ? -1 : static_cast<long>(pos);
}

struct SoundOpenHandler : FileSystem::OpenHandler
{
  SoundBuffer *buffer;

  SoundOpenHandler()
  : buffer(0)
  {}

  bool read_ogg(SDL_RWops &ops)
  {
    OggVorbis_File vf;
    ov_callbacks callbacks;
    callbacks.read_func  = se_read;
    callbacks.seek_func  = se_seek;
    callbacks.close_func = se_close;
    callbacks.tell_func  = se_tell;
    int result = ov_open_callbacks(&ops, &vf, 0, 0, callbacks);
    if (result < 0) {
      SDL_RWclose(&ops);
      return false;
    }
    vorbis_info *info = ov_info(&vf, -1);
    const int channels = info->channels;
    const long rate = info->rate;
    std::vector<char> pcm;
    char temp[8192];
    int bitstream = 0;
    while (true) {
      long decoded = ov_read(&vf, temp, sizeof(temp), 0, 2, 1, &bitstream);
      if (decoded == 0)
        break;
      if (decoded < 0) {
        ov_clear(&vf);
        return false;
      }
      pcm.insert(pcm.end(), temp, temp + decoded);
    }
    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    buffer = new SoundBuffer;
    alBufferData(buffer->alBuffer.al, format, pcm.data(), pcm.size(), rate);
    ov_clear(&vf);
    return true;
  }

  bool read_wav(SDL_RWops &ops)
  {
    SDL_RWseek(&ops, 0, RW_SEEK_SET);
    PcmWav wav;
    unsigned char header[5] = { 0 };
    if (!wav_read_header(ops, header, wav))
      return false;
    if (!wav_read(ops, header, wav))
      return false;
    buffer = new SoundBuffer;
    alBufferData(buffer->alBuffer.al, wav.format, wav.data, wav.data_size, wav.rate);
    return true;
  }

  bool tryRead(SDL_RWops &ops, const char *ext)
  {
    if (!strcmp(ext, "ogg"))
      return read_ogg(ops);
    if (!strcmp(ext, "wav"))
      return read_wav(ops);
  }
};

SoundEmitter::SoundEmitter(const Config &conf)
: srcCount(conf.SE.sourceCount),
  alSrcs(srcCount),
  atchBufs(srcCount)
{
  for (size_t i = 0; i < srcCount; i++) {
    alSrcs[i] = AL::Source::gen();
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
    Debug() << "Unable to decode file" << filename;
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

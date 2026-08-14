/*
** wavesource.cpp
**
** This file is part of HiddenChest.
**
** Copyright (C) 2026 Kyonides <kyonides@gmail.com>
**
*/

#include "aldatasource.h"
#include "wavfile.h"

struct WaveSource : ALDataSource
{
  SDL_RWops &src;
  PcmWav wav;
  WavLoop loop;
  uint32_t currentFrame;
  int smpls;
  double sec;
  std::vector<int16_t> sampleBuf;

  WaveSource(SDL_RWops &ops, bool looped) : src(ops), currentFrame(0)
  {
    sec = 0.0f;
    currentFrame = 0;
    loop.requested = looped;
    sampleBuf.resize(STREAM_BUF_SIZE);
    unsigned char header[5] = { 0 };
    if (!wav_read_header(src, header, wav))
      return;
    if (!wav_read(src, header, wav))
      return;
    smpls = wav.data_size / wav.bpf;
    sec = smpls / wav.rate * 1.0f;
  }

  ~WaveSource()
  {
    if (wav.data)
      delete[] wav.data; 
    SDL_RWclose(&src);
  }

  int sampleRate()
  {
    return wav.rate;
  }

  int samples()
  {
    return smpls;
  }

  double seconds()
  {
    return sec;
  }

  void seekToOffset(float seconds)
  {
    if (seconds < 0)
      seconds = 0;
    currentFrame = wav.rate * seconds;
    if (loop.valid && currentFrame >= loop.end)
      currentFrame = loop.start;
    if (currentFrame >= smpls)
      currentFrame = 0;
    SDL_RWseek(&src, wav.data_offset + currentFrame * wav.bpf, RW_SEEK_SET);
  }

  void seek_to_loop_start()
  {
    if (!loop.valid || !loop.start) {
      SDL_RWseek(&src, wav.data_offset, RW_SEEK_SET);
      currentFrame = 0;
      return;
    }
    currentFrame = loop.start;
    SDL_RWseek(&src, wav.data_offset + currentFrame * wav.bpf, RW_SEEK_SET);
  }

  Status fillBuffer(AL::Buffer::ID alBuffer)
  {
    Status retStatus = ALDataSource::NoError;
    size_t buf_size = sizeof(int16_t);
    void *bufPtr = sampleBuf.data();
    int availBuf = sampleBuf.size();
    int canRead = availBuf;
    int bufUsed  = 0;
    size_t res = 0;
    bool readAgain = false;
    if (loop.valid) {
      int loop_end = loop.end * wav.channels;
      canRead = std::min(availBuf, loop_end);
    }
    while (canRead > 16) {
      res = SDL_RWread(&src, bufPtr, buf_size, canRead);
      if (res < 0) {
        Debug() << "Error: res is less than 0!";
        retStatus = ALDataSource::Error;
        break;
      }
      // EOF
      if (res == 0) {
        Debug() << "EOF?" << (int) SDL_RWtell(&src);
        Debug() << "Data End:" << wav.data_end;
        int end = wav.data_end - wav.data_offset;
        Debug() << "End of file" << (int) (wav.data_size - end);
        if (loop.requested) {
          retStatus = ALDataSource::WrapAround;
          seek_to_loop_start();
          continue;
        } else {
          retStatus = ALDataSource::EndOfStream;
        }
        if (bufUsed > 0)
          break;
        if (readAgain) {
          retStatus = ALDataSource::Error;
          break;
        }
        readAgain = true;
      }
      bufUsed += res;
      bufPtr = sampleBuf.data() + bufUsed;
      currentFrame += res / wav.channels;
      if (loop.valid && currentFrame >= loop.end) {
        int discardFrames = currentFrame - loop.end;
        bufUsed -= discardFrames * wav.channels;
        retStatus = ALDataSource::WrapAround;
        currentFrame = loop.start;
        seek_to_loop_start();
        break;
      }
      canRead -= res;
    }
    if (retStatus != ALDataSource::Error)
      alBufferData(alBuffer.al, wav.format, sampleBuf.data(),
                   bufUsed*sizeof(int16_t), wav.rate);
    return retStatus;
  }

  void loop_set(int start, int length)
  {
    loop.start = (uint32_t)start;
    uint32_t end = length > 16 ? (uint32_t)length : 16;
    loop.end = loop.start + end;
    loop.valid = (loop.start && loop.end);
  }

  uint32_t loopStartFrames()
  {
    return loop.valid ? loop.start : 0;
  }

  bool setPitch(float)
  {
    return false;
  }
};

ALDataSource *create_wave_source(SDL_RWops &ops, bool looped)
{
  return new WaveSource(ops, looped);
}

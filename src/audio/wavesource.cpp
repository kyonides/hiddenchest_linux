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
    unsigned char header[5] = { 0 };
    if (!wav_read_header(ops, header, wav))
      return;
    if (!wav_read(ops, header, wav))
      return;
    smpls = wav.data_size / wav.bpf;
    sec = smpls / wav.rate * 1.0f;
    loop.valid = (loop.start && loop.end);
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
    /*if (seconds <= 0) {
      ov_raw_seek(&vf, 0);
      currentFrame = 0;
    }
    currentFrame = seconds * info.rate;
    if (loop.valid && currentFrame > loop.end)
      currentFrame = loop.start;
    // If seeking fails, just seek back to start
    if (ov_pcm_seek(&vf, currentFrame) != 0)
      ov_raw_seek(&vf, 0);*/
  }

  void seek_to_loop_start()
  {
    /*if (!loop.valid || !loop.start) {
      ov_raw_seek(&vf, 0);
      currentFrame = 0;
      return;
    }
    currentFrame = loop.start;
    int result = ov_pcm_seek(&vf, currentFrame);
    if (result != 0)
      ov_raw_seek(&vf, 0);*/
  }

  Status fillBuffer(AL::Buffer::ID alBuffer)
  {
    Status retStatus = ALDataSource::NoError;
    /*void *bufPtr = sampleBuf.data();
    int availBuf = sampleBuf.size();
    int bufUsed  = 0;
    int canRead = availBuf;
    bool readAgain = false;
    if (loop.valid) {
      int tilLoopEnd = loop.end * info.frameSize;
      canRead = std::min(availBuf, tilLoopEnd);
    }
    while (canRead > 16) {
      long res = ov_read(&vf, static_cast<char*>(bufPtr),
                         canRead, 0, sizeof(int16_t), 1, 0);
      if (res < 0) {
        // Read error
        retStatus = ALDataSource::Error;
        break;
      }
      // EOF
      if (res == 0) {
        if (loop.requested) {
          retStatus = ALDataSource::WrapAround;
          seek_to_loop_start();//seekToOffset(0);
        } else {
          retStatus = ALDataSource::EndOfStream;
        }
        /* If we sought right to the end of the file,
         * we might be EOF without actually having read
         * any data at all yet (which mustn't happen),
         * so we try to continue reading some data. */
        /*if (bufUsed > 0)
          break;
        if (readAgain) {*/
// We're still not getting data though. Just error out to prevent an endless loop
      /*    retStatus = ALDataSource::Error;
          break;
        }
        readAgain = true;
      }
      bufUsed += (res / sizeof(int16_t));
      bufPtr = &sampleBuf[bufUsed];
      currentFrame += (res / info.frameSize);
      if (loop.valid && currentFrame >= loop.end) {
        // Determine how many frames we're over the loop end
        int discardFrames = currentFrame - loop.end;
        bufUsed -= discardFrames * info.channels;
        retStatus = ALDataSource::WrapAround;
        // Seek to loop start
        currentFrame = loop.start;
        int result = ov_pcm_seek(&vf, currentFrame);
        if (result != 0)
          retStatus = ALDataSource::Error;
        break;
      }
      canRead -= res;
    }
    if (retStatus != ALDataSource::Error)
      AL::Buffer::uploadData(alBuffer, info.alFormat, sampleBuf.data(),
                             bufUsed*sizeof(int16_t), info.rate);*/
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

/*
** wavfile.h
** 
** This file is part of HiddenChest.
** Copyright (C) 2026 Kyonides <kyonides@gmail.com>
**
*/

#include "al-util.h"
#include "debugwriter.h"

struct PcmWav {
  unsigned char* data;
  ALenum format;
  uint32_t chunk;
  uint16_t audio_fmt;
  uint16_t channels;
  uint16_t bps;
  uint16_t bpf;
  uint32_t rate;
  uint32_t brate;
  uint32_t data_size;
  void set_format()
  {
    if (channels == 1)
      format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    else
      format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
  }
};

struct WavLoop {
  uint32_t offset;
  uint32_t start;
  uint32_t end;
  bool valid;
  bool requested;
};

bool wav_read_header(SDL_RWops &ops, void *header, PcmWav &wav);
bool wav_read(SDL_RWops &ops, void *header, PcmWav &wav);
bool wav_read_loop(SDL_RWops &ops, void *header, PcmWav &wav, WavLoop &loop);

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
  unsigned char* data = 0;
  ALenum format;
  uint32_t chunk;
  uint16_t audio_fmt;
  uint16_t channels;
  uint16_t bps;
  uint16_t bpf;
  uint32_t rate;
  uint32_t brate;
  uint32_t data_end;
  uint32_t data_size;
  uint32_t data_offset;
  uint32_t fmt_offset;
  uint32_t file_size;
};

struct WavLoop {
  uint32_t offset = 0;
  uint32_t start = 0;
  uint32_t end = 0;
  bool valid = false;
  bool requested = false;
};

void set_format(PcmWav &wav);
bool wav_read_header(SDL_RWops &ops, void *header, PcmWav &wav);
bool wav_read(SDL_RWops &ops, void *header, PcmWav &wav);

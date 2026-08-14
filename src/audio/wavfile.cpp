/*
** wavfile.cpp
** 
** This file is part of HiddenChest.
** Copyright (C) 2026 Kyonides <kyonides@gmail.com>
**
*/

#include "wavfile.h"

#define RIFF 0x46464952 // "RIFF"
#define WAVE 0x45564157 // "WAVE"
#define FMT  0x20746D66 // "fmt "
#define DATA 0x61746164 // "data"
#define SMPL 0x6c706d73 // "smpl"

void set_format(PcmWav &wav)
{
  if (wav.channels == 1) {
    switch(wav.bps) {
    case 8:
      wav.format = AL_FORMAT_MONO8;
      return;
    case 16:
      wav.format = AL_FORMAT_MONO16;
      return;
    case 32:
      wav.format = 0x10010;
      return;
    }
  } else {
    switch(wav.bps) {
    case 8:
      wav.format = AL_FORMAT_STEREO8;
      return;
    case 16:
      wav.format = AL_FORMAT_STEREO16;
      return;
    case 32:
      wav.format = 0x10011;
      return;
    }
  }
}

bool wav_read_chunk(SDL_RWops &ops, void *header, int n, const char *error)
{
  if (SDL_RWread(&ops, header, n, 1))
    return true;
  Debug() << error;
  return false;
}

bool wav_read_header(SDL_RWops &ops, void *header, PcmWav &wav)
{
  if (!wav_read_chunk(ops, header, 4, "No header found!"))
    return false;
  if (strcmp((const char*)header, "RIFF")) {
    Debug() << "No RIFF tag found.";
    return false;
  }
  if (!wav_read_chunk(ops, &wav.file_size, 4, "Missing file size."))
    return false;
  wav.file_size += 8;
  if (!wav_read_chunk(ops, header, 4, "Truncated WAVE header."))
    return false;
  if (strcmp((const char*)header, "WAVE")) {
    Debug() << "Unsupported file format.";
    return false;
  }
  if (!wav_read_chunk(ops, header, 4, "Missing fmt marker."))
    return false;
  if (!strcmp((const char*)header, "fmt ")) {
    wav.fmt_offset = (uint32_t) SDL_RWseek(&ops, 0, RW_SEEK_CUR);
  } else {
    Debug() << "No fmt chunk marker found.";
    return false;
  }
  if (!wav_read_chunk(ops, &wav.chunk, 4, "No fmt chunk found."))
    return false;
  if (!wav_read_chunk(ops, &wav.audio_fmt, 2, "No audio format."))
    return false;
  if (!wav_read_chunk(ops, &wav.channels, 2, "No audio channels."))
    return false;
  if (wav.audio_fmt != 1 && wav.audio_fmt != 3) {
    Debug() << "Unsupported compressed WAV file.";
    return false;
  }
  if (!wav_read_chunk(ops, &wav.rate, 4, "No sample rate.")) 
    return false;
  if (!wav_read_chunk(ops, &wav.brate, 4, "No byte rate."))
    return false;
  if (!wav_read_chunk(ops, &wav.bpf, 2, "No bytes per frame"))
    return false; 
  if (!wav_read_chunk(ops, &wav.bps, 2, "No bytes per second"))
    return false;
  set_format(wav);
  if (wav.chunk > 16)
    SDL_RWseek(&ops, wav.chunk - 16, RW_SEEK_CUR);
  return true;
}

bool wav_read(SDL_RWops &ops, void *header, PcmWav &wav)
{
  while (true) {
    SDL_RWread(&ops, header, 4, 1);
    if (!strcmp((const char*)header, "data"))
      break;
  }
  if (!wav_read_chunk(ops, &wav.data_size, 4, "No data size chunk."))
    return false;
  wav.data_offset = (uint32_t) SDL_RWseek(&ops, 0, RW_SEEK_CUR);
  wav.data_end = wav.data_offset + wav.data_size;
  return true;
}
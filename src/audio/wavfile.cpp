/*
** wavfile.cpp
** 
** This file is part of HiddenChest.
** Copyright (C) 2026 Kyonides <kyonides@gmail.com>
**
*/

#include "wavfile.h"

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
  SDL_RWseek(&ops, 4, RW_SEEK_CUR);
  if (!wav_read_chunk(ops, header, 4, "Truncated WAVE header."))
    return false;
  if (strcmp((const char*)header, "WAVE")) {
    Debug() << "Unsupported file format.";
    return false;
  }
  if (!wav_read_chunk(ops, header, 4, "Missing fmt marker."))
    return false;
  if (strcmp((const char*)header, "fmt ")) {
    Debug() << "No fmt chunk marker found.";
    return false;
  }
  if (!wav_read_chunk(ops, &wav.chunk, 4, "No fmt chunk found."))
    return false;
  if (!wav_read_chunk(ops, &wav.audio_fmt, 2, "No audio format."))
    return false;
  if (!wav_read_chunk(ops, &wav.channels, 2, "No audio channels."))
    return false;
  if (wav.audio_fmt != 1) {
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
  wav.set_format();
  if (wav.chunk > 16)
    SDL_RWseek(&ops, wav.chunk - 16, RW_SEEK_CUR);
  uint32_t offset = (uint32_t) SDL_RWseek(&ops, 0, RW_SEEK_CUR);
  Debug() << "Read Pos:" << offset;
  return true;
}

bool wav_read(SDL_RWops &ops, void *header, PcmWav &wav)
{
  uint32_t offset;
  bool run = true;
  while (run) {
    SDL_RWread(&ops, header, 4, 1);
    offset = (uint32_t) SDL_RWseek(&ops, 0, RW_SEEK_CUR);
    if (!strcmp((const char*)header, "data")) {
      offset = (uint32_t) SDL_RWseek(&ops, 0, RW_SEEK_CUR);
      run = false;
    }
  }
  if (!wav_read_chunk(ops, &wav.data_size, 4, "No data size chunk."))
    return false;
  wav.data = new unsigned char[wav.data_size];
  if (!wav_read_chunk(ops, wav.data, wav.data_size, "Failed to read data.")) {
    delete[] wav.data;
    return false;
  }
  return true;
}

bool wav_read_loop(SDL_RWops &ops, void *header, PcmWav &wav, WavLoop &loop)
{
  loop.valid = false;
  loop.start = 0;
  loop.end = 0;
  loop.offset = 0;
  if (!wav_read_chunk(ops, &wav.data_size, 4, "No data size chunk."))
    return false;
  if (!strcmp((const char*)header, "data"))
    loop.offset = (uint32_t) SDL_RWseek(&ops, 0, RW_SEEK_CUR);
  else
    return false;
  if (!wav_read_chunk(ops, &wav.data_size, 4, "No data size chunk."))
    return false;
  while (true) {
    // Read the next 4-byte chunk identifier
    if (!wav_read_chunk(ops, header, 4, "EOF reached."))
      return false;
    if (!strcmp((const char*)header, "smpl")) {
      SDL_RWseek(&ops, 28, RW_SEEK_CUR);
      uint32_t sample_loops = 0;
      if (!wav_read_chunk(ops, &sample_loops, 4, "Failed to read loop count."))
        continue;
      if (sample_loops > 0) {
        SDL_RWseek(&ops, 12, RW_SEEK_CUR);
        if (!wav_read_chunk(ops, &loop.start, 4, "Failed to read loop start."))
          continue;
        if (!wav_read_chunk(ops, &loop.end, 4, "Failed to read loop end."))
          continue;
        int32_t remaining = wav.data_size - 52;
        if (remaining > 0) {
          SDL_RWseek(&ops, remaining, RW_SEEK_CUR);
        }
        continue;
      }
      SDL_RWseek(&ops, wav.data_size - 32, RW_SEEK_CUR);
      continue;
    }
    // List-like chunk? Skip past it entirely
    SDL_RWseek(&ops, wav.data_size, RW_SEEK_CUR);
  }
  if (!wav_read_chunk(ops, wav.data, RW_SEEK_END, "Failed to read data.")) {
    delete[] wav.data;
    return false;
  }
  if (loop.end < loop.offset)
    loop.end = wav.data_size;
  if (loop.start > loop.end)
    loop.start = loop.offset;
  return true;
}

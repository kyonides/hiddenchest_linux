#ifndef MOVIE_H
#define MOVIE_H

#include "theoraplay/theoraplay.h"
#include "filesystem.h"
#include "bitmap.h"

#define VIDEO_DELAY 10
#define AUDIO_DELAY 100
#define DEF_MAX_VIDEO_FRAMES 30

struct Movie
{
  THEORAPLAY_Decoder *decoder;
  const THEORAPLAY_AudioPacket *audio;
  const THEORAPLAY_VideoFrame *video;
  bool hasVideo;
  bool hasAudio;
  bool skippable;
  Bitmap *videoBitmap;
  SDL_RWops srcOps;
  static float volume;

  Movie(int volume_, bool skippable_);
  bool preparePlayback();
  void play();
  ~Movie();
private:
  bool startAudio();
};

struct MovieOpenHandler : FileSystem::OpenHandler
{
  SDL_RWops *srcOps;
  MovieOpenHandler(SDL_RWops &srcOps);
  bool tryRead(SDL_RWops &ops, const char *ext);
};

#endif

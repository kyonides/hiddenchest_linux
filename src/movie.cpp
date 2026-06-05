
#include <SDL.h>
#include "input.h"
#include "movie.h"
#include "disposable.h"
#include "debugwriter.h"

typedef struct AudioQueue
{
  const THEORAPLAY_AudioPacket *audio;
  int offset;
  struct AudioQueue *next;
} AudioQueue;

static volatile AudioQueue *movieAudioQueue;
static volatile AudioQueue *movieAudioQueueTail;

static long readMovie(THEORAPLAY_Io *io, void *buf, long buflen)
{
  SDL_RWops *f = (SDL_RWops *) io->userdata;
  return (long) SDL_RWread(f, buf, 1, buflen);
} // IoFopenRead

static void closeMovie(THEORAPLAY_Io *io)
{
  SDL_RWops *f = (SDL_RWops *) io->userdata;
  SDL_RWclose(f);
  free(io);
} // IoFopenClose

Movie::Movie(int volume_, bool skippable_)
  : decoder(0), audio(0), video(0), skippable(skippable_), videoBitmap(0)
{
  volume = volume_ * 0.01f;
}

bool Movie::preparePlayback()
{
  // https://theora.org/doc/libtheora-1.0/codec_8h.html
  // https://ffmpeg.org/doxygen/0.11/group__lavc__misc__pixfmt.html
  THEORAPLAY_Io *io = (THEORAPLAY_Io *) malloc(sizeof (THEORAPLAY_Io));
  if(!io) {
    SDL_RWclose(&srcOps);
    return false;
  }
  io->read = readMovie;
  io->close = closeMovie;
  io->userdata = &srcOps;
  decoder = THEORAPLAY_startDecode(io, DEF_MAX_VIDEO_FRAMES, THEORAPLAY_VIDFMT_RGBA);
  if (!decoder) {
    SDL_RWclose(&srcOps);
    return false;
  }
  // Wait until the decoder has parsed out some basic truths from the file.
  while (!THEORAPLAY_isInitialized(decoder)) {
    SDL_Delay(VIDEO_DELAY);
  }
  // Once we're initialized, we can tell if this file has audio and/or video.
  hasAudio = THEORAPLAY_hasAudioStream(decoder);
  hasVideo = THEORAPLAY_hasVideoStream(decoder);
  // Queue up the audio
  if (hasAudio) {
    while ((audio = THEORAPLAY_getAudio(decoder)) == NULL) {
      if ((THEORAPLAY_availableVideo(decoder) >= DEF_MAX_VIDEO_FRAMES))
        break;  // we'll never progress, there's no audio yet but we've prebuffered as much as we plan to.
      SDL_Delay(VIDEO_DELAY);
    }
  }
  // No video, so no point in doing anything else
  if (!hasVideo) {
    THEORAPLAY_stopDecode(decoder);
    return false;
  }
  // Wait until we have video
  while ((video = THEORAPLAY_getVideo(decoder)) == NULL) {
    SDL_Delay(VIDEO_DELAY);
  }
  // Wait until we have audio, if applicable
  audio = NULL;
  if (hasAudio) {
    while ((audio = THEORAPLAY_getAudio(decoder)) == NULL && THEORAPLAY_availableVideo(decoder) < DEF_MAX_VIDEO_FRAMES) {
      SDL_Delay(VIDEO_DELAY);
    }
  }
  videoBitmap = new Bitmap(video->width, video->height);
  movieAudioQueue = NULL;
  movieAudioQueueTail = NULL;
  return true;
}
  
static void queueAudioPacket(const THEORAPLAY_AudioPacket *audio)
{
  AudioQueue *item = NULL;
  if (!audio)
    return;
  item = (AudioQueue *) malloc(sizeof (AudioQueue));
  if (!item) {
    THEORAPLAY_freeAudio(audio);
    return;  // oh well.
  }
  item->audio = audio;
  item->offset = 0;
  item->next = NULL;
  SDL_LockAudio();
  if (movieAudioQueueTail) {
    movieAudioQueueTail->next = item;
  } else {
    movieAudioQueue = item;
  }
  movieAudioQueueTail = item;
  SDL_UnlockAudio();
}

static void queueMoreMovieAudio(THEORAPLAY_Decoder *decoder, const Uint32 now)
{
  const THEORAPLAY_AudioPacket *audio;
  while ((audio = THEORAPLAY_getAudio(decoder)) != NULL) {
    queueAudioPacket(audio);
    if (audio->playms >= now + 2000) // don't let this get too far ahead.
      break;
  }
}

static void SDLCALL movieAudioCallback(void *userdata, uint8_t *stream, int len)
{
  // !!! FIXME: this should refuse to play if item->playms is in the future.
  //const Uint32 now = SDL_GetTicks() - baseticks;
  Sint16 *dst = (Sint16 *) stream;
  while (movieAudioQueue && (len > 0)) {
    volatile AudioQueue *item = movieAudioQueue;
    AudioQueue *next = item->next;
    const int channels = item->audio->channels;
    const float *src = item->audio->samples + (item->offset * channels);
    unsigned int cpy = (item->audio->frames - item->offset) * channels;
    if (cpy > (len / sizeof (Sint16)))
      cpy = len / sizeof (Sint16);
    for (unsigned int i = 0; i < cpy; i++) {
      const float val = (*(src++)) * Movie::volume;
      if (val < -1.0f) {
        *(dst++) = -32768;
      } else if (val > 1.0f) {
        *(dst++) = 32767;
      } else {
        *(dst++) = (Sint16) (val * 32767.0f);
      }
    }
    item->offset += (cpy / channels);
    len -= cpy * sizeof (Sint16);
    if (item->offset >= item->audio->frames) {
      THEORAPLAY_freeAudio(item->audio);
      free((void *) item);
      movieAudioQueue = next;
    }
  }
  if (!movieAudioQueue)
    movieAudioQueueTail = NULL;
  if (len > 0)
    memset(dst, '\0', len);
}
  
bool Movie::startAudio()
{
  SDL_AudioSpec spec{};
  spec.freq = audio->freq;
  spec.format = AUDIO_S16SYS;
  spec.channels = audio->channels;
  spec.samples = 2048;
  spec.callback = movieAudioCallback;
  if (SDL_OpenAudio(&spec, NULL) != 0)
    return false;
  queueAudioPacket(audio);
  audio = NULL;
  queueMoreMovieAudio(decoder, 0);
  SDL_PauseAudio(0);  // Start audio playback
  return true;
}
// Assuming every frame has the same duration.
// Uint32 frameMs = (video->fps == 0.0) ? 0 : ((Uint32) (1000.0 / video->fps));
void Movie::play()
{
  Uint32 baseTicks = SDL_GetTicks();
  bool openedAudio = false;
  while (THEORAPLAY_isDecoding(decoder)) {
    // Check for reset / shutdown input
    if(shState->graphics().updateMovieInput(this)) break;
    // Check for attempted skip
    if (skippable) {
      shState->input().update();
      if (shState->input().isTriggered(Input::C) || shState->input().isTriggered(Input::B)) break;
    }
    const Uint32 now = SDL_GetTicks() - baseTicks;
    if (!video)
      video = THEORAPLAY_getVideo(decoder);
    if (hasAudio) {
      if (!audio)
        audio = THEORAPLAY_getAudio(decoder);
      if (audio && !openedAudio) {
        if(!startAudio()) {
          Debug() << "Error opening movie audio!";
          break;
        }
        openedAudio = true;
      }
    }
    // Got a video frame, now draw it
    if (video) {
      videoBitmap->replaceRaw(video->pixels, video->width * video->height * 4);
      THEORAPLAY_freeVideo(video);
      video = NULL;
    }
    shState->graphics().update();//(false);
    if (openedAudio)
      queueMoreMovieAudio(decoder, now);
  }
}
  
Movie::~Movie()
{
  if (hasAudio) {
    if (movieAudioQueueTail)
      THEORAPLAY_freeAudio(movieAudioQueueTail->audio);
    movieAudioQueueTail = NULL;
    if (movieAudioQueue)
      THEORAPLAY_freeAudio(movieAudioQueue->audio);
    movieAudioQueue = NULL;
  }
  if (video) THEORAPLAY_freeVideo(video);
  if (audio) THEORAPLAY_freeAudio(audio);
  if (decoder) THEORAPLAY_stopDecode(decoder);
  SDL_CloseAudio();
  delete videoBitmap;
}

float Movie::volume;

MovieOpenHandler::MovieOpenHandler(SDL_RWops &srcOps)
  : srcOps(&srcOps)
{}
  
bool MovieOpenHandler::tryRead(SDL_RWops &ops, const char *ext)
{
  *srcOps = ops;
  return true;
}

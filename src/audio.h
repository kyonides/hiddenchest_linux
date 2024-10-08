/*
** audio.h
**
** This file is part of HiddenChest and mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
** Modified  (C) 2018-2024 Kyonides <kyonides@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AUDIO_H
#define AUDIO_H

/* Concerning the 'pos' parameter:
 *   RGSS3 actually doesn't specify a format for this,
 *   it's only implied that it is a numerical value
 *   (must be 0 on invalid cases), and it's not used for
 *   anything outside passing it back into bgm/bgs_play.
 *   We use this freedom to define pos to be a float,
 *   in seconds of playback. (RGSS3 seems to use large
 *   integers that _look_ like sample offsets but I can't
 *   quite make out their meaning yet) */

struct AudioPrivate;
struct RGSSThreadData;

class Audio
{
public:
	void bgmPlay(const char *filename,
	             int volume = 100,
	             int pitch = 100,
	             float pos = 0);
	void bgmStop();
	void bgmFade(int time);
  void bgm_close();
  void bgm_pause();
  void bgm_resume();
  bool bgm_playing();
  bool bgm_stopped();
	bool bgm_closed();
  bool bgm_paused();
	bool bgm_looping();
	void set_bgm_loop(bool state);
	void bgsPlay(const char *filename,
	             int volume = 100,
	             int pitch = 100,
	             float pos = 0);
	void bgsStop();
	void bgsFade(int time);
	void bgs_close();
  void bgs_pause();
  void bgs_resume();
  bool bgs_playing();
  bool bgs_stopped();
	bool bgs_closed();
  bool bgs_paused();
	bool bgs_looping();
	void set_bgs_loop(bool state);
	void mePlay(const char *filename,
	            int volume = 100,
	            int pitch = 100);
	void meStop();
	void meFade(int time);
	void sePlay(const char *filename,
	            int volume = 100,
	            int pitch = 100);
	void seStop();
	void setupMidi();
	float bgmPos();
	float bgsPos();
	void reset();

private:
	Audio(RGSSThreadData &rtData);
	~Audio();

	friend struct SharedStatePrivate;

	AudioPrivate *p;
};

#endif // AUDIO_H

/*
** sharedmidistate.h
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#ifndef SHAREDMIDISTATE_H
#define SHAREDMIDISTATE_H

#include "config.h"
#include "debugwriter.h"
#include "fluid-fun.h"
#include <assert.h>
#include <vector>
#include <string>

#define SYNTH_INIT_COUNT 3
#define SYNTH_SAMPLERATE 44100

struct Synth
{
	fluid_synth_t *synth;
	bool inUse;
};

struct SharedMidiState
{
	bool inited;
	std::vector<Synth> synths;
	std::string soundFont;
	std::string other_soundfont;
	fluid_settings_t *flSettings;

	SharedMidiState(const Config &conf)
	    : inited(false),
	      soundFont(""),
	      other_soundfont("")
	{}

	~SharedMidiState()
	{
		/* We might have initialized, but if the consecutive libfluidsynth
		 * load failed, no resources will have been allocated */
		if (!inited || !HAVE_FLUID)
			return;
		for (size_t i = 0; i < synths.size(); ++i) {
			assert(!synths[i].inUse);
			fluid.delete_synth(synths[i].synth);
		}
		// FluidSynth 2.x Fix: https://github.com/FluidSynth/fluidsynth/issues/748
		// Settings should be the very last thing the developer has to free.
		fluid.delete_settings(flSettings);
	}

	void initIfNeeded(const Config &conf)
	{
		if (inited)
			return;
		inited = true;
		initFluidFunctions();
		if (!HAVE_FLUID)
			return;
		flSettings = fluid.new_settings();
		fluid.settings_setnum(flSettings, "synth.gain", 1.0f);
		fluid.settings_setnum(flSettings, "synth.sample-rate", SYNTH_SAMPLERATE);
		fluid.settings_setint(flSettings, "synth.chorus.active", conf.midi.chorus ? 1 : 0);
		fluid.settings_setint(flSettings, "synth.reverb.active", conf.midi.reverb ? 1 : 0);
		//if (soundFont.empty())
			//Debug() << "Warning: No initial soundfont specified, sound might be mute";
    //if (other_soundfont.empty()) {
		  //Debug() << "Warning: No custom soundfont specified, sound might be mute";
	}

	fluid_synth_t *allocateSynth()
	{
		assert(HAVE_FLUID);
		assert(inited);
		size_t i;
		for (i = 0; i < synths.size(); ++i)
			if (!synths[i].inUse)
				break;
		if (i < synths.size()) {
			fluid_synth_t *syn = synths[i].synth;
			fluid.synth_system_reset(syn);
			synths[i].inUse = true;
			return syn;
		} else {
			return addSynth(true);
		}
	}

	void releaseSynth(fluid_synth_t *synth)
	{
		size_t i;
		for (i = 0; i < synths.size(); ++i)
			if (synths[i].synth == synth)
				break;
		assert(i < synths.size());
		synths[i].inUse = false;
	}

	void set_default_soundfont(std::string def_sf)
	{
		assert(HAVE_FLUID);
		soundFont = def_sf;
		for (size_t i = 0; i < SYNTH_INIT_COUNT; ++i)
			addSynth(false);
	}

	void set_soundfont(std::string new_sf)
	{
		assert(HAVE_FLUID);
		//assert(inited);
		other_soundfont = new_sf;
		Debug() << "New SoundFont:" << new_sf;
		size_t i = 1000;
		fluid_synth_t *syn;
		Debug() << "Max Synths:" << synths.size(); 
		for (size_t n = 0; n < synths.size(); ++n) {
			syn = synths[n].synth;
		  fluid.synth_system_reset(syn);
		  fluid.synth_sfload(syn, new_sf.c_str(), 1);
			if (i == 1000)
				i = n;
		}
		if (synths.size() <= i)
			i = 0;
		Debug() << "Current Synthetizer:" << i + 1;
		synths[i].inUse = true;
	}

private:
	fluid_synth_t *addSynth(bool usedNow)
	{
		fluid_synth_t *syn = fluid.new_synth(flSettings);
		if (other_soundfont.empty()) {
			if (!soundFont.empty()) {
				fluid.synth_sfload(syn, soundFont.c_str(), 1);
			}
		}	else {
			fluid.synth_sfload(syn, other_soundfont.c_str(), 1);
		}
		Synth synth;
		synth.inUse = usedNow;
		synth.synth = syn;
		synths.push_back(synth);
		return syn;
	}
};

#endif // SHAREDMIDISTATE_H

/*
** config.cpp
**
** This file is part of HiddenChest.
**
** Copyright (C) 2013 mkxp Jonas Kulla <Nyocurio@gmail.com>
**           (C) 2019-2024 HiddenChest Kyonides Arkanthes <kyonides@gmail.com>
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
** along with HiddenChest.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "resolution.h"
#include <SDL_filesystem.h>
#include <fstream>
#include <stdint.h>
#include "debugwriter.h"
#include "util.h"
#include "sdl-util.h"

#ifdef INI_ENCODING
extern "C" {
#include <libguess.h>
}
#include <iconv.h>
#include <errno.h>
#endif

/* http://stackoverflow.com/a/1031773 */
static bool validUtf8(const char *string)
{
  const uint8_t *bytes = (uint8_t*) string;
  while(*bytes) {
		if( (/* ASCII * use bytes[0] <= 0x7F to allow ASCII control characters */
				bytes[0] == 0x09 ||
				bytes[0] == 0x0A ||
				bytes[0] == 0x0D ||
				(0x20 <= bytes[0] && bytes[0] <= 0x7E)
			) ) {
			bytes += 1;
			continue;
		}
		if( (/* non-overlong 2-byte */
				(0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
				(0x80 <= bytes[1] && bytes[1] <= 0xBF)
			) ) {
			bytes += 2;
			continue;
		}
		if( (/* excluding overlongs */
				bytes[0] == 0xE0 &&
				(0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			) ||
			(/* straight 3-byte */
				((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
					bytes[0] == 0xEE ||
					bytes[0] == 0xEF) &&
				(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			) ||
			(/* excluding surrogates */
				bytes[0] == 0xED &&
				(0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			) ) {
			bytes += 3;
			continue;
		}
		if( (/* planes 1-3 */
				bytes[0] == 0xF0 &&
				(0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			) ||
			(/* planes 4-15 */
				(0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
				(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			) ||
			(/* plane 16 */
				bytes[0] == 0xF4 &&
				(0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
				(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			) ) {
			bytes += 4;
			continue;
		}
		return false;
	}
	return true;
}

static std::string prefPath(const char *org, const char *app)
{
  char *path = SDL_GetPrefPath(org, app);
  if (!path)
		return std::string();
  std::string str(path);
  SDL_free(path);
  return str;
}

template<typename T>
std::set<T> setFromVec(const std::vector<T> &vec)
{
	return std::set<T>(vec.begin(), vec.end());
}
/*
typedef std::vector<std::string> StringVec;
namespace po = boost::program_options;

#define CONF_FILE "hiddenchest.conf"
*/
Config::Config()
{
	rgssVersion = 0;
	debugMode = false;
	printFPS = false;
	winResizable = true;
	fullscreen = false;
	fixedAspectRatio = true;
	smoothScaling = true;
	vsync = true;
	defScreenW = START_WIDTH;
	defScreenH = START_HEIGHT;
	windowTitle = "";
	fixedFramerate = 0;
	frameSkip = true;
	syncToRefreshrate = false;
	solidFonts = false;
	subImageFix = false;
	enableBlitting = true;
	maxTextureSize = 0;
	gameFolder = ".";
	anyAltToggleFS = false;
	enableReset = true;
	allowSymlinks = false;
	dataPathOrg = "";
	dataPathApp = "";
	customDataPath = ".";
	commonDataPath = ".";
	iconPath = "";
	execName = "Game";
	titleLanguage = "";
	midi.soundFont = "";
	midi.chorus = false;
	midi.reverb = false;
	SE.sourceCount = 12;
	customScript = "";
	pathCache = true;
	font_cache = false;
	useScriptNames = false;
}

void Config::read(int argc, char *argv[])
{
#define GUARD_ALL( exp ) try { exp } catch(...) {}
  editor.debug = false;
  editor.battleTest = false;
  // Read arguments sent from the editor
  if (argc > 1) {
    std::string argv1 = argv[1];
    // RGSS1 uses "debug", 2 and 3 use "test"
    if (argv1 == "debug" || argv1 == "test")
      editor.debug = true;
    else if (argv1 == "btest")
      editor.battleTest = true;
    // Fix offset
    if (editor.debug || editor.battleTest) {
      argc--;
      argv++;
    }
  }
  rgssVersion = clamp(rgssVersion, 0, 4);
  SE.sourceCount = clamp(SE.sourceCount, 6, 64);
  //if (!dataPathOrg.empty() && !dataPathApp.empty())
  //  customDataPath = prefPath(dataPathOrg.c_str(), dataPathApp.c_str());
  //commonDataPath = prefPath(".", "hiddenchest");
}

static std::string baseName(const std::string &path)
{
  size_t pos = path.find_last_of("/\\");
  return pos == path.npos ? path : path.substr(pos + 1);
}

static void setupScreenSize(Config &conf)
{
  if (conf.defScreenW < 1)
		conf.defScreenW = START_WIDTH;
  if (conf.defScreenH < 1)
		conf.defScreenH = START_HEIGHT;
}

HiddenChest is Kyonides Arkanthes's fork of a previous project called
mkxp developed by Ancurio.

** This project is distributed under the terms of the GNU General
** Public License as published by the Free Software Foundation version 2.
** This project is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.

Files included in this ZIP file

h800 or h800.exe
  Resolution 800*608 and includes extra buttons not found in vanilla RGSS games
  plus tons of other features already described in README.md
h*anyotherwidth or h*anyotherwidth*.exe
  Possible resolutions 800*608, 960*736, 1088*608, 1280*768, 1680*1050 and 1920*1080
  Includes extra buttons not found in vanilla RGSS games
Now the executable's filename lets the engine determine which INI file it should
open to setup all of the basic values it needs to run properly.
ClickableWidget.rb
ClickableSprite.rb
ClickableWindow.rb
PrintSceneHC.rb
  Modifications of the default classes that allow you to click and move around
  any kind of widgets, namely sprites and windows. The mouse wheel can also scroll
  the menu window up and down at will.
KSoundFontMenuXP.rb
  A custom menu that allows you to change your current soundfont SF2 on the fly!
MapCustomResFixes.rb
  A Ruby script you can open with your favorite text editor to
  copy and paste it in any RGSS 1 or 2 game to let old scripts
  become compatible with the increased window resolution.
WindowOpenness.rb
  A Ruby script you can copy and paste to check how the window openness feature
  works in a common menu.
SceneExample.rb
  A Ruby script that shows you how to combine a window openness feature with any
  sprite reduce_speed increase_width! and reduce_width! features.

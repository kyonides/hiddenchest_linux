# * KSoundFontMenu XP * # 
#   Scripter : Kyonides Arkanthes
#   2024-05-07

# * Script Call * #
# $scene = KSoundFont::Menu.new

module KSoundFont
  TITLE = "Select a SoundFont"
  THIS_SOUNDFONT = "Current SoundFont"
  TRANSPARENT = Color.new(0, 0, 0, 0)

class PathsWindow < Window_Command
  def initialize(w, paths)
    @paths = paths
    commands = paths.map {|fn| fn.split("/")[-1].sub(".sf2", "") }
    super(w, commands)
  end

  def path
    @paths[@index]
  end

  def name
    @commands[@index]
  end
end

class SmallInfoWindow < Window_Base
  def initialize(wx, wy, w, h)
    super
    self.contents = Bitmap.new(w - 32, h - 32)
    contents.font.color = system_color
    contents.draw_text(0, 0, w - 32, 32, THIS_SOUNDFONT, 1)
    contents.font.color = normal_color
  end

  def set_text(text)
    contents.fill_rect(0, 32, width - 32, 32, TRANSPARENT)
    contents.draw_text(0, 32, width - 32, 32, text, 1)
  end
end

class Menu
  def main
    soundfont = Setup.soundfont.split("/")[-1].sub(".sf2", "")
    @help_window = Window_Help.new
    @help_window.set_text(TITLE, 1)
    @command_window = PathsWindow.new(192, Setup.soundfonts)
    @command_window.y = 64
    @info_window = SmallInfoWindow.new(192, @command_window.y, 240, 96)
    @info_window.set_text(soundfont)
    Graphics.transition
    loop do
      Graphics.update
      Input.update
      update
      break if $scene != self
    end
    Graphics.freeze
    @info_window.dispose
    @command_window.dispose
    @help_window.dispose
  end

  def update
    @command_window.update
    if Input.trigger?(Input::B)
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Map.new
      return
    elsif Input.trigger?(Input::C)
      $game_system.se_play($data_system.decision_se)
      Setup.choose_soundfont(@command_window.index)
      @info_window.set_text(@command_window.name)
    end
  end
end

end
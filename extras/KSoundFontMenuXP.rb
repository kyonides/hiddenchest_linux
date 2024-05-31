# * KSoundFontMenu XP * # 
#   Scripter : Kyonides Arkanthes
#   2024-05-30

# Note: It seems like the MIDI file should be playing BEFORE you proceed to call
#       Game.choose_soundfont while loading a save game. Otherwise, the MIDI
#       synthetizer might ignore your request at that specific point.
#       Calling the same method at the beginning of KSoundFont::Menu ensures it
#       will always play the MIDI using the chosen soundfont.

# * Script Call * #
# $scene = KSoundFont::Menu.new

module KSoundFont
  START_SF = [0]
  TITLE = "Select a SoundFont"
  THIS_SOUNDFONT = "Current SoundFont"
  DELAY_MESSAGE = "Processing New SF..."
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
    @timer = 0
    pos = $game_system.soundfont_index
    Game.choose_soundfont(pos)
    soundfonts = Game.soundfonts
    gs_indexes = $game_system.soundfont_indexes
    if gs_indexes.any?
      @indexes = gs_indexes.sort
      if @indexes.size < soundfonts.size
        soundfonts = @indexes.map{|n| soundfonts[n] }
      end
    end
    @total = soundfonts.size
    @full_list = Game.soundfonts.size == @total
    soundfont = soundfonts[pos]
    dialog_box soundfont
    if soundfont&.empty?
      soundfont = "No SoundFont"
    else
      soundfont = soundfont.split("/")[-1].sub(".sf2", "")
    end
    @help_window = Window_Help.new
    @help_window.set_text(TITLE, 1)
    @command_window = PathsWindow.new(192, soundfonts)
    @command_window.y = 64
    @command_window.index = pos
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
    if @timer > 0
      @timer -= 1
      if @timer == 0
        @info_window.set_text(@command_window.name)
      end
      return
    end
    @command_window.update
    if Input.trigger?(Input::B) or Input.double_right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Map.new
      return
    elsif Input.trigger?(Input::C) or Input.double_left_click?
      if @total < 2
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      $game_system.se_play($data_system.decision_se)
      n = @command_window.index
      n = @indexes[n] unless @full_list
      $game_system.soundfont_index = n
      @info_window.set_text(DELAY_MESSAGE)
      @timer = Graphics.frame_rate * 2
    elsif Input.trigger?(Input::SHIFT)
      $game_system.se_play($data_system.cursor_se)
      Graphics.screenshot
    end
  end
end

end

class Game_System
  alias :kyon_soundfont_menu_gm_sys_init :initialize
  def initialize
    kyon_soundfont_menu_gm_sys_init
    self.soundfont_index = find_soundfont_index
    @soundfont_indexes = KSoundFont::START_SF.dup
  end

  def find_soundfont_index
    Game.soundfont_index || 0
  end

  def soundfont_index
    @soundfont_index ||= find_soundfont_index
  end

  def soundfont_index=(n)
    Game.choose_soundfont(n)
    @soundfont_index = n
  end

  def soundfont_indexes
    @soundfont_indexes ||= []
  end
end

class Scene_Load
  alias :kyon_soundfont_menu_scn_ld_rsd :read_save_data
  def read_save_data(file)
    kyon_soundfont_menu_scn_ld_rsd(file)
    $game_system.bgm_play($game_system.playing_bgm)
    Game.choose_soundfont($game_system.soundfont_index)
  end
end
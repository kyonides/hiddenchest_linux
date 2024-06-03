# * KSoundFontMenu ACE * # 
#   Scripter : Kyonides Arkanthes
#   2024-06-02

# Note: It seems like the MIDI file should be playing BEFORE you proceed to call
#       Game.choose_soundfont while loading a save game. Otherwise, the MIDI
#       synthetizer might ignore your request at that specific point.
#       Calling the same method at the beginning of KSoundFont::Menu ensures it
#       will always play the MIDI using the chosen soundfont.

# * Script Call * #
# SceneManager(KSoundFont::Menu)

module KSoundFont
  START_SF = [0]
  TITLE = "Select a SoundFont"
  THIS_SOUNDFONT = "Current SoundFont"
  DELAY_MESSAGE = "Processing New SF..."
  TRANSPARENT = Color.new(0, 0, 0, 0)

class HelpWindow < Window_Help
  def set_text(text, align)
    return if text == @text and align == @align
    @text = text
    @align = align
    refresh
  end

  def refresh
    contents.clear
    draw_text(contents.rect, @text, @align)
  end
end

class PathsWindow < Window_Selectable
  def initialize(w, paths)
    @paths = paths
    @commands = paths.map {|fn| fn.split("/")[-1].sub(".sf2", "") }
    @item_max = @commands.size
    h = @item_max * line_height + line_height
    super(0, 0, w, h)
    return if @item_max == 0
    refresh
    self.active = true
  end

  def draw_all_items
    @item_max.times do |n|
      rect = @area[n] ||= item_rect_for_text(n)
      draw_text(rect, @commands[n])
    end
  end

  def process_handling
    
  end

  def path
    @paths[@index]
  end

  def name
    @commands[@index]
  end
  attr_reader :item_max
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

class Menu < Scene_Base
  def start
    super
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
    if soundfont.empty?
      soundfont = "No SoundFont"
    else
      soundfont = soundfont.split("/")[-1].sub(".sf2", "")
    end
    @help_window = HelpWindow.new
    @help_window.set_text(TITLE, 1)
    hwy = @help_window.height
    @command_window = PathsWindow.new(192, soundfonts)
    @command_window.y = hwy
    @command_window.index = pos
    @info_window = SmallInfoWindow.new(192, hwy, 240, 96)
    @info_window.set_text(soundfont)
  end

  def update
    Graphics.update
    Input.update
    if @timer > 0
      @timer -= 1
      if @timer == 0
        @info_window.set_text(@command_window.name)
      end
      return
    end
    @command_window.update
    if Input.trigger?(:B) or Input.double_right_click?
      Sound.play_cancel
      SceneManager.return
      return
    elsif Input.trigger?(:C) or Input.double_left_click?
      return Sound.play_buzzer if @total < 2
      Sound.play_ok
      n = @command_window.index
      n = @indexes[n] unless @full_list
      $game_system.soundfont_index = n
      @info_window.set_text(DELAY_MESSAGE)
      @timer = Graphics.frame_rate * 2
    elsif Input.trigger?(:SHIFT)
      Sound.play_cursor
      Graphics.screenshot
    end
  end
end

end

class Game_System
  alias :kyon_soundfont_menu_gm_sys_init :initialize
  alias :kyon_soundfont_menu_gm_sys_on_after_load :on_after_load
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

  def on_after_load
    kyon_soundfont_menu_gm_sys_on_after_load
    Game.choose_soundfont($game_system.soundfont_index)
  end
end
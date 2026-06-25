# * HiddenPlayList XP * #
#   Scripter : Kyonides Arkanthes
#   2026-06-25

# * REQUIRES HiddenChest v1.2.12 * #

# This scriptlet allows you to play a list of songs one after another.

# * Script Call * #

# $scene = HiddenPlay::Scene.new

module HiddenPlay
  @menu_bgm = ["024-Town02", 50]
  MAIN_BGM_CHANNEL = 3
  LIST_BGM_CHANNEL = 1
  class Song < RPG::AudioFile
    DIR = "Audio/BGM/"
    def play(n)
      @channel = n
      Audio.bgms_play(@channel, DIR + @name, @volume, @pitch)
    end

    def pause
      Audio.bgms_pause(@channel)
    end

    def resume
      Audio.bgms_resume(@channel)
    end
    attr_accessor :title
  end

  class List
    def initialize
      @name = "Playlist"
      @files = []
    end
    attr_accessor :name, :files
  end

  extend self
  attr_reader :menu_bgm

  class Scene
    SELECT_LIST = "Select a Playlist"
    EMPTY_LIST = "Empty List"
    MENU_BGM = Song.new(*HiddenPlay.menu_bgm)
    VOLUME = 40
    def initialize
      @loop_states = Audio.bgms_loop_all.dup
      Audio.bgms_loop_set(1, false)
      Audio.bgms_loop_set(2, false)
      Audio.bgms_loop_set(3, true)
      3.times {|n| Audio.bgms_fade(n + 1, 60) }
      MENU_BGM.play(MAIN_BGM_CHANNEL)
      @font_outline = Font.default_outline
      Font.default_outline = true
      @lists = []
      @files = Dir["playlist*.txt"].sort
      @files.each do |file|
        lines = File.readlines(file)
        list = List.new
        list.name = lines.shift.chomp
        lines.each do |line|
          texts = line.split(" > ")
          audio = Song.new
          audio.title = texts.shift
          audio.name = texts.shift.chomp
          audio.volume = VOLUME
          #audio.pitch = 200
          list.files << audio
        end
        @lists << list
      end
      @commands = @lists.map(&:name) || [EMPTY_LIST]
      @playlist_empty = @lists.empty?
      @list_index = @song_index = @timer = 0
      @stop = true
      @stage = :main
    end

    def main
      start
      Graphics.transition
      while @stage
        Graphics.update
        Input.update
        update
      end
      Graphics.freeze
      terminate
    end

    def start
      @help_window = Window_Help.new
      @help_window.set_text(SELECT_LIST, 1)
      @list_window = Window_Command.new(240, @commands)
      @list_window.y = 64
    end

    def update
      case @stage
      when :main
        update_main
      when :list
        update_list
      when :next
        update_next
      end
    end

    def update_main
      @list_window.update
      if Input.trigger?(:B)
        $game_system.se_play($data_system.cancel_se)
        Audio.bgms_stop_all
        $scene = Scene_Map.new
        return @stage = nil
      elsif Input.trigger?(:C)
        if @playlist_empty
          $game_system.se_play($data_system.buzzer_se)
          return
        end
        MENU_BGM.pause
        n = @list_window.index
        @list = @lists[n]
        @files = @list.files
        if @files.empty?
          $game_system.se_play($data_system.buzzer_se)
          @list = nil
          return @files = nil
        end
        $game_system.se_play($data_system.decision_se)
        @song_index = 0 if @list_index != n
        @list_index = n
        @help_window.set_text(@list.name, 1)
        @list_window.active = false
        @list_window.visible = false
        titles = @files.map(&:title)
        @song_window = Window_Command.new(240, titles)
        @song_window.y = 64
        @song_window.index = @song_index
        @file_max = @files.size
        file = @files[@song_index]
        $game_system.bgms_play(LIST_BGM_CHANNEL, file)
        @stage = :list
      end
    end

    def update_next
      @song_index = (@song_index + 1) % @file_max
      @song_window.index = @song_index
      file = @files[@song_index]
      file.play(LIST_BGM_CHANNEL)
      @play_next = nil
      @stage = :list
    end

    def update_list
      if Audio.bgms_stopped?(LIST_BGM_CHANNEL)
        return @stage = :next
      end
      @song_window.update
      if Input.trigger?(:B)
        $game_system.se_play($data_system.cancel_se)
        Audio.bgms_stop(LIST_BGM_CHANNEL)
        MENU_BGM.resume
        @help_window.set_text(SELECT_LIST, 1)
        @song_window.dispose
        @list_window.active = true
        @list_window.visible = true
        return @stage = :main
      elsif Input.trigger?(:C)
        $game_system.se_play($data_system.decision_se)
        return if @song_window.index == @index
        Audio.bgms_stop(LIST_BGM_CHANNEL)
        @song_index = @song_window.index - 1
        @stage = :next
      end
    end

    def terminate
      @loop_states.each {|n, state| Audio.bgms_loop_set(n, state)
                         puts "Previous State: #{state}"
                         puts Audio.bgms_loop(n) }
      Font.default_outline = @font_outline
      @list_window.dispose
      @help_window.dispose
      $game_map.autoplay
    end
  end
end

class Game_System
  def bgms_play(n, bgm)
    @playing_bgms ||= {}
    @playing_bgms[n] = bgm
    if bgm != nil and bgm.name != ""
      Audio.bgms_play(n, "Audio/BGM/" + bgm.name, bgm.volume, bgm.pitch)
    else
      Audio.bgms_stop(n)
    end
    Graphics.frame_reset
  end

  def bgms_stop(n)
    Audio.bgms_stop(n)
  end

  def bgm_fade(n, time)
    @playing_bgms ||= {}
    @playing_bgms[n] = nil
    Audio.bgms_fade(n, time * 1000)
  end

  def bgms_memorize(n)
    @memorized_bgms ||= {}
    @memorized_bgms[n] = @playing_bgm
  end

  def bgms_restore(n)
    bgms_play(n, @memorized_bgm)
  end
end
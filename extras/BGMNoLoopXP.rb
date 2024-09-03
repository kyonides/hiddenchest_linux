# * BGM No Loop XP * #
#   Scripter : Kyonides Arkanthes
#   2024-06-18

# This scriptlet allows you to prevent the game from playing the same old song
# over and over again. Now you can define a list of BGMs for specific maps!

module BgmNoLoop
  WAIT = 10 * 6
  VOLUME = 60
  MAP_BGM = {}
  MAP_BGM[1] = ["015-Theme04"]#, "017-Theme06"]
end

class Game_Map
  alias :kyon_bgm_no_loop_gm_map_setup :setup
  def setup(map_id)
    check_bgm_loop(map_id)
    kyon_bgm_no_loop_gm_map_setup(map_id)
  end

  def check_bgm_loop(map_id)
    Audio.bgm_loop = !BgmNoLoop::MAP_BGM.has_key?(map_id)
    puts "Check Loop in Map ##{map_id}: " + Audio.bgm_loop.to_s
  end

  def map_bgm
    @map.bgm
  end

  def find_bgms
    bgm = @map.bgm
    $game_system.bgm_volume = bgm.volume
    bgms = []
    bgms << bgm unless bgm.name.empty?
    bgms + BgmNoLoop::MAP_BGM[@map_id]
  end
end

class Game_System
  alias :kyon_bgm_no_loop_gm_sys_init :initialize
  alias :kyon_bgm_no_loop_gm_sys_up :update
  def initialize
    kyon_bgm_no_loop_gm_sys_init
    reset_bgm_timer
    reset_bgm_volume
    @bgm_index = 0
  end

  def reset_bgm_timer
    @bgm_timer = BgmNoLoop::WAIT
  end

  def reset_bgm_volume
    @bgm_volume ||= BgmNoLoop::VOLUME
  end

  def update
    kyon_bgm_no_loop_gm_sys_up
    update_bgm
  end

  def update_bgm
    return if $game_temp.in_battle or Audio.bgm_loop
    return unless Audio.bgm_stopped?
    puts "BGM Stopped!" if BgmNoLoop::WAIT == @bgm_timer
    if @bgm_timer > 0
      @bgm_timer -= 1
      return
    end
    puts "Looking for a new BGM to play"
    reset_bgm_timer
    bgms = $game_map.find_bgms
    @bgm_index ||= 0
    @bgm_index = (@bgm_index + 1) % bgms.size
    bgm = bgms[@bgm_index]
    bgm = RPG::AudioFile.new(bgm) if bgm.is_a?(String)
    bgm_play(bgm)
  end

  def bgm_play(bgm)
    reset_bgm_volume
    @playing_bgm = bgm
    if bgm != nil and bgm.name != ""
      Audio.bgm_play("Audio/BGM/" + bgm.name, @bgm_volume, bgm.pitch)
    else
      Audio.bgm_stop
    end
    Graphics.frame_reset
  end
  attr_accessor :bgm_volume
end

class Scene_Title
  alias :kyon_bgm_no_loop_main :main
  def main
    if Audio.bgm_playing?
      Audio.bgm_stop
      Audio.bgm_close rescue nil
    end
    Audio.bgm_loop = true
    kyon_bgm_no_loop_main
  end
end
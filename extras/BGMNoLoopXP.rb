# * BGM No Loop XP * #
#   Scripter : Kyonides Arkanthes
#   2024-06-05

# This scriptlet allows you to prevent the game from playing the same old song
# over and over again. Now you can define a list of BGMs for specific maps!

module BgmNoLoop
  WAIT = 20
  MAP_BGM = {}
  MAP_BGM[1] = ["015-Theme04", "017-Theme06"]
end

class Game_Map
  alias :kyon_bgm_no_loop_gm_map_setup :setup
  def setup(map_id)
    check_bgm_loop(map_id)
    kyon_bgm_no_loop_gm_map_setup(map_id)
  end

  def check_bgm_loop(map_id)
    Audio.bgm_loop = (Audio.default_bgm_loop or !BgmNoLoop::MAP_BGM[map_id])
  end

  def map_bgm
    @map.bgm
  end

  def find_bgms
    bgm = map_bgm
    bgms = []
    bgms += [bgm] unless bgm.name.empty?
    bgms + BgmNoLoop::MAP_BGM[@map_id]
  end
end

class Game_System
  alias :kyon_bgm_no_loop_gm_sys_init :initialize
  alias :kyon_bgm_no_loop_gm_sys_up :update
  def initialize
    kyon_bgm_no_loop_gm_sys_init
    reset_bgm_timer
    @bgm_index = 0
  end

  def reset_bgm_timer
    @bgm_timer = BgmNoLoop::WAIT
  end

  def update
    kyon_bgm_no_loop_gm_sys_up
    return if $game_temp.in_battle or Audio.bgm_playing?
    if @bgm_timer > 0
      @bgm_timer -= 1
      return
    end
    reset_bgm_timer
    @bgm_index ||= 0
    bgms = $game_map.find_bgms
    @bgm_index = (@bgm_index + 1) % bgms.size
    bgm = bgms[@bgm_index]
    bgm = RPG::AudioFile.new(bgm) if bgm.is_a?(String)
    bgm_play(bgm) if bgm != nil
  end
end
# * BGM No Loop XP * #
#   Scripter : Kyonides Arkanthes
#   2024-06-05

module BgmNoLoop
  BGM_WAIT = 20
  NO_LOOP = [1]
  MAP_BGM = {}
  MAP_BGM.default = []
  MAP_BGM[1] = ["015-Theme04", "017-Theme06"]
end

class Game_Map
  alias :kyon_bgm_no_loop_gm_map_setup :setup
  def setup(map_id)
    check_bgm_loop(map_id)
    kyon_bgm_no_loop_gm_map_setup(map_id)
  end

  def check_bgm_loop(map_id)
    can_loop = (Audio.default_bgm_loop or !BgmNoLoop::NO_LOOP[map_id])
    can_loop ? Audio.bgm_loop : Audio.bgm_no_loop
  end

  def map_bgm
    @map.bgm
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
    @bgm_timer = BgmNoLoop::BGM_WAIT
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
    bgms = find_bgms
    @bgm_index = (@bgm_index + 1) % bgms.size
    bgm = bgms[@bgm_index]
    bgm = RPG::AudioFile.new(bgm) if bgm.is_a?(String)
    bgm_play(bgm) if bgm != nil
  end

  def find_bgms
    map_bgm = $game_map.map_bgm
    bgms = $game_map.map_bgm.name.empty? ? [] : [map_bgm]
    bgms + BgmNoLoop::MAP_BGM[$game_map.map_id]
  end
end
# Increased HiddenChest XP Resolution Map Fix * #
# * Scripter : Kyonides * #
# Update:   2026-07-07
# Original: 2019-07-16

# * Optional Script Call * #
# - Change Current Autotile Animation Speed to 1x, 2x or 4x.
# Example: $game_map.autotiles_speed = 4

class Game_Screen
  def weather(type, power, duration)
    @weather_type_target = type
    if @weather_type_target != 0
      @weather_type = @weather_type_target
    end
    if @weather_type_target == 0
      @weather_max_target = 0.0
    else
      @weather_max_target = (power + 1) * 20.0
    end
    @weather_duration = duration
    if @weather_duration == 0
      @weather_type = @weather_type_target
      @weather_max = @weather_max_target
    end
  end
end

class Game_Map
  alias :kyon_map_cstm_res_gm_map_init :initialize
  def initialize
    kyon_map_cstm_res_gm_map_init
    @autotiles_speed = 1
  end

  def scroll_down(distance)
    tiles_max = Graphics.height / 32
    @display_y = [@display_y + distance, (self.height - tiles_max) * 128].min
  end

  def scroll_right(distance)
    tiles_max = Graphics.width / 32
    @display_x = [@display_x + distance, (self.width - tiles_max) * 128].min
  end
  attr_accessor :autotiles_speed
end

class Game_Player < Game_Character
  def center_x
    (Graphics.width / 2 - 16) * 4
  end

  def center_y
    (Graphics.height / 2 - 16) * 4
  end

  def center(x, y)
    max_x = ($game_map.width - Graphics.width / 32) * 128
    max_y = ($game_map.height - Graphics.height / 32) * 128
    $game_map.display_x = [0, [x * 128 - center_x, max_x].min].max
    $game_map.display_y = [0, [y * 128 - center_y, max_y].min].max
  end

  def update
    last_moving = moving?
    unless moving? or $game_system.map_interpreter.running? or
           @move_route_forcing or $game_temp.message_window_showing
      # Move player in the direction the directional button is being pressed
      case Input.dir4
      when 2
        move_down
      when 4
        move_left
      when 6
        move_right
      when 8
        move_up
      end
    end
    last_real_x = @real_x
    last_real_y = @real_y
    super
    if @real_y > last_real_y and @real_y - $game_map.display_y > center_y
      $game_map.scroll_down(@real_y - last_real_y)
    end
    if @real_x < last_real_x and @real_x - $game_map.display_x < center_x
      $game_map.scroll_left(last_real_x - @real_x)
    end
    if @real_x > last_real_x and @real_x - $game_map.display_x > center_x
      $game_map.scroll_right(@real_x - last_real_x)
    end
    if @real_y < last_real_y and @real_y - $game_map.display_y < center_y
      $game_map.scroll_up(last_real_y - @real_y)
    end
    unless moving?
      if last_moving
        result = check_event_trigger_here([1,2])
        if result == false
          unless $DEBUG and Input.press?(Input::CTRL)
            if @encounter_count > 0
              @encounter_count -= 1
            end
          end
        end
      end
      if Input.trigger?(Input::C)
        check_event_trigger_here([0])
        check_event_trigger_there([0,1,2])
      end
    end
  end
end

class Spriteset_Map
  def initialize
    create_viewports
    create_tilemap
    create_panorama
    create_fog
    create_characters
    create_weather
    create_pictures
    create_timer
    update
  end

  def create_viewports
    w, h = Graphics.dimensions
    @viewport1 = Viewport.new(0, 0, w, h)
    @viewport2 = Viewport.new(0, 0, w, h)
    @viewport3 = Viewport.new(0, 0, w, h)
    @viewport2.z = 200
    @viewport3.z = 5000
  end

  def create_tilemap
    @at_speed = $game_map.autotiles_speed
    @tilemap = Tilemap.new(@viewport1)
    @tilemap.z = 0
    @tilemap.autotiles_speed = @at_speed
    @tilemap.tileset = RPG::Cache.tileset($game_map.tileset_name)
    for i in 0..6
      autotile_name = $game_map.autotile_names[i]
      @tilemap.autotiles[i] = RPG::Cache.autotile(autotile_name)
    end
    @tilemap.map_data = $game_map.data
    @tilemap.priorities = $game_map.priorities
    @tilemap.ox = $game_map.display_x / 4
    @tilemap.oy = $game_map.display_y / 4
    @tilemap.update
  end

  def create_panorama
    @panorama = Plane.new(@viewport1)
    @panorama.z = -1000
  end

  def create_fog
    @fog = Plane.new(@viewport1)
    @fog.z = 3000
  end

  def create_characters
    @character_sprites = []
    events = $game_map.events
    keys = events.keys.sort
    for i in keys
      @character_sprites << Sprite_Character.new(@viewport1, events[i])
    end
    @character_sprites << Sprite_Character.new(@viewport1, $game_player)
  end

  def create_weather
    @weather = RPG::Weather.new(@viewport1)
  end

  def create_pictures
    pictures = $game_screen.pictures
    @picture_sprites = []
    for i in 1..50
      @picture_sprites << Sprite_Picture.new(@viewport2, pictures[i])
    end
  end

  def create_timer
    @timer_sprite = Sprite_Timer.new
  end

  def update
    update_autotiles
    update_panorama
    update_fog
    update_tilemap
    update_panorama_plane
    update_fog_plane
    update_characters
    update_weather
    update_pictures
    update_timer
    update_viewports
  end

  def update_autotiles
    if $game_map.autotiles_speed != @at_speed
      @at_speed = $game_map.autotiles_speed
      @tilemap.autotiles_speed = @at_speed
    end
  end

  def update_panorama
    if @panorama_name != $game_map.panorama_name or
       @panorama_hue != $game_map.panorama_hue
      @panorama_name = $game_map.panorama_name
      @panorama_hue = $game_map.panorama_hue
      if @panorama.bitmap != nil
        @panorama.bitmap.dispose
        @panorama.bitmap = nil
      end
      if @panorama_name != ""
        @panorama.bitmap = RPG::Cache.panorama(@panorama_name, @panorama_hue)
      end
      Graphics.frame_reset
    end
  end

  def update_fog
    if @fog_name != $game_map.fog_name or @fog_hue != $game_map.fog_hue
      @fog_name = $game_map.fog_name
      @fog_hue = $game_map.fog_hue
      if @fog.bitmap != nil
        @fog.bitmap.dispose
        @fog.bitmap = nil
      end
      if @fog_name != ""
        @fog.bitmap = RPG::Cache.fog(@fog_name, @fog_hue)
      end
      Graphics.frame_reset
    end
  end

  def update_tilemap
    @tilemap.ox = $game_map.display_x / 4
    @tilemap.oy = $game_map.display_y / 4
    @tilemap.update
  end

  def update_panorama_plane
    @panorama.ox = $game_map.display_x / 8
    @panorama.oy = $game_map.display_y / 8
  end

  def update_fog_plane
    @fog.zoom_x = $game_map.fog_zoom / 100.0
    @fog.zoom_y = $game_map.fog_zoom / 100.0
    @fog.opacity = $game_map.fog_opacity
    @fog.blend_type = $game_map.fog_blend_type
    @fog.ox = $game_map.display_x / 4 + $game_map.fog_ox
    @fog.oy = $game_map.display_y / 4 + $game_map.fog_oy
    @fog.tone = $game_map.fog_tone
  end

  def update_characters
    @character_sprites.each {|sprite| sprite.update }
  end

  def update_weather
    @weather.type = $game_screen.weather_type
    @weather.max = $game_screen.weather_max
    @weather.ox = $game_map.display_x / 4
    @weather.oy = $game_map.display_y / 4
    @weather.update
  end

  def update_pictures
    @picture_sprites.each {|sprite| sprite.update }
  end

  def update_timer
    @timer_sprite.update
  end

  def update_viewports
    @viewport1.tone = $game_screen.tone
    @viewport1.ox = $game_screen.shake
    @viewport3.color = $game_screen.flash_color
    @viewport1.update
    @viewport3.update
  end

  def reset_viewports
    w, h = Graphics.dimensions
    @viewport1.rect.set(0, 0, w, h)
    @viewport2.rect.set(0, 0, w, h)
    @viewport3.rect.set(0, 0, w, h)
  end

  def dispose
    dispose_tilemap
    dispose_fog
    dispose_panorama
    dispose_characters
    dispose_weather
    dispose_pictures
    dispose_timer
    dispose_viewports
  end

  def dispose_tilemap
    @tilemap.tileset.dispose
    7.times {|n| @tilemap.autotiles[n].dispose }
    @tilemap.dispose
  end

  def dispose_fog
    @fog.dispose
  end

  def dispose_panorama
    @panorama.dispose
  end

  def dispose_characters
    @character_sprites.each {|sprite| sprite.dispose }
  end

  def dispose_weather
    @weather.dispose
  end

  def dispose_pictures
    @picture_sprites
  end

  def dispose_timer
    @timer_sprite.dispose
  end

  def dispose_viewports
    @viewport1.dispose
    @viewport2.dispose
    @viewport3.dispose
  end
end

class Window_Message < Window_Selectable
  def initialize
    w = case Graphics.width
    when 640 then 520
    when 800 then 640
    else 800 # 960, 1280, 1680, 1920
    end
    super((Graphics.width - w) / 2, 304, w, 160)
    self.contents = Bitmap.new(width - 32, height - 32)
    self.visible = false
    self.z = 9998
    @face = Bitmap.new(100, 100)
    @fade_in = false
    @fade_out = false
    @contents_showing = false
    @cursor_width = 0
    self.active = false
    self.index = -1
  end

  def reset_window
    if $game_temp.in_battle
      self.y = 16
    else
      case $game_system.message_position
      when 0
        self.y = 16
      when 1
        self.y = Graphics.height / 4
      when 2
        self.y = Graphics.height - 176
      end
    end
    self.opacity = $game_system.message_frame == 0 ? 255 : 0
    self.back_opacity = 160
  end
end
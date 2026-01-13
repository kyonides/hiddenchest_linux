# * KChangeKeys HC for VX + ACE * #
#   Scripter : Kyonides
#   2026-01-11

# * This scripts depends on HiddenChest v1.2.06 exclusively. * #

# With this scriptlet you are now able to change the keys that are normally
# mapped to any of RMXP's default buttons like A, B, C, etc. without pressing
# the F1, F2 & F12 buttons!

# For VX:
# $scene = KChangeKeys::Scene.new
# For VX ACE:
# SceneManager.call(KChangeKeys::Scene)

module KChangeKeys
  BACKDROP = "dwarven mine"
  HELP_BAR = "help bar"
  TARGET_BOX = "target box"
  CURSOR = "cursor"
  HEADING = "Key Bindings"
  CHOOSE_KEY = "Please select a key to edit"
  ENTER_KEY = "Please enter a key now"
# These are the default virtual buttons' names the RM editors have always used,
# but feel free to change them if ever needed.
  TARGETS = %w{Up Down L L2 Left Right R R2 A B C X Y Z}

class Scene
  def main
    setup
    create_sprites
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    terminate
  end

  def setup
    @vx_ace = Game::RGSS_VERSION == 3
    Font.default_outline = true
    @stage = :main
    @index = 0
    @col_index = 0
    @gamepad = Input.gamepad
    @bindings = Input.bindings
    @list = @bindings.list
    @target_names = []
    @bind_names = []
    @binds = []
    @key_names = []
    @changes = []
    @sprites = []
    case Graphics.width
    when 544
      @rows = 1
    when 640
      @rows = 2
    when 800
      @rows = 3
    else
      @rows = 4
    end
    @list.each {|bg| @binds += bg.data.take(@rows) }
    @key_names = @binds.map {|b| b.name || "" }
    @init_names = @key_names.dup
    @temp_names = @key_names.dup
    @back_color = Color.new(0, 0, 0, 120)
    @target_color = Color.new(255, 200, 80)
  end

  def create_sprites
    gw = Graphics.width
    fn = BACKDROP + gw.to_s
    @backdrop = Sprite.new
    @backdrop.bitmap = Cache.system(fn).dup
    b = Bitmap.new(Graphics.width, 48)
    b.font.size = 32
    b.draw_text(:rect, HEADING, 1)
    @heading = Sprite.new
    @heading.y = 4
    @heading.bitmap = b
    b = Cache.picture(HELP_BAR)
    bx = (gw - b.width) / 2
    @help_backdrop = Sprite.new
    @help_backdrop.set_xy(bx, 60)
    @help_backdrop.bitmap = b
    @help_bit = Bitmap.new(gw, 32)
    @help_bit.font.size = 24
    @help_bit.draw_text(:rect, CHOOSE_KEY, 1)
    @help = Sprite.new
    @help.y = 60
    @help.z = 20
    @help.bitmap = @help_bit
    @cursor = Sprite.new
    @cursor.z = 10
    @cursor.bitmap = Cache.picture(CURSOR)
    sw = gw / 206
    ix = (gw - sw * 206) / 2
    b = Cache.picture(TARGET_BOX)
    @key_max = TARGETS.size
    @key_max.times do |n|
      sx = ix + n % sw * 206
      sy = 108 + n / sw * @rows * 32
      @backdrop.bitmap.blt(sx + 2, sy - 2, b, b.rect)
      s = Sprite.new
      s.set_xyz(sx, sy, 20)
      s.bitmap = Bitmap.new(80, 28)
      s.bitmap.font.color = @target_color
      s.bitmap.draw_text(:rect, TARGETS[n], 1)
      @target_names << s
      @rows.times {|i| create_button_box(sx + 84, sy + i * 32) }
    end
    s = @bind_names[0]
    @cursor.set_xy(s.x - 4, s.y - 2)
    b.dispose
    @sprites += @target_names + @bind_names
    @sprites += [@heading, @help, @cursor, @help_backdrop, @backdrop]
  end

  def create_button_box(sx, sy)
    name = @temp_names.shift
    s = Sprite.new
    s.set_xyz(sx, sy, 20)
    b = Bitmap.new(112, 28)
    b.draw_text(:rect, name, 1)
    s.bitmap = b
    @bind_names << s
    rect = b.rect.dup
    rect.x = sx
    rect.y = sy
    @backdrop.bitmap.fill_rect(rect, @back_color)
  end

  def terminate
    @sprites.each do |s|
      s.bitmap.dispose
      s.dispose
    end
    @changes = @changes.flatten.compact
    return if @changes.empty?
    Input.save_key_bindings
  end

  def update
    case @stage
    when :main
      update_main
    when :key
      update_key
    end
  end

  def box_index
    @index * @rows + @col_index
  end

  def update_main
    if Input.trigger?(:B)
      Sound.play_cancel
      $scene = Scene_Map.new
      return @stage = nil
    elsif Input.trigger?(:Up)
      update_cursor(0, -1)
      return
    elsif Input.trigger?(:Down)
      update_cursor(0, 1)
      return
    elsif Input.trigger?(:Left)
      update_cursor(-1, 0)
      return
    elsif Input.trigger?(:Right)
      update_cursor(1, 0)
      return
    elsif Input.trigger?(:Delete)
      Sound.play_buzzer
      n = box_index
      bind = @binds[n]
      bind.value = 0
      @key_names[n] = ""
      b = @bind_names[n].bitmap
      b.clear
      b.draw_text(:rect, "", 1)
      @changes[n] = true
      return
    elsif Input.trigger?(:C)
      @vx_ace ? Sound.play_ok : Sound.play_decision
      Input.keymap_mode!
      Input.update
      n = box_index
      @bind = @binds[n]
      @help_bit.clear
      @help_bit.draw_text(:rect, ENTER_KEY, 1)
      @bind_bit = @bind_names[n].bitmap
      @bind_bit.clear
      @stage = :key
    end
  end

  def update_cursor(m, n)
    Sound.play_cursor
    @index = (@index + m) % @key_max
    @col_index = (@col_index + n) % @rows
    s = @bind_names[box_index]
    @cursor.set_xy(s.x - 4, s.y - 2)
  end

  def reset_help
    Input.play_mode!
    Input.update
    @bind_bit = @bind = nil
    @help_bit.clear
    @help_bit.draw_text(:rect, CHOOSE_KEY, 1)
    @stage = :main
  end

  def update_key
    if Input.trigger?(:MouseRight)
      Sound.play_cancel
      name = @key_names[box_index]
      @bind_bit.draw_text(:rect, name, 1)
      reset_help
      return
    elsif Input.trigger_any?
      @vx_ace ? Sound.play_ok : Sound.play_decision
      case Input.trigger_type
      when 0
        @bind.value = 0
      when 1
        @bind.value = Input.trigger_last
      else
        @bind.value = Input.trigger_gp_value
      end
      n = box_index
      name = @bind.name
      @key_names[n] = name
      @bind_bit.clear
      @bind_bit.draw_text(:rect, name, 1)
      reset_help
      @changes[n] = true
    end
  end
end

end
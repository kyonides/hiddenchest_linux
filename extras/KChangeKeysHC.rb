# * KChangeKeys HC * #
#   v1.0.0 - 2026-01-13

module KChangeKeys
  RGSS = Game::RGSS_VERSION
  MIN_VERSION = "HiddenChest 1.2.07+"
  KEYBOARD = "Keyboard"
  HEADING = "Key Bindings"
  CHOOSE_KEY = "Please select a key to edit"
  ENTER_KEY = "Please enter a key now"
  TARGETS = %w{Up Down L L2 Left Right R R2 A B C X Y Z}
  STYLES = {}
  @last_scene = nil
  @style = nil

class Style
  attr_accessor :backdrop, :cursor, :gamepad
  attr_accessor :help, :keyboard, :target
end

  default = Style.new
  default.backdrop = "kb_backdrop"
  default.gamepad  = "gamepad_black_add"
  default.keyboard = "keyboard_black"
  default.help     = "kb_help_bar"
  default.target   = "kb_target"
  default.cursor   = "kb_cursor"
  STYLES[nil] = default

  extend self
  attr_writer :style
  def open!
    return if Graphics.block_f1
    if RGSS == 3
      SceneManager.call(Scene)
    else
      @last_scene = $scene
      $scene = Scene.new
    end
  end

  def close!
    if RGSS == 3
      SceneManager.return
    else
      $scene = @last_scene
      @last_scene = nil
    end
  end

  def reset!
    if RGSS == 3
      SceneManager.goto(Scene)
    else
      $scene = Scene.new
    end
  end

  def style
    STYLES[@style]
  end
end

module Cache
  extend self
  def kb_backdrop(filename, version)
    case version
    when 1
      RPG::Cache.title(filename)
    when 2
      Cache.int_system(filename)
    else
      Cache.int_title(filename)
    end
  end

  def kb_picture(filename, version)
    if version == 1
      RPG::Cache.picture(filename)
    else
      int_picture(filename)
    end
  end
end

module KChangeKeys

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
    Graphics.block_f1 = true
    Font.default_outline = true
    @stage = :main
    @index = 0
    @col_index = 0
    @gamepad = Input.gamepad
    @bindings = Input.bindings
    @list = @bindings.list
    @style = KChangeKeys.style
    if Input.gamepad?
      @picture = @style.gamepad
      vendor_name = @gamepad.vendor.sub(/(?:inc\.|corp\.)/i, "")
      @name = @gamepad.name.sub(vendor_name, "")
    else
      @picture = @style.keyboard
      @name = KEYBOARD
    end
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

  def create_backdrop(fn)
    case RGSS
    when 1
      dir = "Titles"
    when 2
      dir = "System"
    else
      dir = "Titles1"
    end
    file = Dir["Graphics/#{dir}/#{fn}.*"][0]
    fn = "" unless FileInt.exist?(file)
    if fn.empty?
      @backdrop.bitmap = Bitmap.new(Graphics.width, Graphics.height)
    else
      @backdrop.bitmap = Cache.kb_backdrop(fn, RGSS).dup
    end
  end

  def create_sprites
    gw = Graphics.width
    @backdrop = Sprite.new
    bd = @style.backdrop || ""
    fn = bd + gw.to_s
    create_backdrop(fn)
    name = @name + " - " + HEADING
    b = Cache.kb_picture(@picture, RGSS).dup
    gb = Bitmap.new(gw - 16, 44)
    gb.font.size = 32
    gb.draw_text(54, 4, gb.width - 54, 36, name)
    gb.blt(0, 0, b, b.rect)
    @gp_label = Sprite.new
    @gp_label.set_xy(8, 6)
    @gp_label.bitmap = gb
    help = @style.help
    b = Cache.kb_picture(help, RGSS).dup
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
    cursor = @style.cursor
    @cursor.bitmap = Cache.kb_picture(cursor, RGSS).dup
    sw = gw / 206
    ix = (gw - sw * 206) / 2
    target = @style.target
    b = Cache.kb_picture(target, RGSS).dup
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
    @sprites += [@gp_label, @help, @cursor, @help_backdrop, @backdrop]
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
    Graphics.block_f1 = false
    @changes = @changes.flatten.compact
    Input.save_key_bindings if @changes.empty?
  end

  def update
    if Input.gamepad_updates.any?
      Input.gamepad_update
      @changes.clear
      KChangeKeys.reset!
      return @stage = nil
    end
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
      KChangeKeys.close!
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
      Sound.play_ok
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
    if Input.trigger?(:MouseLeft) or Input.trigger?(:MouseRight)
      Sound.play_cancel
      name = @key_names[box_index]
      @bind_bit.clear
      @bind_bit.draw_text(:rect, name, 1)
      reset_help
      return
    elsif Input.trigger_any?
      Sound.play_ok
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
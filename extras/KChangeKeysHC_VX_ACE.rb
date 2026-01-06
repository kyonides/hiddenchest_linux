# * KChangeKeys HC for VX + ACE * #
#   Scripter : Kyonides
#   2026-01-05

# This scripts depends on HiddenChest version 1.2.04 or higher.

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
  TARGETS = %w{Up Down L L2 Left Right R R2 A B C X Y Z}

class Scene
  def main
    @vx_ace = Game::RGSS_VERSION == 3
    Font.default_outline = true
    @stage = :main
    @index = 0
    @gamepad = Input.gamepad
    @bindings = Input.bindings
    @list = @bindings.list
    @target_names = []
    @bind_names = []
    @binds = []
    @key_names = []
    @changes = []
    @list.each do |bg|
      bind = bg[0]
      @binds << bind
      @key_names << bind.name
    end
    @init_names = @key_names.dup
    @target_color = Color.new(255, 200, 80)
    create_sprites
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    @bind_names << @heading << @help << @cursor
    @bind_names << @help_backdrop << @backdrop
    list = @target_names + @bind_names
    list.each do |s|
      s.bitmap.dispose
      s.dispose
    end
    @changes.compact!
    return if @changes.empty?
    Input.save_key_bindings
  end

  def create_sprites
    @backdrop = Sprite.new
    @backdrop.bitmap = Cache.system(BACKDROP).dup
    b = Bitmap.new(Graphics.width, 48)
    b.font.size = 32
    b.draw_text(b.rect, HEADING, 1)
    @heading = Sprite.new
    @heading.y = 4
    @heading.bitmap = b
    b = Cache.picture(HELP_BAR)
    bx = (Graphics.width - b.width) / 2
    @help_backdrop = Sprite.new
    @help_backdrop.set_xy(bx, 60)
    @help_backdrop.bitmap = b
    @help_bit = Bitmap.new(Graphics.width, 32)
    @help_bit.font.size = 24
    @help_bit.draw_text(@help_bit.rect, CHOOSE_KEY, 1)
    @help = Sprite.new
    @help.y = 60
    @help.z = 20
    @help.bitmap = @help_bit
    @cursor = Sprite.new
    @cursor.z = 10
    @cursor.bitmap = Cache.picture(CURSOR)
    b = Cache.picture(TARGET_BOX)
    @key_max = TARGETS.size
    @key_max.times do |n|
      sx = 6 + n % 3 * 206
      sy = 108 + n / 3 * 44
      @backdrop.bitmap.blt(sx + 2, sy - 2, b, b.rect)
      s = Sprite.new
      s.set_xyz(sx, sy, 20)
      s.bitmap = Bitmap.new(80, 28)
      s.bitmap.font.color = @target_color
      s.bitmap.draw_text(s.bitmap.rect, TARGETS[n], 1)
      @target_names << s
      s = Sprite.new
      s.set_xyz(sx + 84, sy, 20)
      s.bitmap = Bitmap.new(104, 28)
      s.bitmap.draw_text(s.bitmap.rect, @key_names[n], 1)
      @bind_names << s
    end
    s = @bind_names[0]
    @cursor.set_xy(s.x - 4, s.y - 2)
    b.dispose
  end

  def update
    case @stage
    when :main
      update_main
    when :key
      update_key
    end
  end

  def update_main
    if Input.trigger?(:B)
      Sound.play_cancel
      $scene = Scene_Map.new
      return @stage = nil
    elsif Input.trigger?(:Left)
      update_cursor(-1)
      return
    elsif Input.trigger?(:Right)
      update_cursor(1)
      return
    elsif Input.trigger?(:Delete)
      Sound.play_buzzer
      bind = @binds[@index]
      bind.value = 0
      @key_names[@index] = ""
      b = @bind_names[@index].bitmap
      b.clear
      b.draw_text(b.rect, "", 1)
      @changes[@index] = true
      return
    elsif Input.trigger?(:C)
      @vx_ace ? Sound.play_ok : Sound.play_decision
      Input.text_input = 2
      Input.update
      @bind = @binds[@index]
      @help_bit.clear
      @help_bit.draw_text(@help_bit.rect, ENTER_KEY, 1)
      @bind_bit = @bind_names[@index].bitmap
      @bind_bit.clear
      @stage = :key
    end
  end

  def update_cursor(n)
    Sound.play_cursor
    @index = (@index + n) % @key_max
    s = @bind_names[@index]
    @cursor.set_xy(s.x - 4, s.y - 2)
  end

  def reset_help
    Input.text_input = 0
    Input.update
    @bind_bit = nil
    @help_bit.clear
    @help_bit.draw_text(@help_bit.rect, CHOOSE_KEY, 1)
    @stage = :main
  end

  def update_key
    if Input.trigger?(:MouseRight)
      Sound.play_cancel
      name = @key_names[@index]
      @bind_bit.draw_text(@bind_bit.rect, name, 1)
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
      name = @bind.name
      @key_names[@index] = name
      @bind_bit.draw_text(@bind_bit.rect, name, 1)
      reset_help
      @changes[@index] = true
    end
  rescue => e
    puts e.full_message
  end
end

end
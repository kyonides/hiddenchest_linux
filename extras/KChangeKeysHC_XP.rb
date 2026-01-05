# * KChangeKeys HC for XP * #
#   Scripter : Kyonides
#   2026-01-05

# * Script Call * #
# $scene = KChangeKeys::Scene.new

module KChangeKeys
  HEADING = "Key Bindings Menu"
  CHOOSE_KEY = "Please select a key to edit"
  ENTER_KEY = "Please enter a key now"
  TARGETS = %w{Up Down L L2 Left Right R R2 A B C X Y Z}

class Scene
  def main
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
    b = Bitmap.new(Graphics.width, 40)
    b.font.size = 28
    b.draw_text(b.rect, HEADING, 1)
    @heading = Sprite.new
    @heading.bitmap = b
    @help_bit = Bitmap.new(Graphics.width, 32)
    @help_bit.font.size = 24
    @help_bit.draw_text(@help_bit.rect, CHOOSE_KEY, 1)
    @help = Sprite.new
    @help.y = 44
    @help.bitmap = @help_bit
    @cursor_bit = Bitmap.new(120, 32)
    @cursor_bit.font.size = 24
    @cursor_bit.draw_text(@cursor_bit.rect, "Index: #{@index}")
    @cursor = Sprite.new
    @cursor.set_xy(8, 44)
    @cursor.bitmap = @cursor_bit
    @key_max = TARGETS.size
    @key_max.times do |n|
      s = Sprite.new
      s.set_xy(12 + n % 3 * 206, 92 + n / 3 * 44)
      s.bitmap = Bitmap.new(80, 28)
      s.bitmap.font.color = @target_color
      s.bitmap.draw_text(s.bitmap.rect, TARGETS[n], 1)
      @target_names << s
      s = Sprite.new
      s.set_xy(92 + n % 3 * 206, 92 + n / 3 * 44)
      s.bitmap = Bitmap.new(108, 28)
      s.bitmap.draw_text(s.bitmap.rect, @key_names[n], 1)
      @bind_names << s
    end
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    @bind_names << @heading << @help << @cursor
    list = @target_names + @bind_names
    list.each do |s|
      s.bitmap.dispose
      s.dispose
    end
    @changes.compact!
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

  def update_main
    if Input.trigger?(:B)
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Map.new
      return @stage = nil
    elsif Input.trigger?(:Left)
      update_cursor(-1)
      return
    elsif Input.trigger?(:Right)
      update_cursor(1)
      return
    elsif Input.trigger?(:Delete)
      $game_system.se_play($data_system.buzzer_se)
      bind = @binds[@index]
      bind.value = 0
      @key_names[@index] = ""
      b = @bind_names[@index].bitmap
      b.clear
      b.draw_text(b.rect, "", 1)
      @changes[@index] = true
      return
    elsif Input.trigger?(:C)
      $game_system.se_play($data_system.decision_se)
      Input.text_input = 2
      Input.update
      @bind = @binds[@index]
      @help_bit.clear
      @help_bit.draw_text(@help_bit.rect, ENTER_KEY, 1)
      @bind_bit = @bind_names[@index].bitmap
      @bind_bit.clear
      @timer = Graphics.frame_rate / 2
      @stage = :key
    end
  end

  def update_cursor(n)
    $game_system.se_play($data_system.cursor_se)
    @index = (@index + n) % @key_max
    @cursor_bit.clear
    @cursor_bit.draw_text(@cursor_bit.rect, "Index: #{@index}")
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
    if @timer > 0
      @timer -= 1
      return
    end
    if Input.trigger?(:MouseRight)
      $game_system.se_play($data_system.cancel_se)
      name = @key_names[@index]
      @bind_bit.draw_text(@bind_bit.rect, name, 1)
      reset_help
      return
    elsif Input.trigger_any?
      $game_system.se_play($data_system.decision_se)
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
    puts Input.trigger_last, name
    puts e.full_message
  end
end

end
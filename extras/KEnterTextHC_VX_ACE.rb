# * KEnterText HC for VX + ACE * #
#   Scripter : Kyonides
#   2025-12-18

# For VX:
# $scene = KEnter::TextScene.new

# For VX ACE:
# SceneManager.call(KEnter::TextScene)

module KEnter

class TextBox
  REGEX_INT = /\-?[0-9]/
  REGEX_DBL = /\-?[0-9]+[\.,]?[0-9]?/
  def initialize(bw, bh, color)
    @index = 0
    @char_limit = 1
    @number_mode = nil
    @viewport = Viewport.new(0, 0, bw, bh)
    bitmap = Bitmap.new(bw, bh)
    bitmap.fill_rect(bitmap.rect, color)
    @box = Sprite.new(@viewport)
    @box.bitmap = bitmap
    @bitmap = Bitmap.new(bw - 8, bh - 4)
    @bitmap.font.size = 24
    @bitmap.font.outline = true
    @rect = @bitmap.rect
    @text = Sprite.new(@viewport)
    @text.bitmap = @bitmap
    bitmap = Bitmap.new(20, 28)
    bitmap.font.size = 24
    bitmap.font.outline = true
    bitmap.draw_text(bitmap.rect, "|")
    @cursor = Sprite.new(@viewport)
    @cursor.visible = false
    @cursor.bitmap = bitmap
    @sprites = [@text, @box, @cursor]
  end

  def move2(bx, by, n, lh)
    @viewport.x = bx
    @viewport.y = by + n * lh
    @text.set_xy(4, 2)
    @cursor.set_xy(8, 2)
  end

  def reset_cursor
    @index = @chars.size - 1
    set_cursor(@chars.join)
  end

  def chars=(ary)
    @chars = ary
    @index = ary.size - 1
    refresh
    set_cursor(@chars.join)
  end

  def move_back
    @index -= 1 if @index > 0
    text = @chars[0, @index] || [""]
    set_cursor(text.join)
  end

  def move_forward
    @index += 1 if @index < @chars.size - 1
    text = @chars[0, @index] || [""]
    set_cursor(text.join)
  end

  def process_char
    char = Input.last_char
    return unless char
    case @number_mode
    when :int
      char = char[REGEX_INT]
    when :dbl
      char = char[REGEX_DBL]
    else
      if Input.press?(Input::SHIFT)
        char = Input.capslock_state ? char.downcase : char.upcase
      end
    end
    return unless char
    @index += 1
    if @chars.size == @index
      @chars << char
    else
      @chars[@index, 0] = char
    end
    @chars.compact!
    text = @chars[0, @index]
    refresh
    set_cursor(text.join)
  end

  def delete_prev_char
    if @chars.size - 1 == @index
      @chars.pop
      @index = [@chars.size - 1, 0].max
      text = @chars
    else
      @chars.delete_at(@index)
      @index = [@index - 1, 0].max
      text = @chars[0, @index] || [""]
    end
    refresh
    set_cursor(text.join)
  end

  def delete_next_char
    @chars.delete_at(@index + 1)
    refresh
  end

  def refresh
    @bitmap.clear
    @bitmap.draw_text(@rect, @chars.join)
  end

  def set_cursor(text)
    tw = @bitmap.text_width(text)
    @cursor.x = tw + 8
  end

  def char_max?
    @chars.size == @char_limit
  end

  def integer_only!
    @number_mode = :int
  end

  def double_only!
    @number_mode = :dbl
  end

  def include_all!
    @number_mode = nil
  end

  def dispose
    @sprites.each do |sprite|
      sprite.bitmap.dispose
      sprite.dispose
    end
    @sprites.clear
    @viewport.dispose
  end
  attr_writer :char_limit
  attr_reader :chars, :index, :cursor
end

class TextScene
  CHAR_LIMIT = 18
  BACKDROP = "catacombs"
  LABELS = ["Your Name", "Your Nickname", "Your Age"]
  @@texts = Array.new(3) {[]}
  def main
    @vx_ace = Game::RGSS_VERSION == 3
    @stage = :main
    @blink_timer = Graphics.frame_rate / 5
    @blink_state = false
    @index = 0
    @text_sprites = []
    black = Color.new(0, 0, 0)
    @blue = Color.new(80, 80, 255)
    custom = Color.new(255, 255, 40)
    lbit = Bitmap.new(208, 180)
    lbit.font.outline = true
    3.times {|n| lbit.draw_text(16, 78 * n, 220, 24, LABELS[n]) }
    @backdrop = Sprite.new
    @backdrop.bitmap = Cache.system(BACKDROP)
    @label_sprite = Sprite.new
    @label_sprite.set_xy(16, 12)
    @label_sprite.bitmap = lbit
    make_box_text_sprites
    cb = Bitmap.new(208, 4)
    @main_cursor = Sprite.new
    @main_cursor.set_xy(20, 38)
    @main_cursor.bitmap = cb
    cb.fill_rect(cb.rect, black)
    cb.fill_rect(2, 1, 204, 2, custom)
    @text_cursor = @text_sprites[0].cursor
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    cb.dispose
    @main_cursor.dispose
    @text_sprites.each_with_index do |box, n|
      box.dispose
      @@texts[n] = box.chars
    end
    @label_sprite.dispose
    @backdrop.bitmap.dispose
    @backdrop.dispose
  end

  def make_box_text_sprites
    3.times do |n|
      textbox = TextBox.new(n < 2 ? 228 : 116, 32, @blue)
      textbox.move2(16, 48, n, 78)
      textbox.chars = @@texts[n]
      textbox.char_limit = n < 2 ? CHAR_LIMIT : NUM_LIMIT
      textbox.integer_only! if n == 2
      @text_sprites << textbox
    end
  end

  def update
    case @stage
    when :main
      update_index
    when :text_input
      update_text_input
    end
  end

  def update_blink
    @blink_timer -= 1
    if @blink_timer == 0
      @blink_timer = Graphics.frame_rate / 5
      @blink_state = !@blink_state
      @text_cursor.visible = @blink_state
    end
  end

  def update_index
    if Input.trigger?(Input::Escape)
      Sound.play_cancel
      $scene = Scene_Map.new
      return @sprite = @stage = nil
    elsif Input.trigger?(Input::UP)
      set_index(-1)
      return
    elsif Input.trigger?(Input::DOWN)
      set_index(1)
      return
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      @vx_ace ? Sound.play_ok : Sound.play_decision
      Input.text_input = 1
      Input.update
      @stage = :text_input
      @sprite = @text_sprites[@index]
      @blink_timer = Graphics.frame_rate / 5
      @text_cursor.visible = @blink_state = true
    end
  end

  def set_index(n)
    Sound.play_cursor
    @index = (@index + n) % 3
    @main_cursor.y = 38 + @index * 78
    @text_cursor = @text_sprites[@index].cursor
  end

  def update_text_input
    update_blink
    if Input.trigger?(Input::Left)
      $game_system.se_play($data_system.cursor_se)
      @sprite.move_back
      return
    elsif Input.trigger?(Input::Right)
      $game_system.se_play($data_system.cursor_se)
      @sprite.move_forward
      return
    elsif Input.trigger?(Input::Escape)
      Sound.play_cancel
      process_text_box_cancel
      return
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      @vx_ace ? Sound.play_ok : Sound.play_decision
      Graphics.screenshot
      process_text_box_cancel
      return
    elsif Input.repeat?(Input::Backspace)
      Sound.play_buzzer
      @sprite.delete_char
      return
    elsif Input.repeat?(Input::Delete)
      $game_system.se_play($data_system.buzzer_se)
      @sprite.delete_next_char
      return
    elsif Input.trigger_any? and Input.last_key?
      if @sprite.char_max?
        Sound.play_buzzer
        return
      end
      Sound.play_equip
      @sprite.process_char
    end
  end

  def process_text_box_cancel
    Input.text_input = 0
    Input.clear_text_input
    Input.update
    @sprite.reset_cursor
    @text_cursor.visible = @blink_state = false
    @blink_timer = Graphics.frame_rate / 5
    @stage = :main
  end
end

end
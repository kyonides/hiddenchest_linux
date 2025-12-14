# * KEnterText HC for VX + ACE * #
#   Scripter : Kyonides
#   2025-12-13

# For VX:
# $scene = KEnter::TextScene.new

# For VX ACE:
# SceneManager.call(KEnter::TextScene)

module KEnter

class TextBox
  def initialize(bw, bh, color)
    @char_limit = 1
    bitmap = Bitmap.new(bw, bh)
    bitmap.fill_rect(bitmap.rect, color)
    @box = Sprite.new
    @box.bitmap = bitmap
    bitmap = Bitmap.new(bw - 8, bh - 4)
    bitmap.font.size = 24
    bitmap.font.outline = true
    @text = Sprite.new
    @text.bitmap = bitmap
    @sprites = [@text, @box]
  end

  def move2(bx, by, n, lh)
    @box.set_xy(bx, by + n * lh)
    @text.set_xy(bx + 4, by + 2 + n * lh)
  end

  def chars=(ary)
    @chars = ary
  end

  def process_char
    char = Input.last_char
    if Input.press?(Input::SHIFT)
      char = Input.capslock_state ? char.downcase : char.upcase
    end
    @chars << char
    @chars.compact!
    refresh
  end

  def delete_char
    @chars.pop
    refresh
  end

  def refresh
    @text.bitmap.clear
    @text.bitmap.draw_text(@text.bitmap.rect, @chars.join)
  end

  def bitmap
    @text.bitmap
  end

  def char_max?
    @chars.size == @char_limit
  end

  def dispose
    @sprites.each do |sprite|
      sprite.bitmap.dispose
      sprite.dispose
    end
    @sprites.clear
  end
  attr_writer :char_limit
  attr_reader :chars
end

class TextScene
  CHAR_LIMIT = 18
  BACKDROP = "catacombs"
  LABELS = ["Your Name", "Your Nickname"]
  @@texts = Array.new(2) {[]}
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
    lbit = Bitmap.new(208, 160)
    lbit.font.outline = true
    lbit.draw_text(16, 0, 208, 24, LABELS[0])
    lbit.draw_text(16, 78, 208, 24, LABELS[1])
    @backdrop = Sprite.new
    @backdrop.bitmap = Cache.system(BACKDROP)
    @label_sprite = Sprite.new
    @label_sprite.set_xy(16, 12)
    @label_sprite.bitmap = lbit
    make_box_text_sprites
    cb = Bitmap.new(200, 4)
    @main_cursor = Sprite.new
    @main_cursor.set_xy(20, 38)
    @main_cursor.bitmap = cb
    cb.fill_rect(cb.rect, black)
    cb.fill_rect(2, 1, 196, 2, custom)
    cbit = Bitmap.new(20, 28)
    cbit.font.size = 24
    cbit.font.outline = true
    cbit.draw_text(cbit.rect, "|")
    @text_cursor = Sprite.new
    @text_cursor.set_xy(22, 50)
    @text_cursor.visible = @blink_state
    @text_cursor.bitmap = cbit
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    cbit.dispose
    cb.dispose
    @text_cursor.dispose
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
    2.times do |n|
      textbox = TextBox.new(208, 32, @blue)
      textbox.move2(16, 48, n, 78)
      textbox.chars = @@texts[n]
      textbox.char_limit = CHAR_LIMIT
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

  def process_text_box_cancel
    Input.text_input = false
    Input.clear_text_input
    Input.update
    @text_cursor.visible = @blink_state = false
    @blink_timer = Graphics.frame_rate / 5
    @stage = :main
  end

  def update_index
    if Input.trigger?(Input::Escape)
      Sound.play_cancel
      $scene = Scene_Map.new
      return @stage = nil
    elsif Input.trigger?(Input::UP)
      set_index(-1)
      return
    elsif Input.trigger?(Input::DOWN)
      set_index(1)
      return
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      @vx_ace ? Sound.play_ok : Sound.play_decision
      Input.text_input = true
      Input.update
      @stage = :text_input
      @sprite = @text_sprites[@index]
      @bitmap = @sprite.bitmap
      text = @sprite.chars.join
      tw = @bitmap.text_width(text)
      @text_cursor.x = 22 + tw
      @text_cursor.visible = @blink_state = true
    end
  end

  def set_index(n)
    Sound.play_cursor
    @index = (@index + n) % 2
    @main_cursor.y = 38 + @index * 78
    @text_cursor.y = 50 + @index * 78
  end

  def update_text_input
    update_blink
    if Input.trigger?(Input::Escape)
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
      refresh_text(@sprite.chars.join)
      return
    elsif Input.trigger_any? and Input.last_key?
      if @sprite.char_max?
        Sound.play_buzzer
        return
      end
      Sound.play_equip
      @sprite.process_char
      refresh_text(@sprite.chars.join)
    end
  end

  def refresh_text(text)
    tw = @bitmap.text_width(text)
    @text_cursor.x = 22 + tw
  end
end

end
# * KEnterText HC for VX + ACE * #
#   Scripter : Kyonides
#   2025-12-11

class KEnterText
  CHAR_LIMIT = 18
  BACKDROP = "catacombs"
  LABELS = ["Your Name", "Your Nickname"]
  def main
    @vx_ace = Game::RGSS_VERSION == 3
    @stage = :main
    @blink_timer = Graphics.frame_rate / 5
    @blink_state = false
    @index = 0
    @name_chars = []
    @nick_chars = []
    @box_sprites = []
    @text_sprites = []
    black = Color.new(0, 0, 0)
    blue = Color.new(80, 80, 255)
    custom = Color.new(255, 255, 40)
    lbit = Bitmap.new(208, 160)
    lbit.font.outline = true
    lbit.draw_text(16, 0, 208, 24, LABELS[0])
    lbit.draw_text(16, 78, 208, 24, LABELS[1])
    @backdrop = Sprite.new
    @backdrop.bitmap = RPG::Cache.title(BACKDROP)
    @label_sprite = Sprite.new
    @label_sprite.set_xy(16, 12)
    @label_sprite.bitmap = lbit
    bbit = Bitmap.new(208, 32)
    bbit.fill_rect(bbit.rect, blue)
    make_box_text_sprites(bbit)
    bbit.dispose
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
    @name_bitmap = @text_sprites[0].bitmap
    @nick_bitmap = @text_sprites[1].bitmap
    @texts = { name: @name_chars, nick: @nick_chars }
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
    list = @box_sprites + @text_sprites
    list.each do |sprite|
      sprite.bitmap.dispose
      sprite.dispose
    end
    @label_sprite.dispose
    @backdrop.bitmap.dispose
    @backdrop.dispose
  end

  def make_box_text_sprites(bit)
    2.times do |n|
      sprite = Sprite.new
      sprite.set_xy(16, 48 + n * 78)
      sprite.bitmap = bit.dup
      @box_sprites << sprite
      bitmap = Bitmap.new(200, 28)
      bitmap.font.size = 24
      bitmap.font.outline = true
      sprite = Sprite.new
      sprite.set_xy(20, 50 + n * 78)
      sprite.bitmap = bitmap
      @text_sprites << sprite
    end
  end

  def update
    case @stage
    when :main
      update_index
    when :name
      update_name
    when :nick
      update_nickname
    end
  end

  def update_blink
    @blink_timer -= 1
    if @blink_timer == 0
      @blink_timer = Graphics.frame_rate / 5
      @blink_state = !@blink_state
      @cursor_sprite.visible = @blink_state
    end
  end

  def process_text_box_cancel
    Input.text_input = false
    Input.clear_text_input
    Input.update
    @cursor_sprite.visible = @blink_state = false
    @blink_timer = Graphics.frame_rate / 5
    @stage = :main
  end

  def process_char(bitmap, ary)
    char = Input.last_char
    if Input.press?(Input::SHIFT)
      char = Input.capslock_state ? char.downcase : char.upcase
    end
    ary << char
    ary.compact!
    refresh_text(bitmap, ary.join)
  end

  def refresh_text(bitmap, text)
    tw = bitmap.text_width(text)
    @cursor_sprite.x = 22 + tw
    bitmap.clear
    bitmap.draw_text(bitmap.rect, text)
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
      @stage = set_stage
      text = @texts[@stage].join
      tw = @name_bitmap.text_width(text)
      @cursor_sprite.x = 22 + tw
      @cursor_sprite.visible = @blink_state = true
    end
  end

  def set_index(n)
    Sound.play_cursor
    @index = (@index + n) % 2
    @main_cursor.y = 38 + @index * 78
    @text_cursor.y = 50 + @index * 78
  end

  def set_stage
    case @index
    when 0
      :name
    when 1
      :nick
    end
  end

  def update_name
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
      @name_chars.pop
      refresh_text(@name_bitmap, @name_chars.join)
      return
    elsif Input.trigger_any? and Input.last_key?
      if @name_chars.size == CHAR_LIMIT
        Sound.play_buzzer
        return
      end
      Sound.play_equip
      process_char(@name_bitmap, @name_chars)
    end
  end

  def update_nickname
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
      @nick_chars.pop
      refresh_text(@nick_bitmap, @nick_chars.join)
      return
    elsif Input.trigger_any? and Input.last_key?
      if @nick_chars.size == CHAR_LIMIT
        Sound.play_buzzer
        return
      end
      Sound.play_equip
      process_char(@nick_bitmap, @nick_chars)
    end
  end
end
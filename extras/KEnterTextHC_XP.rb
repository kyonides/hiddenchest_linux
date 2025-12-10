# * KEnterText HC for XP * #
#   Scripter : Kyonides
#   2025-12-09

class KEnterText
  def main
    @stage = :main
    @blink_timer = Graphics.frame_rate / 5
    @blink_state = false
    @index = 0
    @name_chars = []
    @nick_chars = []
    @box_sprites = []
    lbit = Bitmap.new(208, 160)
    lbit.draw_text(16, 0, 208, 24, "Your Name")
    lbit.draw_text(16, 78, 208, 24, "Your Nickname")
    @label_sprite = Sprite.new
    @label_sprite.set_xy(16, 12)
    @label_sprite.bitmap = lbit
    blue = Color.new(80, 80, 255)
    bbit = Bitmap.new(208, 32)
    bbit.fill_rect(bbit.rect, blue)
    2.times do |n|
      sprite = Sprite.new
      sprite.set_xy(16, 48 + n * 76)
      sprite.bitmap = bbit.dup
      @box_sprites << sprite
    end
    bbit.dispose
    cbit = Bitmap.new(20, 28)
    cbit.font.size = 24
    cbit.font.outline = true
    cbit.draw_text(cbit.rect, "|")
    @cursor_sprite = Sprite.new
    @cursor_sprite.set_xy(22, 50)
    @cursor_sprite.bitmap = cbit
    @name_bitmap = Bitmap.new(200, 28)
    @name_bitmap.font.size = 24
    @name_bitmap.font.outline = true
    @name_sprite = Sprite.new
    @name_sprite.set_xy(20, 50)
    @name_sprite.bitmap = @name_bitmap
    @nick_bitmap = Bitmap.new(200, 28)
    @nick_bitmap.font.size = 24
    @nick_bitmap.font.outline = true
    @nick_sprite = Sprite.new
    @nick_sprite.set_xy(20, 124)
    @nick_sprite.bitmap = @nick_bitmap
    @texts = { name: @name_chars, nick: @nick_chars }
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    @nick_bitmap.dispose
    @nick_sprite.dispose
    @name_bitmap.dispose
    @name_sprite.dispose
    cbit.dispose
    @cursor_sprite.dispose
    @box_sprites.each do |sprite|
      sprite.bitmap.dispose
      sprite.dispose
    end
    @label_sprite.dispose
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
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Map.new
      return @stage = nil
    elsif Input.trigger?(Input::UP)
      $game_system.se_play($data_system.cursor_se)
      @index = (@index - 1) % 2
      @cursor_sprite.y = 50 + @index * 76
      return
    elsif Input.trigger?(Input::DOWN)
      $game_system.se_play($data_system.cursor_se)
      @index = (@index + 1) % 2
      @cursor_sprite.y = 50 + @index * 76
      return
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      $game_system.se_play($data_system.decision_se)
      Input.text_input = true
      Input.update
      @stage = set_stage
      text = @texts[@stage].join
      tw = @name_bitmap.text_width(text)
      @cursor_sprite.x = 22 + tw
      @cursor_sprite.visible = @blink_state = true
    end
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
      $game_system.se_play($data_system.cancel_se)
      process_text_box_cancel
      return
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      $game_system.se_play($data_system.decision_se)
      Graphics.screenshot
      process_text_box_cancel
      return
    elsif Input.repeat?(Input::Backspace)
      $game_system.se_play($data_system.buzzer_se)
      @name_chars.pop
      refresh_text(@name_bitmap, @name_chars.join)
      return
    elsif Input.trigger_any? and Input.last_key?
      $game_system.se_play($data_system.equip_se)
      process_char(@name_bitmap, @name_chars)
    end
  end

  def update_nickname
    update_blink
    if Input.trigger?(Input::Escape)
      $game_system.se_play($data_system.cancel_se)
      process_text_box_cancel
      return
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      $game_system.se_play($data_system.decision_se)
      Graphics.screenshot
      process_text_box_cancel
      return
    elsif Input.repeat?(Input::Backspace)
      $game_system.se_play($data_system.buzzer_se)
      @nick_chars.pop
      refresh_text(@nick_bitmap, @nick_chars.join)
      return
    elsif Input.trigger_any? and Input.last_key?
      $game_system.se_play($data_system.equip_se)
      process_char(@nick_bitmap, @nick_chars)
    end
  end
end
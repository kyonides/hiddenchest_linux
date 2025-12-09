# * KEnterText HC for XP * #
#   Scripter : Kyonides
#   2025-12-09

class KEnterText
  def main
    @run = true
    @blink_timer = Graphics.frame_rate / 5
    @blink_state = true
    @chars = []
    lbit = Bitmap.new(208, 32)
    lbit.draw_text(lbit.rect, "Your name")
    @label_sprite = Sprite.new
    @label_sprite.set_xy(16, 12)
    @label_sprite.bitmap = lbit
    blue = Color.new(80, 80, 255)
    bbit = Bitmap.new(208, 32)
    bbit.fill_rect(bbit.rect, blue)
    @box_sprite = Sprite.new
    @box_sprite.set_xy(16, 48)
    @box_sprite.bitmap = bbit
    cbit = Bitmap.new(20, 28)
    cbit.font.size = 24
    cbit.font.outline = true
    cbit.draw_text(cbit.rect, "|")
    @cursor_sprite = Sprite.new
    @cursor_sprite.set_xy(22, 50)
    @cursor_sprite.bitmap = cbit
    @text_bitmap = Bitmap.new(200, 28)
    @text_bitmap.font.size = 24
    @text_bitmap.font.outline = true
    @text_sprite = Sprite.new
    @text_sprite.set_xy(20, 50)
    @text_sprite.bitmap = @text_bitmap
    Input.text_input = true
    Graphics.transition
    while @run
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    @text_bitmap.dispose
    @text_sprite.dispose
    cbit.dispose
    @cursor_sprite.dispose
    bbit.dispose
    @box_sprite.dispose
    lbit.dispose
    @label_sprite.dispose
  end

  def update
    @blink_timer -= 1
    if @blink_timer == 0
      @blink_timer = Graphics.frame_rate / 5
      @blink_state = !@blink_state
      @cursor_sprite.visible = @blink_state
    end
    if Input.trigger?(Input::Escape)
      $game_system.se_play($data_system.cancel_se)
      Input.clear_text_input
      $scene = Scene_Map.new
      return @run = nil
    elsif Input.trigger?(Input::Enter) or Input.trigger?(Input::Return)
      Graphics.screenshot
      return
    elsif Input.trigger?(Input::Backspace)
      $game_system.se_play($data_system.buzzer_se)
      @chars.pop
      refresh_text
      return
    elsif Input.trigger?(Input::LeftShift) or Input.trigger?(Input::RightShift)
      return
    elsif Input.last_key?
      $game_system.se_play($data_system.equip_se)
      char = Input.last_char
      @chars << char
      @chars.compact!
      refresh_text
    end
  end

  def refresh_text
    text = @chars.join
    tw = @text_bitmap.text_width(text)
    @cursor_sprite.x = 20 + tw + 2
    @text_bitmap.clear
    @text_bitmap.draw_text(@text_bitmap.rect, text)
  end
end
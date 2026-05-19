# * KuickTimeEvent XP * # 
#   Scripter : Kyonides
#   2026-05-19

class KTEScene
  def main
    @stage = :main
    @help_window = Window_Help.new
    @help_window.set_text("Press Start", 1)
    @bitmap = Bitmap.new(368, 96)
    @result_window = Window_Base.new(120, 128, 400, 128)
    @result_window.contents = @bitmap
    @result_window.visible = false
    Graphics.transition
    while @stage
      Graphics.update
      Input.update
      update
    end
    Graphics.freeze
    dispose
  end

  def update
    case @stage
    when :main
      if Input.trigger?(:C)
        $game_system.se_play($data_system.decision_se)
        @help_window.set_text("Enter the button sequence now", 1)
        Input.set_sequence(:Up, :Down, :Left, :Right)
        @stage = :test
      end
      return
    when :test
      unless Input.store_sequence
        $game_system.se_play($data_system.shop_se)
        @help_window.set_text("The Outcome", 1)
        @stage = :check
        if Input.same_sequence?
          text = "Success!"
        else
          text = "Epic Fail!"
        end
        targets  = "Targets:  " + Input.button_targets.join(", ")
        sequence = "Sequence: " + Input.button_sequence.join(", ")
        rect = @bitmap.rect.dup
        rect.height = 32
        @bitmap.draw_text(rect, text, 1)
        rect.y = 32
        @bitmap.draw_text(rect, targets)
        rect.y = 64
        @bitmap.draw_text(rect, sequence)
        @result_window.visible = true
      end
      return
    when :check
      if Input.trigger_any?
        $game_system.se_play($data_system.cancel_se)
        Input.clear_button_sequence
        $scene = Scene_Map.new
        return @stage = nil
      end
    end
  end

  def dispose
    @result_window.dispose
    @help_window.dispose
  end
end
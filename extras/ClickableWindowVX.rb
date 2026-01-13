# * ClickableWindow VX * #
#   Scripter : Kyonides Arkanthes
#   2026-01-11

# This is a script demo that shows you how it is now possible to click once on
# a menu window to choose an option while ignoring the surrounding area.
# Normally, you would have to add some calls to Input.left_click? or
# Input.right_click? or even Input.middle_click? to your target scenes to make
# this work.

class Window_Selectable
  def one_click_selection
    # Set Index to invisible if clicked outside this window
    self.index = -1
    # Check each area - a list of Rect's
    @area.size.times do |n|
      next unless mouse_inside?(n)
      self.index = n
      Sound.play_cursor
      break
    end
  end

  def cursor_movable?
    return false if !visible or !active
    return false if @item_max == 0 or @opening or @closing
    return true
  end

  def update
    super
    if cursor_movable?
      last_index = @index
      # Added Left Click Check
      if Input.left_click?
        one_click_selection
        return
      end
      if mouse_inside?
        if Mouse.scroll_y?(:UP)
          Sound.play_cursor
          self.index = (@index - @column_max) % @item_max
          return
        elsif Mouse.scroll_y?(:DOWN)
          Sound.play_cursor
          self.index = (@index + @column_max) % @item_max
          return
        end
      end
      if Input.repeat?(Input::DOWN)
        cursor_down(Input.trigger?(Input::DOWN))
      end
      if Input.repeat?(Input::UP)
        cursor_up(Input.trigger?(Input::UP))
      end
      if Input.repeat?(Input::RIGHT)
        cursor_right(Input.trigger?(Input::RIGHT))
      end
      if Input.repeat?(Input::LEFT)
        cursor_left(Input.trigger?(Input::LEFT))
      end
      if Input.repeat?(Input::R)
        cursor_pagedown
      end
      if Input.repeat?(Input::L)
        cursor_pageup
      end
      if @index != last_index
        Sound.play_cursor
      end
    end
    update_cursor
    call_update_help
  end
end

class Window_Command
  def draw_item(n, enabled = true)
    rect = @area[n] ||= item_rect(n)
    rect.x += 4
    rect.width -= 8
    self.contents.clear_rect(rect)
    self.contents.font.color = normal_color
    self.contents.font.color.alpha = enabled ? 255 : 128
    self.contents.draw_text(rect, @commands[n])
  end
end

class Scene_Title
  def update
    super
    @command_window.update
    # Added Left Click Check
    if Input.trigger?(Input::C) or Input.double_left_click?
      case @command_window.index
      when 0    #New game
        command_new_game
      when 1    # Continue
        command_continue
      when 2    # Shutdown
        command_shutdown
      end
    # Added Right Click Check
    elsif Input.right_click?
      $game_system.se_play($data_system.cursor_se)
      if @command_window.index < 2
        @command_window.index = 2
      end
    end
  end
end

class Scene_Map
  def update_call_menu
    if Input.trigger?(Input::B) or Input.double_right_click?
      return if $game_map.interpreter.running?
      return if $game_system.menu_disabled
      $game_temp.menu_beep = true
      $game_temp.next_scene = "menu"
    end
  end
end

class Scene_Menu
  def update_command_selection
    if Input.trigger?(Input::B) or Input.double_right_click?
      Sound.play_cancel
      $scene = Scene_Map.new
    elsif Input.trigger?(Input::C) or Input.double_left_click?
      if $game_party.members.size == 0 and @command_window.index < 4
        Sound.play_buzzer
        return
      elsif $game_system.save_disabled and @command_window.index == 4
        Sound.play_buzzer
        return
      end
      Sound.play_decision
      case @command_window.index
      when 0      # Item
        $scene = Scene_Item.new
      when 1,2,3  # Skill, equipment, status
        start_actor_selection
      when 4      # Save
        $scene = Scene_File.new(true, false, false)
      when 5      # End Game
        $scene = Scene_End.new
      end
    end
  end
end

class Scene_End
  def update
    super
    update_menu_background
    @command_window.update
    if Input.trigger?(Input::B) or Input.double_right_click?
      Sound.play_cancel
      return_scene
    elsif Input.trigger?(Input::C) or Input.double_left_click?
      case @command_window.index
      when 0  # to title
        command_to_title
      when 1  # shutdown
        command_shutdown
      when 2  # quit
        command_cancel
      end
    end
  end
end
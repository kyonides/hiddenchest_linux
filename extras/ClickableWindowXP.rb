# * ClickableWindow XP * #
#   Scripter : Kyonides Arkanthes
#   2024-05-22

# This is a script demo that shows you how it is now possible to click once on
# a menu window to choose an option while ignoring the surrounding area.
# Normally, you would have to add some calls to Input.left_click? or
# Input.right_click? or even Input.middle_click? to your target scenes to make
# this work.

class Window_Selectable
  def initialize(x, y, width, height)
    super(x, y, width, height)
    @item_max = 1
    @column_max = 1
    @index = -1
    # Initialized Area Array that will hold all Rect's
    @area = []
  end

  def update
    super
    return unless self.active
    return if @item == 0
    # Added Left Click Check
    if Input.left_click?
      # Set Index to invisible if clicked outside this window
      self.index = -1
      # Check each area or list of Rect's
      @area.size.times do |n|
        next unless mouse_inside?(n)
        @index = n
        update_help if @help_window 
        update_cursor_rect
        break
      end
      return
    end
    return if @index < 0
    if Input.repeat?(Input::DOWN)
      if (@column_max == 1 and Input.trigger?(Input::DOWN)) or
         @index < @item_max - @column_max
        $game_system.se_play($data_system.cursor_se)
        @index = (@index + @column_max) % @item_max
      end
    end
    if Input.repeat?(Input::UP)
      if (@column_max == 1 and Input.trigger?(Input::UP)) or
         @index >= @column_max
        $game_system.se_play($data_system.cursor_se)
        @index = (@index - @column_max + @item_max) % @item_max
      end
    end
    if Input.repeat?(Input::RIGHT)
      if @column_max >= 2 and @index < @item_max - 1
        $game_system.se_play($data_system.cursor_se)
        @index += 1
      end
    end
    if Input.repeat?(Input::LEFT)
      if @column_max >= 2 and @index > 0
        $game_system.se_play($data_system.cursor_se)
        @index -= 1
      end
    end
    if Input.repeat?(Input::R)
      if self.top_row + (self.page_row_max - 1) < (self.row_max - 1)
        $game_system.se_play($data_system.cursor_se)
        @index = [@index + self.page_item_max, @item_max - 1].min
        self.top_row += self.page_row_max
      end
    end
    if Input.repeat?(Input::L)
      if self.top_row > 0
        $game_system.se_play($data_system.cursor_se)
        @index = [@index - self.page_item_max, 0].max
        self.top_row -= self.page_row_max
      end
    end
    if self.active and @help_window != nil
      update_help
    end
    update_cursor_rect
  end
end

class Window_Command
  def draw_item(n, color)
    c = self.contents
    c.font.color = color
    rect = @area[n] ||= Rect.new(4, 32 * n, c.width - 8, 32)
    c.fill_rect(rect, Color.new(0, 0, 0, 0))
    c.draw_text(rect, @commands[n])
  end
end

class Scene_Title
  def update
    @command_window.update
    # Added Right Click Check
    if Input.right_click?
      $game_system.se_play($data_system.cursor_se)
      if @command_window.index < 2
        @command_window.index = 2
      end
    # Added Left Click Check
    elsif Input.trigger?(Input::C) or Input.left_click?
      case @command_window.index
      when 0  # New game
        command_new_game
      when 1  # Continue
        command_continue
      when 2  # Shutdown
        command_shutdown
      end
    end
  end
end

class Scene_Map
  def update
    loop do
      $game_map.update
      $game_system.map_interpreter.update
      $game_player.update
      $game_system.update
      $game_screen.update
      break unless $game_temp.player_transferring
      transfer_player
      break if $game_temp.transition_processing
    end
    @spriteset.update
    @message_window.update
    if $game_temp.gameover
      $scene = Scene_Gameover.new
      return
    end
    if $game_temp.to_title
      $scene = Scene_Title.new
      return
    end
    if $game_temp.transition_processing
      $game_temp.transition_processing = false
      if $game_temp.transition_name == ""
        Graphics.transition(20)
      else
        Graphics.transition(40, "Graphics/Transitions/" +
          $game_temp.transition_name)
      end
    end
    return if $game_temp.message_window_showing
    if $game_player.encounter_count == 0 and $game_map.encounter_list != []
      unless $game_system.map_interpreter.running? or
             $game_system.encounter_disabled
        n = rand($game_map.encounter_list.size)
        troop_id = $game_map.encounter_list[n]
        if $data_troops[troop_id] != nil
          $game_temp.battle_calling = true
          $game_temp.battle_troop_id = troop_id
          $game_temp.battle_can_escape = true
          $game_temp.battle_can_lose = false
          $game_temp.battle_proc = nil
        end
      end
    end
    # Added Right Click Check
    if Input.trigger?(Input::B) or Input.right_click?
      unless $game_system.map_interpreter.running? or
             $game_system.menu_disabled
        $game_temp.menu_calling = true
        $game_temp.menu_beep = true
      end
    end
    if $DEBUG and Input.press?(Input::F9)
      $game_temp.debug_calling = true
    end
    unless $game_player.moving?
      if $game_temp.battle_calling
        call_battle
      elsif $game_temp.shop_calling
        call_shop
      elsif $game_temp.name_calling
        call_name
      elsif $game_temp.menu_calling
        call_menu
      elsif $game_temp.save_calling
        call_save
      elsif $game_temp.debug_calling
        call_debug
      end
    end
  end
end

class Scene_Menu
  def update_command
    # Added Right Click Check
    if Input.trigger?(Input::B) or Input.right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Map.new
      return
    end
    # Added Left Click Check
    if Input.trigger?(Input::C) or Input.left_click?
      if $game_party.actors.size == 0 and @command_window.index < 4
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      case @command_window.index
      when 0  # item
        $game_system.se_play($data_system.decision_se)
        $scene = Scene_Item.new
      when 1  # skill
        $game_system.se_play($data_system.decision_se)
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
      when 2  # equipment
        $game_system.se_play($data_system.decision_se)
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
      when 3  # status
        $game_system.se_play($data_system.decision_se)
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
      when 4  # save
        if $game_system.save_disabled
          $game_system.se_play($data_system.buzzer_se)
          return
        end
        $game_system.se_play($data_system.decision_se)
        $scene = Scene_Save.new
      when 5  # end game
        $game_system.se_play($data_system.decision_se)
        $scene = Scene_End.new
      end
      return
    end
  end

  def update_status
    # Added Right Click Check
    if Input.trigger?(Input::B) or Input.right_click?
      $game_system.se_play($data_system.cancel_se)
      @command_window.active = true
      @status_window.active = false
      @status_window.index = -1
      return
    end
    # Added Left Click Check
    if Input.trigger?(Input::C) or Input.left_click?
      case @command_window.index
      when 1  # skill
        if $game_party.actors[@status_window.index].restriction >= 2
          $game_system.se_play($data_system.buzzer_se)
          return
        end
        $game_system.se_play($data_system.decision_se)
        $scene = Scene_Skill.new(@status_window.index)
      when 2  # equipment
        $game_system.se_play($data_system.decision_se)
        $scene = Scene_Equip.new(@status_window.index)
      when 3  # status
        $game_system.se_play($data_system.decision_se)
        $scene = Scene_Status.new(@status_window.index)
      end
      return
    end
  end
end

class Scene_End
  def update
    @command_window.update
    # Added Right Click Check
    if Input.trigger?(Input::B) or Input.right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Menu.new(5)
      return
    # Added Left Click Check
    elsif Input.trigger?(Input::C) or Input.left_click?
      case @command_window.index
      when 0  # to title
        command_to_title
      when 1  # shutdown
        command_shutdown
      when 2  # quit
        command_cancel
      end
      return
    end
  end
end
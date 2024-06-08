# * ClickableWidget XP * #
#   Scripter : Kyonides Arkanthes
#   2024-06-08

# This is a script demo that shows you how it is now possible to click once on
# a menu window to choose an option while ignoring the surrounding area.
# Normally, you would have to add some calls to Input.left_click? or
# Input.right_click? or even Input.middle_click? to your target scenes to make
# this work.

class Sprite
  alias :kyon_click_sprite_sprite_up :update
  def update
    kyon_click_sprite_sprite_up
    # Added Draggable Window Check
    check_draggable
    check_mouse_inside
  end

  def check_draggable
    return unless draggable?
    if Input.press_left_click?
      if Mouse.no_target?
        if mouse_inside_color? and drag_color?
          Mouse.target = self
          @mouse_x = Mouse.x
          @mouse_y = Mouse.y
        end
      elsif Mouse.target?(self)
        self.x += Mouse.x - @mouse_x
        self.y += Mouse.y - @mouse_y
        @mouse_x = Mouse.x
        @mouse_y = Mouse.y
      end
      return
    elsif Mouse.target?(self)
      Mouse.target = nil
    end
  end

  def hover_can_change?
    return false if @hover_changed
    return false if @base_name.nil? or @base_name.empty?
    return false if @hover_name.nil? or @hover_name.empty?
    @hover_filetype != nil
  end

  def check_mouse_inside
    return if Mouse.target?(self)
    if mouse_inside?
      if hover_can_change?
        set_hover_bitmap
        @hover_changed = true
        return
      end
    elsif @hover_changed
      self.bitmap.dispose
      set_base_bitmap
      @hover_changed = false
    end
  end

  def set_hover_bitmap
    filetype = @hover_filetype.to_sym
    case filetype
    when :battler 
      self.bitmap = RPG::Cache.battler(@hover_name, @hover_hue)
    when :character
      self.bitmap = RPG::Cache.character(@hover_name, @hover_hue)
    when :icon
      self.bitmap = RPG::Cache.icon(@hover_name)
    when :picture
      self.bitmap = RPG::Cache.picture(@hover_name)
    end
  end

  def set_base_bitmap
    filetype = @hover_filetype.to_sym
    case filetype
    when :battler 
      self.bitmap = RPG::Cache.battler(@base_name, @hover_hue)
    when :character
      self.bitmap = RPG::Cache.character(@base_name, @hover_hue)
    when :icon
      self.bitmap = RPG::Cache.icon(@base_name)
    when :picture
      self.bitmap = RPG::Cache.picture(@base_name)
    end
  end
end

class Window_Base
  def update
    super
    if $game_system.windowskin_name != @windowskin_name
      @windowskin_name = $game_system.windowskin_name
      self.windowskin = RPG::Cache.windowskin(@windowskin_name)
    end
    # Added Draggable Window Check
    check_draggable
  end

  def check_draggable
    return unless draggable?
    if Input.press_left_click?
      if Mouse.no_target? and mouse_inside?
        Mouse.target = self
        @mouse_x = Mouse.x
        @mouse_y = Mouse.y
      elsif Mouse.target?(self)
        self.x += Mouse.x - @mouse_x
        self.y += Mouse.y - @mouse_y
        @mouse_x = Mouse.x
        @mouse_y = Mouse.y
      end
      return
    elsif Mouse.target?(self)
      Mouse.target = nil
    end
  end
end

class Window_Selectable
  def one_click_selection
    # Set Index to invisible if clicked outside this window
    self.index = -1
    # Check each area - a list of Rect's
    @area.size.times do |n|
      next unless mouse_inside?(n)
      self.index = n
      break
    end
  end

  def update
    super
    return unless self.active
    return if @item_max == 0
    # Added Left Click Check
    if Input.left_click?
      one_click_selection
      return
    end
    if mouse_inside?
      if Mouse.scroll_y?(:UP)
        $game_system.se_play($data_system.cursor_se)
        self.index = (@index - @column_max) % @item_max
        return
      elsif Mouse.scroll_y?(:DOWN)
        $game_system.se_play($data_system.cursor_se)
        self.index = (@index + @column_max) % @item_max
        return
      end
    end
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
      update_help if @index >= 0
    end
    update_cursor_rect
  end
 
  def get_area_rect(n)
    rw = self.width / @column_max - 32
    rx = 8 + n % @column_max * (rw + 32)
    ry = n / @column_max * 32 - self.oy
    Rect.new(rx, ry, rw, 32)
  end
end

class Window_Command
  def draw_item(n, color)
    @area[n] ||= get_area_rect(n)
    c = self.contents
    c.font.color = color
    rect = Rect.new(4, 32 * n, c.width - 8, 32)
    c.fill_rect(rect, Color.new(0, 0, 0, 0))
    c.draw_text(rect, @commands[n])
  end
end

class Window_MenuStatus
  def refresh
    self.contents.clear
    @item_max = $game_party.actors.size
    for i in 0...$game_party.actors.size
      x = 64
      y = i * 116
      @area[i] ||= Rect.new(0, i * 116, self.width - 32, 96)
      actor = $game_party.actors[i]
      draw_actor_graphic(actor, x - 40, y + 80)
      draw_actor_name(actor, x, y)
      draw_actor_class(actor, x + 144, y)
      draw_actor_level(actor, x, y + 32)
      draw_actor_state(actor, x + 90, y + 32)
      draw_actor_exp(actor, x, y + 64)
      draw_actor_hp(actor, x + 236, y + 32)
      draw_actor_sp(actor, x + 236, y + 64)
    end
  end
end

class Window_Target
  def refresh
    self.contents.clear
    for i in 0...$game_party.actors.size
      x = 4
      y = i * 116
      @area[i] ||= Rect.new(0, i * 116, self.width - 32, 96)
      actor = $game_party.actors[i]
      draw_actor_name(actor, x, y)
      draw_actor_class(actor, x + 144, y)
      draw_actor_level(actor, x + 8, y + 32)
      draw_actor_state(actor, x + 8, y + 64)
      draw_actor_hp(actor, x + 152, y + 32)
      draw_actor_sp(actor, x + 152, y + 64)
    end
  end
end

class Window_EquipRight
  def initialize(actor)
    super(272, 64, 368, 192)
    @click_width = width - 32
    self.contents = Bitmap.new(@click_width, height - 32)
    @actor = actor
    create_areas
    refresh
    self.index = 0
  end

  def create_areas
    5.times {|n| @area << Rect.new(4, 32 * n, @click_width, 32) }
  end
end

class Window_SaveFile
  alias :kyon_click_win_win_svfl_init :initialize
  def initialize(file_index, filename)
    kyon_click_win_win_svfl_init(file_index, filename)
    @area << self.contents.rect
  end
end

class Window_Message
  alias :kyon_click_win_win_mess_init :initialize
  def initialize
    kyon_click_win_win_mess_init
    self.pause_x = self.width - 80
  end
end

class Scene_Title
  LOGO = "icon_sound"
  def load_database
    $data_actors        = load_data("Data/Actors.rxdata")
    $data_classes       = load_data("Data/Classes.rxdata")
    $data_skills        = load_data("Data/Skills.rxdata")
    $data_items         = load_data("Data/Items.rxdata")
    $data_weapons       = load_data("Data/Weapons.rxdata")
    $data_armors        = load_data("Data/Armors.rxdata")
    $data_enemies       = load_data("Data/Enemies.rxdata")
    $data_troops        = load_data("Data/Troops.rxdata")
    $data_states        = load_data("Data/States.rxdata")
    $data_animations    = load_data("Data/Animations.rxdata")
    $data_tilesets      = load_data("Data/Tilesets.rxdata")
    $data_common_events = load_data("Data/CommonEvents.rxdata")
    $data_system        = load_data("Data/System.rxdata")
  end

  def reset_game_system
    $game_system = Game_System.new
  end

  def start
    create_backdrop
    create_command_window
    create_title
    create_extras
    check_continue
    setup_audio
  end

  def create_backdrop
    @sprite = Sprite.new
    @sprite.bitmap = RPG::Cache.title($data_system.title_name)
  end

  def create_command_window
    s1 = "New Game"
    s2 = "Continue"
    s3 = "Shutdown"
    @command_window = Window_Command.new(192, [s1, s2, s3])
    @command_window.back_opacity = 160
    @command_window.x = 320 - @command_window.width / 2
    @command_window.y = 288
    @command_window.draggable = true
  end

  def create_title
    @title = Sprite.new
    @title.set_xyz(0, 60, 100)
    @title.bitmap = b = Bitmap.new(Graphics.width, 60)
    font = b.font
    font.size = 52
    font.outline = true
    font.outline_size = 4
    b.draw_text(b.rect, Game::TITLE, 1)
  end

  def create_extras
    @logo = Sprite.new
    @logo.set_xyz(24, 24, 100)
    @logo.drag_condition = :color
    @logo.base_name = LOGO
    @logo.hover_name = LOGO + "2"
    @logo.hover_filetype = :picture
    @logo.set_base_bitmap
    @day_index = 0
    @days = %w{Sunday Monday Tuesday Wednesday Thursday Friday Saturday}
    # Make day of the week graphic
    @block = Sprite.new
    @block.set_xyz(12, Graphics.height - 40, 100)
    @bitmap = Bitmap.new(200, 36)
    font = @bitmap.font
    font.size = 32
    font.outline = true
    font.outline_size = 2
    @bitmap.draw_text(@bitmap.rect, @days[@day_index], 1)
    @block.bitmap = @bitmap
  end

  def check_continue
    @continue_enabled = Dir["Save*.rxdata"].any?
    if @continue_enabled
      @command_window.index = 1
    else
      @command_window.disable_item(1)
    end
  end

  def setup_audio
    $game_system.bgm_play($data_system.title_bgm)
    Audio.me_stop
    Audio.bgs_stop
  end

  def terminate
    @command_window.dispose
    @bitmap.dispose
    @block.dispose
    @logo.bitmap.dispose
    @logo.dispose
    @title.bitmap.dispose
    @title.dispose
    @sprite.bitmap.dispose
    @sprite.dispose
  end

  def main
    if $BTEST
      battle_test
      return
    end
    load_database
    reset_game_system
    start
    Graphics.transition
    loop do
      Graphics.update
      Input.update
      update
      break if $scene != self
    end
    Graphics.freeze
    terminate
  end

  def update
    @logo.update
    @command_window.update
    # Added Right Click Check
    if Input.right_click?
      $game_system.se_play($data_system.cursor_se)
      if @command_window.index < 2
        @command_window.index = 2
        Audio.bgm_pause
      else
        @command_window.index = -1
        Audio.bgm_resume
      end
    # Added Left Click Check
    elsif Input.trigger?(Input::C) or Input.double_left_click?
      case @command_window.index
      when 0  # New game
        command_new_game
      when 1  # Continue
        command_continue
      when 2  # Shutdown
        command_shutdown
      end
    elsif Input.repeat?(Input::KeyD) or Input.repeat_right_click?
      $game_system.se_play($data_system.cursor_se)
      @day_index = (@day_index + 1) % 7
      @bitmap.clear
      @bitmap.draw_text(@bitmap.rect, @days[@day_index], 1)
    elsif Input.trigger?(Input::LeftShift)
      $game_system.se_play($data_system.equip_se)
      Graphics.screenshot
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
    if Input.trigger?(Input::KeyP)
      $game_system.se_play($data_system.shop_se)
      Graphics.screenshot
      return
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
    if Input.trigger?(Input::B) or Input.double_right_click?
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
  def main
    start
    Graphics.transition
    loop do
      Graphics.update
      Input.update
      update
      if $scene != self
        break
      end
    end
    Graphics.freeze
    terminate
  end

  def start
    create_command_window
    create_windows
    Mouse.set_xy(80, 28 + @menu_index * 32)
  end

  def create_command_window
    s1 = $data_system.words.item
    s2 = $data_system.words.skill
    s3 = $data_system.words.equip
    s4 = "Status"
    s5 = "Save"
    s6 = "End Game"
    @command_window = Window_Command.new(160, [s1, s2, s3, s4, s5, s6])
    @command_window.index = @menu_index
    if $game_party.actors.size == 0
      @command_window.disable_item(0)
      @command_window.disable_item(1)
      @command_window.disable_item(2)
      @command_window.disable_item(3)
    end
    if $game_system.save_disabled
      @command_window.disable_item(4)
    end
  end

  def create_windows
    @playtime_window = Window_PlayTime.new
    @playtime_window.x = 0
    @playtime_window.y = 224
    @steps_window = Window_Steps.new
    @steps_window.x = 0
    @steps_window.y = 320
    @gold_window = Window_Gold.new
    @gold_window.x = 0
    @gold_window.y = 416
    @status_window = Window_MenuStatus.new
    @status_window.x = 160
    @status_window.y = 0
  end

  def terminate
    @command_window.dispose
    @playtime_window.dispose
    @steps_window.dispose
    @gold_window.dispose
    @status_window.dispose
  end

  def update_command
    # Added Right Click Check
    if Input.trigger?(Input::B) or Input.double_right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Map.new
      return
    end
    # Added Left Click Check
    if Input.trigger?(Input::C) or Input.double_left_click?
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
        Mouse.set_xy(@command_window.width + 120, 28)
      when 2  # equipment
        $game_system.se_play($data_system.decision_se)
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
        Mouse.set_xy(@command_window.width + 120, 28)
      when 3  # status
        $game_system.se_play($data_system.decision_se)
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
        Mouse.set_xy(@command_window.width + 120, 28)
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
    # Added Double Right Click Check
    if Input.trigger?(Input::B) or Input.double_right_click?
      $game_system.se_play($data_system.cancel_se)
      @command_window.active = true
      @status_window.active = false
      @status_window.index = -1
      mx = @command_window.x + 80
      my = 28 + @command_window.index * 32
      Mouse.set_xy(mx, my)
      return
    end
    # Modified OK Button Functionality
    if Input.trigger?(Input::C)
      process_status
      return
    # Added Double Left Click Check
    elsif Input.double_left_click?
      return unless @status_window.mouse_inside?(@status_window.index)
      process_status
    end
  end

  def process_status
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
  end
end

class Scene_Item
  alias :kyon_click_win_scn_item_up_item :update_item
  def update_item
    kyon_click_win_scn_item_up_item
    # Added Double Right Click Check
    if Input.double_right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Menu.new(0)
      return
    end
  end
end

class Scene_Equip
  alias :kyon_click_win_scn_equip_up_right :update_right
  def update_right
    kyon_click_win_scn_equip_up_right
    if Input.double_right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Menu.new(2)
      return
    elsif Input.double_left_click?
      n = @right_window.index
      if !@right_window.mouse_inside?(n) or @actor.equip_fix?(n)
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      $game_system.se_play($data_system.decision_se)
      @right_window.active = false
      @item_window.active = true
      @item_window.index = 0
      return
    end
  end
end

class Scene_File
  alias :kyon_click_win_scn_fl_up :update
  def update
    kyon_click_win_scn_fl_up
    if Input.double_right_click?
      on_cancel
      return
    elsif Input.double_left_click?
      @savefile_windows.each_with_index do |win, n|
        next unless win.mouse_inside?(0)
        choose_file(n)
        on_decision(make_filename(n))
        $game_temp.last_file_index = n
        break
      end
      return
    elsif Input.left_click?
      @savefile_windows.each_with_index do |win, n|
        next unless win.mouse_inside?(0)
        choose_file(n)
        break
      end
      return
    end
  end

  def choose_file(n)
    @savefile_windows[@file_index].selected = false
    @savefile_windows[n].selected = true
    @file_index = n
  end
end

class Scene_End
  def main
    s1 = "To Title"
    s2 = "Shutdown"
    s3 = "Cancel"
    @command_window = Window_Command.new(192, [s1, s2, s3])
    @command_window.x = 320 - @command_window.width / 2
    @command_window.y = 240 - @command_window.height / 2
    @command_window.draggable = true
    Graphics.transition
    loop do
      Graphics.update
      Input.update
      update
      break if $scene != self
    end
    Graphics.freeze
    @command_window.dispose
    if $scene.is_a?(Scene_Title)
      Graphics.transition
      Graphics.freeze
    end
  end

  def update
    @command_window.update
    # Added Right Click Check
    if Input.trigger?(Input::B) or Input.double_right_click?
      $game_system.se_play($data_system.cancel_se)
      $scene = Scene_Menu.new(5)
      return
    # Added Left Click Check
    elsif Input.trigger?(Input::C) or Input.double_left_click?
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
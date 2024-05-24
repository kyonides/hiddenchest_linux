# * ClickableWindow ACE * #
#   Scripter : Kyonides Arkanthes
#   2024-05-23

# This is a script demo that shows you how it is now possible to click once on
# a menu window to choose an option while ignoring the surrounding area.
# Normally, you would have to add some calls to Input.left_click? or
# Input.right_click? or even Input.middle_click? to your target scenes to make
# this work.

class Window_Selectable
  def process_cursor_move
    return unless cursor_movable?
    last_index = @index
    if Input.left_click?
      # Set Index to invisible if clicked outside this window
      self.index = -1
      # Check each area - a list of Rect's
      @area.size.times do |n|
        next unless mouse_inside?(n)
        self.index = n
        Sound.play_cursor
        break
      end
      return
    end
    cursor_down (Input.trigger?(:DOWN))  if Input.repeat?(:DOWN)
    cursor_up   (Input.trigger?(:UP))    if Input.repeat?(:UP)
    cursor_right(Input.trigger?(:RIGHT)) if Input.repeat?(:RIGHT)
    cursor_left (Input.trigger?(:LEFT))  if Input.repeat?(:LEFT)
    cursor_pagedown   if !handle?(:pagedown) && Input.trigger?(:R)
    cursor_pageup     if !handle?(:pageup)   && Input.trigger?(:L)
    Sound.play_cursor if @index != last_index
  end

  def process_handling
    return unless open? && active
    return process_ok       if ok_enabled? && Input.double_left_click?
    return process_cancel   if cancel_enabled? && Input.double_right_click?
    return process_ok       if ok_enabled?        && Input.trigger?(:C)
    return process_cancel   if cancel_enabled?    && Input.trigger?(:B)
    return process_pagedown if handle?(:pagedown) && Input.trigger?(:R)
    return process_pageup   if handle?(:pageup)   && Input.trigger?(:L)
  end
end

class Window_Command
  def draw_item(n)
    rect = @area[n] ||= item_rect_for_text(n)
    change_color(normal_color, command_enabled?(n))
    draw_text(rect, command_name(n), alignment)
  end
end

class Window_SaveFile
  alias :kyon_click_win_win_svfl_init :initialize
  def initialize(height, index)
    kyon_click_win_win_svfl_init(height, index)
    @area << self.contents.rect
  end
end

class Scene_File
  def update_savefile_selection
    if Input.double_left_click?
      @savefile_windows.each_with_index do |win, n|
        next unless win.mouse_inside?(0)
        choose_file(n)
        on_savefile_ok
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
    elsif Input.double_right_click?
      on_savefile_cancel
      return
    end
    return on_savefile_ok     if Input.trigger?(:C)
    return on_savefile_cancel if Input.trigger?(:B)
    update_cursor
  end

  def choose_file(n)
    @savefile_windows[@index].selected = false
    @savefile_windows[n].selected = true
    @index = n
    ensure_cursor_visible
  end
end
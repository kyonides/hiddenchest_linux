# * PrintScene HC * #
#   Scripter : Kyonides Arkanthes
#   2024-05-29

# * Script Dependency: ClickableWindow

# This script mimicks what the popup windows do, including letting you move them
# around. The only issue I have found so far is that you need to move it slowly.

# Script Call: dialog_box("As many", "lines", "as possible")

module Kernel
  def dialog_box(*lines)
    scene = PrintScene.new(lines)
    scene.main
  end
end

class PrintSprite < Sprite
  WIDTH = 320
  BUTTON_OK = "OK"
  def initialize(messages, w=WIDTH, vp=nil)
    super(vp)
    @messages = messages
    lines = @messages.size
    h = lines * 32 + 64
    self.x = (Graphics.width - w) / 2
    self.y = (Graphics.height - h) / 2
    self.bitmap = Bitmap.new(w, h)
    @margin_x = 16
    @margin_y = 32
    @mouse_x = Input.mouse_x
    @mouse_y = Input.mouse_y
    @width, @height = Graphics.dimensions
  end

  def titlebar_color=(color)
    @titlebar_color = color
    w = src_rect.width
    @area << rect = Rect.new(0, 0, w, 24,)
    self.bitmap.fill_rect(rect, color)
    @area << rect = Rect.new(w - 22, 2, 20, 20)
    red = Color.new(220, 40, 40)
    self.bitmap.fill_rect(rect, red)
    icon_name = Game::ICON
    if FileInt.exist?(icon_name)
      icon = Bitmap.new(icon_name)
    else
      icon = Bitmap.new("app_logo", nil)
    end
    rect = Rect.new(4, 4, 16, 16)
    self.bitmap.stretch_blt(rect, icon, icon.rect)
    self.bitmap.font.size = 16
    self.bitmap.draw_text(0, 0, w, 24, Game::TITLE, 1)
  end

  def message_box_color=(color)
    r = src_rect
    rect = Rect.new(@margin_x, r.height - 24, 80, 28)
    self.bitmap.fill_rect(0, 24, r.width, r.height - 24, color)
  end

  def ok_box_color=(color)
    r = src_rect
    cx = (r.width - 80) / 2
    @area << rect = Rect.new(cx, r.height - 36, 80, 28)
    self.bitmap.fill_rect(rect, color)
    self.bitmap.font.size = 16
    self.bitmap.draw_text(cx, r.height - 36, 80, 28, BUTTON_OK, 1)
  end

  def refresh
    w = src_rect.width
    @messages.size.times do |n|
      self.bitmap.draw_text(12, @margin_y + n * 20, w - 24, 18, @messages[n])
    end
    @messages.clear
  end

  def display
    self.bitmap.font.size = 15
  end

  def update
    super
    if Input.trigger?(Input::B) or Input.trigger?(Input::C)
      PrintScene.running = nil
    elsif Input.left_click? or Input.right_click?
      1.upto(2) do |n|
        next unless click_area?(n)
        PrintScene.running = nil
        break
      end
    elsif Input.press?(Input::MouseLeft)
      if press_click_area?(0)
        mx = (Input.mouse_x - @mouse_x) * 2
        my = (Input.mouse_y - @mouse_y) * 2
        self.x += mx.clamp(-@width, @width * 2)
        self.y += my.clamp(-@height, @height * 2)
      end
    end
    @mouse_x = Input.mouse_x
    @mouse_y = Input.mouse_y
  end
  attr_reader :titlebar_color
  attr_writer :margin_x, :margin_y
end

class PrintScene
  def self.running=(state)
    @@running = state
  end

  def initialize(lines)
    @@running = true
    @messages = lines.map {|line| line.to_s.split("\n") }
    @messages = @messages.flatten.compact
  end

  def main
    Graphics.freeze
    start
    Graphics.transition
    update_loop
    Graphics.freeze
    terminate
  end

  def start
    @backdrop = Sprite.new
    @backdrop.bitmap = Graphics.snap_to_gray_bitmap.dup
    @print = PrintSprite.new(@messages)
    @print.z = 1000
    black = Color.new(0, 0, 0)
    gray = Color.new(80, 80, 80)
    @print.titlebar_color = black
    @print.message_box_color = gray
    @print.ok_box_color = black
    @print.refresh
  end

  def update_loop
    while @@running
      Graphics.update
      Input.update
      update
    end
  end

  def update
    @print.update
    if Input.trigger?(Input::SHIFT)
      Graphics.screenshot
    end
  end

  def terminate
    @print.bitmap.dispose
    @print.dispose
    @backdrop.bitmap.dispose
    @backdrop.dispose
  end
end
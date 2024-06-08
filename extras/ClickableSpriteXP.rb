# * ClickableSprite XP * #
#   Scripter : Kyonides Arkanthes
#   2024-06-07

# This is a script demo that shows you how it is now possible to click once on
# a sprite or one of its areas or maybe just the colored ones to drag it all
# over the game screen.
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
    puts draggable?.class
    if Input.press_left_click?
      if Mouse.no_target?
        puts "No target"
        if mouse_inside_color? and drag_color?
          puts "Logo as target"
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

  def check_mouse_inside
  end
end

class Scene_Title
  alias :kyon_click_sprite_scn_ttl_main :main
  def main
    unless $BTEST
      create_logo
    end
    kyon_click_sprite_scn_ttl_main
    unless $BTEST
      dispose_logo
    end
  end

  def create_logo
    @logo = Sprite.new
    @logo.set_xyz(24, 24, 100)
    @logo.drag_condition = :color
    puts @logo.draggable?
    @logo.bitmap = RPG::Cache.picture(LOGO)
  end

  def dispose_logo
    @logo.bitmap.dispose
    @logo.dispose
  end
end
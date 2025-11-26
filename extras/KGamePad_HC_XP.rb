# * KGamePad HC for XP * #
#   Scripter : Kyonides
#   2025-11-24

module KGamePad
  GAMEPAD_PIC = "gamepad_black"
  GAMEPAD_XY = [12, 12]
  GAMEPAD_FRAMES = 90

  def create_gamepad_icon
    @gamepad_timer = 0
    @gamepad = Sprite.new
    @gamepad.set_xy(*GAMEPAD_XY)
    @gamepad.z = 10000
    @gamepad.visible = false
    @gamepad.bitmap = Bitmap.new(32, 32)
  end

  def dispose_gamepad_icon
    @gamepad.bitmap.dispose
    @gamepad.dispose
  end

  def update_gamepad_status
    return if Input.gamepad_updates.empty?
    type = Input.gamepad_update.to_s
    @gamepad.bitmap.dispose
    @gamepad.bitmap = RPG::Cache.picture(GAMEPAD_PIC + "_" + type)
    @gamepad.visible = true
    @gamepad_timer = GAMEPAD_FRAMES
    @gamepad_pause = true
  end

  def update_gamepad_timer
    return unless @gamepad_pause
    @gamepad_timer -= 1
    @gamepad_pause = @gamepad_timer > 0
    @gamepad.bitmap.clear unless @gamepad_pause
  end
end

class Scene_Title
  include KGamePad
  alias :kyon_gamepad_scn_ttl_main :main
  alias :kyon_gamepad_scn_ttl_up :update
  def main
    create_gamepad_icon unless $BTEST
    kyon_gamepad_scn_ttl_main
    dispose_gamepad_icon unless $BTEST
  end

  def update
    update_gamepad_status
    update_gamepad_timer
    kyon_gamepad_scn_ttl_up
  end
end
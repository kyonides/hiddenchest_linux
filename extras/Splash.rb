# * Game::SplashScene * #
# - Default HiddenChest Version - #
#   Scripter : Kyonides Arkanthes
#   2024-05-28

# If you ever overwrite the main method in a custom script, you got to add
# a call to Game.setup or the normal loading processing will end abruptly.

class SplashScene
  def main
    Graphics.freeze
    start
    Graphics.transition
    Game.setup
    Graphics.freeze
    terminate
  end

  def start
    @visual = nil
    filename = "Graphics/Splash/screen"
    if FileInt.exist?(filename)
      @visual = true
      @backdrop = Sprite.new
      @backdrop.bitmap = Bitmap.new(filename)
      @title = Sprite.new
      @title.x = (Graphics.width - 400) / 2
      @title.y = (Graphics.height - 60) / 2
      b = Bitmap.new(400, 60)
      font = b.font
      font.size = 40
      font.outline = true
      font.outline_size = 3
      b.draw_text(b.rect, "Loading...", 1)
      @title.bitmap = b
    end
  end

  def terminate
    if @visual
      @title.bitmap.dispose
      @title.dispose
      @backdrop.bitmap.dispose
      @backdrop.dispose
      @visual = nil
    end
  end
end

SplashScene.new.main
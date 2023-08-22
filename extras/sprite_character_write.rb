class Sprite_Character
  def direction_suffix
    case @character.direction
    when 2
      "_down"
    when 4
      "_left"
    when 6
      "_right"
    when 8
      "_up"
    end
  end

  def write(filename)
    b = self.bitmap
    dir = @character.direction / 2 - 1
    bx = @cw * @character.pattern
    by = @ch * dir
    rect = Rect.new(bx, by, @cw, @ch)
    filename += direction_suffix
    temp_bitmap = Bitmap.new(@cw, @ch)
    temp_bitmap.blt(0, 0, b, rect)
    temp_bitmap.write(filename)
    temp_bitmap.dispose
  end
end

class Spriteset_Map
  def save_player_sprite
    filename = $game_party.actors[0].character_name
    sprite = @character_sprites.last
    sprite.bitmap.write(filename)
    sprite.write(filename)
  end
end

class Scene_Map
  def save_player_sprite
    @spriteset.save_player_sprite
  end
end
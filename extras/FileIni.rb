module System
  LOGO = "app_logo"
  DESCRIPTION = "An RGSS-based engine"
  def self.user_language
    @user_language
  end

  def self.platform
    NAME
  end

  def self.linux?
    FAMILY_NAME[/linux/i] != nil
  end

  def self.windows?
    FAMILY_NAME[/windows/i] != nil
  end
end

module Game
  SCENE_SCRIPT = "Data/Splash"
  SCENE_SCRIPT_EXT = ".rhc"
  DATA = {}
  DATA.default = ""
  RTP = []
  SOUNDFONT_REGEX = "/*.sf{2,3}"
  extend self
  attr_reader(:shot_formats, :shot_format, :shot_dir)
  attr_reader(:shot_filename, :show_splash)
  attr_reader(:soundfont, :soundfonts)
  attr_accessor(:save_dir, :save_filename, :setup_screen)
  def self.clear
    @shot_formats = ["jpg", "png"]
    @shot_format = "jpg"
    @shot_dir = "Screenshots"
    @shot_filename = "screenshot"
    @save_dir = "Saves"
    @save_filename = "Save"
    @soundfont = ""
    @show_splash = false
  end

  def self.auto_create_dir
    unless Dir.exist?(@shot_dir)
      Dir.mkdir(@shot_dir)
    end
  end

  def self.init_size
    [WIDTH, HEIGHT]
  end

  def self.normal_size?
    (WIDTH >= 320 && HEIGHT >= 240)
  end

  def self.change_size?
    (WIDTH != START_WIDTH || HEIGHT != START_HEIGHT)
  end

  def self.process_exe_name(list)
    version = 0
    5.times do |n|
      if File.exist?(EXE_BASE_NAME + "." + list[n])
        version = n
      end
    end
    return version
  end

  def self.set_exe_ini_names
    exe_name = RAW_EXE_NAME.gsub(/\.\//, "")
    const_set("EXE_NAME", exe_name)
    const_set("EXE_BASE_NAME", exe_name.sub(/\.\w*/i, ""))
    const_set("INI_FILENAME", EXE_BASE_NAME + ".ini")
  end
    
  def self.process_main_ini
    begin
      lines = File.readlines(INI_FILENAME)
    rescue => e
      puts e.class, e.message, e.backtrace
      return
    end
    lines.size.times do |n|
      key, value = lines.shift.split(/[\s]{0,}=[\s]{0,}/i)
      if !key
        next
      end
      value = !value ? "" : value.chomp
      if key[/RTP/i] != nil
        if value.size > 1
          RTP << value
        end
      else
        DATA[key] = value
      end
    end
  end

  def self.add_soundfonts(dir)
    @soundfonts += Dir[dir + SOUNDFONT_REGEX].sort
  end

  def self.set_basic_values
    if System.windows?
      default = DATA["SoundFontWin"].to_s
      sf_dir = DATA["SoundFontPathWin"].to_s
      sf_dir = sf_dir.gsub(/[\\]/, "/")
    elsif System.linux?
      default = DATA["SoundFontLnx"].to_s
      sf_dir = DATA["SoundFontPathLnx"].to_s
    end
    System.const_set("SOUNDFONT_DIR", sf_dir)
    System.const_set("SOUNDFONT", default)
    @soundfonts = []
    @soundfont = default
    self.add_soundfonts(sf_dir)
    self.add_soundfonts(Dir.pwd + "/Audio/SF2")
    if @soundfont.empty?
      if @soundfonts.size > 0
        @soundfont = @soundfonts[0]
      end
    else
      @soundfonts.delete(@soundfont)
      @soundfonts.unshift(@soundfont)
    end
    DATA["Scripts"].sub!(/[\\]/, "/")
    const_set("SCRIPTS", DATA["Scripts"].to_s)
    const_set("TITLE", DATA["Title"].to_s)
    const_set("VERSION", DATA["Version"].to_s)
    const_set("AUTHOR", DATA["Author"].to_s)
    icon_name = DATA["Icon"].to_s
    icon_name = Dir[icon_name + "*"][0]
    unless FileInt.exist?(icon_name)
      icon_name = ""
    end
    const_set("ICON", icon_name)
    project_ext = ["rhproj", "rxproj", "rvproj", "rvproj2"]
    encrypt_ext = ["rhc", "rgssad", "rgss2a", "rgss3a"]
    last_proj = DATA["ProjectFileExt"] ||= "rxvproj"
    last_encr = DATA["CompressedFileExt"] ||= "rgss4a"
    project_ext << last_proj
    encrypt_ext << last_encr
    makers = ["HiddenChest", "XP", "VX", "VX ACE", "XP ACE"]
    n = self.process_exe_name(encrypt_ext)
    encrypted = n > 0
    unless encrypted
      n = self.process_exe_name(project_ext)
    end
    Font.default_shadow = n == 2
    Font.default_outline = n == 3
    debug = (!encrypted && DATA["Debug"][/true/i] != nil)
    enc_name = EXE_BASE_NAME + "." + encrypt_ext[n]
    const_set("DEBUG", debug)
    const_set("ENCRYPTED_NAME", enc_name)
    const_set("MAKER", makers[n])
    const_set("RGSS_VERSION", n)
    w = DATA["Width"].to_i
    h = DATA["Height"].to_i
    w = (w == 0 ? START_WIDTH : w)
    h = (h == 0 ? START_HEIGHT : h)
    const_set("WIDTH", w)
    const_set("HEIGHT", h)
    state = DATA["Fullscreen"][/true/i] != nil
    const_set("FULLSCREEN", state)
    state = DATA["SubImageFix"][/true/i] != nil
    const_set("SUBIMAGEFIX", state)
    puts "Ruby Version:       #{RUBY_VERSION}"
    puts "RGSS Version:       #{n} (#{MAKER})"
    puts "Game Screen Size:   #{WIDTH}x#{HEIGHT}"
    puts "Enable Debug:       #{DEBUG}"
    puts "Trigger Fullscreen: #{FULLSCREEN}"
    puts "SubImage Fix:       #{SUBIMAGEFIX}"
    Font.solid_fonts = DATA["SolidFonts"].to_s[/true/i] != nil
  end

  def self.soundfont_index
    @soundfonts.index(@soundfont)
  end

  def self.custom_scene_file
    SCENE_SCRIPT + SCENE_SCRIPT_EXT
  end

  def self.setup_custom_scene
    @show_splash = DATA.delete("ShowSplash").to_s[/true/i] != nil
    if FileInt.exist?(SCENE_SCRIPT + ".rb")
      lines = File.binread(SCENE_SCRIPT + ".rb")
      lines = Zlib::Deflate.deflate(lines)
      save_data(lines, custom_scene_file)
      return true
    end
  end

  def self.setup
    auto_create_dir
    set_exe_ini_names
    process_main_ini
    set_basic_values
    set_internal_values
    if System.linux?
      Graphics.wait(60)
    end
    $scene = nil
  end

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
  end

  def terminate
  end
end

end

begin
  $LOAD_PATH << Dir.pwd
  Game.clear
  Game.setup_custom_scene
  if FileInt.exist?(Game.custom_scene_file)
    lines = load_data(Game.custom_scene_file)
    lines = Zlib::Inflate.inflate(lines)
    eval lines
  end
  Graphics.hide_window
  if Game.show_splash
    Graphics.hide_window
    $scene = Game::SplashScene.new
      while $scene
      $scene.main
    end
    Graphics.hide_window
  else
    Game.setup
  end
  puts "Completed setup successfully."
  Game.window_show_borders = true
  Graphics.resize(Game::WIDTH, Game::HEIGHT)
  Game.icon = Game::ICON
  Game.title = Game::TITLE
  if System.windows?
    Graphics.wait(4)
    Graphics.transition(1)
    Graphics.freeze
  end
  Graphics.show_window
rescue => e
  puts e.class, e.message, e.backtrace
  Game.setup
  Graphics.show_window
end
# * FileIni Class - HC Version * #
#   Scripter : Kyonides
#   2025-07-15

# This scripting tool lets you open, cache and write contents to INI files.

class FileIni
  class NoError
    def initialize
      @message = "Everything is working fine."
      @backtrace = []
    end
    attr_accessor :message
    attr_reader :backtrace
  end

  class NoFileError < NoError
    def set(filename)
      @message = filename + " file could not be found!\n" +
                 "A new INI file will be created!"
      @backtrace.clear
      @backtrace << "Backtrace:"
      @backtrace << "FileIni:41:in 'initialize'"
      @backtrace << "FileIni:46:in 'exist?'"
    end
  end

  class Section
    def initialize(name)
      @name = name
      @keys = []
      @values = []
      @lines = []
    end
    attr_reader :name, :keys, :values, :lines
  end

  @@last = nil
  @@last_error = NoError.new
  attr_reader :filename, :section_names
  def initialize(filename)
    @filename = filename
    @section_names = []
    @sections = []
    @@last = self
    if File.exist?(filename)
      lines = File.readlines(filename)
    else
      @@last_error = NoFileError.new
      @@last_error.set(filename)
      version = Game::RGSS_VERSION
      if version == 3
        msgbox set_error_str
      else
        print set_error_str
      end
      File.open(filename, "w") {}
      return
    end
    section = nil
    lines.size.times do |n|
      line = lines[n]
      key, value = line.split(/[\s]{0,}=[\s]{0,}/i)
      if !value and !line[/=/]
        name = key.gsub(/\[|\]/, "").chomp
        @section_names << name
        @sections << section = Section.new(name)
        next
      end
      section.keys << key
      section.values << (value ? value.chomp : "")
      section.lines << line
    end
  end

  def self.open(filename)
    FileIni.new(filename)
  end

  def self.last
    unless @@last
      raise "Failed to load INI data!\n" +
            "Did you forget to open that file first?"
      return
    end
    @@last.filename
  end

  def self.get_last_error
    set_error_str
  end

  def self.flush_error
    @@last_error = NoError.new
  end

  def read(section_name, key, default)
    n = @section_names.index(section_name)
    section = @sections[n]
    n = section.keys.index(key)
    value = section.values[n]
    value&.empty? ? default : value
  rescue
    default
  end

  def write(section_name, key, value)
    section = find_section(section_name)
    keys = section.keys
    n = find_key_index(section.keys, key, false)
    section.keys[n] = key
    section.values[n] = value.to_s
    section.lines[n] = key + "=#{value}\r\n"
    write_all_entries
    return value.size
  rescue => @@last_error
    return 0
  end

  def comment_out(section_name, key, default)
    section = find_section(section_name)
    keys = section.keys
    n = find_key_index(section.keys, key, true)
    section.keys[n] = ";" + key
    value = section.values[n] || default.to_s
    section.values[n] = value
    section.lines[n] = ";#{key}=#{value}\r\n"
    write_all_entries
    return value.size
  rescue => @@last_error
    return 0
  end
  private
  def set_error_str
    msg = "#{@@last_error.class}\n#{@@last_error.message}\n"
    msg += @@last_error.backtrace.join("\n")
    msg
  end

  def find_section(section_name)
    n = @section_names.index(section_name)
    section = @sections[n]
    unless section
      section = Section.new(section_name)
      @sections << section
    end
    section
  end

  def find_key_index(keys, key, commented)
    this_key = commented ? ";" + key : key
    n = keys.index(this_key)
    unless n
      this_key = commented ? key : ";" + key
      n = keys.index(this_key) || keys.size
    end
    n
  end

  def write_all_entries
    File.open(@filename, "w") do |f|
      @sections.each do |section|
        f.puts "[#{section.name}]"
        f.puts section.lines
      end
    end
  end
end
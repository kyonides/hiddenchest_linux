<a name="top"></a>
# FileIni
A class that handles reading and writing to Windows INI files.
It intends to replace the corresponding Win32API calls using pure Ruby code.

### Superclass
**Object**

### Embedded Classes
- **NoError**
- **NoFileError**
- **Section**

### Class Variables
##### **@@last**
> Returns the last handle opened during execution if any.

##### **@@last_error**
> Returns the last error as a Ruby object.
  The engine will keep running as usual unless a fatal error occurs.

### Singleton Methods

[open](#open)  
[get_last_error](#get_last_error)  
[flush_error](#flush_error)  

##### **open(_filename_)**
<a name="open"></a>
> Wrapper of the new method.
  Opens the given file named filename (a String).
  If it does not exist, it will create one.

##### **get_last_error**
<a name="get_last_error"></a>
> Returns a custom Ruby error object or an instance of FileIni::NoError if there is none.

##### **flush_error**
<a name="flush_error"></a>
> Resets the error which becomes an instance of FileIni::NoError at once.

### Methods

[new](#new)  
[read](#read)  
[write](#write)  
[comment_out](#comment_out)  
[filename](#filename)  
[section_names](#section_names)  

##### **new(_filename_)** 
<a name="new"></a>
> Opens the file specified in _filename_ (a String).
  If it does not exist, it will create one.

##### **read(_section_name_, _key_, _default_)**
<a name="read"></a>
> Searches for the section specified in _section_name_

##### **write(_section_name_, _key_, _value_)**
<a name="write"></a>
> Searches for the section specified in _section_name_

##### **comment_out(_section_name_, _key_, _default_)**
<a name="comment_out"></a>
> Searches for the section specified in _section_name_

##### **filename**
<a name="filename"></a>
> Gets the file name (a String).

##### **section_names**
<a name="section_names"></a>
> Gets the Array filled with all of the available section names.

### Private Methods

[find_section](#find_section)  
[find_key_index](#find-key-index)  
[write_all_entries](#write-all-entries)  

##### **find_section(_section_name_)**
<a name="find-section"></a>
> Returns the index of an existing section named section_name (a String).
  It can create a new section if none was found.

##### **find_key_index(_keys_, _key_, _commented_out_)**
<a name="find-key-index"></a>
> Returns the key's index or the current size of keys if none could be found.
  If might also try searching for a key that has been commented_out.

##### **write_all_entries**
<a name="write-all-entries"></a>
> Writes all sections and their corresponding lines to the INI file.
  The actual INI file gets closed right after finishing the writing process.

[Back to top](#top)
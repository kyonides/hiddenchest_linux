<a name="top"></a>
# Input
> A module that handles all kinds of devices like keyboards, gamepads, and mice.  
It processes their key values to let you use them in game in several ways.

### Superclass
**Object**

### Constants
##### **DEFAULT BUTTONS**
> A, B, C, X, Y, Z, L, L2, R, R2, LEFT, RIGHT, UP & DOWN

##### **CUSTOM BUTTONS**
> Too Many to list them here thanks to HiddenChest's full keyboard support.

##### **BUTTON_SCANCODES**
> A Hash that returns the SDL value of a specific default button.

##### **SCANCODE_BUTTONS**
> A Hash that returns the RGSS value of a SDL code.

##### **BUTTON_CODES**
> A Hash that returns the value of a default button by passing it a symbol like :C.

##### **CODE_BUTTONS**
> Does it the exact opposite of what BUTTON_CODES Hash does.

##### **KEY2CHAR**
> A Hash that returns a Ruby String like "G" or "9" by passing it a button value.

##### **KEY2NAME**
> A Hash that returns a Ruby String like "Return" or "Delete" for keyboard keys.

### Singleton Methods

[dir4](#dir4)  
[dir4?](#dir42)  
[dir8](#dir8)  
[dir8?](#dir82)  
[gamepad](#gamepad)  
[gamepad?](#gamepad?)  
[total_gamepads](#total_gamepads)  
[gamepad_update](#gamepad_update)  
[gamepad_updates](#gamepad_updates)  
[gamepad_open!](#gamepad_open)  
[gamepad_close!](#gamepad_close)  
[gamepad_reset!](#gamepad_reset)  
[mode](#mode)  
[play_mode!](#play_mode)  
[text_mode!](#text_mode)  
[keymap_mode!](#keymap_mode)  
[capslock_state](#capslock_state)  
[last_key](#last_key)  
[last_key?](#last_key2)  
[last_char](#last_char)  
[clear_text_input](#clear_text_input)  
[save_key_bindings](#save_key_bindings)  

##### **dir4**
<a name="dir4"></a>
> Returns a number based on the keypad's values or 0 if no button was pressed.

##### **dir4?**
<a name="dir42"></a>
> Returns true if you have pressed a directional button, false otherwise.

##### **dir8**
<a name="dir8"></a>
> Returns a number based on the keypad's values or 0 if no button was pressed.

##### **dir8?**
<a name="dir82"></a>
> Returns true if you have pressed a directional button, false otherwise.

##### **gamepad**
<a name="gamepad"></a>
> Returns the current gamepad or a placeholder object.

##### **gamepad?**
<a name="gamepad?"></a>
> Returns true if a gamepad could be found, false otherwise.

##### **total_gamepads**
<a name="total_gamepads"></a>
> Returns the number of gamepads discovered by the engine.

##### **gamepad_update**
<a name="gamepad_update"></a>
> Returns the last gamepad connection or disconnection event or nil.  
Clears the array automatically.

##### **gamepad_updates**
<a name="gamepad_updates"></a>
> Returns the last gamepad connection or disconnection event as an array.

##### **gamepad_open!**
<a name="gamepad_open"></a>
> A method that can be used to virtually open a gamepad from RGSS.

##### **gamepad_close!**
<a name="gamepad_close"></a>
> A method that can be used to virtually close a gamepad from RGSS.

##### **gamepad_reset!**
<a name="gamepad_reset"></a>
> A method that can be used to virtually close and reopen a gamepad from RGSS.

##### **mode**
<a name="mode"></a>
> Returns 1 out of 3 Ruby Strings:  
- Play Mode  
- Text Input Mode  
- Key Mapping Mode  

##### **play_mode!**
<a name="play_mode"></a>
> Default mode.

##### **text_mode!**
<a name="text_mode"></a>
> Lets you enter keys and it will return a Ruby string ignoring most of the key bindings the engine normally uses.

##### **keymap_mode!**
<a name="keymap_mode"></a>
> Lets you change the key bindings at will.  
Under normal circumstances, only a Mouse button could let you exit this mode.  
Certain Function keys will be ignored, nonetheless.

##### **capslock_state**
<a name="capslock_state"></a>
> Returns the current CapsLock key state as a boolean, i.e. true or false.

##### **last_key]**
<a name="last_key"></a>
> Returns the last key you have pressed.

##### **last_key?**
<a name="last_key2"></a>
> Returns true if you have pressed a key, false otherwise.

##### **last_char**
<a name="last_char"></a>
> Returns the last key as a 1 character only Ruby String.

##### **clear_text_input**
<a name="clear_text_input"></a>
> Clears the last key and calls the play_mode method.

##### **save_key_bindings**
<a name="save_key_bindings"></a>
> Lets you save your current key bindings at once. Even the typical F1 menu will recognize them.

[Back to top](#top)
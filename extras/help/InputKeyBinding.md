<a name="top"></a>
# Input::KeyBinding
A class that handles 1 out of 5 types of key bindings.

### Superclass
**Object**

### Related Classes
**Input::KeyBindings**  
**Input::KeyBindingGroup**

### Types ###
0 - Invalid  
1 - Key  
2 - Gamepad's Button (JS)  
3 - Gamepad's Axis (Axis)  
4 - Gamepad's Hat (Hat)  

### Methods

[name](#name)  
[name=](#set_name)  
[type](#type)  
[type_name](#type_name)  
[symbol](#symbol)  
[value](#value)  
[value=](#set_value)  
[scancode](#scancode)  
[dir](#dir)  
[index](#index)  

##### **name**
<a name="name"></a>
> The current name of the key, button, axis or hat.

##### **name=**
<a name="set_name"></a>
> Sets the current name of the key, button, axis or hat.  
This will also call the internal refresh_type method that will extract all the other values from this new name.  
It is your best option for parsing INI files.

##### **type**
<a name="type"></a>
> Returns 1 of the types listed above.

##### **type_name**
<a name="type_name"></a>
> Returns 1 of the type names listed above.

##### **symbol**
<a name="symbol"></a>
> Returns a symbol based on the button's name.  
Used internally when you change the binding's contents by changing the button's name.

##### **value**
<a name="value"></a>
> Returns the RGSS or HiddenChest internal value of a key, button, axis or hat.

##### **value=**
<a name="set_value"></a>
> Sets the RGSS or HiddenChest internal value of a key, button, axis or hat.  
It also updates all of the other variables accordingly.  
It is your best option for setting buttons' new values using some RGSS GUI.

##### **scancode**
<a name="scancode"></a>
> Returns a custom SDL scan code based on USB device specifications.  
Keys on your keyboard totally depend on this value to work normally.

##### **dir**
<a name="dir"></a>
> Returns 0 or 1 for axes or a custom direction for hats, nil otherwise.

##### **index**
<a name="index"></a>
> Returns the position of the current binding in the corresponding Input::KeyBindingGroup.

[Back to top](#top)
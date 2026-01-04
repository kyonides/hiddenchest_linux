<a name="top"></a>
# Input::KeyBinding
A class that handles 1 of 5 types of key bindings.

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
[scancode](#scancode)  
[dir](#dir)  
[index](#index)  

##### **name**
<a name="name"></a>
> The current name of the key, button, axis or hat.

##### **name=**
<a name="set_name"></a>
> Set the current name of the key, button, axis or hat.  
This will also call the internal refresh_type method that will extract all the other values from this new name.

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
> Returns RGSS or HiddenChest internal value of a key, button, axis or hat.

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
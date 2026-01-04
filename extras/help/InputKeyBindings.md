<a name="top"></a>
# Input::KeyBindings
A class that handles all of the currently available key bindings.
It includes all keys, gamepad buttons, axis & hats.

### Superclass
**Object**

### Related Classes
**Input::KeyBindingGroup**  
**Input::KeyBinding**

### Methods

[name](#name)  
[version](#version)  
[list](#list)  

##### **name**
<a name="name"></a>
> Custom gamepad's profile name.

##### **version**
<a name="version"></a>
> Returns the RGSS version your game is running on.

##### **list**
<a name="list"></a>
> Returns 14 buttons groups (as Input::KeyBindingGroup objects) that the game might need to make custom RGSS virtual buttons like A or C work with keyboards & gamepads simultaneously.\  
The list also include 2 new additions, namely L2 & R2.

[Back to top](#top)
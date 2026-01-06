<a name="top"></a>
# Input::KeyBindingGroup
A class that handles up to 4 key bindings for 1 of RGSS virtual buttons like A or C.
It might include keys, gamepad buttons, axis or hats.

### Superclass
**Object**

### Related Classes
**Input::KeyBindings**  
**Input::KeyBinding**

### Methods

[name](#name)  
[target](#target)  
[data](#data)  
[\[\](n)](#getter)  
[\[\]=(n,v)](#setter)  

##### **name**
<a name="name"></a>
> The current key binding group's name (A, X or Left).

#### **target**
<a name="target"></a>
> The numerical value of the target button as seen by RGSS.  
The corresponding SDL's values are totally different.

##### **data**
<a name="data"></a>
> Returns all 4 Input::KeyBinding objects it contains.

##### **\[\](n)**
<a name="getter"></a>
> Returns the Input::KeyBinding object currently stored at that position.

##### **\[\]=(n,v)**
<a name="setter"></a>
> Sets a given Input::KeyBinding's value.

[Back to top](#top)
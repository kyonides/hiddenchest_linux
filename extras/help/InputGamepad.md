<a name="top"></a>
# Input::Gamepad
A class that handles some game controller data.

### Superclass
**Object**

### Constants
##### **DEFAULT_NAME**
> Ruby String. Default value: "None"

##### **DEFAULT_VENDOR**
> Ruby String. Default value: "None"

### Class Variables
##### **@@types**
> Returns the types array stored in this class.
  Default Values:
  "Unknown"
  "Controller"
  "Wheel"
  "Arcade Stick"
  "Flight Stick"
  "Dance Pad"
  "Guitar"
  "Drum Kit"
  "Arcade Pad"
  "Throttle"

##### **@@levels**
> Returns the levels hash stored in this class.
  Default Values:
  @@levels[-1] = "Unknown"
  @@levels[0]  = "Empty"
  @@levels[1]  = "Low"
  @@levels[2]  = "Medium"
  @@levels[3]  = "Full"
  @@levels[4]  = "Wired"
  @@levels[5]  = "Maximum"

### New Input's Singleton Methods

[gamepad](#gamepad)  
[joystick](#gamepad)  
[gamepad?](#gamepad?)  
[joystick?](#gamepad?)  
[gamepad_update](#gamepad_update)  
[joystick_update](#gamepad_update)  
[gamepad_updates](#gamepad_updates)  
[joystick_updates](#gamepad_updates)  

##### **gamepad**
<a name="gamepad"></a>
> The gamepad's getter method. (Read only)

##### **gamepad?**
<a name="gamepad?"></a>
> Returns true if a gamepad could be found, false otherwise.

##### **gamepad_update**
<a name="gamepad_update"></a>
> Returns the last gamepad connection or disconnection event or nil.
  Clears the array automatically.

##### **gamepad_updates**
<a name="gamepad_updates"></a>
> Returns the last gamepad connection or disconnection event as an array.

### Methods (Read Only)

[name](#name)  
[vendor](#vendor)  
[type](#type)  
[power](#power)  
[rumble](#rumble)  
[last_rumble](#last_rumble)  
[set_rumble](#set_rumble)  

##### **name** 
<a name="name"></a>
> Returns the gamepad's full name as a Ruby String.

##### **vendor**
<a name="vendor"></a>
> Returns the gamepad's vendor ID.

##### **type**
<a name="type"></a>
> Returns the gamepad's type as a Ruby String.

##### **power**
<a name="power"></a>
> Returns the gamepad's power level as a Ruby String.

##### **rumble**
<a name="rumble"></a>
> Returns true if the gamepad has the rumble or vibration feature, false otherwise.

##### **last_rumble**
<a name="last_rumble"></a>
> Returns true if the last gamepad's rumble feature was successful, false otherwise.

##### **set_rumble(_left_frequency_, _right_frequency_, _milliseconds_)**
<a name="set_rumble"></a>
> Sends a rumble requests. It returns true if successful, false otherwise.
  _left_frequency_ & _right_frequency_ value range: 0 through 65535.

[Back to top](#top)
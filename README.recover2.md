# Introduction to Retroradio
## Summary
This project is based on the ESP32-Radio by Edzelf.

This project extends the capabilities of the ESP32-Radio, so you will need to familiarise with this project first.

The project called Retroradio, because the basic idea was to use an existing "retro radio" as 
amplifier and front-end for the ESP32-radio.

These type of radio usally have:
  - a power LED
  - a switch for selecting AM/FM/AUX
  - a volume knob (variable resistor)
  - a tuning knob (variable capacitor) 
  
In the original version that type of inputs and outputs are not supported directly. This
project now supports:
   
   - analog input (i. e. to control volume by turning the variable resistor attached to
     the volume knob). Two additional lines in the preferences settings will achieve that.
   - extended touch input (i. e. to evaluate the position of a variable capacitor to switch
     presets accordingly). (Also two additional lines only)
   - TODO: led output. With PWM capability / blink pattern support
   - TODO: the functionality of inputs can be extended to define reactions
     to double-click like events (or longpress)
   - the capabilities of of the commands/preference settings have been extended to include 
     (a limited) scripting 'programming language'. You can react on events (changing inputs or
     changing system variables). Silly example: if the user does not like the current program
     he might turn the volume down to 0. You can have a 'hook' linked to the internal variable
     that holds the current volume level. If by that hook, you identify that '0' is reached, you
     can switch the preset to a station that hopefully plays something better. As that hook can be 
     linked to the internal variable, it does not matter which input channel is used to set the volume to
     zero...
	 
All of these options can be configured by the preference settings. So if you compile this version
and use your standard preference settings as before you should notice no difference.


There are also some enhancements which are not necessarily associated with the extended input capabilities. Those are found
in the section Generic enhancements below.

## Generic considerations for migration

If you have the ESP32 radio installed and preferences set up, so it is up and running, the RetroRadio software should run out of the box
with a few things to notice:

- 

- This version needs more NVS-entries. If you are using already much space in NVS (namely if you have a lot of presets defined) you might
  need to increase the size of the NVS-partition. 
- You can use the command _test_ to check if there are still entries in NVS available. If the number of free entries is close to zero, or
  you see something like *nvssetstr failed!* in Serial output, or if entries have vanished from your preference settings, you are probably 
  out of NVS-space.

## Storing values into RAM (or NVS)
In the original version, _key_-_value_-pairs are stored into NVS. There is now a way to store such pairs into RAM as well. So from now on, 
if the text references to _key_ or _value_ associated to _key_, keep in mind that the actual storage 
location can either be in NVS as usual or in RAM. (If a specific _key_ exists in both NVS and RAM, the one in RAM will be used. This allows
to simply override a 'default setting' in NVS by storing the same key in RAM). 

NVS keys can be defined in the preferences as usual.

- There is a command _nvs_ now to set an NVS-Entry. Syntax is either _nvs = key = value_ to assign the _value_ to the given key or
  _nvs.key = value_. There is a syntactical difference: the former does not interpret _value_ in any case, while in the latter, if 
  _value_ starts with '@', then _value_ (without leading '@') is considered to be a key-name by itself and is exchanged with the 
  content of the resulting search for the dereferenced key.
- There is a command _ram_ now to set an RAM-Entry. Syntax is equivalent for the _nvs_-command above. 
- In both cases, if _key_ exists the value associated to set key will be replaced. Otherwise the _key_-_value_-pair will be created and stored.
- Example:
  * _ram.tst1 = 42_ will create a RAM-Entry with _key_ == _tst_ and _value_ == _'42'_
  * _ram.tst2 = @tst1_ will create entry _tst2_ in RAM with the _value_ == _'42'_
  * _ram = tst3 = @tst1_ will create entry _tst3_ in RAM with the _value_ == _'@tst1'_ 
  * The susbtitution for the dereferenced key _@tst1_ is done at command execution time. If _tst1_ will change in value later, _tst2_ will
    still stay at _'42'_.
- you can list the RAM/NVS command with the commands _nvs?=Argument_ or _ram?=Argument_. _Argument_ is optional, if set only keys that 
  contain _Argument_ as substring are listed. Try _ram?=tst_ for example.
- RAM or NVS entries can be deleted using the command _nvs- = key_ or _ram- = key_ which will delete _key_ and the associated value from RAM/NVS.


## Command handling enhacements
When a command is to be executed as a result of an event, you can now not just execute one command but a sequence of 
command. Commands in a sequence must be seperated by ';'. 
Consider a "limp home" button on the remote, which brings you to your favorite station (_preset_0_) played
at an convenient volume (_volume = 75_). So you can define in the preferences _ir_XXXX = volume = 75; preset = 0_
to achieve that.
Command values can also be referenced to other preference settings. So if you have a setting _limp_home = volume = 75; preset = 0_
in the preset, you can reference that by using "@": _ir_XXXX = @limp_home_ (spaces between "@" and the key name, here _limp_home_,
are not permitted). If the given key does not exist, an empty string is used as replacement.
Commands in a sequence are executed from left to right.

## The execute-command
The execute-command takes the form _execute = key-name_. If _key-name_ is defined (either NVS or RAM) the associated value is
retrieved and executed as commands-list (whith each command being seperated by semicolon).

## Startup event
If the key-value-pair _::start = value_ is defined, the associated _value_ is retrieved and executed as commands-list.
The 'event' is generated after everything else in _setup()_ is finished just before the first _call to loop()_ (So network is up and
running, preferences are read, VS module is initialized. I. e. ready to play).

If you set _::start = volume = 70;preset = 0_ in the preferences, then at start the radio will always tune to _preset_0_ and set the volume 
to level 70.




# Generic additions
## Summary
Generic additions are not specific to the Retro radio idea but can be used in general with the ESP32 Radio. You can skip to section extended
input handling if you are not interested to use/understand the following features for now:
 - Ethernet can be used (I had to place one radio at a spot with weak WLAN reception)
 - Philips RC5 protocol is implemented in addition to the NEC protocol. (you can configure in the preference
	  to use both or only either one of that protocol. By default both are enabled)
 - A channel concept is introduced to simplify a re-mapping of presets.
 - IR remote handling has been extended to recognise longpress and release events
 - When assigning commands to an event (IR pressed, touch pressed or new events like tune knob turned) you
	  can execute not only a single command, but command sequences also. A command sequence is a list of commands
	  separated by _;_
 - when a value for a command is evaluated, you can reference an additional nvs element by _@key_. Simple 
	  example: if the command _volume = @def_volume_ is encountered, the preferences are searched for key _def_volume_ 
	  which will be used as paramter (if not found, will be substituted by empty string)
 

## Ethernet support
### Caveat Emptor
Ethernet support is the most experimental feature. It is just tested with one specific board, namely the Olimex Esp POE.
I am pretty sure it will work the same for the Olimex Esp32 POE ISO.
I have not tested it for any other hardware configurations, but it should work with any hardware combination that is based
on the native ethernet implementation of the Esp32 chip. 

**USE AT YOUR OWN RISK!**

### Compile time settings for Ethernet
There is a define now in the very first line of ***RetroRadio.ino*** that reads `#define ETHERNET 1`
	
* this will compile **with** Ethernet support
* if changed to `#define ETHERNET 0` (or any value different from '1'), support for Ethernet is **not** compiled
* if this line is deleted/commented out, Ethernet support will be compiled depending on the Boards setting in the Tools menu of Arduino IDE.
	  Currently, only the settings **OLIMEX ESP32-PoE** and **OLIMEX ESP32-PoE-ISO** would then compile Ethernet support.
* if compiled with Ethernet support by the above rules, Ethernet can then be configured at runtime by preferences setting.
* if compiled with Ethernet support, there is another define that controls the ethernet connection: `#define ETHERNET_CONNECT_TIMEOUT 5`
	  that defines how long the radio should wait for an ethernet connection to be established (in seconds). If no IP connection over 
	  ethernet is established in that timeframe, WiFi will be used. That value can be extended by preference settings (but not lowered). In
	  my experience 4 seconds is too short. If the connection succeds earlier, the radio will commence earlier (and will not wait to consume
	  the full timeframe defined by 'ETHERNET_CONNECT_TIMEOUT'). 
* the following defines are used. They are set to default values in 'ETH.h' (and 'pins_ardunio.h' for ethernet boards). If you need to change
	  those (not for **OLIMEX ESP32-PoE...**), you need to re-define them before '#include ETH.h' (search in ***RetroRadio.ino***) or set them in the preferences (see below):
  * ETH_PHY_ADDR
  * ETH_PHY_POWER
  * ETH_PHY_MDC
  * ETH_PHY_MDIO
  * ETH_PHY_TYPE
  * ETH_CLK_MODE
	  
### Preference settings for Ethernet	  
This section is only valid if you compiled with Ethernet support as described in the paragraph above. If not, all preference settings in this
paragraph are ignored.

The easy part here is the preference setting of **eth_timeout**:

* if **eth_timeout = 0** is found, ethernet connection will not be used
* if any other setting **eth_timeout = x** is found, x is evaluated as number and that number will be used as timeout (in seconds) to wait for an 
  IP connection to be established over Ethernet.
  * If that is not a valid number, '0' will be assumed as that number.
  * If that number is below the value defined by *ETHERNET_CONNECT_TIMEOUT* it will be set to *ETHERNET_CONNECT_TIMEOUT*.
  * If that number is bigger than two times *ETHERNET_CONNECT_TIMEOUT* a debug warning will be issued (but the value would be used anyway).

The other Ethernet settings that can be configured are:
1. "eth_phy_addr" to override the `#define` for ETH_PHY_ADDR
1. "eth_phy_power" to override the `#define` for ETH_PHY_POWER
1. "eth_phy_mdc" to override the `#define` for ETH_PHY_MDC
1. "eth_phy_mdio" to override the `#define` for ETH_PHY_MDIO
1. "eth_phy_type" to override the `#define` for ETH_PHY_TYPE
1. "eth_clk_mode" to override the `#define` for ETH_CLK_MODE

The problem here is, that *`eth_phy_type`* and *`eth_clk_mode`* are enums. They are defined in the sdk include file ***esp_eth.h***. In the preferences they
are expected as int-type. For the current core 1.0.4 implementation the mapping is defined as follow:

```
typedef enum {
    ETH_CLOCK_GPIO0_IN = 0,   /*!< RMII clock input to GPIO0 */ 
    ETH_CLOCK_GPIO0_OUT = 1,  /*!< RMII clock output from GPIO0 */
    ETH_CLOCK_GPIO16_OUT = 2, /*!< RMII clock output from GPIO16 */
    ETH_CLOCK_GPIO17_OUT = 3  /*!< RMII clock output from GPIO17 */
} eth_clock_mode_t;
```

and

```
typedef enum {
    ETH_MODE_RMII = 0, /*!< RMII mode */
    ETH_MODE_MII,      /* equals 1 !< MII mode */
} eth_mode_t;
```

**For future/different core versions that assignment might change.**


## Channel concept
The channel concept allows a simple re-mapping of the preferences. There are two new commands to implement the channel concept. (as usual, the
commands can be defined either by preference settings or through the input channels at runtime, i. e. Serial input).

- _channels = comma-delimited-integer-list_ will (re-)define the channel list. In the list numbers (decimal) are expected which are treated as 
   reference to _preset_X_ in the preferences. So _channels = 1, 2, 10, 11, 12, 13, 14, 15, 16_ defines 9 channels in total witch Channel1 mapped to 
   _preset_1_ to Channel9 mapped to _preset_16_
- That assignment does not change the current preset. To use that channel-list (i. e. to switch channel), the command _channel = Argument_ must be
    used.
- Argument can be:
  - A number between '1' and 'max' whith 'max' being the number of channels as defined by the command _channels_ above. In our example '9'. This
    will change the preset accordingly (but only if different from current channel).
  - The word 'any': chose a random preset from the channellist. (It is guaranteed that the chosen preset will be different from current preset.)
  - The word 'up': if the current channel is not the last channel, tune to next preset in channel-list. (if the current preset is not in the channel
    list, i. e. if tuned to by other means, make Channel1 the current channel).
  - The word 'down': if the current channel is not the Channel1, tune to previous preset in channel-list. (if the current preset is not in the channel
    list, i. e. if tuned to by other means, make ChannelMax the current channel).



## IR remote enhancements
### Added support for RC5 remotes (Philips)
Now also RC5 remotes (Philips protocol) can be decoded. RC5 codes are 14 bits, where the highest bit (b13) is always 1.
Bit b11 is a toggle bit that will change with every press/release cycle of a certain key. That will make each key to generate
two different codes. To keep things simple that bit will always be set to 0.

So if you have a RC5 remote and and pin_ir is defined in preferences and a VS1838B IR receiver is connected to that pin, you
should see some output on Serial if a key on the remote is pressed (and debug is set to 1 of course). The code reported should
be in the range 0x2000 to 0x37FF. (Also you should see "(RC5)" mentioned in the output. You can attach commands to the observed codes
with the usual ir_XXXX scheme (or the extended repeat schemes below).

If you want your radio to react on NEC or RC5 codes only, change the pin-setting in the preferences as follows:
- use pin_ir_nec = x to use only NEC-decoder 
- use pin_ir_rc5 = x to use only RC5-decoder
- only one of pin_ir, pin_ir_nec, pin_ir_rc5 should be defined. If more than one is defined, the preference is pin_ir, pin_ir_nec and pin_ir_rc5.
  (so if for instance pin_ir_nec and pin_ir_rc5 are defined, the setting for pin_ir_rc5 is ignored and only NEC protocol is decoded using the pin
  specified by pin_ir_rc5)

### Added support for longpress and release on IR-Remotes
Sometimes it is desirable to have the radio to react on longpress of an IR remote key (Vol+/Vol- for instance). As implemented in the original 
version, you would get only one ir_XXXX event on pressing a certain key, no matter how long the key is pressed.

Therefore a few additional events are generated on longpress events. The first press is always reported as _ir_XXXX_. 
Longpress events follwoing that initial press can be catched by the following:
- _ir_XXXXr_ will be called repeatedly as long as the key is pressed. So for the Vol+ key the setting could be:
  _ir_XXXXr = upvolume = 1. For both protocols this repeat events will be generated with a distance of about 100ms (could be longer)
- it is guaranteed that no _ir_XXXXr_ event will be generated, if the leading _ir_XXXX_ event has not been evaluated before. That does
  not mean, that the _ir_XXXX_ event must be defined in preferences. It means that if both _ir_XXXX_ and _ir_XXXXr_ are defined that there
  will be no execution of _ir_XXXXr_ without previous execution of _ir_XXXX_.
- if you want to catch a specific repeat event, you can define _ir_XXXXrY_ where Y is any number >= 1 (decimal, leading 0 not needed) that will
  fire on the Yth repeat of the keypress repeat indicator. That is roughly Y * 0.1 sec. So _ir_XXXXr10 = mute_ will mute the radio if key XXXX
  is pressed for about 1 second. If both _ir_XXXXr_ and _ir_XXXXrY_ are defined, only _ir_XXXXrY_ is executed if the repeat counter reaches Y.
- the order of sequence is guaranteed, so it will always be _ir_XXXX_ as start followed by _ir_XXXXr1_ and _ir_XXXXr2_ and so on.  
- if you define an event with the pattern _ir_XXXXRZ_ where Z is again a decimal number and R is in fact the uppercase R you define a recurring event.
  The repeat counter will reset to 0. So in the Vol+ example above: if you define _ir_XXXXR3 = upvolume = 1_ the command _upvolume = 1_ will be 
  called at slower rate (roughly every 0.3 seconds instead every 0.1 seconds). *This really resets the repeat counter.* That means any command 
  with a repeat counter > 3 in this example will never get executed. If _ir_XXXXRZ_ is defined, neither _ir_XXXXrZ_ (if defined) nor _ir_XXXXr_ (if
  defined) are executed.
- If there are no more keypress repeats detected the entry _ir_XXXXx_ will be searched in preferences and (if found) its value will be executed as 
  command sequence. As said above, the order will always be maintained, so _ir_XXXXx_ will always be last in a key press sequence. 
- Key release is detected after timeout. This timeout is set to 500ms which counts from the last valid key information reported to scanIR().

### Detailed IR-Example
For this example, I use a cheap generic ebay-IR from China.
The following buttons will be supported by assigned:
- (CH-) to switch channel down, (CH) to tune to another (random) channel, (CH+) to tune one channel up.
- (Vol-) and (Vol+) to decrease/increase the volume
- (Play/Pause) to toggle mute
- (PREV) and (NEXT) will be used to alter the channel assignment for 2 different users.
- Number keys (1) to (9) switch to Channel1..Channel9
(A few buttons are not assigned in this example).

Lets start with the (CHANNEL)-keys. Here _channel = down_ is executed, if (CH-) is pressed on remote, _channel = up_ if (CH+) has been pressed and
_channel = any_ if (CH) has been pressed. If (CH) is hold for about 2 seconds, the command _channel = any_ is executed again. And again if the key
is still pressed 2 seconds later... (If the channels are linked to reliable streams, that should give some time to listen before switching to the 
next channel).
For (CH-) and (CH+) no longpress events are defined.
```
ir_A25D = channel = down    # (CH-) pressed
ir_629D = channel = any     # (CH)  pressed
ir_629DR22 = channel = any  # (CH)  (repeatedly) longpressed for about 2 seconds
ir_E21D = channel = up      # (CH+) pressed
```

For the volume keys (and (Play/Pause)), the settings are similar:
```
ir_E01F = downvolume = 2    # (Vol-) pressed
ir_E01Fr = downvolume = 1   # (Vol-) longpressed
ir_A857 = upvolume = 2      # (Vol+) pressed
ir_A857r = upvolume = 1     # (Vol+) longpressed
ir_C23D = mute              # (Play/Pause)
```
So if (Vol-) or (Vol+) is pressed, volume will be decreased/increased by 2 on initial press and decreased/increased by 1 as long as the key is being
pressed at roughly 100ms intervals.

The setting for the 'Number-Keys' (1)..(9) is obvious (only 3 examples shown). There are no longpress-events used.
```
...
ir_42BD = channel = 7 # (7)
ir_4AB5 = channel = 8 # (8)
ir_52AD = channel = 9 # (9)
```

The 'tricky' part is the switching the user (and hence the channel assignment). I use a separate (NVS)-entry to store the commands list 
to be executed if switching between users. That way, the same sequence can be re-used later if the switch is happening by another input event.
By my convention, I use '$' sign as starting character for NVS-Keys that store constants for later use:
```
$user1 = channels=0,1,2,3,4,5,6,7,10;channel=1;volume=70
$user2 = channels=1,2,10,11,12,13,14,15,16;channel=1;
```
So if that commands are executed, for both users different presets would be assigned to Channel1 to Channel9, but in both cases the preset
referenced by Channel1 would be tuned to (i. e. _preset_00_ for _$user1_ and _preset_1_ for _$user2_). For _$user1_ the volume would also
be set to 70 (but will stay at current value if switching to _$user2_).

With these two settings the user switch by the intended remote-keys (PREV) and (NEXT) can be achieved:
```
ir_22DD = execute = $user1 # (PREV)
ir_02FD = execute = $user2 # (NEXT)
```

By the execute command, the contents of the value linked to _$user1_ or _$user2_ are executed as command sequence.

Now there is only one thing left to do: at start, no channels are defined. You should use _::start_ to define channel settings. The most 
obvious way is to force user1 at start:
```
::start = execute = $user1
```

If this assignment would not have been made, all (CH)-keys and the (1) to (9) keys would have no effect (until a user setting would have been 
done by pressing (PREV) or (NEXT)).

# Extended Input Handling
## General
- Inputs can be generated from Digital Pins, Analog Pins, Touch Pins or (some) internal variables
- For each of these Inputs, the handling is identical, the difference is just in the value range used:
  - Digital Input pins return 0 or 1
  - Analog Input pins return 0..4095
  - Touch Pins return 0..1023 (in theory, in practice the real values will be somewhere in between
    (For touch pins the native Espressif-IDE is used, that increases the granularity and accuracy of the readout by magnitudes)
  - For internal variables, the possible return range is -32768..32767 (int16 is used)
- The Inputs could either be read cyclic in the loop() or just on request
- In 'cyclic mode' a change event can be assigned that is executed on any change of the input detected.
- The 'physical readout' of an input can be mapped to a different value range.
- Each Input can be configured and reconfigured freely during runtime. 

## First input example: Volume 
We will assume the following setting for the example:
- A variable resistor is attached to an analog pin (lets say 39) in such a way, that it applies a voltage between 0 (if turned to low) and VCC (if 
turned to max).
- Accordingly, we want to  change the Volume between 0 and 100.
- However, since we know that with our device a volume setting between 1..49 is effectively not audible we would prefer the following mapping
   * Volume = 0 if resistor is turned to low (or close to low), say if the readout of the analog pin is between 0 and 100
   * Volume should be between 50..100 if the readout of the analog pin is between 101 and 4095

That can be achieved with the following to settings in the preferences:
```
$:volchange=volume=?
in.vol = src=a39,map=(101..4095=50..100)(0..100=0),event=$:volchange,start
```

Or, if you just want to try at runtime, you can also set this from the (Serial) commandline (but then settings will be lost at next power-up)
by entering the following lines:
```
ram.$:volchange = volume = ?
in.vol = src=a39,mode=28,map=(101..4095=50..100)(0..100=0),event=$:volchange,delta=2,start
```

The first line defines a _key_-_value_-Pair with _key_ set to _'$>volchange'_ and the corresponding _value_ set to _'volume = ?'_. 
If set in preferences as in first snippet that would be stored to NVS, into RAM in second snippet.
That assignment does not do anything for now, it is only used in the second line.

The second line contains a new command _'in'_.
- The command _'in'_ sets or updates the properies of a specific input.
- Each input has a name, that name is indicated by the string following the '.' after _in_
- In our example we have defined an input with the name 'vol'
- The _in_ command allows to set or change the "properties" of an input.
  * If more than one property is defined, they are separated by ','. If more than one property is set in this list, they are evaluated from left to right.
  * most properties take an argument, that argument is assigned as usual with _propery = argument_
  * The argument of a property is dereferenced to NVS/RAM if it starts with '@'
  * If _argument_ is not provided, it is assumed to be empty String ''.
  * Some properties (_start_ or _stop_ for instance) do not require an _argument_ (in that case, _argument_ will be ignored if set)
- The property _'src'_ specifies the link to an input
  * Here _'a39'_ is set. This means "use pin39 as analog input" There is no checking, if that is a valid (analog) pin. In practice, this assignment will result in an 
  _analogRead(39)_. (The setting _in.vol=src=a42_ would not result in any complaints, just render the input useless, as _analogRead(42)_ will always return _0_.)
- _mode_ is a property that details a few aspects of the physical input. 
  * It is specific (has a different meaning) to a specific input type.
  * _mode_ is bit-coded, and for analog input b0-b4 are evaluated (28 from the example is 0b11100 in binary):
  * b0: if set, readout is inverted (so 4095 if 0V are applied to input pin and 0 is returned if pin is at Vcc). Not inverted in our example.
  * b1: if set, pin is configured as Input with internal PullUp (_pinMode(INPUT_PULLUP)_), otherwise (as in this example) just as Input (_pinMode(INPUT)_).
  * b2..b4: set a filter size from 0 to 7 to smoothen out glitches/noise on the input line. The higher the filter is set (max 7 as in our example) the less noise
      will be on readout with the drawback that actual changes (user change of variable resistor for instance) will be less instantaneous but will take some time
      to propagate to the readout. For volume settings this is not a bad thing at all, and the maximum size of 7 is still fine with that usecase.
  * all other bits are reserved for future ideas and should for now be set to '0'.
  * So in our case the input is configured with the maximum filter size 7, not inverted, internal PullUp not set.  
- The property _map_ allows a re-mapping of the physical read input value arbitrary different range
  * a _map_ can contain any number of entries
  * each entry must be surrounded by paranthesis _'(x1..x2 = y1..y2)'_. _x1..x2_ is (a part) of the input range that will be transformed into the output range _y1..y2_
    > In our example, if _analogRead(39)_ returns something between 101 or 4095, the input itself will return something between 50 to 100. If _analogRead()_ returns
      less, (100 or less), the input will yield 0.
  * The arduino _map()_ function is used internally, so reversing lower and upper bounds or using negative numbers is no problem as long its within the limits of int16
  * _map_-entries can overlap both on the x and the y side. When actually performing the mapping, the entries are evaluated from left to right.
  * if a matching entry is found, that entry will be used for mapping. Further entries (to the right) are no longer considered.
  * '..y2' and/or '..x2' can be omnited. (so you can have just one number on either side). 
    > If there is only one entry in the _y_-part, all of the input range
      from _'x1..x2'_ will be mapped to that single _y_-Value (see second entry in the example with _'(0..100=0)'_. If there is only one entry in the _x_-part, that 
      value will be mapped to _y_ (a setting of _y2_ will be ignored in that case).
  * A single map entry can be read from NVS/RAM if it is derefenced by '@'. 
    > So _'(@x1..@x2=0)'_ would search for the keys _x1_ and _x2_ in NVS/RAM and would use 
      this values at the time the 'in' command is executed. The resulting map-Entry will keep these values, even if later _x1_ or _x2_ are changed.
- The property _event_ defines a key to NVS/RAM that is searched for if the input (after mapping) has changed since last check. If a change is detected,
  the NVS/RAM will be searched for _key_ and (if key is found) the corresponding value will be executed as command-sequence. (note that exactly the key name
  must be given here, do not dereference by adding '@')
  * you need to explicitely start cyclic polling of the input, see below.
  * within the _value_ associated to the event key, any occurance of the '?' will be replaced by the current reading of the 
- The property _delta_ defines the magnitude of change that is required on the (mapped) input for an change event to be triggered.
  * this allows, especially for analog input, to eleminate noise on the line that would lead to alternate readings and hence events on every other readout. Must be 
    one (any change in the input would lead to an event) or above. Will be forced to 1 if set by user to 0 or below.
- The property _start_ starts the cylic polling of the input. Only if polling is startet, the event will be generated on change.
  * _start_ results in a direct readout of the input. If an _event_ is defined, when _start_ is set (it is in our example), that event will be called direct (to do 
    an initial setting of volume at startup depending of the position of the variable resistor in our case). If that is not desired, _start_ must be called before 
    _event_ is set.
- The property _stop_ (not used in the example) is the opposite of _start_: it stops the cyclic polling (or does just nothing if cyclic polling has not yet started).

There are a few more properties. However, this explanation should be enough to understand what is needed to translate the changes on a variable resistor to 
an appropriate volume setting. If everything is connected correctly and if there were no typos in entering the commands (or the preference settings) above, it should
already work.

If not, it is time for some debugging. There are two properties, that are concerned with debugging:
- The property _info_ shows the settings of the input (no argument needed).
- The property _show = TimeInSeconds_ starts a cyclic output of the readout of the input. _TimeInSeconds_ is the distance between two obersvations in seconds. 
  If TimeInSeconds is 0 or not given, the output on Serial will stop.
  > The output will show regardless of the setting of DEBUG. If you want to concentrate on the Input-information, you probably want to stop all other output
    by entering the command _debug=0_ first.
    
If you enter the command _in.vol=info_ you should read:

```
20:28:38.956 -> D: Command: in.vol with parameter info
20:28:38.956 -> D: Info for Input "in.vol":
20:28:38.956 -> D:  * Src: Analog Input, pin: 39, Inverted: 0, PullUp: 0, Filter: 7
20:28:38.956 -> D:  * Cyclic polling is active
20:28:38.956 -> D:  * Value map contains 2 entries:
20:28:38.956 -> D:       1: (101..4095 = 50..100)
20:28:38.956 -> D:       2: (0..100 = 0..0)
20:28:38.956 -> D:  * Change-event: "$:volchange"
20:28:38.956 -> D:     defined in RAM as: "volume = ?"
20:28:38.956 -> D:  * Delta: 2
20:28:38.956 -> D:  * Show: 0 (seconds)
20:28:38.956 -> D:  * Debounce: 0 (ms)
```

If your output is different, you probably had a type somewhere. Please check and correct the settings.

If that is comparable to your readout, everything should work. If not, it is most likely a hardware setup issue (like shortening of the input to Vcc or Gnd or wrong
pin connected).

in that case, or just to have more insight, you should run the _in_-command with the property _show_set, for instance to 1, i. e. _in.vol=show=1_. 
That should result in somewhat like this:


```
20:38:29.086 -> D: Command: in.vol with parameter show=1
20:38:29.264 -> D: Command accepted
20:38:29.264 -> D: Input "in.vol" physRead= 2900 (mapped to:   85)
20:38:30.112 -> D: Input "in.vol" physRead= 2901 (mapped to:   85)
20:38:31.109 -> D: Input "in.vol" physRead= 2894 (mapped to:   84)
20:38:32.136 -> D: Input "in.vol" physRead= 2885 (mapped to:   85)
20:38:33.128 -> D: Input "in.vol" physRead= 2893 (mapped to:   85)
20:38:34.121 -> D: Input "in.vol" physRead= 2907 (mapped to:   84)
20:38:34.452 -> D: Input "in.vol": change to 86 (event "$:volchange")
20:38:34.631 -> D: Command: volume with parameter 86
20:38:34.631 -> D: Input "in.vol": change to 88 (event "$:volchange")
20:38:34.631 -> D: Command: volume with parameter 88
20:38:34.864 -> D: Input "in.vol": change to 90 (event "$:volchange")
20:38:34.864 -> D: Command: volume with parameter 90
20:38:35.094 -> D: Input "in.vol": change to 92 (event "$:volchange")
20:38:35.094 -> D: Command: volume with parameter 92
20:38:35.094 -> D: Input "in.vol": change to 94 (event "$:volchange")
20:38:35.094 -> D: Command: volume with parameter 94
20:38:35.329 -> D: Input "in.vol" physRead= 3654 (mapped to:   94)
20:38:35.556 -> D: Input "in.vol": change to 96 (event "$:volchange")
20:38:35.556 -> D: Command: volume with parameter 96
20:38:36.140 -> D: Input "in.vol" physRead= 3930 (mapped to:   97)
20:38:36.370 -> D: Input "in.vol": change to 98 (event "$:volchange")
20:38:36.370 -> D: Command: volume with parameter 98
20:38:37.133 -> D: Input "in.vol" physRead= 3929 (mapped to:   97)
20:38:37.894 -> D: Input "in.vol": change to 96 (event "$:volchange")
20:38:38.124 -> D: Command: volume with parameter 96
20:38:38.124 -> D: Input "in.vol" physRead= 3751 (mapped to:   95)
20:38:38.353 -> D: Input "in.vol": change to 94 (event "$:volchange")
20:38:38.353 -> D: Command: volume with parameter 94
20:38:38.353 -> D: Input "in.vol": change to 92 (event "$:volchange")
20:38:38.353 -> D: Command: volume with parameter 92
20:38:38.590 -> D: Input "in.vol": change to 90 (event "$:volchange")
20:38:38.590 -> D: Command: volume with parameter 90
20:38:38.590 -> D: Input "in.vol": change to 88 (event "$:volchange")
20:38:38.590 -> D: Command: volume with parameter 88
20:38:38.847 -> D: Input "in.vol": change to 86 (event "$:volchange")
20:38:38.847 -> D: Command: volume with parameter 86
20:38:39.076 -> D: Input "in.vol": change to 84 (event "$:volchange")
20:38:39.076 -> D: Command: volume with parameter 84
```


Here, for the first 6 seconds the volume changing resistor has not been touched. Note that there is however still some change on the input line (the physical read
value jitters between 2885 and 2907) that also results in a change to the mapped result, that changes from 85 to 84 and back. However, since the property _delta_ is
set to 2, that will not result in a change event. That will only happen after the change in the mapped value was >=2 (from 84 to 86) after 6 seconds. The volume was 
turned up to 98 and then back to 84.

While you have that output up and running, you can play around with the properties setting to understand some effects. 
Try _in.vol=mode=0_ to see how much noise is then on the input before going back to filter=7 again by _in.vol=mode=28_:

```
20:57:09.113 -> D: Command: in.vol with parameter mode=0
20:57:09.358 -> D: Command accepted
20:57:09.358 -> D: Input "in.vol" physRead= 2256 (mapped to:   76)
20:57:10.289 -> D: Input "in.vol" physRead= 2275 (mapped to:   77)
20:57:10.984 -> D: Input "in.vol": change to 78 (event "$:volchange")
20:57:10.984 -> D: Command: volume with parameter 78
20:57:10.984 -> D: Input "in.vol": change to 76 (event "$:volchange")
20:57:11.210 -> D: Command: volume with parameter 76
20:57:11.434 -> D: Input "in.vol" physRead= 2261 (mapped to:   77)
20:57:12.275 -> D: Input "in.vol" physRead= 2245 (mapped to:   76)
20:57:12.496 -> D: Input "in.vol": change to 78 (event "$:volchange")
20:57:12.496 -> D: Command: volume with parameter 78
20:57:12.496 -> D: Input "in.vol": change to 76 (event "$:volchange")
20:57:12.496 -> D: Command: volume with parameter 76
20:57:13.302 -> D: Input "in.vol" physRead= 2245 (mapped to:   76)
20:57:14.296 -> D: Input "in.vol" physRead= 2237 (mapped to:   76)
20:57:15.290 -> D: Input "in.vol" physRead= 2256 (mapped to:   76)
20:57:16.283 -> D: Input "in.vol" physRead= 2278 (mapped to:   77)
20:57:17.310 -> D: Input "in.vol" physRead= 2239 (mapped to:   76)
20:57:18.005 -> D: Command: in.vol with parameter mode=28
20:57:18.005 -> D: Command accepted
20:57:18.303 -> D: Input "in.vol" physRead= 2270 (mapped to:   77)
20:57:19.330 -> D: Input "in.vol" physRead= 2253 (mapped to:   76)
20:57:20.323 -> D: Input "in.vol" physRead= 2261 (mapped to:   77)
20:57:21.317 -> D: Input "in.vol" physRead= 2261 (mapped to:   77)
20:57:22.348 -> D: Input "in.vol" physRead= 2253 (mapped to:   76)
20:57:23.343 -> D: Input "in.vol" physRead= 2257 (mapped to:   76)
```

Whith _mode=0_ (means filter=0) the readout at the observation points was distributed between 2239 and 2278 (distance 39) and also some (unwanted) volume change
events occured. With _mode=28_ the distance was lower at 17 (2270 - 2253) and no volume change event has been generated. So _mode=28_ is probably the better setting.

While we are at it, play with _b0_ of mode. As effect, the physical readout should inverse, and the direction of increasing/decreasing voltage should reverse. Enter 
_in.vol=mode=29_ to see (hear) the difference. Switch back to 'normal' using the command _in.vol=mode=28_.

You can change every property of an input at any time. Whenever a property is changed, the others are not affected: a change to _mode_ as just done will not affect 
the _map_ (or any other entry). You could even switch to a different input-pin, if this makes any sense from application perspective.

Try: _in.vol=map=(100..2000=50..100)(2100..4000=100..50)(2000..2100=100)(0..4095=0)_

This will make that the volume at either end of the tuning range (from physical read 0..100 and 4000..4095) is 0 and will increase to max volume of 100 in the middle
if turned to the other side and will go down to 0 from the middle again. Is this a useful feature? Probably not, but its cool to be able to do this.


## Second example: tuning
If erverything is up and running, we can also chnage the input to have a totally different meaning: not to change the volume, but to change the preset.
That is normally achieved by a variable capacitor, but since a variable resistor is already at hand and only two input modifications are required (change from 
analog pin to touch pin and use a different mapping), we can use the input with the variable resistor as simulation.

First, stop the volume input using the command _in.vol=stop_

Then, create a tune-event: _ram.$:tune=preset=?_ and use this for the input: _in.vol=event=$:tune_
Next, simplify the mapping to: _in.vol=map=(0..4095=0..2)_ to just tune between presets 0 and 2. Set _delta=
Then, set delta to 1 and re-start the cyclic polling of the (and the debug show) input with the new mapping and event: _in.vol=start, show=1_
Verify the settings with _in.vol=info_, output on Serial should be this:

```
22:13:08.766 ->  * Src: Analog Input, pin: 39, Inverted: 0, PullUp: 0, Filter: 7
22:13:08.766 ->  * Cyclic polling is active
22:13:08.766 ->  * Value map contains 1 entries:
22:13:08.766 ->       1: (0..4095 = 0..2)
22:13:08.766 ->  * Change-event: "$:tune"
22:13:08.766 ->     defined in RAM as: "preset=?"
22:13:08.766 ->  * Delta: 1
22:13:08.766 ->  * Show: 1 (seconds)
```

If you play around, you will notice: it works in principle, but not quite. The first thing you will notice is that the mapped input value will be 0 for the first half
, 1 for the second half and only jump to 2 with the input tuned to a full physical reading of 4095. So the _map()_ function is skewed.

This can be solved with a first improvement of the input map:
_in.vol=map=(0..1365=0)(1366..2730=1)(2731..4095=2)_

This will solve the problem of the skewed distribution. However it does not solve another problem. Try to tune to a physical readout close to 1365 (or 2731, if there
is more noise on the input). You will notice a frequent toggle between presets. To increase the problem, set the filter to 0 by using the command _in.vol=mode=0_. 

To solve this problems, Gaps are permitted in the value map. If the current physical read falls within a gap, the input is considered to be unchanged. For the
first start, if the physical read is within a gap, the next matching value in the mapping entries closest to the physical readout is assumed.

_in.vol=map=(0..1165=0)(1566..2530=1)(2931..4095=2)_ should do the trick.

## Using touch input (also to read a variable capacitor)
### General precondition for reading variable capacitor values
The touch inputs of ESP32 can be used to read variable capacitors. The typical AM radios use variable capacitors in the range of around 300 pF. These can be
read through the touch pins. If using the native Espressif-IDF APIs for touch sensors the readings are stable and precise enough to give a reading to distinguish
for at least 10 positions (hence "tunable" stations) of the tuning knob.

One terminal of the capacitor must be connected to the pin direct, the other to ground (when recycling an old radio make sure any other connection to the 
capacitor is cut off, especially be sure that the coil of the oscillator circuit is no longer connected in parallel to the capacitor).

As a rule of thumb, the wire between capacitor and input pin should be as short as possible and as freely running as possible to improve the readings.
Especially avoid twisting with other wires if possible.

### Calibrating the input
The touch pin could return any reading between 0 and 1023 in theory. In practice, if a variable capacitor is connected, the reading would be somewhere in 
between. The reading will also change depending on the wiring (length and path) between capacitor and ESP32. So you probably will have different readings 
using the same capacitor on the breadboard and in the final product.

For undisturbed reading during calibration it is recommended to stop any other output to Serial by entering _DEBUG = 0_ on the Serial command input.

For our example we assume the capacitor to be connected to T9. To read the capacitor we can use the _'in'_ command and link the source to the touch pin T9
with the follwing line from the serial input:

_in.tune=src=t9,show=1_

That should give an output on Serial like this when tuning from highest to lowest capacity (or lowest to highest frequency on the associated radio scale):
```
21:12:30.398 -> D: Input "in.tune" (is stopped) physRead=  104
21:12:31.392 -> D: Input "in.tune" (is stopped) physRead=  105
21:12:32.385 -> D: Input "in.tune" (is stopped) physRead=  107
21:12:33.379 -> D: Input "in.tune" (is stopped) physRead=  117
21:12:34.406 -> D: Input "in.tune" (is stopped) physRead=  152
21:12:35.399 -> D: Input "in.tune" (is stopped) physRead=  190
21:12:36.392 -> D: Input "in.tune" (is stopped) physRead=  276
21:12:37.386 -> D: Input "in.tune" (is stopped) physRead=  394
21:12:38.379 -> D: Input "in.tune" (is stopped) physRead=  477
21:12:39.408 -> D: Input "in.tune" (is stopped) physRead=  480
```

The numbers are abritrary of course, your mileage may vary. In this example the reading range is between 104 and 480. You should observe a rather stable 
reading if the position of the tuning knob is not changed (jitter by one only, no matter what the tuning position is). You should also observe that the 
change is nonlinear. The higher the reading the less you have to turn for a change in the input readout.

For instance on my scale I have the the frequencies 530, 600, 800, 1200 and 1600 written on the AM scale. The corresponding readings are 113, 140, 215, 365
and 460 in my example. A possible tuning map (with some fishing range and gaps as discussed above) for 5 'channels' mapped to that positions could be defined
(in our example into RAM, for the final product it should be defined in NVS preferences) as:
_ram=$tuningmap=(0..120=1)(130..160=2)(180..250=3)(330..390=4)(420..600=5)_

This mapping must then be linked to the input:
_in.tune=map=$tuningmap_

The reading on Serial monitor should change to something like this (depending on mapping and the position of the capacitor):
```
22:31:28.766 -> D: Input "in.tune" (is stopped) physRead=  480 ( mapped to:    5)
22:31:29.759 -> D: Input "in.tune" (is stopped) physRead=  444 ( mapped to:    5)
22:31:30.786 -> D: Input "in.tune" (is stopped) physRead=  426 ( mapped to:    5)
22:31:31.812 -> D: Input "in.tune" (is stopped) physRead=  417 (nearest is:    5)
22:31:32.806 -> D: Input "in.tune" (is stopped) physRead=  411 (nearest is:    5)
22:31:33.800 -> D: Input "in.tune" (is stopped) physRead=  401 (nearest is:    4)
22:31:34.826 -> D: Input "in.tune" (is stopped) physRead=  394 (nearest is:    4)
22:31:35.820 -> D: Input "in.tune" (is stopped) physRead=  389 ( mapped to:    4)
22:31:36.813 -> D: Input "in.tune" (is stopped) physRead=  378 ( mapped to:    4)
22:31:37.807 -> D: Input "in.tune" (is stopped) physRead=  344 ( mapped to:    4)
22:31:38.800 -> D: Input "in.tune" (is stopped) physRead=  291 (nearest is:    4)
22:31:39.827 -> D: Input "in.tune" (is stopped) physRead=  223 ( mapped to:    3)
22:31:40.821 -> D: Input "in.tune" (is stopped) physRead=  167 (nearest is:    2)
22:31:41.815 -> D: Input "in.tune" (is stopped) physRead=  133 ( mapped to:    2)
22:31:42.841 -> D: Input "in.tune" (is stopped) physRead=  106 ( mapped to:    1)
22:31:43.835 -> D: Input "in.tune" (is stopped) physRead=  105 ( mapped to:    1)
```
If everything is expected, the calibration output could be stopped by issuing the command:
_in.tune=show=0_

Now the mapped reading can be used to switch presets. All what is needed to use this input to change presets is to start the cyclic polling of the preset
and assign a 'change event'. Lets start with the change event. That could be labelled '$switchpreset'. For this example it will be stored in RAM again:
_ram=$switchpreset=preset=?_

This event must be linked to the tune input by the command:
_in.tune=onchange=$switchpreset_

Check the settings by entering the command:
_in.tune=info_

The resulting printout on Serial should be:
```
2:38:39.762 -> D: Info for Input "in.tune":
22:38:39.762 -> D:  * Src: Touch Input: T9, Auto: 0 (pin value is used direct w/o calibration)
22:38:39.795 -> D:  * Input is not started (no cyclic polling)
22:38:39.795 -> D:  * Value map contains 5 entries:
22:38:39.795 -> D:       1: (0..120 = 1..1)
22:38:39.795 -> D:       2: (130..160 = 2..2)
22:38:39.795 -> D:       3: (180..250 = 3..3)
22:38:39.795 -> D:       4: (330..390 = 4..4)
22:38:39.795 -> D:       5: (420..600 = 5..5)
22:38:39.795 -> D:  * Change-event: "$switchpreset"
22:38:39.795 -> D:     defined in RAM as: "preset=?"
22:38:39.795 -> D:  * Delta: 1
22:38:39.828 -> D:  * Show: 0 (seconds)
22:38:39.828 -> D:  * Debounce: 0 (ms)
```

To actually make the radio change the preset by changing the tuning knob you need to start cyclic polling of the input:

_in.tune=start_

So if you are satisfied with those settings, the final entries in the NVS preferences could be given as follows to take effect at every start of the radio:
```
$tunemap =      (0..120 = 1)(130..160 = 2)(180..250 = 3)(330..390 = 4)(420..600 = 5)
# Arbritary key to store the map for later use

in.tune = src=t9,map=@$tunemap, onchange=:tune, start
# Use touch T9 as input, using the predefined $tunemap, execure ':tune' on change of input and start cyclic polling of the input

:tune = preset=?
# Called on change of tune input, '?' is replaced by (mapped) input value (so 1..5 in this example)
```

Or you could use the variable capacitor for different thinks, like changing the volume:
_ram=:vol=volume=?_
_in.tune=src=t9,map=(110..475=50..100)(0..109=0)(476..500=100),onchange=:vol,start_

Together with the [example above](#second-example-tuning) it is thus possible to use the frequency knob to change volume and the volume knob to change presets. What a crazy world!

Of course it is also possible to change the direction of volume increase/decrease by changing the input map:
_in.tune=map=(110..475=100..50)(0..109=100)(476..500=0)_


### Classic touch reading

If you want to use touch inputs as usual (detecting if touched or not) you can no longer use the *touch_*-settings of the preference. Any such setting from
the preferences will be ignored. (Reason is that for the *touch_* settings the touchRead() function of the ESP32 Arduino core is used which corrupts the 
readings of the ESP32 IDF API in my experience).

You can use the input mechanism provided here to mimic that behaviour. In the following example touchpin T8 is used.
First is, to define a touch input for T8:
_in.touch=src=t8,show=1_

Will show some output on Serial as such:

```
15:29:21.781 -> D: Input "in.touch" (is stopped) physRead=  856
15:29:22.807 -> D: Input "in.touch" (is stopped) physRead=  852
15:29:23.800 -> D: Input "in.touch" (is stopped) physRead=  245
15:29:24.794 -> D: Input "in.touch" (is stopped) physRead=  187
15:29:25.787 -> D: Input "in.touch" (is stopped) physRead=  845
15:29:26.814 -> D: Input "in.touch" (is stopped) physRead=  859
15:29:27.806 -> D: Input "in.touch" (is stopped) physRead=  846
15:29:28.799 -> D: Input "in.touch" (is stopped) physRead=  175
15:29:29.825 -> D: Input "in.touch" (is stopped) physRead=  162
15:29:30.818 -> D: Input "in.touch" (is stopped) physRead=  858
15:29:31.845 -> D: Input "in.touch" (is stopped) physRead=  859
15:29:32.838 -> D: Input "in.touch" (is stopped) physRead=  858
```

The higher readings (above 800 in this example) are while not touched (open), the lower readings (below 300) when touched. To convert the readings into 
digital information you could apply a map to the input:
_in.touch=map=(400..1023=1)(=0)_ 

With this map the input will read 1 if untouched and 0 if touched:
```
15:34:11.560 -> D: Input "in.touch" (is stopped) physRead=  857 ( mapped to:    1)
15:34:12.587 -> D: Input "in.touch" (is stopped) physRead=  621 ( mapped to:    1)
15:34:13.579 -> D: Input "in.touch" (is stopped) physRead=  182 ( mapped to:    0)
15:34:14.572 -> D: Input "in.touch" (is stopped) physRead=  274 ( mapped to:    0)
15:34:15.565 -> D: Input "in.touch" (is stopped) physRead=  806 ( mapped to:    1)
15:34:16.591 -> D: Input "in.touch" (is stopped) physRead=  205 ( mapped to:    0)
15:34:17.584 -> D: Input "in.touch" (is stopped) physRead=  145 ( mapped to:    0)
15:34:18.577 -> D: Input "in.touch" (is stopped) physRead=  816 ( mapped to:    1)
15:34:19.570 -> D: Input "in.touch" (is stopped) physRead=  856 ( mapped to:    1)
```

There is also a simplified way using the _mode_-property of the touch input. If bit b0 is set, the input is automatically converted to a binary reading. 
In our example, first delete the input-map by assigning an 'empty' string as map:
_in.touch=map=_

And then set bit b0 in the _mode_-property:
_in.touch=mode=1_

And the result will be something like this:
```
15:38:00.198 -> D: Input "in.touch" (is stopped) physRead=    1
15:38:01.191 -> D: Input "in.touch" (is stopped) physRead=    0
15:38:02.184 -> D: Input "in.touch" (is stopped) physRead=    0
15:38:03.177 -> D: Input "in.touch" (is stopped) physRead=    0
15:38:04.203 -> D: Input "in.touch" (is stopped) physRead=    1
15:38:05.197 -> D: Input "in.touch" (is stopped) physRead=    1
15:38:06.222 -> D: Input "in.touch" (is stopped) physRead=    1
15:38:07.215 -> D: Input "in.touch" (is stopped) physRead=    0
15:38:08.242 -> D: Input "in.touch" (is stopped) physRead=    0
15:38:09.235 -> D: Input "in.touch" (is stopped) physRead=    1
15:38:10.229 -> D: Input "in.touch" (is stopped) physRead=    1
```

Using the _mode_-bit b0 should be the preferred option if a touch-input shall be used as binary input (touched/not touched).




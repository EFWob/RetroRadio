# ESP32-Retroradio
This project is based on the excellent ESP32-Radio of Edzelf.

Please have a look there to get you started.

It is called Retroradio, because the basic idea was to use an existing "retro radio" as 
amplifier and front-end for the ESP32-radio.

These type of radio usally have:
  - a power LED
  - a switch for selecting AM/FM/AUX
  - a volume knob (variable resistor)
  - a tuning knob (variable capacitor) 
  
In the original version that type of inputs and outputs are not supported directly. This
version here now supports:
   
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
and use your standard preference settings as before you should notice only little difference.

There are also some enhancements which are not necessarily associated with the extended input capabilities:
 - Ethernet can be used (I had to place one radio at a spot with weak WLAN reception)
 - Philips RC5 protocol is implemented in addition to the NEC protocol. (you can configure in the preference
	  to use both or only either one of that protocol. By default both are enabled)
 - IR remote handling has been extended to recognise longpress and release events
 - When assigning commands to an event (IR pressed, touch pressed or new events like tune knob turned) you
	  can execute not only a single command, but command sequences also. A command sequence is a list of commands
	  separated by _;_
 - when a value for a command is evaluated, you can reference an additional nvs element by _@key_. Simple 
	  example: if the command _volume = @def_volume_ is encountered, the preferences are searched for key _def_volume_ 
	  which will be used as paramter (if not found, will be substituted by empty string)

# Generic additions
Generic additions are not specific to the Retro radio idea but can be used in general with the ESP32 Radio.

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


## Storing values into RAM (or NVS)
In the original version, _key_-_value_-pairs are stored into NVS. There is now a way to store such pairs into RAM as well. So from now on, 
if the text references to _key_ or _value_ associated to _key_, keep in midn that the actual storage 
location can either be in NVS as usual or in RAM. (If a specific _key_ exists in both NVS and RAM, the one in NVS will be used.
TODO:: may be it is better the other way round!?).
- NVS pairs can be set using the preferences as usual
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
- you can list the RAM/NVS command with the commands _nvslist=Argument_ or _ramlist=Argument_. _Argument_ is optional, if set only keys that 
  contain _Argument_ as substring are listed. Try _ramlist=tst_ for example.
- TODO:: there is no command for deleting RAM or NVS-entries.

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
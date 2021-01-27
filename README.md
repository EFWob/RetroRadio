# ESP32-Retroradio
This project is based on the excellent ESP32-Radio of Edzelf.

Please have a look there to get you started.

It is called Retroradio, because the basic idea was to use an existing "retro radio" as 
amplifier and front-end for the ESP32-radio.

These type of radio usally have:
  - a power LED
  - a switch for selecting AM/FM/AUX
  - a volume knob (variable resistor)
  - a tuning knob (variable resistor) 
  
In the original version that type of inputs and outputs are not supported directly. This
version here now supports:
   
   - analog input (i. e. to control volume by turning the variable resistor attached to
     the volume knob)
   - extended touch input (i. e. to evaluate the position of a variable capacitor to switch
     presets accordingly)
   - led output. Up to 10 LEDs can be controlled with PWM capability
   - the functionality of digital inputs and touch inputs can be extended to define reactions
     to double-click like events (or longpress)
	 
All of these options can be configured by the preference settings. So if you compile this version
and use your standard preference settings as before you should notice no difference.

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
There is a define now in the very first line of ***RetroRadio.ino*** that reads '#define ETHERNET 1'
	
* this will compile **with** Ethernet support
* if changed to '#define ETHERNET 0' (or any value different from '1'), support for Ethernet is **not** compiled
* if this line is deleted/commented out, Ethernet support will be compiled depending on the Boards setting in the Tools menu of Arduino IDE.
	  Currently, only the settings **OLIMEX ESP32-PoE** and **OLIMEX ESP32-PoE-ISO** would then compile Ethernet support.
* if compiled with Ethernet support by the above rules, Ethernet can then be configured at runtime by preferences setting.
* if compiled with Ethernet support, there is another define that controls the ethernet connection: '#define ETHERNET_CONNECT_TIMEOUT 5'
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
1. "eth_phy_addr" to override the '#define' for ETH_PHY_ADDR
1. "eth_phy_power" to override the '#define' for ETH_PHY_POWER
1. "eth_phy_mdc" to override the '#define' for ETH_PHY_MDC
1. "eth_phy_mdio" to override the '#define' for ETH_PHY_MDIO
1. "eth_phy_type" to override the '#define' for ETH_PHY_TYPE
1. "eth_clk_mode" to override the '#define' for ETH_CLK_MODE

The problem here is, that *eth_phy_type* and *eth_clk_mode* are enums. They are defined in the sdk include file ***esp_eth.h***. In the preferences they
are expected as int-type. For the current core 1.0.4 implementation the mapping is defined as follow:

`
typedef enum {
    ETH_CLOCK_GPIO0_IN = 0,   /*!< RMII clock input to GPIO0 */ 
    ETH_CLOCK_GPIO0_OUT = 1,  /*!< RMII clock output from GPIO0 */
    ETH_CLOCK_GPIO16_OUT = 2, /*!< RMII clock output from GPIO16 */
    ETH_CLOCK_GPIO17_OUT = 3  /*!< RMII clock output from GPIO17 */
} eth_clock_mode_t;
`

and

`
typedef enum {
    ETH_MODE_RMII = 0, /*!< RMII mode */
    ETH_MODE_MII,      /* equals 1 !< MII mode */
} eth_mode_t;
`

**For future/different core versions that assignment might change.**


## Command handling enhacements
When a command is to be executed as a result of an event, you can now not just execute one command but a sequence of 
command. Commands in a sequence must be seperated by ';'. 
Consider a "limp home" button on the remote, which brings you to your favorite station (_preset_0_) played
at an convenient volume (_volume = 75_). So you can define in the preferences _ir_XXXX = volume = 75; preset = 0_
to achieve that.
Command values can also be referenced to other preference settings. So if you have a setting _limp_home = volume = 75; preset = 0_
in the preset, you can reference that by using "@": _ir_XXXX = @limp_home_ (spaces between "@" and the key name, here _limp_home_,
are not permitted). If the given key does not exist, an empty string is used as replacement.
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




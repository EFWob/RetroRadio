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
 - IR remote handling has been extended to recognise longpress events
 - When assigning commands to an event (IR pressed, touch pressed or new events like tune knob turned) you
	  can execute not a single command only, but command sequences also. A command sequence is a list of commands
	  separated by _;_
 - when a value for a command is evaluated, you can reference an additional nvs element by _@key_. Simple 
	  example: if the command _volume = @def_volume_ is encountered, the preferences are searched for key _def_volume_ 
	  which will be used as paramter (if not found, will be substituted by empty string)


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

### Added support for longpress on IR-Remotes
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
- if you define an event with the pattern _ir_XXXXRZ_ where Z is again a decimal number and R is in fact the uppercase R you define a recurring event.
  The repeat counter will reset to 0. So in the Vol+ example above: if you define _ir_XXXXR3 = upvolume = 1_ the command _upvolume = 1_ will be 
  called at slower rate (roughly every 0.3 seconds instead every 0.1 seconds). *This really resets the repeat counter.* That means any command 
  with a repeat counter > 3 in this example will never get executed. If _ir_XXXXRZ_ is defined, neither _ir_XXXXrZ_ (if defined) or _ir_XXXXr (if
  defined) are executed.


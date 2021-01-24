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
	  
	  

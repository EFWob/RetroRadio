# Introduction to Retroradio
## Summary
This project is based on the ESP32-Radio by Edzelf.

This project extends the capabilities of the ESP32-Radio, so you will need to familiarise with this project first.

The current state is WorkInProgress. This README is up to date, but the source code documentation is not. Some
refactoring is needed, and also some rework. The plan is however, to keep the functionality described here as is.  
I also try to commit only working versions to the master branch.

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
     the volume knob. One additional line in the preferences settings will achieve that.)
   - extended touch input (i. e. to evaluate the position of a variable capacitor to switch
     presets accordingly). Also one additional line only)
   - TODO: led output. With PWM capability / blink pattern support
   - the functionality of inputs can be extended to define reactions
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
in the section [Generic additions](#generic-additions) below.

## Considerations for migration

If you have the ESP32 radio installed and preferences set up, so it is up and running, the RetroRadio software should run out of the box
with a few things to notice:

- This version needs some more NVS-entries. If you are using already much space in NVS (namely if you have a lot of presets defined) you might
  need to increase the size of the NVS-partition. 
- You can use the command _test_ to check if there are still entries in NVS available. If the number of free entries is close to zero, or
  you see something like *nvssetstr failed!* in Serial output, or if entries have vanished from your preference settings, you are probably 
  out of NVS-space.
- for 4MB (standard) flash sizes I use the partion-table _radio4MB_default.csv_ that can be found at the _/etc_ folder in this repository. This layout is not compatible to the _default_ partition, notably for the NVS section. That means you must reload the default preferences (and edit the wifi credentials) if you flash using this partition table.
- Currently it is tested in platformio-environment only. The platformio.ini file has already some entries. Mainly to maintain different radios (with differen MCUs and pinouts). For starters, you can try the environment _plain devkit_, which uses the default partition table and thus should keep your NVS-settings.

# Generic additions
## Summary
Generic additions are not specific to the Retro radio idea but can be used in general with the ESP32 Radio. You can skip to section [Extended
Input Handling](#extended-input-handling) if you are not interested to use the following features for now:
 - [Ethernet](#ethernet-support) can be used (I had to place one radio at a spot with weak WLAN reception)
 - [Philips RC5 protocol](#added-support-for-rc5-remotes-philips) is implemented to be used in addition/instead of to the NEC protocol. 
 - A [channel concept](#channel-concept) is introduced to simplify a re-mapping of presets.
 - Radio can now play from [genre playlists](#genre-playlists) that can be downloaded from a internet radio database server.
 - [IR remote handling](#added-support-for-longpress-and-release-on-ir-remotes) has been extended to recognise longpress and release events
 - When assigning commands to an event (IR pressed, touch pressed or new events like tune knob turned) you
	  can execute not only a single command, but command sequences also. A command sequence is a list of commands
	  separated by _;_
 - when a value for a command is evaluated, you can dereference to a RAM/NVS entry by _@key_. Simple 
	  example: if the command _volume = @def_volume_ is encountered, then RAM/NVS are searched for key _def_volume_ 
	  which will be used as paramter (if not found, will be substituted by empty string)

## Storing values into RAM (or NVS)
In the original version, _key_-_value_-pairs are stored into NVS. There is now a way to store such pairs into RAM as well. So from now on, 
if the text references to _key_ or _value_ associated to _key_, keep in mind that the actual storage 
location can either be in NVS as usual or in RAM. (If a specific _key_ exists in both NVS and RAM, the one in RAM will be used. This allows
to simply override a 'default setting' in NVS by storing the same key in RAM). 

NVS keys can be defined in the preferences as usual.

- There is a command _nvs_ now to set an NVS-Entry. Syntax is _nvs.key = value_. That will set the preference entry _key_ to the specified
  value (or create it, if it did not exist before). NVS (or RAM entries) can be dereferenced to be used as arguments for commands.
  For details see [summary below](#storing-and-retrieving-values), for now it is sufficient to know that _@key_, if used as an argument of
  a command, will replaced by the value of that _key_ stored in RAM, or, if it does not exist in RAM, with the value assigned to _key_ in 
  the preferences, or the empty string "", if defined in neither RAM nor NVS.
- There is a command _ram_ now to set an RAM-Entry. Syntax is equivalent to the _nvs_-command above. 
- In both cases, if _key_ exists the value associated to set key will be replaced. Otherwise the _key_-_value_-pair will be created and stored.
- Example:
  * _ram.tst1 = 42_ will create a RAM-Entry with _key_ == _tst_ and _value_ == _'42'_
  * _ram.tst2 = @tst1_ will create entry _tst2_ in RAM with the _value_ == _'42'_
  * The susbtitution for the dereferenced key _@tst1_ is done at command execution time. If _tst1_ will change in value later, _tst2_ will
    still stay at _'42'_.
- you can list the RAM/NVS entries with the commands _nvs?=Argument_ or _ram?=Argument_. _Argument_ is optional, if set only keys that 
  contain _Argument_ as substring are listed. Try _ram?=tst_ for example.
- RAM or NVS entries can be deleted using the command _nvs- = key_ or _ram- = key_ which will delete _key_ and the associated value from RAM/NVS.
  * WARNING: you probably do not want to delete your presets from NVS. There is no "Are you sure"-popup. The entry will be deleted.

## Command handling enhacements
When a command is to be executed as a result of an event, you can now not just execute one command but a sequence of 
command. Commands in a sequence must be seperated by ';'. 
Commands in a sequence are executed from left to right.
Consider a "limp home" button on the IR-remote, which brings you to your favorite station (_preset_0_) played
at a convenient volume level (_volume = 75_). So you can define in the preferences _ir_XXXX = volume = 75; preset = 0_
to achieve that.
Command values can also be referenced to other preference settings. So if you have a setting _limp_home = volume = 75; preset = 0_
in the NVS-preferences, you can reference that by using "@": _ir_XXXX = @limp_home_ (spaces between "@" and the key name, here _limp_home_,
are not permitted). If the given key does not exist, an empty string is used as replacement.

## The call-command
The _call_-command takes the form _call = key-name_. If _key-name_ is defined (RAM/NVS with RAM overriding NVS) the associated value is
retrieved and executed as commands-list (whith each command being seperated by semicolon).

## Setup event
If in NVS-preferences the key-value-pair _::setup = key-name_ is defined, the value associated _key-name_ is retrieved from NVS and executed as 
commands-list. The _::setup_-'calls' will happen after everything else in _setup()_ is finished just before the first _call to loop()_ 
(So network is up and running, preferences are read, VS module is initialized. I. e. ready to play).

If you set _::setup = volume = 70;preset = 0_ in the preferences, then at start the radio will always tune to _preset_0_ and set the volume 
to level 70. (If one line is not enough, you can also use _::setup0_ to _::setup9_ for executing additional command-sequences, see 
[below](#scripting-summary)) .

## Loop event
If the key-value-pair _::loop = key-name_ is defined, the associated contents of _key-name_ are retrieved from NVS and executed as command-list.
This is called frequently (directly from main _loop()_ of the program). Activities here must be short to avoid delays in playing the stream.
For convenience (if more then one line is needed) _::loop0_ to _::loog9_ can be used in addition.

More on this (and on control flow in general) will be introduced if we get along with the examples and is [summarized below](#scripting-summary).
 

## Ethernet support
### Caveat Emptor
Ethernet support is the most experimental feature. It is just tested with one specific board, namely the Olimex Esp POE.
I am pretty sure it will work the same for the Olimex Esp32 POE ISO.
I have not tested it for any other hardware configurations, but it should work with any hardware combination that is based
on the native ethernet implementation of the Esp32 chip. 

**USE AT YOUR OWN RISK!**

Note that the initial setup (of the preferences) must be done through the access point as usual. Please do not connect the ethernet to the LAN as then the access point will not show and you cannot run the initial configuration.

### Compile time settings for Ethernet
There is a define in ***addRR.h*** that reads `#define ETHERNET 2`
	
* this will compile **with** Ethernet support, if the selected board is known to have Ethernet capabilities. As of now, that is true
  for Olimex PoE/PoE ISO only
* if changed to `#define ETHERNET 0` (or any value different from '1' or '2'), support for Ethernet is **not** compiled
* if changed to `#define ETHERNET 1`, support for Ethernet is compiled
* if this line is deleted/commented out, Ethernet support will **not** compiled in.
* if compiled with Ethernet support by the above rules, Ethernet can then be configured at runtime by preferences setting.
* if compiled with Ethernet support, there is another define that controls the ethernet connection: `#define ETHERNET_CONNECT_TIMEOUT 5`
	  that defines how long the radio should wait for an ethernet connection to be established (in seconds). If no IP connection over 
	  ethernet is established in that timeframe, WiFi will be used. That value can be extended by preference settings (but not lowered). In
	  my experience 4 seconds is too short. If the connection succeds earlier, the radio will commence earlier (and will not wait to consume
	  the full timeframe defined by 'ETHERNET_CONNECT_TIMEOUT'). 
* the following defines are used. They are set to default values in 'ETH.h' (and 'pins_ardunio.h' for ethernet boards). If you need to change
	  those (not for **OLIMEX ESP32-PoE...**), you need to re-define them before '#include ETH.h' (search in ***addRR.h***) or set them in the 
	  preferences (see below):
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
are expected as int-type. For the current core 1.0.6 implementation the mapping is defined as follow:

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
commands can be defined either by preference settings or through the Input channels at runtime, i. e. Serial input).

- _channels = comma-delimited-integer-list_ will (re-)define the channel list. In the list numbers (decimal) are expected which are treated as 
   reference to _preset_X_ in the preferences. So _channels = 1, 2, 10, 11, 12, 13, 14, 15, 16_ defines 9 channels in total with Channel1 mapped to 
   _preset_1_ to Channel9 mapped to _preset_16_
- That assignment does not change the current preset. To use that channel-list (i. e. to switch channel), the command _channel = Argument_ must be
    used.
- Argument can be:
  - A number between '1' and 'max' whith 'max' being the number of channels as defined by the command _channels_ above. In our example '9'. This
    will change the preset accordingly (but only if different from current channel).
  - The word 'any': chose a random preset from the channellist. (It is guaranteed that the chosen preset will be different from current preset.)
  - The word 'up': if the current preset is not the last channel in channel-list, tune to next preset in channel-list. (if the current preset is not 
    in the channel list, i. e. if tuned to by other means, tune to Channel1).
  - The word 'down': if the current preset is not the first channel in the channe-list, tune to previous preset in channel-list. (if the current preset 
  	is not in the channel-list, i. e. if tuned to by other means, tune to ChannelMax).
  - The number -1. This will not do any change to the current playing, but can be useful to force the radio to switch to a specific channel (which would not happen if that channel was the same that has been set by the previous call to the channel-command). 

## Genre playlists
### General idea
The main idea is to use a public radio database server (https://www.radio-browser.info/#/). This database has more
than 27,000 stations organised in different categories (by reagion or genre tags). With this addition, you can 
download stations from that radio database as given by their genre-tag. As a result you can have more station lists
(in addition to the default preset list in NVS) to chose from.
By now, that lists are organized by genre tag only. So you can create lists like "Pop", "Rock", etc. but not for instance by country.
### Creating genre playlists
A specific playlist can have any number of stations, as long as there is free flash left in the flash file system.
Unlike the preset list, the station URLs are not entered direct, but are downloaded from the database above. 
For maintaining the genre playlist, you need to open the URL http://RADIO-IP/genre.html. The first time you do so
you should see the following. 
![Web API for genre](pics/genre0.jpg?raw=true "Empty genre playlists")

There are three main parts on that page:
- on top (currently empty) is the list of genres that are already loaded into the radio.
- the center section (between two bar lines) allows to transfer the genre lists from another radio to this radio.
- the bottom section is the interface to the internet radio database.

You first should start with the interface to the radio database. In the leftmost input, enter (part) of a genre tag name, e. g. "rock". Enter a minimum number of stations that should be returned for each list (if the number of 
stations is less than that number, that specific list is not returned). Can be left empty, try 20 for now. If you press "Apply Filter" (ignore the right input field for now) you should see the result of the database request after 
a few moments:

![Web API for genre](pics/genre1.jpg?raw=true "Radio database answered the request")

The result list at the bottom just shows the result of the request, the lists are not yet loaded to the radio.
In the dropdown at the right side of the result list you can choose which entries you want to "Load" to the radio.
When decided, press the button labelled "HERE" at the bottom of the page. Only then the station lists selected will
be loaded into the radio. The website will show the progress:

![Web API for genre](pics/genre2.jpg?raw=true "Transfer from radio database to radio")

While you see that page, do not reload the page or load any other page into this tab as otherwise transfer will be
cancelled. If you accidentially cancel the tansfer, the radio will still be in a consistent state. Just the list(s) of stations will be truncated/incomplete.
After the transfer is completed, the page will automatically reload and should look like this:

![Web API for genre](pics/genre3.jpg?raw=true "First genre playlists loaded to radio")

More often than not, you will notice that the stations of your interest are distributed over several genre groups on 
the database. Like in our example "indie rock", "pop rock" and "progressive rock" we just loaded would be an example of "subgenres" for rock. 

As you will see later, you can only select one of those genres. You can however cluster the result list from the database into a "Cluster" called "Rock". There is only requirement for the clustername: it must start with a letter. And that first letter will be converted into uppercase automatically. All genre names in the database are returned in always lowercase letters. That way, you can have a cluster called "Rock" that can be distinguished from the "native" genre "rock".

So instead of loading 3 seperate genres, you could chose the Action "Add to:" for the genres of choice. You must enter the desired cluster name ("Rock" in our example) into the input field right of the button "Apply Filter".

![Web API for genre](pics/genre4.jpg?raw=true "Creating cluster Rock from 3 genres")


Clusters must not be created in one step, you can always add more stations from another database request later.
You can however not delete a subgenre from a cluster (but only delete the whole cluster).
If you press on the station number of a Cluster in the page section "Maintain loaded Genres" a popup will show which genres from the database are clustered into.

![Web API for genre](pics/genre5.jpg?raw=true "In the maintenance section")


In the maintenance section you can also Delete or Refresh any or all of the genres loaded into the database (a cyclic refresh is needed to throw out station URLS that got nonoperational in between).

If you click on a genre name on the left side in this list, the radio will play a (random) station from that genre.
(If you press again, another random station from that list will be played).
That is currently the only way to play from a genre using the web-interface.

Some details:
Each genre name (either if coming direct from the database or a user defined clustername) must be unique. From the database that is guaranteed. The API does not allow to download the same genre twice. Clusternames are different 
from loaded genre names because they always start with an uppercase letter. You can use the same cluster name again in further database requests to add more genres into it. You cannot add the same genre twice to the same cluster, but you can add the same genre to more than one cluster.
You cannot edit the resulting playlist any further. You can not add single station URLs "by hand", you can not delete a genre from a cluster. You can only delete the full cluster (or a whole genre).

With the default partition the file system is big enough to support (estimate) between 10,000 and 12,000 stations in total. (With the radio4MB_default partition it is still somewhat in between 8,000 and 10,000). For instance the radio that I just use for debugging has a total of 3182 stations stored which consumes 360,448 of 1,114,112 bytes of the flash file system leaving more than 66 per cent free for further playlists.


The interface can hande extended unicode characters. The only thing thats not working currently is if the genre name of the database contains the '/'-character. That I have seen on one genre so far and already forgotten again what it was, so it is currently no priority...

![Web API for genre](pics/genreunicode.jpg?raw=true "Unicode is fine")

Once you have one radio up and running with the playlists to your liking, you can simply transfer that settings to a new radio by opening http://NEWRADIO-IP/genre.html and entering the OLDRADIO-IP in the input field in the middle section and press the button Synchronize. If that connection was successfull, a dialog will pop up to verify you really want to transfer the settings. If you press OK you can just wait to see the magic unfold.

### Using genre playlists
A playlist can be selected by the command interface by using the command
_genre=Rock_
to play one genre playlist. If that genre exists (as a cluster in this example, but could also be a native genre name from a direct download). 
**Remeber that genre playlist names are case sensitive.** Genres loaded from database direct will always have 
lowercase letters only, while cluster names start with an uppercase letter (followed by only lowercase letters).
So, _Rock_ is a valid name for a cluster, _rock_ is a valid name for a genre tag from the internet radio database,
but constucts like _ROCK_ or _rOcK_ are invalid. When in doubt, copy the name from the web API.

If a valid genre name has been assigned (and that genre exists in SPIFFS), the radio will start to play a random station from that genre. If the same command is issued again (with the same name), another (random) station from that genre will play.

You can switch to a station direct using the command 

_gpreset=Number_  

where Number can be any number. If this number (lets call it n) is between 0 and 'number of stations in that genre - 1', the n-th entry of that list is selected. N can also be greater than the number of stations (or even below zero), modulo function is used in that case to map n always between 0 and 'number of stations in that genre' - 1.

If no genre has been set by the _genre=XXXX_ command above, _gpreset=n_ has no effect.

So selecting a station is still "fishing in the dark", but it is at least reproducible (if you find that favorite station of yours at index 4711).

If you issue command _genre=Anothername_ with another genre name, the radio will switch to that genre.

If you issue command with no name (empty), the genre playmode will be stopped. The current station will continue to
play (until another preset or channel is selected). _gpreset=n_ however will have no effect. You can also issue the command _genre_ with parameter _genre=--stop_ to achieve the same if you prefer a more clear reading.

You can also switch stations within a genre using the _channel_ command from above. To do so, a channel-list must be defined.
The preset-numbers assigned to the channel entries are ignored, just the size of the channel list is important.
In the channel example above we defined a channel list with 9 channels. If you switch to genre-playmode, that channel
list can be used to change stations within that genre by the following algorithm:
- each channel has a random number between 0 and 'number of channels in that genre - 1' assigned.
- one of the channel entries is the current channel (the one last selected by _channel=n_)
- the distance between two channel stations is the same for all neighbors (and wrapped around between channel 9 and 1) in our example. So if in our case we would have a genre list with 90 stations, the distance between two channels would be 10.
- example list in that case could be Channel 1: 72, Channel 2: 82, Channel 3: 2, Channel 4: 12 .... Channel 9: 62
- that assignment stays stable until:
  - a new random station is forced by the command _genre = Xxxxxx_, this will result in a completely different list.
  - a new station is forced by the command _gpreset = n_
  - the command _channe=up_ is issued. The numbers associated to the channels are increased by one (as well as the currently playing station from the genre will be switched to next in list)
  - the command _channel=down_ is issued. The numbers in the channellist are decreased by 1 (also for the current station)
  - the command _channel=any_ is issued. This will result in playing another (random) station and will also result in
    a completely different list.


Some fine detail (ToDo): if the radio stream stalls, the radio has a fallback strategy to switch to another preset. If that happens when playing from a genre playlist, the radio would fall back to a preset from the preset list in preferences.
You will notice that in genre playlist mode this can happen (for remote stations with a bad connection). In a next step, this fallback needs to be recoded to use another station from the current genre playlist. If you want to avoid the fallback to a preset from NVS, use the command _preset=--stop_. This will block fallback to a station from presets until a station from presets is requested (for instance by _preset=n_  or _channel=m_ if genre play mode has stopped).

When in genre play mode, you can still issue a _preset=n_ command and the radio will play the according preset from the preferences. However, genre playlist mode will not be stopped: the command _gpreset=n_ as well as _channel=m_ (if channellist is defined) will still operate on the current genre playlist.

### Configuring anything around genres

To configure genre settings use the command 

_gcfg.subcommand=value_

The command can be used from command line or from the preference settings in NVS. If not set in the preferences (NVS) all settings are set to the defaults described below.

The following commands (including subcommands) are defined:

- _gcfg.path=/root/path_ All playlist information is stored in Flash (using LITTLEFS) or on SD-Card. If path value does start with 'SD:' (case ignored), the genre lists will be stored on SD card using the path following the token 'SD:'. If not, the genre information is stored in Flash, using LITTLEFS with path '/root/path' in this example.
No validity checking, if the given path does not exists or is invalid, genre playlists will be dysfunctional. (For historic reason, *defaults to '/____gen.res/genres'*). Must not end with '/'! Can be changed at any time (to allow for different genre playlists for different users).
- _gcfg.host=hostURL_ Set the host to RDBS. *Defaults to 'de1.api.radio-browser.info'* if not set. 'de', 'nl', 'fr' can be used (as short cuts) to address 'de1.api.radio-browser.info', 'nl1.api.radio-browser.info' or 'fr1.api.radio-browser.info' respectively. Otherwise full server name must be given.
- _gcfg.nonames=x_ if x is nonZero, station names will be stored in genre playlists (not just URLs). Currently, station names from genre playlists are not used at all but might be useful in future versions. When short on storage, set to '1'. *Defaults to 0*, so station names will be stored.


## IR remote enhancements
### Added support for RC5 remotes (Philips)
Now also RC5 remotes (Philips protocol) can be decoded. RC5 codes are 14 bits, where the highest bit (b13) is always 1.
Bit b11 is a toggle bit that will change with every press/release cycle of a certain key. That will make each key to generate
two different codes. To keep things simple that bit will always be set to 0.


So if you have a RC5 remote and and pin_ir_rc5 (or pin_ir_necrc5, see below) is defined in preferences and a VS1838B IR receiver is connected to that pin, you
should see some output on Serial if a key on the remote is pressed (and debug is set to 1 of course). The code reported should
be in the range 0x2000 to 0x37FF. (Also you should see "(RC5)" mentioned in the output. You can attach commands to the observed codes
with the usual ir_XXXX scheme (or the extended repeat schemes below).

If you want your radio to react on NEC or RC5 codes only, change the pin-setting in the preferences as follows:
- use pin_ir = x to use only NEC-decoder 
- use pin_ir_rc5 = x to use only RC5-decoder
- use pin_ir_necrc5 = x if you want to use both protocols at the same time. Be aware that I have seen occasional
  crashes during "wild monkey testing" the remote, so use at your own risk.
- only one of pin_ir, pin_ir_rc5, pin_ir_necrc5 should be defined. If more than one is defined, the preference is pin_ir, pin_ir_rc5 and pin_ir_necrc5.
  (so if for instance pin_ir and pin_ir_rc5 are defined, the setting for pin_ir_rc5 is ignored and only NEC protocol is decoded using the pin specified by pin_ir)

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
(By my convention, I use the ':' as starting character for NVS-Keys that store for later execution.)
```
:user1 = channels=0,1,2,3,4,5,6,7,10;channel=1;volume=70
:user2 = channels=1,2,10,11,12,13,14,15,16;channel=1;
```
So if these command sequences are executed, for both users different presets would be assigned to Channel1 to Channel9, but in both cases the preset
referenced by Channel1 would be tuned to (i. e. _preset_00_ for _:user1_ and _preset_1_ for _:user2_). For _:user1_ the volume would also
be set to 70 (but will stay at current value if switching to _:user2_).

With these two settings the user switch by the intended remote-keys (PREV) and (NEXT) can be achieved:
```
ir_22DD = call = :user1 # (PREV)
ir_02FD = call = :user2 # (NEXT)
```

By the _call_ command, the contents of the value linked to _:user1_ or _:user2_ are executed as command sequence (If either key is defined
both in RAM and NVS-Preferences, the RAM setting will override the NVS setting).

Now there is only one thing left to do: at start, no channels are defined. You should use _::setup_ to define channel settings. The most 
obvious way is to force user1 at start by adding the following entry to the NVS preferences:
```
::setup = call = :user1
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
- In 'cyclic mode' a three different change events can be assigned that are executed on any change, if the Input goes to zero or if the Input
  leaves zero.
- The 'physical readout' of an Input can be mapped to a different value range.
- Each Input can be configured and reconfigured freely during runtime. 

## First Input example Volume 
We will assume the following setting for the example:
- A variable resistor is attached to an analog pin (lets say 39) in such a way, that it applies a voltage between 0 (if turned to low) and VCC (if 
  turned to max).
- Accordingly, we want to  change the Volume between 0 and 100.
- However, since we know that with our device a volume setting between 1..49 is effectively not audible we would prefer the following mapping
   * Volume = 0 if resistor is turned to low (or close to low), say if the readout of the analog pin is between 0 and 100
   * Volume should be between 50..100 if the readout of the analog pin is between 101 and 4095

That can be achieved with the following to settings in the preferences:
```
in.vol = src=a39, map=(101..4095=50..100)(0..100=0), onchange={volume=?}, delta = 2, start
```

You can also just try this at runtime by entering the above line from Serial input (but then the functionality will be gone on next reset
of the radio).


This line contains the new command _'in'_.
- The command _'in'_ sets or updates the properies of a specific Input.
- Each Input has a name, that name is indicated by the string following the '.' after _in_
- In our example we have defined an Input with the name 'vol'
- The _in_ command allows to set or change the "properties" of an Input.
  * If more than one property is defined, they are separated by ','. If more than one property is set in this list, they are evaluated from left to right.
  * most properties take an argument, that argument is assigned as usual with _property = argument_
  * The argument of a property is dereferenced to RAM/NVS if it starts with '@', to RAM only if it starts with '.', to NVS only if it starts with '&'.
  	- If an argument is dereferenced, it is used when the _in_-command is executed. If it is changed later, the Input will not change.
  * If _argument_ is not provided, it is assumed to be the empty String ''.
  * Some properties (_start_ or _stop_ for instance) do not require an _argument_ (in that case, _argument_ will be ignored if set)
- The property _'src'_ specifies the link to an input
  * Here _'a39'_ is set. This means "use pin39 as analog input" There is no checking, if that is a valid (analog) pin. In practice, this assignment will result in an 
  _analogRead(39)_. (The setting _in.vol=src=a42_ would not result in any complaints, just render the Input useless, as _analogRead(42)_ will always return _0_.)
- _mode_ is a property that details a few aspects of the physical input. 
  * It is specific (has a different meaning) to a specific Input type.
  * _mode_ is bit-coded, and for analog Input b0-b4 are evaluated (28 from the example is 0b11100 in binary):
  * b0: if set, readout is inverted (so 4095 if 0V are applied to Input pin and 0 is returned if pin is at Vcc). Not inverted in our example.
  * b1: if set, pin is configured as Input with internal PullUp (_pinMode(INPUT_PULLUP)_), otherwise (as in this example) just as Input (_pinMode(INPUT)_).
  * b2..b4: set a filter size from 0 to 7 to smoothen out glitches/noise on the input line. The higher the filter is set (max 7 as in our example) the less noise
      will be on readout with the drawback that actual changes (user change of variable resistor for instance) will be less instantaneous but will take some time
      to propagate to the readout. For volume settings this is not a bad thing at all, and the maximum size of 7 is still fine with that usecase.
  * all other bits are reserved for future ideas and should for now be set to '0'.
  * So in our case the Input is configured with the maximum filter size 7, not inverted, internal PullUp not set.  
- The property _map_ allows a re-mapping of the physical read input value to an arbitrary different output range
  * a _map_ can contain any number of entries
  * each entry must be surrounded by paranthesis _'(x1..x2 = y1..y2)'_. _x1..x2_ is (a part) of the input range that will be transformed into the output range _y1..y2_
    > In our example, if _analogRead(39)_ returns something between 101 or 4095, the Input itself will return something between 50 to 100. If _analogRead()_ returns
      less, (100 or less), the Input will yield 0.
  * The arduino _map()_ function is used internally, so reversing lower and upper bounds or using negative numbers is no problem as long its within the limits of int16
  * _map_-entries can overlap both on the x and the y side. When actually performing the mapping, the entries are evaluated from left to right.
  * if a matching entry is found, that entry will be used for mapping. Further entries (to the right) are no longer considered.
  * '..y2' and/or '..x2' can be omnited. (so you can have just one number on either side). 
    > If there is only one entry in the _y_-part, all of the input range
      from _'x1..x2'_ will be mapped to that single _y_-Value (see second entry in the example with _'(0..100=0)'_. If there is only one entry in the _x_-part, that 
      value will be mapped to _y_ (a setting of _y2_ will be ignored in that case).
  * A single map entry can be dereferenced to RAM/NVS using either of _'@/./&key-name_. 
    > So _'(@x1..@x2=0)'_ would search for the keys _x1_ and _x2_ in RAM/NVS and would use 
      this values at the time the 'in' command is executed. The resulting map-Entry will keep these values, even if later _x1_ or _x2_ are changed.
- The property _onchange_ defines either a inline command sequence or a _key_ to RAM/NVS that is searched for if the Input (after mapping) has changed since 
  last check. 
  * if the argument is enclosed in '{}', it es executed as command-sequence if a change on the Input is detected. 
  * if the argument is not enclosed in curly braces, it is expected to be a _key_ name that exists in either RAM/NVS. If the Input changes, that key name
  	will be searched for in RAM/NVS and (if found) the value associated to that key is assumed to be a commands sequence that will be executed.
  * In the commands sequence, every occurance of '?' will be replaced by the current value of the Input.
  * (to generate _onchange_-events, you need to explicitely start cyclic polling of the Input, see below.)
- The property _delta_ defines the magnitude of change that is required on the (mapped) Input for any change event to be triggered.
  * this allows, especially for analog input, to eleminate noise on the line that would lead to alternate readings and hence events on every other readout. Must be 
    one (any change in the input would lead to an event) or above. Will be forced to 1 if set by user to 0 or below.
- The property _start_ starts the cylic polling of the Input. Only if polling is started, any of the events (click and/or change) will be generated.
  * _start_ results in a direct readout of the Input. If _onchange_ is defined, when _start_ is set (it is in our example), that event will be called direct 
  	(to do an initial setting of volume at startup depending of the position of the variable resistor in our case). If that is not desired, _start_ must 
	be called before _onchange_ is set.
- The property _stop_ (not used in the example) is the opposite of _start_: it stops the cyclic polling (or does just nothing if cyclic polling has 
  not yet started).

There are a few more properties. However, this explanation should be enough to understand what is needed to translate the changes on a variable resistor to 
an appropriate volume setting. If everything is connected correctly and if there were no typos in entering the commands (or the preference settings) above, it should
already work.

If not, it is time for some debugging. There are two properties, that are concerned with debugging:
- The property _info_ shows the settings of the Input (no argument needed).
- The property _show = TimeInSeconds_ starts a cyclic output of the readout of the Input. _TimeInSeconds_ is the distance between two observations in seconds. 
  If TimeInSeconds is 0 or not given, the output on Serial will stop.
  > The output will show regardless of the setting of DEBUG. If you want to concentrate on the Input-information, you probably want to stop all other output
    by entering the command _debug=0_ first.
  > If _show_ is not zero, information will also show if events are happening (changes or clicks). So _show=3000_ can also be useful, if you want to debug
    for events. _show=1_ is useful if you want to monitor an Input continuously (e. g. for deriving a _map_ for that Input).
    
If you enter the command _in.vol=info_ you should read:

```
D: Info for Input "in.vol":
D:  * Src: Analog Input, pin: 39, Inverted: 0, PullUp: 0, Filter: 7
D:  * Cyclic polling is active
D:  * Value map contains 2 entries:
D:       1: (101..4095 = 50..100)
D:       2: (0..100 = 0..0)
D:  * Delta: 2
D:  * Show: 0 (seconds)
D:  --Event-Info--
D:  * onchange-event: "volume=?" (inline)
D:  * There are no click-event(s) defined.
D:  --Timing-Info--
D:  * t-debounce:     0 ms
D:  * t-click   :   300 ms (wait after short click)
D:  * t-long    :   500 ms (detect first longpress)
D:  * t-repeat  :   500 ms (repeated longpress)
```

If your output is different, you probably had a typo somewhere. Please check and correct the settings.

If all is expected, to have more insight, you should run the _in_-command with the property _show_ set, for instance to 1, i. e. _in.vol=show=1_. 
That should result in somewhat like this:


```
D: Input "in.vol" (is running) physRead= 2047 ( mapped to:   74)
D: Input "in.vol" (is running) physRead= 2049 ( mapped to:   74)
D: Input "in.vol" (is running) physRead= 2046 ( mapped to:   74)
D: Input "in.vol": change to 76 from 74, checking for events!
D: Executing onchange: 'volume=76'
D: Command: volume with parameter 76
D: Input "in.vol": change to 78 from 76, checking for events!
D: Executing onchange: 'volume=78'
D: Command: volume with parameter 78
D: Input "in.vol" (is running) physRead= 2342 ( mapped to:   78)
D: Input "in.vol": change to 80 from 78, checking for events!
D: Executing onchange: 'volume=80'
D: Command: volume with parameter 80
D: Input "in.vol": change to 82 from 80, checking for events!
D: Executing onchange: 'volume=82'
D: Command: volume with parameter 82
D: Input "in.vol" (is running) physRead= 2643 ( mapped to:   82)
D: Input "in.vol": change to 84 from 82, checking for events!
D: Executing onchange: 'volume=84'
D: Command: volume with parameter 84
D: Input "in.vol" (is running) physRead= 2799 ( mapped to:   84)
D: Input "in.vol" (is running) physRead= 2768 ( mapped to:   83)
D: Input "in.vol": change to 82 from 84, checking for events!
D: Executing onchange: 'volume=82'
D: Command: volume with parameter 82
D: Input "in.vol": change to 80 from 82, checking for events!
D: Executing onchange: 'volume=80'
D: Command: volume with parameter 80
D: Input "in.vol" (is running) physRead= 2445 ( mapped to:   79)
D: Input "in.vol": change to 78 from 80, checking for events!
D: Executing onchange: 'volume=78'
D: Command: volume with parameter 78
D: Input "in.vol": change to 76 from 78, checking for events!
D: Executing onchange: 'volume=76'
D: Command: volume with parameter 76
D: Input "in.vol" (is running) physRead= 2194 ( mapped to:   76)
D: Input "in.vol": change to 74 from 76, checking for events!
D: Executing onchange: 'volume=74'
D: Command: volume with parameter 74
D: Input "in.vol" (is running) physRead= 2054 ( mapped to:   74)
D: Input "in.vol" (is running) physRead= 2058 ( mapped to:   74)
D: Input "in.vol" (is running) physRead= 2036 ( mapped to:   74)
```


Here, for the first 3 seconds the volume changing resistor has not been touched. The volume is then changed up to 84 (by changing the variable resistor)
and then back to 74 again.

While you have that output up and running, you can play around with the properties setting to understand some effects. 
Try _in.vol=mode=0_ to see how much noise is then on the input before going back to filter=7 again by _in.vol=mode=28_:

```
D: Command: in.vol with parameter mode=0
D: Input "in.vol" (is running) physRead= 2209 ( mapped to:   76)
D: Input "in.vol": change to 76 from 78, checking for events!
D: Executing onchange: 'volume=76'
D: Command: volume with parameter 76
D: Input "in.vol" (is running) physRead= 2196 ( mapped to:   76)
D: Input "in.vol" (is running) physRead= 2180 ( mapped to:   76)
D: Input "in.vol" (is running) physRead= 2225 ( mapped to:   77)
D: Input "in.vol": change to 78 from 76, checking for events!
D: Executing onchange: 'volume=78'
D: Command: volume with parameter 78
D: Input "in.vol" (is running) physRead= 2203 ( mapped to:   76)
D: Input "in.vol": change to 76 from 78, checking for events!
D: Executing onchange: 'volume=76'
D: Command: volume with parameter 76
D: Input "in.vol" (is running) physRead= 2223 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2253 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2237 ( mapped to:   77)
D: Command: in.vol with parameter mode=28
D: Input "in.vol" (is running) physRead= 2237 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2231 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2240 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2207 ( mapped to:   76)
D: Input "in.vol" (is running) physRead= 2231 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2225 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2230 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2230 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2241 ( mapped to:   77)
D: Input "in.vol" (is running) physRead= 2220 ( mapped to:   77)
```

Whith _mode=0_ (means filter=0) the readout at the observation points was distributed between 2180 and 2253 (distance 73) and also some (unwanted) volume 
change events occured. With _mode=28_ the distance was lower at 34 (2241 - 2207) and no volume change event has been generated. So _mode=28_ is probably 
the better setting.

While we are at it, play with _b0_ of mode. As effect, the physical readout should inverse, and the direction of increasing/decreasing voltage should reverse. Enter 
_in.vol=mode=29_ to see (hear) the difference. Switch back to 'normal' using the command _in.vol=mode=28_.

You can change every property of an Input at any time. Whenever a property is changed, the others are not affected: a change to _mode_ as just done will not affect 
the _map_ (or any other property). You could even switch to a different input-pin, if this makes any sense from application perspective. (Remember that 
if an argument is not given for a property, that argument is assumed to be the empty string '', that reads 0 if a numeric property is set).

Try: _in.vol=map=(100..2000=50..100)(2100..4000=100..50)(2000..2100=100)(0..4095=0)_

This will make that the volume at either end of the tuning range (from physical read 0..100 and 4000..4095) is 0 and will increase to max volume of 100 in the middle
if turned to the other side and will go down to 0 from the middle again. Is this a useful feature? Probably not, but its cool to be able to do this.


## Second example: tuning
If everything is up and running, we can also change the Input to have a totally different meaning: not to change the volume, but to change the preset.
That is normally achieved by a variable capacitor, but since a variable resistor is already at hand and only two Input modifications are required (change from 
analog pin to touch pin and use a different mapping), we can use the Input with the variable resistor as simulation.

First, stop the volume Input using the command _in.vol=stop_

Then, change the "onchange" event of the Input: _in.vol=onchange={preset=?}_
Next, simplify the mapping to: _in.vol=map=(0..4095=0..2)_ to just tune between presets 0 and 2. 
Then, set delta to 1 and re-start the cyclic polling of the (and the debug show) Input with the new mapping and event: _in.vol=start, delta=1, show=1_
Verify the settings with _in.vol=info_, output on Serial should be this:

```
D: Info for Input "in.vol":
D:  * Src: Analog Input, pin: 39, Inverted: 0, PullUp: 0, Filter: 7
D:  * Cyclic polling is active
D:  * Value map contains 1 entries:
D:       1: (0..4095 = 0..2)
D:  * Delta: 1
D:  * Show: 1 (seconds)
D:  --Event-Info--
D:  * onchange-event: "preset=?" (inline)
D:  * There are no click-event(s) defined.
D:  --Timing-Info--
D: .....
```

If you play around, you will notice: it works in principle, but not quite. The first thing you will notice is that the mapped input value will be 0 for the 
physical readings of 0 to 1023, 1 for 1024 to 3073 and 2 for 3074 and up. So the map is skewed (the catching range for 1 is 2048 wide, but only 1024 wide
for 0 and 2. So the _map()_ function is skewed.

The next problem is that the presets will jitter if the reading is just between two presets. To increase the problem, set the filter to 0 by using the 
command _in.vol=mode=0_. Then try to tune close to a readout of 1024. You will notice a constant change between presets.

To solve this problems, Gaps are permitted in the value map. If the current physical read falls within a gap, the Input is considered to be unchanged. For the
first start, if the physical read is within a gap, the next matching value in the mapping entries closest to the physical readout is assumed.

_in.vol=map=(0..1200=0)(1448..2648=1)(2894..4095=2)_ will do the trick.

## Using touch input (also to read a variable capacitor)
### General precondition for reading variable capacitor values
The touch inputs of ESP32 can be used to read variable capacitors. The typical AM radios use variable capacitors in the range of around 300 pF. These can be
read through the touch pins. If using the native Espressif-IDF APIs for touch sensors the readings are stable and precise enough to give a reading to distinguish
for at least 10 positions (hence "tunable" stations) of the tuning knob.

One terminal of the capacitor must be connected to the pin direct, the other to ground (when recycling an old radio make sure any other connection to the 
capacitor is cut off, especially be sure that the coil of the oscillator circuit is no longer connected in parallel to the capacitor).

As a rule of thumb, the wire between capacitor and input pin should be as short as possible and as freely running as possible to improve the readings.
Especially avoid twisting with other wires if possible.

### Calibrating the Input
The touch pin could return any reading between 0 and 1023 in theory. In practice, if a variable capacitor is connected, the reading would be somewhere in 
between. The reading will also change depending on the wiring (length and path) between capacitor and ESP32. So you probably will have different readings 
using the same capacitor on the breadboard and in the final product.

For undisturbed reading during calibration it is recommended to stop any other output to Serial by entering _DEBUG = 0_ on the Serial command input.

For our example we assume the capacitor to be connected to T9. To read the capacitor we can use the _'in'_ command and link the source to the touch pin T9
with the follwing line from the serial input:

_in.tune=src=t9,show=1_

That should give an output on Serial like this when tuning from highest to lowest capacity (or lowest to highest frequency on the associated radio scale):
```
Input "in.tune" (is stopped) physRead=  103
Input "in.tune" (is stopped) physRead=  115
Input "in.tune" (is stopped) physRead=  131
Input "in.tune" (is stopped) physRead=  144
Input "in.tune" (is stopped) physRead=  184
Input "in.tune" (is stopped) physRead=  230
Input "in.tune" (is stopped) physRead=  294
Input "in.tune" (is stopped) physRead=  372
Input "in.tune" (is stopped) physRead=  422
Input "in.tune" (is stopped) physRead=  467
```

The numbers are abritrary of course, your mileage may vary. In this example the reading range is between 103 and 467. You should observe a rather stable 
reading if the position of the tuning knob is not changed (jitter by one only, no matter what the tuning position is). You should also observe that the 
change is nonlinear. The higher the reading the less you have to turn for a change in the Input readout.

For instance on my scale I have the the frequencies 530, 600, 800, 1200 and 1600 written on the AM scale. The corresponding readings are 113, 140, 212, 355
and 460 in my example. A possible tuning map (with some fishing range and gaps as discussed above) for 5 'channels' mapped to that positions could be defined
as:
_in.tune=map=(0..120=1)(130..160=2)(180..250=3)(330..390=4)(420..600=5)_

The reading on Serial monitor should change to something like this (depending on mapping and the position of the capacitor):
```
Input "in.tune" (is stopped) physRead=  118 ( mapped to:    1)
Input "in.tune" (is stopped) physRead=  126 (nearest is:    2)
Input "in.tune" (is stopped) physRead=  139 ( mapped to:    2)
Input "in.tune" (is stopped) physRead=  174 (nearest is:    3)
Input "in.tune" (is stopped) physRead=  218 ( mapped to:    3)
Input "in.tune" (is stopped) physRead=  282 (nearest is:    3)
Input "in.tune" (is stopped) physRead=  334 ( mapped to:    4)
Input "in.tune" (is stopped) physRead=  377 ( mapped to:    4)
Input "in.tune" (is stopped) physRead=  460 ( mapped to:    5)

```
If everything is as expected, the calibration output could be stopped by issuing the command:
_in.tune=show=0_

Now the mapped reading can be used to switch presets. All what is needed to use this Input to change presets is to start the cyclic polling of the preset
and assign a _onchange_-event: 
```
_in.tune=onchange={preset=?}_
```

Check the settings by entering the command:
_in.tune=info_

The resulting printout on Serial should be:
```
D: Info for Input "in.tune":
D:  * Src: Touch Input: T9, Digital use: 0, Auto: 0 (pin value is used direct w/o calibration)
D:  * Input is not started (no cyclic polling)
D:  * Value map contains 5 entries:
D:       1: (0..120 = 1..1)
D:       2: (130..160 = 2..2)
D:       3: (180..250 = 3..3)
D:       4: (330..390 = 4..4)
D:       5: (420..600 = 5..5)
D:  * Delta: 1
D:  * Show: 0 (seconds)
D:  --Event-Info--
D:  * onchange-event: "preset=?" (inline)
D:  * There are no click-event(s) defined.
D:  --Timing-Info--
D:  * t-debounce:     0 ms
D:  * t-click   :   300 ms (wait after short click)
D:  * t-long    :   500 ms (detect first longpress)
D:  * t-repeat  :   500 ms (repeated longpress)
```

To actually make the radio change the preset by changing the tuning knob you need to start cyclic polling of the Input:

_in.tune=start_

So if you are satisfied with those settings, the final entries in the NVS preferences could be given as follows to take effect at every start of the radio:
```
$tunemap =      (0..120 = 1)(130..160 = 2)(180..250 = 3)(330..390 = 4)(420..600 = 5)
# Arbritary key to store the map for later use

in.tune = src=t9,map=@$tunemap, onchange={preset=?}, start
# Use touch T9 as Input, using the predefined $tunemap, execute 'preset=?' on change of Input and start cyclic polling of the Input 
```

Or you could use the variable capacitor for different thinks, like changing the volume:
_in.tune=src=t9,map=(110..475=50..100)(0..109=0)(476..500=100),onchange={volume=?},start_

Together with the [example above](#second-example-tuning) it is thus possible to use the frequency knob to change volume and the volume knob to change presets. 
What a crazy world!

Of course it is also possible to change the direction of volume increase/decrease by changing the Input map:
_in.tune=map=(110..475=100..50)(0..109=100)(476..500=0)_


### Classic touch reading

If you just want to detect touch input, you can also use the new Input mechanism. We assume the touch input is T8. So define a touch Input like this:

```
in.touch=src=t8,show=1
```

This will result in an output on Serial like such:

```
D: Input "in.touch" (is stopped) physRead=  848
D: Input "in.touch" (is stopped) physRead=  842
D: Input "in.touch" (is stopped) physRead=  223
D: Input "in.touch" (is stopped) physRead=  190
D: Input "in.touch" (is stopped) physRead=  847
D: Input "in.touch" (is stopped) physRead=  777
D: Input "in.touch" (is stopped) physRead=  283
D: Input "in.touch" (is stopped) physRead=  518
D: Input "in.touch" (is stopped) physRead=  846
```

The higher readings (above 800 in this example) are while not touched (open), the lower readings (below 600) when touched. To convert the readings into 
digital information you could apply a map to the Input:
_in.touch=map=(600..1023=1)(=0)_ 

With this map the Input will read 1 if untouched and 0 if touched:
```
D: Input "in.touch" (is stopped) physRead=  856 ( mapped to:    1)
D: Input "in.touch" (is stopped) physRead=  176 ( mapped to:    0)
D: Input "in.touch" (is stopped) physRead=  135 ( mapped to:    0)
D: Input "in.touch" (is stopped) physRead=  854 ( mapped to:    1)
D: Input "in.touch" (is stopped) physRead=  855 ( mapped to:    1)
D: Input "in.touch" (is stopped) physRead=  136 ( mapped to:    0)
D: Input "in.touch" (is stopped) physRead=  815 ( mapped to:    1)
D: Input "in.touch" (is stopped) physRead=  856 ( mapped to:    1)
```

There is also a simplified way using the _mode_-property of the touch Input. If bit b0 is set, the Input is automatically converted to a binary reading. 
In our example, first delete the Input-map by assigning an 'empty' string as map:
_in.touch=map=_

And then set bit b0 in the _mode_-property:
_in.touch=mode=1_

And the result will be something like this (Input is touched/untouched in this example):
```
D: Input "in.touch" (is stopped) physRead=    1
D: Input "in.touch" (is stopped) physRead=    0
D: Input "in.touch" (is stopped) physRead=    1
D: Input "in.touch" (is stopped) physRead=    0
D: Input "in.touch" (is stopped) physRead=    1
D: Input "in.touch" (is stopped) physRead=    0
D: Input "in.touch" (is stopped) physRead=    0
D: Input "in.touch" (is stopped) physRead=    1
```

Using the _mode_-bit b0 should be the preferred option if a touch-Input shall be used as binary Input (touched/not touched).

For reacting on Input changes, using the _onchange_ event is problematic, as it triggers both if the Input is changing from 1 to 0 (touch detected) and
if the Input changes from 0 to 1 (touch released). There are two more change-properties that can be used (on any other type of Input as well, of course):
- _on0_ will trigger if the Input changes from not Zero to Zero. If that happens the command sequence associated to on0 will be executed.
- _onnot0_ will triggered on change of the Input from Zero to not Zero.
- _on0_ and _onnot0_ can be used on any Input, also on analog reads. 
- _onchange_ will always trigger before either of _on0_ or _onnot0_ triggers (if appropriate).
- the value associated to _on0_ or _onnot0_ can either be a command sequence direct (enclosed in {}) or a _key_-name that is searched for in RAM/NVS.
  (value substituation is still done for "?", though in case of _on0_ it will always be "0").
- to be clear: _onnot0_ will only trigger if the last read was 0. So if an analog Input changes from 0 to 1, _onchange_ and _onnot0_ will trigger. If later
  the Input changes to 2, _onchange_ will trigger again, but not _onnot0_ 

As a simple example, to use the touch pin T8 to toggle mute, the command could be:
```
in.touch=src=t8,mode=1,on0={mute},start
```


That way, also the _touch_XX_ settings from preferences are translated to the following at startup:

```
in.touch_XX = src=tXX, mode=1, on0=touch_XX, start
```

## Using digital input (GPIO)

The handling is quite similar. Consider for instance that we have the following entry in the preferences:
```
gpio_33 = mute
```

This will translate into:
```
in.gpio_33 = src=d33, mode=2, on0=gpio_33, start
```

Nothing spectacular here, just notice:
- _src=d33_ makes this a digital Input linked to GPIO33 (again, no checking if this is a valid pin-nr and also no checking if this pin has not been 
  reserved before).
- for the _mode_ property bits b0 and b1 are evaluated:
	* b0: if set, the readout is inversed (not inversed in our case)
	* b1: if set (it is in our case), GPIO is configured with __INPUT_PULLUP__
    * all other bits are reseved for future ideas and should be __0__ for now.
- _on0_ holds the key (to NVS) that contains _mute_ as command.
- _start_ activates the Input that is now listening on GPIO33

Now the radio should toggle mute whenever GPIO33 goes low. If this does not work, use property _show=1_ for debugging.

The good thing is, you can now easily change the assignment of the action that the GPIO performs. Try:
```
in.gpio_33 = on0={uppreset=1}
```

## Click-events
Any Input can be used to react to click-events. A Input is considered pressed if it reads __0__, unpressed if it reads something other then zero.
Depending on the pressing sequence, the following events can be used:
- _on1click_ the Input has been short pressed once.
- _on2click_ the Input has been short pressed twice.
- _on3click_ the Input has been short pressed thrice.
- _onlong_ the Input is longpressed.
- _on1long_ the Input is longpressed after a single click.
- _on2long_ the Input is longpressed after a double click.

If you want the radio to toggle mute on a shortpress, and to run _uppreset=1_ on doubleclick, all you need to do is to enter:
```
in.gpio_33 = on0=       # "delete" the old event {uppreset=1}
in.gpio_33 = on1click={mute}, on2click={uppreset=1}
```
You could (in addition) also add some functionality to increase/decrease volume on longpress (decrease) or longpress after 1 shortclick (increase):
```
in.gpio_33 = onlong={downvolume=1}, on1long={upvolume=1}
```
You will notice that the up/down-volume is working, albeit slow. There are a few properties that control the timing behaviour of the _click_-events:
- _t-long=500_ (default setting is 500): if Input is pressed, how long to wait before "longpress" is detected (in ms)?
- _t-repeat=500_ distance between longpress events (in ms)
- _t-click=300_ after Input has been released (and that release was before _t-long_ was reached, how long to wait for a next button press to decide
  if it was just a single click or a start of a double-click.
  
Especially _t-click_ is somewhat critical: if set too high, it takes to long till the _click_-event is evaluated. If set too low, a double-click will get
lost and two single clicks will be reported instead.
If you start debug-Output for the Input again with the _show_-property set to a high number, you will see what is going on with the click-events.
Take the longpress for volume-down as example (with default timings:)
```
in.gpio_33=show=3000
```

If the GPIO is pulled low, Serial should read now:
```
17:34:08.815 -> D: Input "in.gpio_33": change to 0 from 1, checking for events!
17:34:09.312 -> D: Click event for Input in.gpio_33: long (Parameter: 1)
17:34:09.312 -> D: in.gpio_33 onlong='downvolume=1'
17:34:09.345 -> D: Command: downvolume with parameter 1
17:34:09.809 -> D: Click event for Input in.gpio_33: long (Parameter: 2)
17:34:09.842 -> D: in.gpio_33 onlong='downvolume=1'
17:34:09.842 -> D: Command: downvolume with parameter 1
17:34:10.339 -> D: Click event for Input in.gpio_33: long (Parameter: 3)
17:34:10.339 -> D: in.gpio_33 onlong='downvolume=1'
17:34:10.339 -> D: Command: downvolume with parameter 1
17:34:10.835 -> D: Click event for Input in.gpio_33: long (Parameter: 4)
17:34:10.835 -> D: in.gpio_33 onlong='downvolume=1'
17:34:10.868 -> D: Command: downvolume with parameter 1
17:34:11.365 -> D: Click event for Input in.gpio_33: long (Parameter: 5)
17:34:11.365 -> D: in.gpio_33 onlong='downvolume=1'
17:34:11.365 -> D: Command: downvolume with parameter 1
17:34:11.597 -> D: Input "in.gpio_33": change to 1 from 0, checking for events!
17:34:11.597 -> D: Click event for Input in.gpio_33: long (Parameter: 0)
17:34:11.597 -> D: in.gpio_33 onlong='downvolume=1'
17:34:11.597 -> D: Command: downvolume with parameter 1
```

So falling edge of the Input was detected at 17:34:08.815, around 500ms (give or take a few ms for jitter on the sampling timescale) later (t-long-time) 
at 17:34:09.312, the first long press event was triggered and then a few more in again 500ms distance (now t-repeat-time) until at 17:34:11.597 the GPIO 
was released and a last longpress event has been triggered.
Note that long-press-events have a parameter that can be evaluated by substituting "?" in the associated commands sequence (not used in this example).
If the parameter is "1", it is the first long press detected, the parameter will be increased by one for any additional event, and will be 0 at the end
to indicate that the Input has been released.

If you repeat again with 
```
in.gpio_33=t-repeat=100
```
And repeat the longpress-testing, the result on Serial should be something like:
```
17:43:48.826 -> D: Input "in.gpio_33": change to 0 from 1, checking for events!
17:43:49.355 -> D: Click event for Input in.gpio_33: long (Parameter: 1)
17:43:49.355 -> D: in.gpio_33 onlong='downvolume=1'
17:43:49.355 -> D: Command: downvolume with parameter 1
17:43:49.455 -> D: Click event for Input in.gpio_33: long (Parameter: 2)
17:43:49.455 -> D: in.gpio_33 onlong='downvolume=1'
17:43:49.455 -> D: Command: downvolume with parameter 1
17:43:49.554 -> D: Click event for Input in.gpio_33: long (Parameter: 3)
17:43:49.554 -> D: in.gpio_33 onlong='downvolume=1'
17:43:49.554 -> D: Command: downvolume with parameter 1
17:43:49.687 -> D: Click event for Input in.gpio_33: long (Parameter: 4)
17:43:49.687 -> D: in.gpio_33 onlong='downvolume=1'
17:43:49.687 -> D: Command: downvolume with parameter 1
17:43:49.786 -> D: Click event for Input in.gpio_33: long (Parameter: 5)
17:43:49.786 -> D: in.gpio_33 onlong='downvolume=1'
17:43:49.786 -> D: Command: downvolume with parameter 1
17:43:49.886 -> D: Click event for Input in.gpio_33: long (Parameter: 6)
17:43:49.886 -> D: in.gpio_33 onlong='downvolume=1'
17:43:49.886 -> D: Command: downvolume with parameter 1
17:43:50.018 -> D: Click event for Input in.gpio_33: long (Parameter: 7)
17:43:50.018 -> D: in.gpio_33 onlong='downvolume=1'
17:43:50.018 -> D: Command: downvolume with parameter 1
17:43:50.118 -> D: Click event for Input in.gpio_33: long (Parameter: 8)
17:43:50.118 -> D: in.gpio_33 onlong='downvolume=1'
17:43:50.118 -> D: Command: downvolume with parameter 1
17:43:50.217 -> D: Click event for Input in.gpio_33: long (Parameter: 9)
17:43:50.250 -> D: in.gpio_33 onlong='downvolume=1'
```

You will notice, that the first longpress is still reportet after 500ms (t-long has not been changed and is still 500), but all the following events are
coming at a shorter distance of around 100ms as expected. 
If not, you should debug the Input by running:
```
in.gpio_33=info
```
The output should be:
```
D: Command: in.gpio_33 with parameter info
D: Info for Input "in.gpio_33":
D:  * Src: Digital Input, pin: 33, Inverted: 0, PullUp: 1
D:  * Cyclic polling is active
D:  * Value map is NOT set!
D:  * Delta: 1
D:  * Show: 3000 (seconds)
D:  --Event-Info--
D:  * there are no change-event(s) defined.
D:  * on1click-event: "mute" (inline)
D:  * on2click-event: "uppreset=1" (inline)
D:  * onlong-event: "downvolume=1" (inline)
D:  * on1long-event: "upvolume=1" (inline)
D:  --Timing-Info--
D:  * t-debounce:     0 ms
D:  * t-click   :   300 ms (wait after short click)
D:  * t-long    :   500 ms (detect first longpress)
D:  * t-repeat  :   100 ms (repeated longpress)
```


## Using Inputs to read internal variables

Some internal variables can be read through Inputs. The list of available variables changes. You can check the current implementation by entering just 
the "~" as sole character. The output should be like this:

```
23:11:17.173 -> D: Known system variables (8 entries)
23:11:17.173 -> D:  0: 'volume' = 70
23:11:17.173 -> D:  1: 'preset' = 0
23:11:17.173 -> D:  2: 'toneha' = 5
23:11:17.173 -> D:  3: 'tonehf' = 3
23:11:17.173 -> D:  4: 'tonela' = 15
23:11:17.173 -> D:  5: 'tonelf' = 12
23:11:17.173 -> D:  6: 'channel' = 1
23:11:17.173 -> D:  7: 'channels' = 9
```

These variables can be used (read only!) if the name is preceeded by "~". This is also possible for using those in an Input to the RetroRadio.
That way, we are able to do some neat things. Like solving the problem "if volume is below 50 I do not hear anything". We have partly solved that
for the analog Input of a [variable resistor](#first.Input.example.volume), but that will only prevent volume settings between 1 and 49 from the
"volume knob" attached to the analog Input. Here is the solution to limit volume settings to 50 to 100 (or 0) only, from any source:
```
in.minvol=src=~volume,map=(50..100=1)(0=2)(=0),onchange=:v_limit,start
ram.:v_limit=if(?)={.lastv = ~volume}{if(.lastv)={volume=0}{volume=50}}
```
The first line defines the Input that observes the internal value "~volume" (which equals the _ini_block.volume_ setting). The second line stores
the command-sequence to RAM that will be executed if the volume changes.
The Input will read as 1 if the volume setting is between 50 and 100, it will read 2 if volume setting is 0 and it will read 0 on any other value. 
So we have the following possible transitions:
 - volume is 0, and goes to 1..49: the Input readout will change from 2 to 0
 - volume is 0, and it goes to 50..100: readout will change from 2 to 1
 - volume is between 50..100 and goes to 0: readout will change from 1 to 2
 - volume is between 50..100 and goes to 1..49: readout will change from 1 to 0
We do not want to be in the range of 1..49. If we enter that range, we want to go either to 0 (if coming from >= 50) or 50 (if coming from 0).
Unfortunately, with the _onchange_ event, we do only get the current readout. So we need to store the last state, which is done in the assigned
_onchange_-event ":v_limit". Lets assume the first readout was within one of the desired ranges. Then the Input will call the _:vlimit_ with 
either '1' or '2'. So _if(?)_ evaluates to true,  _RAM _.lastv_ stores the current volume (Either '0' or anything between '50' and '100').
if later a change into the undesired zone happens, _.lastv_ will be checked. If it is "0", the command _volume=50_ is executed, otherwise 
_volume=0_. This will in return change the Input readout again, thus calling _:v_limit_ again direct which will again store _.lastv_ as either "0"
or "50".



# Scripting Summary 
## Storing and retrieving values
- values can be stored to NVS (known as preferences) and now also to RAM
- in both cases, values are identified by a _key-name_, which can hold a value (assigned to by "=" in preferences in case of NVS).
	- example from preferences: _pin_ir = 0_ defines the value of _pin_ir_ to be 5.
- there is some mixup between commands and preferences. For instance you will find _volume = 75_ in preferences. This line is (at startup) interpreted
  as command to set the volume to 75, but it might also change during runtime to store the current value of volume setting.
- the value associated to a key can be any arbritrary string constant. 

We will start with RAM, as this is more safe. Accidentially deleting a vital NVS entry while playing around might cause trouble. The basic commands
to manipulate RAM and NVS entries are similar.
RAM entries can be used to store and retrieve information that can be used for commands. They are only available through the current power cycle of 
course.

Using command ram to list RAM entries:
- you can list the contents of RAM using the command 
```
	ram?[ = key-name-part ]
```
- this lists the current entries in RAM. If _key-name-part_ is given, only entries that have _key-name-part_ as substring in their key are listed.
- listing is shown on Serial (even if _DEBUG=0_).
- if nothing has been set by your scripting so far, _ram?_ should list nothing

Using command ram to add/update a RAM entry:
```
	ram.key-name = value
```
- this sets the value of _key-name_ in RAM to _value_. If _key-name_ does not exist, it is created by that command.


Using command ram to delete a RAM-entry
- you can delete an entry in RAM using the command 
```
	ram- = key-name
```
- this deletes the entry associated to _key-name_ from RAM. There will be no complaints if _key-name_ does not exist in RAM.
- full _key-name_ must be specified, no pattern matching, so only one entry can be deleted by this command.
- from Serial, try:
```
	DEBUG = 0
	ram? = test
	ram.test = 77
	ram? = test
	ram.test = 0
	ram? = test	
	ram- = test
	ram? = test
```

The dot "." can be used as shortcut for the _ram_-command:
- _.? = key-name-part_ is the same as _ram? = key-name-part_
- _.key-name = value_ is equivalent to _ram.key-name = value_
- _.- = key-name_ is equivalent to _ram- = key-name_

The dot is also used to dereference RAM keys to use them in commands, for instance as argument. Example:
```
	DEBUG = 1
	.test = 77
	volume = .test
```
This sequence will store 77 to RAM entry named "test" and this value is then passed as argument to the _volume_ command. As a result, radio will play
at volume level 77.

Handling/using of NVS entries is similar. Whenever possible, RAM entries should be preferred to store information. Use NVS entries only if your 
usecase requires the data to be persistent over power cycles.

Using the nvs command:
- you can list the contents of NVS using the command _nvs?[ = key-name-part ]_ (shortcut: _&? = key-name-part_)
- you can add/update an NVS entry using the command _nvs.key-name = value_ (shortcut: _&key-name = value_)
- you can remove an NVS entry using the command _nvs-=key-name_ (shortcut: _&- = key-name_). It should be clear that deleting NVS entries has the 
  possibility to push you into danger zone again, if for instance pin entries are deleted).

The ampersand "&" can also be used to dereference NVS-entries to be used as argument to commands. For demonstration purposes, try:
```
	DEBUG = 1
	volume = 80
	&test = 52
	volume = &test
```

You can also dereference a key by using "@". Try:
```
	volume = @test
```
The logic for dereferencing _@key-name_ is as follows: if _key-name_ exists in RAM, this value is used. If not, the value from NVS is used.
If _key-name_ also does not exist in NVS, empty string (evaluated to "0" if number is expected) is used. So volume should be set to 77 if you 
followed the steps so far, as there still should be a RAM-entry _test_ with value "77".

If you do not want the argument to be evaluated, but just store a string constant, you have to surround this string constant by round brackets.
The brackets will be removed before the command is executed. In our example, try: 
```
	DEBUG = 1
	volume = ( @test )
```
That should result in an output on Serial like: _Command: volume with parameter @test_ which means: the brackets have been removed and parameter
passed to command volume is "@test". As volume expects a number (as string of digits) that will fail. If converted to a number using _atoi("@test")_
the result would be "0". So say good-bye to whatever you just heared.

This is also important to understand if you want to assign for instance a command-sequence to a RAM (or NVS) key. Consider the following idea:
you want to go "home", where "home" means: _preset=1_ and _volume = 70_ To be available for later _calls_, you want to store that command sequence
into RAM using the key _:home_. Entering _.:home=preset=1;volume=70_ will not do what is wanted. Output on Serial would be something like this:
```
23:58:59.026 -> D: Command: .:home with parameter preset=1
23:58:59.026 -> D: Command: volume with parameter 70
23:58:59.026 -> D: Executed 2 command(s) from sequence .:home=preset=1
```
If you run the command _.?=home_ you will see that the value for RAM-key _:home_ is just _preset = 1_.
The reason is that in fact the line _:home=preset=1;volume=70_ is interpreted as two commands:
```
	.:home = preset = 1
	volume = 70
```

To achieve what was wanted, you have to use round brackets in this case:
```
	.:home = (preset = 1; volume = 70)
```

You can verify by listening the content of the key using _.?=:home_ Or you can just use that entry by _call = :home_


## Control flow 

Word of warning: there are internal limits (like NVS keylen or NVS content len or NVS size). There is no protection against buffer overruns or creating 
endless loops. So keep that in mind with your design. In theory you can deeply nest control structures (calling a subroutine in an else case of another if in
a while loop). In practice that is error prone, difficult to read and maintain. Better solution is probably to define and use flags to flatten the nesting.

Another word of warning: the "RadioScriptingLanguage" was not planned but has happened (was developed on the go). The language grammer might be not be bullet
prove, so observe whats happening and dont be surprised if some scripting constructs are not behaving as expected (well, I am surprised sometimes). 
There are no "compiler errors" generated nor is there any form of "exception handling". Best case it will just not work, worse it will show an unexpected 
behaviour or worst it might crash.
The examples in this README or the defaultprefs.h do work, though. 

The advantage is, that you can try scripting using the Serial interface. So you can test your ideas on the fly, and if you came up with a working solution
you can integrate that solution to preferences. There are some debug facilities (like verbose command execution) included in the scripting language.

- Sequences are allowed. Several commands can be separated using the semicolon ';' Commands will be executed from left to right. Last command does not need 
  to be terminated by semicolon.
- So on serial, you can still just enter one command without any change for the known commands. Or you can enter a sequence on serial that will be executed 
  directly, for instance: _preset=1;volume=70_ to set both preset and volume.

Executing commands at startup:
- At startup NVS is searched for key _::setup_. The content of this key is expected do be a command sequence that will be executed. At this
  point in time, the radio is about to start player task. So there is no audio yet, and you can still alter the settings 
  (for instance for preset or volume) to override the "defaults" based on your usecase.
- For convenience, (if one line is not enough), after _::setup_ has been searched (and executed if found), _::setup0_ to _::setup9_ are searched and executed
  in that order. Gaps dont matter. So it is perfectly legal, if _::setup7_ is the only entry, it will be executed even though _::setup_ (or any other of 
  _::setup0_ to _::setup6_) does not exist. 

Executing commands during runtime:
- At every _loop()_ of the software, NVS is searched for key _::loop_. The content of that key are expected do be a command sequence and will be executed.
- For convenience, (if one line is not enough), after _::loop_ has been searched (and executed if found), _::loop0_ to _::loop9_ are seearched and executed 
	(if defined) in that order. Gaps dont matter. So it is perfectly legal, if _::loop7_ is the only entry, it will be executed even though _::loop_ 
	(or any other of _::loop0_ to _::loop6_) does not exist. 
- To speed things up, _::loopx_ is not retrieved from NVS every time, but a RAM copy will be taken at startup. Therefore if you change any _::loop_-key
  that change will only take effect after next start of the radio.
- DEBUG is set to 0 before executing any _::loop_ key to avoid flooding of Serial output. If you need it within a _::loop_-Sequence, you need to turn it
  on again from within the sequence.
  
  
If-Command:
- is implemented as follows: 
```
	if[.result-key](condition)= {if-command-sequence}[{else-command-sequence}]
```
- parts in square brackets (_.result_key_ and _{else-command-sequence}_ are optional.
- condition is an unary or binary expression. Can be empty and is evaluated as false (=0) then. A unary (rvalue only) is true, if the rvalue is not zero.
- an rvalue can be a _number_, a reference to NVS-key (_&key_), a reference to RAM-key (_.key_), a reference to either RAM (if defined there) or NVS (if
  not defined in RAM) using _@key_ or a system variable (_~variable_). Undefined rvalues are assumed to be 0.
- numbers are calculated on _int_16_ base, so range is limited to _-32768..32767_. There is no protection against over/underruns. Only decimal numbers 
  can be used (no hex or binary).
- two rvalues can be combined by a binary operator. The following operators are permitted: 
	 - "==", "!=", "<=", ".." , ">=", "<", "+", "^", "*", "/", "%", "&&", "&", "||", "|", "-", ">"
- work exactly as in C, with the exception of "..". With _op1 .. op2_ a random number is generated within the range of _op1_ (included) up to 
  _op2_ (_op2_ is also included). So _if(1 .. 3)_ will generate numbers between 1 and 3 (hence never 0 or always true for if) whereas so _if(0..3)_ will generate
  numbers between 0 and 3 (and will therefore be true in 3 out of 4 cases only).
- the result of the expression is stored into RAM using key   _"result-key"_ if the key (preceeded by '.') is placed after _if_ but before the condition. 
  Result-key can be any string that is suitable as key into RAM and is purely optional. So _if.xyz(0..3)={}_ will store a number between 0 and 3 to RAM-key 
  "xyz" (and do nothing else as the _if-command-sequence_ is empty). (with the command _.?=xyz_ you can list the contents of RAM for all keys that 
  contain "xyz" in the key name. Or you can chain _if.xyz(0..4)={};.?=xyz;_ The result of the calculation is already stored into RAM and available within the _if-_ or _else-_
  command sequence (whichever fires).
- The result of the calculation will also be used to substitute all occurances of the question mark "?" within the _if_/_else_ command sequence before 
  execution. This is a strict textual replacement, "?" will be replaced by the resulting number without any leading/trailing spaces or "0"-characters.
  That allows for a "simulation of array data structures": _if(1..4)={ram.test??=????};.?=test_ will store _xxxx_ into RAM-key _testxx_ (with x being one
  of the digits between '1' and '4').
- Both _if-_ and _else-_ command sequence must be surrounded by curly braces, even if they contain only 1 (or 0) commands. _else_-Sequence is optional. 
- For convenience, there is also the _ifnot_ command, that inverts the logical evaluation. So in cases where you do not need the _if_ but the _else_ part only
  you can avoid writing empty _if_-sequence by using _ifnot_: _ifnot.xyz(0..4)={.xyz=88};.?=xyz_ will _xyz_ to 1..4 (due to calculation of condition) or
  88 if the condition was calculated to be 0.
- For debugging, you can add the letter 'v' (verbose) to the command word (so _ifv_ or _ifnotv_) that will output some (hopefully usefull) information on the
  serial monitor (also if _DEBUG=0_)

A note on substituting question marks: the scope of the question mark is respected. So for instance if if-commands are nested, question marks in each 
command-sequence will be substituted correctly. For instance: _ifv.x(7) = {ifv(.x == 7) = { .x = ? }};.?=x_ will set x to 1. As the verbose command option
has been used, Serial output gives some details on what happened:
```
18:54:34.379 -> ParseResult: if(7)={ifv(.x == 7) = { .x = ? }}; Ramkey=x
18:54:34.379 -> Start if(7)="{ifv(.x == 7) = { .x = ? }}"
18:54:34.379 -> Calculate: ("7" [-1] "")
18:54:34.379 -> Caclulation result=7
18:54:34.379 -> Calculation result set to ram.x=7
18:54:34.413 ->   IfCommand = "ifv(.x == 7) = { .x = ? }"
18:54:34.413 -> ElseCommand = ""
18:54:34.413 -> Condition is TRUE (7)
18:54:34.413 -> Running "if(true)" with (substituted) command "ifv(.x == 7) = { .x = ? }"
18:54:34.413 -> ParseResult: if(.x == 7)={ .x = ? }; Ramkey=
18:54:34.413 -> Start if(.x == 7)="{ .x = ? }"
18:54:34.413 -> Calculate: ("7" [0] "7")
18:54:34.413 -> Caclulation result=1
18:54:34.413 ->   IfCommand = ".x = ?"
18:54:34.413 -> ElseCommand = ""
18:54:34.413 -> Condition is TRUE (1)
18:54:34.446 -> Running "if(true)" with (substituted) command ".x = 1"
18:54:34.446 -> RAM content (3 entries)
18:54:34.446 ->   1: 'x' = '1'
```
In a nutshell: condition of the left _if()_ was evaluated first to be "7", that "7" has been stored to RAM using key _x_. So expression is true, and the
_if-command-sequence_ gets executed: _ifv( .x == ?) = { .x = ? }_ Before execution, only the "?" in the condition expression is substituted but not the second.
And that is as it should be, as the second is part of another _if()_-command. This has the condition (after substitution) as: _( 7 == 7 )_ which is "1",
and this is then used to substitute the "?" in the associated _if-command-sequence_ to yield _.x = 1_ which finally sets _x_ to 1. (_.?=x_ is a command 
to list all RAM-entrys which have "x" as part of their _key-name_, which is only _'x'_ in our case).



While-Command:
- is implemented as follows: 
```
	while[.result-key](condition)= {while-command-sequence}
```
- condition is evaluated as with if command. As there, the result of the calculation can be stored to RAM using key _.result_key_ after keyword _while_ and
  before condition.
- inverted version (_whilenot()_) and verbose version (_whilev()_ or _whilenotv()_) can be used.  
- _while-command-sequence_ is executed as long as the condition is true (so must in fact change the conditions such that it will evaluate to "0" and hence
  terminate the the loop. So _while(1)={}_ is a bad idea, the loop will idle forever, and other tasks (like playing music or listening on Serial) will not 
  work. Only way to terminate is a HW- or power-on reset.
- in general, use this command with special caution.   

Calc-Command:
- is implemented as follows: 
```
	calc[.result-key](expression) [= {calc-command-sequence}]
```
- this is not exactly a command that influences the control flow (_calc-command-sequence_ will always be executed, if given), just the syntax is identical
  to if. (you can consider this as if with identical _if_ and _else_ sequence.
- expression is evaluated same way as for _if_-condition. The result can be stored to RAM using the given _result-key_ or used in the (optional) 
  _calc-command-sequence_. Both _result_key_ and _calc-command-sequence_ are optional, however if none is given, the command takes no effect.
  is evaluated as with if command. As there, the result of the calculation can be stored to RAM using key _.result_key_ after keyword _calc_ and before the expression.
- the result of the calculation can be used for '?'-substitution within the _calc-command-sequence_
- _calcv()_ for verbose version can be used.

Idx-Command:
- is implemented as follows: 
```
	idx[.result-key](list-index, list) [= {idx-command-sequence}]
```
- this command does also not influences the control flow (_idx-command-sequence_ will always be executed, if given). - _list-index_ is expected to be an integer. If the integer is greater or equal to 1 (so 'n') the _n_-th element of the following (comma-delimited) _list_ beginning from the leftmost _list_-element. 
- _list_index_  must be either a literal or can be dereferenced (by @, &, . or ~)
- if _list-index_ is the literal "0", the _idx-command_ will return the number of elements in the list
- if _list-index_ is bigger than the number of elements (or below 0), the empty string "" is returned.
- if the optional _result-key_ is given, the result will be stored into RAM using _result-key_.  
- the result can be used for '?'-substitution within the _idx-command-sequence_
- list element can be any type of string (not only numbers) but must not contain ',' or ')'.
- each list element can be dereferenced to RAM/NVS using the usual _@key_-notation. Every dereferenced value can be either a single value or a (sub-) list in itself.
- _idxv()_ for verbose version can be used.

```
.list=(@list1, @list2)      # define list in RAM holding a list with two entries @list1, @list2 (references to list1 and list2)
.list1=(@list11, @list12)   # define list1 in RAM holding a list with two entries @list11, @list12 (references)
.list11=entry1, entry2    # define list11 in RAM with two list-items entry1, entry2
.list12=entry3             
.list2=entry4              # list2 holds just entry4
```
For convenience, here all of the above as command sequence in one line to copy it to Serial input direct:
```
.list=(@list1, @list2);.list1=(@list11, @list12);.list11=entry1, entry2;.list12=entry3;.list2=entry4
```


so if evaluated in _idx_-command, @list will finally expand to ((Entry 1, Entry 2), Entry 3), Entry 4 (Paranthesis are just for clarification to indicate the bounderies of each key in RAM).

For example, you can load a random element of that list into RAM key _entry_ by using

```
calc(1..4)={idx.entry(?, @list)}
```




Call-Command:
- is implemented as follows: 
```
	call[( parameter )] = key-name
```
- _key-name_ is used to search for in first RAM and if not found there in NVS. If a match is found, the content of the RAM/NVS entry are expected to
  be a command-sequence and executed.
- when this sequence is finished (all commands of the sequence have been executed or a return command was found), the sequence the _call()_ command is in
  is continued after the _call()_ command.
- if optional parameter is given, everything between '(' and ')' is used as a string to replace any "?" using the known mechanism in the command-sequence
  given by _key-name_. If parameter is given, leading and trailing spaces are removed: _call (   param ) =_ would substitute any "?" by "param".
- again danger zone here: do not create loops. This innocent looking sequence will crash the radio: _.x = call = y; .y = call = x; call = x_
	- with the first command, a RAM entry is created with name "x" that holds "call y"
	- second command creates RAM entry "y" with "call x"
	- the third command starts the call cycle "x->y->x->y..." that will trigger the stack watchdog eventually.
	
Return-Command:
- is implemented as follows: 
```
	return[.return-key] [= value ]
```
- this command will cancel the execution of the current sequence. If that sequence was _called_ from another sequence, execution will commence there  (or
  just stop if there was no caller, like from serial).
- if _return-key_ is given, _value_ will be stored to that RAM-key. (The caller "must know" the RAM-key to evaluate it)




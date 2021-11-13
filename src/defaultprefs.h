// Default preferences in raw data format for PROGMEM
//
#define defaultprefs_version 1808016
const char defprefs_txt[] PROGMEM = 
R"=====(
# Define credentials for WiFi you want to use
wifi_00 = ssid/passwd
#wifi_01 = ssid/passwd
)====="
// Definition: Pins
#if defined(ARDUINO_ESP32_POE) || defined(ARDUINO_ESP32_POE_ISO)
R"=====(
# Default pin configuration for Olimex PoE/PoE-ISO boards
# VS1053 connection defined. IR receiver connected to GPIO0
#pin_ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
pin_ir = 0             # GPIO Pin for IR receiver
pin_rr_switch = a36    # Analog input pin for "Band Switch"
pin_rr_tspkr = t8      # Touch input for "Speaker maze"
pin_rr_tune = t9       # Touch input for "Tuning" capacitor
pin_rr_vol = a39       # Analog input pin for "Volume"
pin_sd_cs = -1         # GPIO Pin number for SD card "CS"
pin_spi_miso = 15
pin_spi_mosi = 2
pin_spi_sck = 14
pin_vs_cs = 13         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 16        # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 4        # GPIO Pin number for VS1053 "DREQ"

# Default Ethernet settings for Olimex PoE/PoE-ISO
eth_clk_mode = 3
eth_phy_power = 12

)====="
#elif defined(RADIO_JENS)
R"=====(
# Special pin configuration for Jens/Devkitv1 layout
# VS1053 connection defined. IR receiver connected to GPIO15.
# Other settings for documentation, but not evaluated by SW currently
pin_ir = 15           # GPIO Pin for IR receiver
pin_rr_led = 27       # GPIO Pin for RetroRadio LED
pin_rr_switch = a35   # GPIO Pin for RetroRadio Switch Knob (ADC1-Pin required)
pin_rr_tune = t6      # GPIO Pin for RetroRadio Tune Knob (Touch-Pin required)
pin_rr_vol = a33      # GPIO Pin for RetroRadio Volume Potentiometer (ADC1-Pin required)
pin_sd_cs = 22        # GPIO Pin number for SD card "CS"
pin_vs_cs = 5         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 21       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
)====="
#elif defined(ARDUINO_ODROID_ESP32)
R"=====(
# Default pin configuration for Odroid Go
# Only VS1053 connection defined. No IR receiver connected
pin-ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
pin_ir = -1    # GPIO Pin for IR receiver
pin_sd_cs = -1        # GPIO Pin number for SD card "CS" (22)
pin_spi_miso = 19
pin_spi_mosi = 23
pin_spi_sck = 18
pin_vs_cs = 12         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 15       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
)====="
#elif defined(ARDUINO_TTGO_T8)
R"=====(
# Default pin configuration for TTGO-T8
# Only VS1053 connection defined. No IR receiver connected
pin-ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
pin_ir = -1    # GPIO Pin for IR receiver
pin_sd_cs = 33        # GPIO Pin number for SD card "CS" (22)
pin_spi_miso = 25
pin_spi_mosi = 26
pin_spi_sck = 27
pin_vs_cs = 14         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 13       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 15       # GPIO Pin number for VS1053 "DREQ"
)====="
#elif defined(ARDUINO_LOLIN32)
R"=====(
# Default pin configuration for Wemos Lolin32
# Only VS1053 connection defined. No IR receiver connected
pin_ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
pin_ir = -1    # GPIO Pin for IR receiver
pin_reset = -1 #25
pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
pin_spi_miso = 19
pin_spi_mosi = 23
pin_spi_sck = 18
pin_vs_cs = 5         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 17       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 16       # GPIO Pin number for VS1053 "DREQ"
)====="
#else
R"=====(
# This default setting fits for DEVKIT-Version and for Lolin D32Pro V1.0.0
# No IR-Receiver is connected here.
pin_ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
pin_ir = -1    # GPIO Pin for IR receiver
pin_reset = 25
pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
pin_spi_miso = 12
pin_spi_mosi = 14
pin_spi_sck = 27
pin_vs_cs = 26         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 33       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 32       # GPIO Pin number for VS1053 "DREQ"
)====="
#endif  // Specific pin definitions
// Definition: Presets
#if defined(OMARADIO)
R"=====(
preset_00 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
preset_01 = avw.mdr.de/streams/284290-0_mp3_high.m3u # MDR Sachsen Anhalt
)====="
//#elif defined(RADIO_JENS)
R"=====(
)====="
#else                       // default presets
R"=====(
preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
preset_11 = br-brklassik-live.cast.addradio.de/br/brklassik/live/mp3/mid  #   BR Klassik
preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
preset_17 = 92.27.224.83:8000/ # Legacy 90.1
preset_18 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
preset_19 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_20 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
preset_21 = www.memoryradio.de:4000/                 #   Memoryradio 1
preset_22 = www.memoryradio.de:5000/                 #   Memoryradio 2
preset_23 = streamplus25.leonex.de:26116 # Radio Okerwelle
preset_24 = live.radioart.com/fCello_works.mp3       #   Cello Works
preset_25 = player.ffn.de/ffn.mp3 # Radio ffn
preset_26 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
preset_27 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
preset_28 = rs27.stream24.net/radio38.mp3 # Radio 38
preset_29 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
preset_30 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
preset_31 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
preset_32 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
preset_33 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
preset_34 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
preset_35 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
preset_36 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
preset_37 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
preset_38 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
preset_39 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
preset_40 = www.fro.at:8008/fro-128.ogg # Radio FRO
preset_41 = fc.macjingle.net:8200/ # Radio Technikum
preset_42 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
preset_44 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
preset_45 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
preset_46 = 212.77.178.166:80                        #  Radio Heimatmelodie
preset_47 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
)====="
#endif
// Definition: Generic for all environments
R"=====(
preset = 0
volume = 70
toneha = 5
tonehf = 3
tonela = 15
tonelf = 12
)====="
#if defined(BADRADIO)
#if defined(ARDUINO_ESP32_POE) || defined(ARDUINO_ESP32_POE_ISO)
R"=====(
$channels_am = 1,2,10,11,12,13,14,15,16
$channels_fm = 0,1,2,3,4,5,6,7,10
#
$equalizermax = 3
#
$$genres = brazilian music, 80s, oldies, japanese, eclectic, world music, classical, jazz, disco                         #
#
$tunemap = (0..95=1)(103..114=2)(123..136=3)(146..166=4)(185..209=5)(219..240=6)(254..264=7)(274..445=8)
#
$volmap = (100..4095=50..100)(=0)
$volmin = 50
#
::loop1 = if(.vol >= 0) = {ifnot(@hmilock & 2) = {volume=.vol}};.vol=-1
::loop2 = if(.channel > 0) = {if(.channel <= ~channels) = {ifnot(@hmilock & 1)={channel=.channel}};.-=channel}
::loop3 = if(.preset < 0)={return}; if(.preset==~preset)={return.preset=-1};ifnot(@hmilock&1)={preset=.preset};.preset=-1
::setup0 = in.tspkr=src=&pin_rr_tspkr,map=(0..250=0)(=1),start,on1click=:chan_any,onlong={downvolume=2},on1long={upvolume=2},t-repeat=150,t-click=450
::setup1 = in.switch=mode=0,src=&pin_rr_switch,map=(0..500=3)(3300..3600=2)(3900..4095=1)(=0),start,onchange={call=:user?},t-deb=150
::setup2 = in.vol = src=&pin_rr_vol,map=@$volmap,delta=2,start,onchange={.vol=?}
::setup3 = .preset=-1;in.tune = src=&pin_rr_tune,map=@$tunemap,start,onchange={.channel=?}
::setup4 = .eq_idx = 1;in.equalizer=src=.eq_idx,start,onchange={call=:equalizer?}
::setup5 = call=:user1
::setup6 = in.volobserver= src=~volume,map=(@$volmin..100=2)(0=1)(=0),onchange=:vol_adjust,start
#
:chan_any = if(~genre)={call=:chan_anyg;return};if(~channels > 1)={.x=~channel;while(.x == ~channel)={calc.x(1..~channels)};.channel=@x}
:chan_anyg = ifnot(@hmilock & 1) = {channel = any ;}
:chan_tune = if(.channel) = {channel=.channel};ram- = channel;
#
:eq_down = if(.eq_idx > 0)={calc.eq_idx(.eq_idx-1)}{.eq_idx = 0}
:eq_downwrap = if(.eq_idx > 0) = {calc.eq_idx(.eq_idx-1)}{.eq_idx = @$equalizermax}
:eq_set = call = :equalizer?
:eq_up = if(.eq_idx < @$equalizermax) = {calc.eq_idx(.eq_idx+1))}{.eq_idx = @$equalizermax}
:eq_upwrap = if(.eq_idx < @$equalizermax)= {calc.eq_idx(.eq_idx+1)}{.eq_idx = 0}
:equalizer0 = toneha = 5; tonehf = 3; tonela = 15; tonelf = 12
:equalizer1 = toneha = 7; tonehf = 4; tonela = 8; tonelf = 14
:equalizer2 = toneha=7; tonehf=4; tonela=15; tonelf=15
:equalizer3 = toneha=0; tonehf=3; tonela=0; tonelf=13
#
:genre = idx(?,@$$genres)={genre=?};preset=-1
:genreload = genre=--maintain 1;genre=--clearall;genre=--load @$$genres;genre=--pushback --maintain 0;genre=--pushback --cmd reset
:genrestop = genre=--stop;in.tune=onchange={.channel=?}
#
:presetforce=channel=-1;preset=-1;preset=?
#
:user1 = call=:genrestop;channels=@$channels_fm;.channel=1;.vol=70;.eq_idx=0;in.vol=on0=
:user2 = call=:genrestop;channels=@$channels_am;.channel=0;in.vol=start;in.tune=start;.eq_idx=1;in.vol=on0=:volchan_any
:user3 = in.tune=onchange={call(?)=:genre},start
#
:vol_adjust = if(?)={.lastvol = ~volume}{if(.lastvol)={volume=0}{volume=@$volmin}}
:volchan_any = if(@hmilock & 2)={}{call=:chan_any}
#
ir_02FD = call= :user2 # (>>|)
ir_10EF = ram.channel = 4 # (4)
ir_10EFr4 = call(4)=:genre # (4) longpressed
ir_18E7 = ram.channel = 2 # (2)
ir_18E7r4 = call(2)=:genre # (2) longpressed
ir_22DD = call = :user1 #(|<<)
ir_22DDR4 = upvolume = 2 # (|<<) longpressed
ir_30CF = ram.channel = 1 # (1)
ir_30CFr4 = call(1)=:genre # (1) longpressed
ir_38C7 = ram.channel = 5 # (5)
ir_38C7r4 = call(5)=:genre # (5) longpressed
ir_42BD = ram.channel = 7 # (7)
ir_42BDr4 = call(7)=:genre # (7) longpressed
ir_4AB5 = ram.channel = 8 # (8)
ir_4AB5r4 = call(8)=:genre # (8) longpressed
ir_52AD = ram.channel = 9 # (9)
ir_52ADr4 = call(9)=:genre # (9) longpressed
ir_5AA5 = ram.channel = 6 # (6)
ir_5AA5r4 = call(6)=:genre # (6) longpressed
ir_629D = call = :chan_any    #(CH)
ir_629DR22 = call = :chan_any #(CH) longpressed
ir_6897 = call(1)=:presetforce # (0)
ir_7A85 = ram.channel = 3 # (3)
ir_7A85r4 = call(3)=:genre # (3) longpressed
ir_906F = call = :eq_upwrap      #(EQ)
ir_906FR10 = call = :eq_upwrap   #(EQ) (longpressed)
ir_9867 = call(11)=:presetforce      #(100+)
ir_A25D = channel = down    #(CH-)
ir_A857 = upvolume = 2      #(+)
ir_A857r = upvolume = 1     #(+) repeat
ir_B04F = call(7)=:presetforce      # (200+) Radio Paradise
ir_C23D = mute              #(>||)
ir_E01F = downvolume = 2    #(-)
ir_E01Fr = downvolume = 1   #(-) repeat
ir_E21D = channel = up      #(CH+)
#
#pin-ir_rc5 = 0       # GPIO Pin for IR receiver (RC5 only)
#pin_ir = 0    # GPIO Pin for IR receiver
#pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
#pin_spi_miso = 15
#pin_spi_mosi = 2
#pin_spi_sck = 14
#pin_vs_cs = 13         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 16       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
#
#preset = 0
#preset = 1
#preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
#preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
#preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
#preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
#preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
#preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
#preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
#preset_11 = br-brklassik-live.cast.addradio.de/br/brklassik/live/mp3/mid  #   BR Klassik
#preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
#preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
#preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
#preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
#preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
#preset_17 = 92.27.224.83:8000/ # Legacy 90.1
#preset_18 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
#preset_19 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
#preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_20 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
#preset_21 = www.memoryradio.de:4000/                 #   Memoryradio 1
#preset_22 = www.memoryradio.de:5000/                 #   Memoryradio 2
#preset_23 = streamplus25.leonex.de:26116 # Radio Okerwelle
#preset_24 = live.radioart.com/fCello_works.mp3       #   Cello Works
#preset_25 = player.ffn.de/ffn.mp3 # Radio ffn
#preset_26 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
#preset_27 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
#preset_28 = rs27.stream24.net/radio38.mp3 # Radio 38
#preset_29 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
#preset_30 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
#preset_31 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
#preset_32 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
#preset_33 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
#preset_34 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
#preset_35 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
#preset_36 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
#preset_37 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
#preset_38 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
#preset_39 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
#preset_40 = www.fro.at:8008/fro-128.ogg # Radio FRO
#preset_41 = fc.macjingle.net:8200/ # Radio Technikum
#preset_42 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
#preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
#preset_44 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
#preset_45 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
#preset_46 = 212.77.178.166:80                        #  Radio Heimatmelodie
#preset_47 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
#
toneha = 5
tonehf = 3
tonela = 15
tonelf = 12
#
volume = 88
#
)=====" ;
#else           // from if defined(Olimex PoE) within BADRADIO
R"=====(
$channels_am = 1,2,10,11,12,13,14,15,16
$channels_fm = 0,1,2,3,4,5,6,7,10
#
$equalizermax = 3
#
$$genres = brazilian music, 80s, oldies, japanese, eclectic, world music, classical, jazz, disco                         #
#
$tunemap = (0..95=1)(103..114=2)(123..136=3)(146..166=4)(185..209=5)(219..240=6)(254..264=7)(274..445=8)
#
$volmap = (100..4095=50..100)(=0)
$volmin = 50
#
::loop1 = if(.vol >= 0) = {ifnot(@hmilock & 2) = {volume=.vol}};.vol=-1
::loop2 = if(.channel)={if(~channels < 2)={.-=channel;return;};if(@hmilock&1)={.-=channel;return};if ( .channel > ~channels) = {.-=channel;return};channel=.channel;.-=channel}
::loop3 = if(.preset < 0)={return}; if(.preset==~preset)={return.preset=-1};ifnot(@hmilock&1)={preset=.preset};.preset=-1
::setup0 = #in.switch=mode=0,src=a35,map=(0..500=3)(1500..2500=2)(3900..4095=1)(=0),start,onchange={call=:user?}
::setup1 = #in.vol = src=a33,map=@$volmap,delta=2,start,onchange={.vol=?}
::setup2 = .preset=-1;#in.tune = src=&pin_rr_tune,map=@$tunemap,start,onchange={.channel=?}
::setup3 = .eq_idx = 1;in.equalizer=src=.eq_idx,start,onchange={call=:equalizer?}
::setup4 = call=:user1
::setup5 = in.volobserver= src=~volume,map=(@$volmin..100=2)(0=1)(=0),onchange=:vol_adjust,start
::setup6 = #in.tspkr=src=t5,map=(0..250=0)(=1),on1click=:chan_any,onlong={downvolume=1},on1long={upvolume=1},t-repeat=150,t-click=450,start
#
:chan_any = if(~genre)={call=:chan_anyg;return};if(~channels > 1)={.x=~channel;while(.x == ~channel)={calc.x(1..~channels)};.channel=@x}
:chan_anyg = ifnot(@hmilock & 1) = {channel = any ;}
:chan_tune = if(.channel) = {channel=.channel};ram- = channel;
#
:eq_down = if(.eq_idx > 0)={calc.eq_idx(.eq_idx-1)}{.eq_idx = 0}
:eq_downwrap = if(.eq_idx > 0) = {calc.eq_idx(.eq_idx-1)}{.eq_idx = @$equalizermax}
:eq_set = call = :equalizer?
:eq_up = if(.eq_idx < @$equalizermax) = {calc.eq_idx(.eq_idx+1))}{.eq_idx = @$equalizermax}
:eq_upwrap = if(.eq_idx < @$equalizermax)= {calc.eq_idx(.eq_idx+1)}{.eq_idx = 0}
:equalizer0 = toneha = 5; tonehf = 3; tonela = 15; tonelf = 12
:equalizer1 = toneha = 7; tonehf = 4; tonela = 8; tonelf = 14
:equalizer2 = toneha=7; tonehf=4; tonela=15; tonelf=15
:equalizer3 = toneha=0; tonehf=3; tonela=0; tonelf=13
#
:genre = idx(?,@$$genres)={genre=?};preset=-1
:genreload = genre=--maintain 1;genre=--clearall;genre=--load @$$genres;genre=--pushback --maintain 0;genre=--pushback --cmd reset
:genrestop = genre=--stop;in.tune=onchange={.channel=?}
#
:presetforce=channel=-1;preset=-1;preset=?
#
:user1 = call=:genrestop;channels=@$channels_fm;.channel=1;.vol=70;.eq_idx=0;in.vol=on0=
:user2 = call=:genrestop;channels=@$channels_am;.channel=0;in.vol=start;in.tune=start;.eq_idx=1;#in.vol=on0=:volchan_any
:user3 = in.tune=onchange={call(?)=:genre},start
#
:vol_adjust = if(?)={.lastvol = ~volume}{if(.lastvol)={volume=0}{volume=@$volmin}}
:volchan_any = if(@hmilock & 2)={}{call=:chan_any}
#
ir_02FD = call= :user2 # (>>|)
ir_10EF = ram.channel = 4 # (4)
ir_10EFr4 = call(4)=:genre # (4) longpressed
ir_18E7 = ram.channel = 2 # (2)
ir_18E7r4 = call(2)=:genre # (2) longpressed
ir_22DD = call = :user1 #(|<<)
ir_22DDR4 = upvolume = 2 # (|<<) longpressed
ir_30CF = ram.channel = 1 # (1)
ir_30CFr4 = call(1)=:genre # (1) longpressed
ir_38C7 = ram.channel = 5 # (5)
ir_38C7r4 = call(5)=:genre # (5) longpressed
ir_42BD = ram.channel = 7 # (7)
ir_42BDr4 = call(7)=:genre # (7) longpressed
ir_4AB5 = ram.channel = 8 # (8)
ir_4AB5r4 = call(8)=:genre # (8) longpressed
ir_52AD = ram.channel = 9 # (9)
ir_52ADr4 = call(9)=:genre # (9) longpressed
ir_5AA5 = ram.channel = 6 # (6)
ir_5AA5r4 = call(6)=:genre # (6) longpressed
ir_629D = call = :chan_any    #(CH)
ir_629DR22 = call = :chan_any #(CH) longpressed
ir_6897 = call(1)=:presetforce # (0)
ir_7A85 = ram.channel = 3 # (3)
ir_7A85r4 = call(3)=:genre # (3) longpressed
ir_906F = call = :eq_upwrap      #(EQ)
ir_906FR10 = call = :eq_upwrap   #(EQ) (longpressed)
ir_9867 = call(11)=:presetforce      #(100+)
ir_A25D = channel = down    #(CH-)
ir_A857 = upvolume = 2      #(+)
ir_A857r = upvolume = 1     #(+) repeat
ir_B04F = call(7)=:presetforce      # (200+) Radio Paradise
ir_C23D = mute              #(>||)
ir_E01F = downvolume = 2    #(-)
ir_E01Fr = downvolume = 1   #(-) repeat
ir_E21D = channel = up      #(CH+)
#
#pin_ir = 15           # GPIO Pin for IR receiver
#pin_rr_led = 27       # GPIO Pin for RetroRadio LED
#pin_rr_switch = a35   # GPIO Pin for RetroRadio Switch Knob (ADC1-Pin required)
#pin_rr_tune = t6      # GPIO Pin for RetroRadio Tune Knob (Touch-Pin required)
#pin_rr_vol = a33      # GPIO Pin for RetroRadio Volume Potentiometer (ADC1-Pin required)
#pin_sd_cs = 22        # GPIO Pin number for SD card "CS"
#pin_vs_cs = 5         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 21       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
#
#preset = 1
#preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
#preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
#preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
#preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
#preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
#preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
#preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
#preset_11 = br-brklassik-live.cast.addradio.de/br/brklassik/live/mp3/mid  #   BR Klassik
#preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
#preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
#preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
#preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
#preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
#preset_17 = 92.27.224.83:8000/ # Legacy 90.1
#preset_18 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
#preset_19 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
#preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_20 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
#preset_21 = www.memoryradio.de:4000/                 #   Memoryradio 1
#preset_22 = www.memoryradio.de:5000/                 #   Memoryradio 2
#preset_23 = streamplus25.leonex.de:26116 # Radio Okerwelle
#preset_24 = live.radioart.com/fCello_works.mp3       #   Cello Works
#preset_25 = player.ffn.de/ffn.mp3 # Radio ffn
#preset_26 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
#preset_27 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
#preset_28 = rs27.stream24.net/radio38.mp3 # Radio 38
#preset_29 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
#preset_30 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
#preset_31 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
#preset_32 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
#preset_33 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
#preset_34 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
#preset_35 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
#preset_36 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
#preset_37 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
#preset_38 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
#preset_39 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
#preset_40 = www.fro.at:8008/fro-128.ogg # Radio FRO
#preset_41 = fc.macjingle.net:8200/ # Radio Technikum
#preset_42 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
#preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
#preset_44 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
#preset_45 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
#preset_46 = 212.77.178.166:80                        #  Radio Heimatmelodie
#preset_47 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
#
#toneha = 7
#tonehf = 4
#tonela = 8
#tonelf = 14
#
volume = 68
#
)=====" 
R"=====(
#wifi_00 = SSID/passwd
)=====" ;
#endif              // (of if defined(Olimex PoE) within Badradio)
#elif defined(ARDUINO_ODROID_ESP32)               // (of if defined BadRadio) (i. e. generic from here..., Start with OdroidGo)
R"=====(
$channels_am = 1,2,10,11,12,13,14,15,16
$channels_fm = 0,1,2,3,4,5,6,7,10
#
$equalizermax = 3
#
$$genres = brazilian music, 80s, oldies, japanese, eclectic, world music, classical, jazz, disco                         #
#
$tunemap = (100..115=1)(120..130=2)(135..150=3)(160..185=4)(205..250=5)(285..330=6)(350..400=7)(410..445=8) (460..590=9)
#
$volmap = (100..4095=50..100)(=0)
$volmin = 50
#
::loop1 = if(.vol >= 0) = {ifnot(@hmilock & 2) = {volume=.vol}};.vol=-1
::loop2 = if(.channel > 0) = {if(.channel <= ~channels) = {ifnot(@hmilock & 1)={channel=.channel}};.-=channel}
::loop3 = if(.preset < 0)={return}; if(.preset==~preset)={return.preset=-1};ifnot(@hmilock&1)={preset=.preset};.preset=-1
::setup0 = #in.switch=mode=0,src=&pin_rr_switch,map=(0..500=1)(0..4095=3),start,onchange={call=:user?}
::setup1 = #in.vol = src=a39,map=@$volmap,delta=2,start,onchange={.vol=?}
::setup2 = .preset=-1;#in.tune = src=&pin_rr_tune,map=@$tunemap,start,onchange={.channel=?}
::setup3 = .eq_idx = 1;in.equalizer=src=.eq_idx,start,onchange={call=:equalizer?}
::setup4 = call=:user1
::setup5 = in.volobserver= src=~volume,map=(@$volmin..100=2)(0=1)(=0),onchange=:vol_adjust,start
#
:chan_any = if(~genre)={call=:chan_anyg;return};if(~channels > 1)={.x=~channel;while(.x == ~channel)={calc.x(1..~channels)};.channel=@x}
:chan_anyg = ifnot(@hmilock & 1) = {channel = any ;}
:chan_tune = if(.channel) = {channel=.channel};ram- = channel;
#
:eq_down = if(.eq_idx > 0)={calc.eq_idx(.eq_idx-1)}{.eq_idx = 0}
:eq_downwrap = if(.eq_idx > 0) = {calc.eq_idx(.eq_idx-1)}{.eq_idx = @$equalizermax}
:eq_set = call = :equalizer?
:eq_up = if(.eq_idx < @$equalizermax) = {calc.eq_idx(.eq_idx+1))}{.eq_idx = @$equalizermax}
:eq_upwrap = if(.eq_idx < @$equalizermax)= {calc.eq_idx(.eq_idx+1)}{.eq_idx = 0}
:equalizer0 = toneha = 5; tonehf = 3; tonela = 15; tonelf = 12
:equalizer1 = toneha = 7; tonehf = 4; tonela = 8; tonelf = 14
:equalizer2 = toneha=7; tonehf=4; tonela=15; tonelf=15
:equalizer3 = toneha=0; tonehf=3; tonela=0; tonelf=13
#
:genre = idx(?,@$$genres)={genre=?};preset=-1;
:genreload = genre=--maintain 1;genre=--clearall;genre=--load @$$genres;genre=--pushback --maintain 0;genre=--pushback --cmd reset
:genrestop = genre=--stop;in.tune=onchange={.channel=?}
#
:presetforce=channel=-1;preset=-1;preset=?
#
:user1 = call=:genrestop;channels=@$channels_fm;.channel=1;.vol=70;.eq_idx=0;in.vol=on0=
:user2 = call=:genrestop;channels=@$channels_am;.channel=0;in.vol=start;in.tune=start;.eq_idx=1;in.vol=on0=:volchan_any
:user3 = in.tune=onchange={call(?)=:genre},start
#
:vol_adjust = if(?)={.lastvol = ~volume}{if(.lastvol)={volume=0}{volume=@$volmin}}
:volchan_any = if(@hmilock & 2)={}{call=:chan_any}
#
eth_clk_mode = 3
eth_phy_power = 12
#
ir_02FD = call= :user2 # (>>|)
ir_10EF = ram.channel = 4 # (4)
ir_10EFr4 = call(4)=:genre # (4) longpressed
ir_18E7 = ram.channel = 2 # (2)
ir_18E7r4 = call(2)=:genre # (2) longpressed
ir_22DD = call = :user1 #(|<<)
ir_22DDR4 = upvolume = 2 # (|<<) longpressed
ir_30CF = ram.channel = 1 # (1)
ir_30CFr4 = call(1)=:genre # (1) longpressed
ir_38C7 = ram.channel = 5 # (5)
ir_38C7r4 = call(5)=:genre # (5) longpressed
ir_42BD = ram.channel = 7 # (7)
ir_42BDr4 = call(7)=:genre # (7) longpressed
ir_4AB5 = ram.channel = 8 # (8)
ir_4AB5r4 = call(8)=:genre # (8) longpressed
ir_52AD = ram.channel = 9 # (9)
ir_52ADr4 = call(9)=:genre # (9) longpressed
ir_5AA5 = ram.channel = 6 # (6)
ir_5AA5r4 = call(6)=:genre # (6) longpressed
ir_629D = call = :chan_any    #(CH)
ir_629DR22 = call = :chan_any #(CH) longpressed
ir_6897 = call(1)=:presetforce # (0)
ir_7A85 = ram.channel = 3 # (3)
ir_7A85r4 = call(3)=:genre # (3) longpressed
ir_906F = call = :eq_upwrap      #(EQ)
ir_906FR10 = call = :eq_upwrap   #(EQ) (longpressed)
ir_9867 = call(11)=:presetforce      #(100+)
ir_A25D = channel = down    #(CH-)
ir_A857 = upvolume = 2      #(+)
ir_A857r = upvolume = 1     #(+) repeat
ir_B04F = call(7)=:presetforce      # (200+) Radio Paradise
ir_C23D = mute              #(>||)
ir_E01F = downvolume = 2    #(-)
ir_E01Fr = downvolume = 1   #(-) repeat
ir_E21D = channel = up      #(CH+)
#
#pin-ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
#pin_ir = -1    # GPIO Pin for IR receiver
#pin_sd_cs = -1        # GPIO Pin number for SD card "CS" (22)
#pin_spi_miso = 19
#pin_spi_mosi = 23
#pin_spi_sck = 18
#pin_vs_cs = 12         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 15       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"

#{ "pin_spi_sck",   &ini_block.spi_sck_pin,      18 },
#{ "pin_spi_miso",  &ini_block.spi_miso_pin,     19 },
#{ "pin_spi_mosi",  &ini_block.spi_mosi_pin,     23 },
#{ "pin_sd_cs",     &ini_block.sd_cs_pin,        22 },
#{ "pin_vs_dreq",   &ini_block.vs_dreq_pin,      4 },
#{ "pin_vs_cs",     &ini_block.vs_cs_pin,        12 },
#{ "pin_vs_dcs",    &ini_block.vs_dcs_pin,       15 },
#{ "pin_tft_cs",    &ini_block.tft_cs_pin,       5 },
    

#
#preset = 6
#preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
#preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
#preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
#preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
#preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
#preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
#preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
#preset_11 = br-brklassik-live.cast.addradio.de/br/brklassik/live/mp3/mid  #   BR Klassik
#preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
#preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
#preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
#preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
#preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
#preset_17 = 92.27.224.83:8000/ # Legacy 90.1
#preset_18 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
#preset_19 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
#preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_20 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
#preset_21 = www.memoryradio.de:4000/                 #   Memoryradio 1
#preset_22 = www.memoryradio.de:5000/                 #   Memoryradio 2
#preset_23 = streamplus25.leonex.de:26116 # Radio Okerwelle
#preset_24 = live.radioart.com/fCello_works.mp3       #   Cello Works
#preset_25 = player.ffn.de/ffn.mp3 # Radio ffn
#preset_26 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
#preset_27 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
#preset_28 = rs27.stream24.net/radio38.mp3 # Radio 38
#preset_29 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
#preset_30 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
#preset_31 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
#preset_32 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
#preset_33 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
#preset_34 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
#preset_35 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
#preset_36 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
#preset_37 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
#preset_38 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
#preset_39 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
#preset_40 = www.fro.at:8008/fro-128.ogg # Radio FRO
#preset_41 = fc.macjingle.net:8200/ # Radio Technikum
#preset_42 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
#preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
#preset_44 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
#preset_45 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
#preset_46 = 212.77.178.166:80                        #  Radio Heimatmelodie
#preset_47 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
#
#toneha = 5
#tonehf = 3
#tonela = 15
#tonelf = 12
#
#volume = 70
#
#wifi_00 = SSID/passwd
)=====" ;
#elif defined(OMARADIO)                  // for #elif defined(ODROID_ESP32)
R"=====(
$vol = 83
#
::setup1 = in.tspkr= src=t0,map=(0..275=0)(=1),start,on1click={call=:changechan},onlong={if(?)={call=:volchange}{calc.voldir(.voldir^1)}}
::setup2 = channels=0,1;channel=1;volume=@$vol;.voldir=1
::setup3 = in.tspkr = t-repeat=200
::setup4 = in.tspkr=on3click={call=:genrechange}
::setup5 = in.tspkr=on2click={if(~genre)={call=:gpresetchange}}
#
:changechan = if(~genre)={genre=--stop};channel=any
#
:genrechange = ifnot(~genre)={idx.x(0, ~glist);ifnot(.x)={return;}};idx.y(1...x,~glist)={genre=?}
:gpresetchange = calc(1..~gpresets)={gpreset=?}
#
:volchange = if(.voldir)={call=:voldown}{call=:volup}
:voldown = if(~volume > 90)={downvolume = 1;return};if(~volume > 60)={downvolume = 1}{volume = 60}
:volup = if(~volume <= 90)={upvolume = 1}{upvolume=1}
#
#mqttbroker = hcuc9yjisphkpbzs.myfritz.net
#mqttpasswd = 
#mqttport = 1883
#mqttprefix = opa/radio/oma
#mqttuser = opa
#
#pin-ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
#pin_ir = -1    # GPIO Pin for IR receiver
#pin_reset = 25
#pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
#pin_spi_miso = 12
#pin_spi_mosi = 14
#pin_spi_sck = 27
#pin_vs_cs = 26         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 33       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 32       # GPIO Pin number for VS1053 "DREQ"
#
#preset = 0
#preset_00 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_01 = avw.mdr.de/streams/284290-0_mp3_high.m3u # MDR Sachsen Anhalt
#
#toneha = 4
#tonehf = 3
#tonela = 2
#tonelf = 12
#
#volume = 85
#
#wifi_00 = SSID/passwd
)=====" ;
#elif defined (DEVKIT4MB)                                   // for #elif defined(OMARADIO)
R"=====(
$channels = 1,2,10,11,12,13,14,15,16
::setup1 = in.tspkr= src=t3,map=(0..300=0)(=1),start,on1click={channel=any},onlong={if(?)={call=:volchange}{calc.voldir(.voldir^1)}}
::setup2 = channels=@$channels;channel=1;volume=85;.voldir=1
::setup3 = in.tspkr = t-click=0,t-repeat=200
::setup4 = in.tcase = src=t3,map=(420..470=0)(=1),start,onlong={if(?)={genre=--stop;preset=0;channel=1,volume=85}},on2long={if(?)={genre=jazz}},t-click=2000,t-long=2000,t-repeat=50000
#
:volchange = if(.voldir)={call=:voldown}{call=:volup}
:voldown = if(~volume > 90)={downvolume = 1;return};if(~volume > 60)={downvolume = 2}{volume = 60}
:volup = if(~volume <= 90)={upvolume = 2}{upvolume=1}
#
mqttbroker = hcuc9yjisphkpbzs.myfritz.net
mqttpasswd = 
mqttport = 1883
mqttprefix = opa/radio/oma
mqttuser = opa
#
#pin-ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
#pin_ir = -1    # GPIO Pin for IR receiver
#pin_reset = 25
#pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
#pin_spi_miso = 12
#pin_spi_mosi = 14
#pin_spi_sck = 27
#pin_vs_cs = 26         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 33       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 32       # GPIO Pin number for VS1053 "DREQ"
#
#preset = 0
#preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
#preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
#preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
#preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
#preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
#preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
#preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
#preset_11 = br-brklassik-live.cast.addradio.de/br/brklassik/live/mp3/mid  #   BR Klassik
#preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
#preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
#preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
#preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
#preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
#preset_17 = 92.27.224.83:8000/ # Legacy 90.1
#preset_18 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
#preset_19 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
#preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_20 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
#preset_21 = www.memoryradio.de:4000/                 #   Memoryradio 1
#preset_22 = www.memoryradio.de:5000/                 #   Memoryradio 2
#preset_23 = streamplus25.leonex.de:26116 # Radio Okerwelle
#preset_24 = live.radioart.com/fCello_works.mp3       #   Cello Works
#preset_25 = player.ffn.de/ffn.mp3 # Radio ffn
#preset_26 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
#preset_27 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
#preset_28 = rs27.stream24.net/radio38.mp3 # Radio 38
#preset_29 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
#preset_30 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
#preset_31 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
#preset_32 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
#preset_33 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
#preset_34 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
#preset_35 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
#preset_36 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
#preset_37 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
#preset_38 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
#preset_39 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
#preset_40 = www.fro.at:8008/fro-128.ogg # Radio FRO
#preset_41 = fc.macjingle.net:8200/ # Radio Technikum
#preset_42 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
#preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
#preset_44 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
#preset_45 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
#preset_46 = 212.77.178.166:80                        #  Radio Heimatmelodie
#preset_47 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
#
#toneha = 4
#tonehf = 3
#tonela = 2
#tonelf = 12
#
#volume = 85
#
#wifi_00 = SSID/passwd
)=====" ;
#elif defined(ARDUINO_LOLIN32)                              // for #elif defined(DEVKIT4MB)
R"=====(
$$genres = brazilian music, 80s, oldies, japanese, eclectic, world music, classical, jazz, disco          
::setup1 = #in.tspkr= src=t3,map=(0..300=0)(=1),start,on0={channel=any}
::setup2 = channels=0,1;channel=1
#
#pin-ir_rc5 = -1       # GPIO Pin for IR receiver (RC5 only)
#pin_ir = -1    # GPIO Pin for IR receiver
#pin_reset = -1 #25
#pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
#pin_spi_miso = 19
#pin_spi_mosi = 23
#pin_spi_sck = 18
#pin_vs_cs = 5         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 17       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 16       # GPIO Pin number for VS1053 "DREQ"
#
#preset = 0
#preset_00 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_01 = avw.mdr.de/streams/284290-0_mp3_high.m3u # MDR Sachsen Anhalt
#
#toneha = 4
#tonehf = 3
#tonela = 2
#tonelf = 12
#
#volume = 90
#
#wifi_00 = SSID/passwd
)=====" ;
#else                               //for #elif defined(ARDUINO_WEMOS_LOLIN32)
R"=====(
$channels_am = 1,2,10,11,12,13,14,15,16
$channels_fm = 0,1,2,3,4,5,6,7,10
#
$equalizermax = 3
#
$$genres = brazilian music, 80s, oldies, japanese, eclectic, world music, classical, jazz, disco                         #
#
$tunemap = (100..115=1)(120..130=2)(135..150=3)(160..185=4)(205..250=5)(285..330=6)(350..400=7)(410..445=8) (460..590=9)
#
$volmap = (100..4095=50..100)(=0)
$volmin = 50
#
::loop1 = if(.vol >= 0) = {ifnot(@hmilock & 2) = {volume=.vol}};.vol=-1
::loop2 = if(.channel > 0) = {if(.channel <= ~channels) = {ifnot(@hmilock & 1)={channel=.channel}};.-=channel}
::loop3 = if(.preset < 0)={return}; if(.preset==~preset)={return.preset=-1};ifnot(@hmilock&1)={preset=.preset};.preset=-1
::setup0 = in.switch=mode=0,src=&pin_rr_switch,map=(0..500=1)(0..4095=3),start,onchange={call=:user?}
::setup1 = in.vol = src=&pin_rr_vol,map=@$volmap,delta=2,start,onchange={.vol=?}
::setup2 = .preset=-1;in.tune = src=&pin_rr_tune,map=@$tunemap,start,onchange={.channel=?}
::setup3 = .eq_idx = 1;in.equalizer=src=.eq_idx,start,onchange={call=:equalizer?}
::setup4 = call=:user1
::setup5 = in.volobserver= src=~volume,map=(@$volmin..100=2)(0=1)(=0),onchange=:vol_adjust,start
#
:chan_any = if(~genre)={call=:chan_anyg;return};if(~channels > 1)={.x=~channel;while(.x == ~channel)={calc.x(1..~channels)};.channel=@x}
:chan_anyg = ifnot(@hmilock & 1) = {channel = any ;}
:chan_tune = if(.channel) = {channel=.channel};ram- = channel;
#
:eq_down = if(.eq_idx > 0)={calc.eq_idx(.eq_idx-1)}{.eq_idx = 0}
:eq_downwrap = if(.eq_idx > 0) = {calc.eq_idx(.eq_idx-1)}{.eq_idx = @$equalizermax}
:eq_set = call = :equalizer?
:eq_up = if(.eq_idx < @$equalizermax) = {calc.eq_idx(.eq_idx+1))}{.eq_idx = @$equalizermax}
:eq_upwrap = if(.eq_idx < @$equalizermax)= {calc.eq_idx(.eq_idx+1)}{.eq_idx = 0}
:equalizer0 = toneha = 5; tonehf = 3; tonela = 15; tonelf = 12
:equalizer1 = toneha = 7; tonehf = 4; tonela = 8; tonelf = 14
:equalizer2 = toneha=7; tonehf=4; tonela=15; tonelf=15
:equalizer3 = toneha=0; tonehf=3; tonela=0; tonelf=13
#
:genre = idx(?,@$$genres)={genre=?};preset=-1;
:genreload = genre=--maintain 1;genre=--clearall;genre=--load @$$genres;genre=--pushback --maintain 0;genre=--pushback --cmd reset
:genrestop = genre=--stop;in.tune=onchange={.channel=?}
#
:presetforce=channel=-1;preset=-1;preset=?
#
:user1 = call=:genrestop;channels=@$channels_fm;.channel=1;.vol=70;.eq_idx=0;in.vol=on0=
:user2 = call=:genrestop;channels=@$channels_am;.channel=0;in.vol=start;in.tune=start;.eq_idx=1;in.vol=on0=:volchan_any
:user3 = in.tune=onchange={call(?)=:genre},start
#
:vol_adjust = if(?)={.lastvol = ~volume}{if(.lastvol)={volume=0}{volume=@$volmin}}
:volchan_any = if(@hmilock & 2)={}{call=:chan_any}
#
eth_clk_mode = 3
eth_phy_power = 12
#
gpio_33 = call=:chan_any
#
ir_02FD = call= :user2 # (>>|)
ir_10EF = ram.channel = 4 # (4)
ir_10EFr4 = call(4)=:genre # (4) longpressed
ir_18E7 = ram.channel = 2 # (2)
ir_18E7r4 = call(2)=:genre # (2) longpressed
ir_22DD = call = :user1 #(|<<)
ir_22DDR4 = upvolume = 2 # (|<<) longpressed
ir_30CF = ram.channel = 1 # (1)
ir_30CFr4 = call(1)=:genre # (1) longpressed
ir_38C7 = ram.channel = 5 # (5)
ir_38C7r4 = call(5)=:genre # (5) longpressed
ir_42BD = ram.channel = 7 # (7)
ir_42BDr4 = call(7)=:genre # (7) longpressed
ir_4AB5 = ram.channel = 8 # (8)
ir_4AB5r4 = call(8)=:genre # (8) longpressed
ir_52AD = ram.channel = 9 # (9)
ir_52ADr4 = call(9)=:genre # (9) longpressed
ir_5AA5 = ram.channel = 6 # (6)
ir_5AA5r4 = call(6)=:genre # (6) longpressed
ir_629D = call = :chan_any    #(CH)
ir_629DR22 = call = :chan_any #(CH) longpressed
ir_6897 = call(1)=:presetforce # (0)
ir_7A85 = ram.channel = 3 # (3)
ir_7A85r4 = call(3)=:genre # (3) longpressed
ir_906F = call = :eq_upwrap      #(EQ)
ir_906FR10 = call = :eq_upwrap   #(EQ) (longpressed)
ir_9867 = call(11)=:presetforce      #(100+)
ir_A25D = channel = down    #(CH-)
ir_A857 = upvolume = 2      #(+)
ir_A857r = upvolume = 1     #(+) repeat
ir_B04F = call(7)=:presetforce      # (200+) Radio Paradise
ir_C23D = mute              #(>||)
ir_E01F = downvolume = 2    #(-)
ir_E01Fr = downvolume = 1   #(-) repeat
ir_E21D = channel = up      #(CH+)
#
#pin-ir_rc5 = 0       # GPIO Pin for IR receiver (RC5 only)
#pin_ir = 0    # GPIO Pin for IR receiver
#pin_sd_cs = -1        # GPIO Pin number for SD card "CS"
#pin_spi_miso = 15
#pin_spi_mosi = 2
#pin_spi_sck = 14
#pin_vs_cs = 13         # GPIO Pin number for VS1053 "CS"
#pin_vs_dcs = 16       # GPIO Pin number for VS1053 "DCS" (war 32)
#pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
#
#preset = 6
#preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
#preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
#preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
#preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
#preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
#preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
#preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
#preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
#preset_11 = br-brklassik-live.cast.addradio.de/br/brklassik/live/mp3/mid  #   BR Klassik
#preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
#preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
#preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
#preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
#preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
#preset_17 = 92.27.224.83:8000/ # Legacy 90.1
#preset_18 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
#preset_19 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
#preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
#preset_20 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
#preset_21 = www.memoryradio.de:4000/                 #   Memoryradio 1
#preset_22 = www.memoryradio.de:5000/                 #   Memoryradio 2
#preset_23 = streamplus25.leonex.de:26116 # Radio Okerwelle
#preset_24 = live.radioart.com/fCello_works.mp3       #   Cello Works
#preset_25 = player.ffn.de/ffn.mp3 # Radio ffn
#preset_26 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
#preset_27 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
#preset_28 = rs27.stream24.net/radio38.mp3 # Radio 38
#preset_29 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
#preset_30 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
#preset_31 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
#preset_32 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
#preset_33 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
#preset_34 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
#preset_35 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
#preset_36 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
#preset_37 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
#preset_38 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
#preset_39 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
#preset_40 = www.fro.at:8008/fro-128.ogg # Radio FRO
#preset_41 = fc.macjingle.net:8200/ # Radio Technikum
#preset_42 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
#preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
#preset_44 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
#preset_45 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
#preset_46 = 212.77.178.166:80                        #  Radio Heimatmelodie
#preset_47 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
#
#toneha = 5
#tonehf = 3
#tonela = 15
#tonelf = 12
#
#volume = 70
#
#wifi_00 = SSID/passwd
)=====" ;


#endif // #if defined(BADRADIO) #else
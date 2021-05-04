// Default preferences in raw data format for PROGMEM
//
#define defaultprefs_version 1808016
const char defprefs_txt[] PROGMEM = 
R"=====(
$channels_am = 1,2,10,11,12,13,14,15,16
$channels_fm = 0,1,2,3,4,5,6,7,10
#
$equalizermax = 3
#
$tunemap = (100..115=1)(120..130=2)(135..150=3)(160..185=4)(205..250=5)(285..330=6)(350..400=7)(410..445=8) (460..590=9)
#
$volmap = (100..4095=50..100)(=0)
$volmin = 50
#
::loop1 = if(.vol >= 0) = {ifnot(@hmilock & 2) = {volume=.vol}};.vol=-1
::loop2 = if(.channel)={if(~channels < 2)={.-=channel;return;};if(@hmilock&1)={.-=channel;return};if ( .channel > ~channels) = {.-=channel;return};channel=.channel;.-=channel}
::loop3 = if(.preset < 0)={return}; if(.preset==~preset)={return.preset=-1};ifnot(@hmilock&1)={preset=.preset};.preset=-1
::setup0 = in.switch=mode=0,src=a36,map=(0..500=1)(0..4095=2),start,onchange={call=:user?}
::setup1 = in.vol = src=a39,map=@$volmap,delta=2,start,onchange={.vol=?}
::setup2 = .preset=-1;in.tune = src=t9,map=@$tunemap,start,onchange={.channel=?}
::setup3 = .eq_idx = 1;in.equalizer=src=.eq_idx,start,onchange={call=:equalizer?}
::setup4 = call=:user1
::setup5 = in.volobserver= src=~volume,map=(@$volmin..100=2)(0=1)(=0),onchange=:vol_adjust,start
#
:chan_any = if(~channels > 1)={.x=~channel;while(.x == ~channel)={calc.x(1><~channels)};.channel=@x}
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
:user1 = channels=@$channels_fm;.channel=1;.vol=70;.eq_idx=0;in.vol=on0=
:user2 = channels=@$channels_am;.channel=0;in.vol=start;in.tune=start;.eq_idx=1;in.vol=on0=:volchan_any
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
ir_18E7 = ram.channel = 2 # (2)
ir_22DD = call = :user1 #(|<<)
ir_22DDR4 = upvolume = 2 # (|<<) longpressed
ir_30CF = ram.channel = 1 # (1)
ir_38C7 = ram.channel = 5 # (5)
ir_42BD = ram.channel = 7 # (7)
ir_4AB5 = ram.channel = 8 # (8)
ir_52AD = ram.channel = 9 # (9)
ir_5AA5 = ram.channel = 6 # (6)
ir_629D = call = :chan_any    #(CH)
ir_629DR22 = call = :chan_any #(CH) longpressed
ir_6897 = .preset=1      # (0)
ir_7A85 = ram.channel = 3 # (3)
ir_906F = call = :eq_upwrap      #(EQ)
ir_906FR10 = call = :eq_upwrap   #(EQ) (longpressed)
ir_9867 = .preset = 11      #(100+)
ir_A25D = channel = down    #(CH-)
ir_A857 = upvolume = 2      #(+)
ir_A857r = upvolume = 1     #(+) repeat
ir_C23D = mute              #(>||)
ir_E01F = downvolume = 2    #(-)
ir_E01Fr = downvolume = 1   #(-) repeat
ir_E21D = channel = up      #(CH+)
#
pin-ir_rc5 = 0       # GPIO Pin for IR receiver (RC5 only)
pin_ir = 0    # GPIO Pin for IR receiver
pin_sd_cs = 5        # GPIO Pin number for SD card "CS"
pin_spi_miso = 15
pin_spi_mosi = 2
pin_spi_sck = 14
pin_vs_cs = 13         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 16       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
#
preset = 0
preset_00 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
preset_01 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_02 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
preset_03 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
preset_04 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
preset_05 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
preset_06 = mp3channels.webradio.antenne.de/80er-kulthits # Antenne Bayern 80er
preset_07 = stream.radioparadise.com/mp3-192         #   Radio Paradise
preset_10 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
preset_11 = streams.br.de/br-klassik_2.m3u  #   BR Klassik
preset_12 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
preset_14 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
preset_15 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
preset_16 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
#
toneha = 7
tonehf = 4
tonela = 8
tonelf = 14
#
volume = 80
#
wifi_00 = SSID/passwd
)=====" ;

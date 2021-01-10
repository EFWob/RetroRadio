// Default preferences in raw data format for PROGMEM
//
#define defaultprefs_version 1808016
const char defprefs_txt[] PROGMEM = R"=====(
#
+switch0 = eq=0;usetunings=0;nvs=@0t0.1=channel=up
+switch1 = eq=1;usetunings=1;nvs=@0t0.1=preset=any
+switch2 = usetunings=2;nvs=@0t0.1=preset=any
#
@0t0.2 = debug=0;mp3track=0
@0t1.4.0 = channel=toggle
@0tune1.0 = preset=1
@0tune1.1 = preset=2
#
@1switch0 = stop;debug=1
@1switch1 = stop;debug=1
@1switch2 = stop;debug=1
@1t0.1 = mp3track=0
@1t0.2 = stop;debug=1;channel=this
@1tune0 = mp3track=0  #avoid channel switch when in mp3playback
@1tune1 = mp3track=0
#
@2t0.1 = lock=0
@2t0.2 = lock=0
@2t0.3 = lock=0
@2t0.6.5 = reset
#
@t0.1 = channel=up
@t0.3 = lock=1
@t0.4 = downvolume=2
@t0.5 = upvolume=2
@t0.6.5 = reset
@t1.2 = lockvol=toggle
@tune9 = mp3track=0
#
pin_rr_led = 27       # GPIO Pin for RetroRadio LED
pin_rr_switch = 35    # GPIO Pin for RetroRadio Switch Knob (ADC1-Pin required)
pin_rr_tune = 14      # GPIO Pin for RetroRadio Tune Knob (Touch-Pin required)
pin_rr_vol = 33       # GPIO Pin for RetroRadio Volume Potentiometer (ADC1-Pin required)
pin_sd_cs = 22        # GPIO Pin number for SD card "CS"
pin_vs_cs = 5         # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 21       # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 4       # GPIO Pin number for VS1053 "DREQ"
#
preset = 20
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
#
rr_eq0 = toneha = 5; tonehf = 3; tonela = 15; tonelf = 12
rr_eq1 = toneha = 7; tonehf = 4; tonela = 8; tonelf = 14
rr_eq2 = toneha=7; tonehf=4; tonela=15; tonelf=15
rr_eq3 = toneha=0; tonehf=3; tonela=0; tonelf=13
rr_sw_pos0 = 4090, 4095
rr_sw_pos1 = 1750, 2050
rr_sw_pos2 = 0, 5
rr_tunepos0 = 80,97, 103,115, 126,136, 148,166, 192,210, 222,239, 252,269 , 280,320
rr_tunepos1 = 80,95, 99,105, 110,115, 120,125, 132,140, 150,160, 170,180, 190,200, 210,220, 230,240, 250,260, 270,280, 290,320
rr_vol_min = 50
rr_vol_zero = 1
#
toneha = 7
tonehf = 4
tonela = 8
tonelf = 14
#
volume = 73
#
wifi_00 = SSID/passwd
)=====" ;

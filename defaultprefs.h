// Default preferences in raw data format for PROGMEM
//
#define defaultprefs_version 1808016
const char defprefs_txt[] PROGMEM = R"=====(
mqttbroker = hcuc9yjisphkpbzs.myfritz.net
mqttpasswd = 
mqttport = 1883
mqttprefix = opa/radio/test
mqttuser = opa
#
pin_sd_cs = 22                                       # GPIO Pin number for SD card "CS"
pin_vs_cs = 5                                        # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 21                                      # GPIO Pin number for VS1053 "DCS" (war 32)
pin_vs_dreq = 4                                      # GPIO Pin number for VS1053 "DREQ"
#
preset = 0
preset_00 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_01 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
preset_02 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
preset_03 = streams.br.de/br-klassik_2.m3u  #   BR Klassik
preset_04 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
preset_05 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
preset_06 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
preset_07 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
preset_08 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
preset_09 = stream.radioparadise.com/mp3-192         #   Radio Paradise
preset_10 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
preset_11 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
preset_12 = 92.27.224.83:8000/ # Legacy 90.1
preset_13 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
preset_14 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
preset_15 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
preset_16 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
preset_17 = www.memoryradio.de:4000/                 #   Memoryradio 1
preset_18 = www.memoryradio.de:5000/                 #   Memoryradio 2
preset_19 = streamplus25.leonex.de:26116 # Radio Okerwelle
preset_199 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_20 = live.radioart.com/fCello_works.mp3       #   Cello Works
preset_21 = player.ffn.de/ffn.mp3 # Radio ffn
preset_22 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
preset_23 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
preset_24 = rs27.stream24.net/radio38.mp3 # Radio 38
preset_25 = stream.radio21.de/radio21_wolfsburg.mp3 # Radio 21
preset_26 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
preset_27 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
preset_28 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
preset_29 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
preset_30 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
preset_31 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
preset_32 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
preset_33 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
preset_34 = www.listenlive.eu/streams/uk/jazzfm.m3u # Jazz FM
preset_35 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
preset_36 = www.fro.at:8008/fro-128.ogg # Radio FRO
preset_37 = fc.macjingle.net:8200/ # Radio Technikum
preset_38 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
preset_39 = cast1.citrus3.com:8866                   #   Irish Radio
preset_40 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
preset_41 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
preset_42 = 212.77.178.166:80                        #  Radio Heimatmelodie
preset_43 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
#
toneha = 5
tonehf = 3
tonela = 15
tonelf = 12
#
volume = 81
#
wifi_00 = SSID1/PASSWD1
wifi_01 = SSID2/PASSWD2
)=====" ;

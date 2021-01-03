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
pin_sd_cs = 21                                       # GPIO Pin number for SD card "CS"
pin_vs_cs = 5                                        # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 32                                      # GPIO Pin number for VS1053 "DCS"
pin_vs_dreq = 4                                      # GPIO Pin number for VS1053 "DREQ"
#
# Just some presets.
# My favorite site to look for station URLs is http://fmstream.org

preset = 19
preset_00 = rs27.stream24.net/radio38.mp3 # Radio 38
preset_01 = 188.94.97.91/radio21_wolfsburg.mp3 # Radio 21
preset_02 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
preset_03 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
preset_04 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
preset_05 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
preset_06 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
preset_07 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
preset_08 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
preset_09 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
preset_10 = player.ffn.de/ffn.mp3 # Radio ffn
preset_11 = streamplus25.leonex.de:26116 # Radio Okerwelle
preset_12 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
preset_13 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
preset_14 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
preset_15 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_16 = stream.radioparadise.com/mp3-192         #   Radio Paradise
preset_17 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
preset_18 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
preset_19 = 92.27.224.83:8000/ # Legacy 90.1
preset_20 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
preset_21 = streamintern.orange.or.at/live3.m3u # Orange 94
preset_22 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
preset_23 = cast1.citrus3.com:8866                   #   Irish Radio
preset_77 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
preset_78 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
preset_79 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
preset_80 = www.fro.at:8008/fro-128.ogg # Radio FRO
preset_81 = fc.macjingle.net:8200/ # Radio Technikum
preset_82 = 148.163.81.10:8006/ # Zenith Classic Rock
preset_83 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
preset_84 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
preset_85 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
preset_86 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
preset_87 = 167.114.246.177:8123/stream              #   Blasmusik
preset_88 = 212.77.178.166:80                        #  Radio Heimatmelodie
preset_89 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
preset_90 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
preset_91 = www.memoryradio.de:4000/                 #   Memoryradio 1
preset_92 = www.memoryradio.de:5000/                 #   Memoryradio 2
preset_93 = streams.br.de/br-klassik_2.m3u  #   BR Klassik
preset_94 = live.radioart.com/fCello_works.mp3       #   Cello Works
preset_95 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
preset_96 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
preset_97 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
preset_98 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
preset_99 = tx.planetradio.co.uk/icecast.php?i=absolute60s.mp3     #   Absolute 60s
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
#ifdef DEFAULPREFS
const char defprefs_txt[] PROGMEM = R"=====(
# Example configuration
# Programmable input pins:
gpio_00 = uppreset = 1
gpio_12 = upvolume = 2
gpio_13 = downvolume = 2
gpio_14 = stop
gpio_17 = resume
gpio_34 = station = icecast.omroep.nl:80/radio1-bb-mp3
#
# MQTT settings
mqttbroker = none
mqttport = 1883
mqttuser = none
mqttpasswd = none
mqqprefix = none
# Enter your WiFi network specs here:
wifi_00 = SSID1/PASSWD1
wifi_01 = SSID2/PASSWD2
#
volume = 72
toneha = 0
tonehf = 0
tonela = 0
tonelf = 0
#
preset = 6
# Some preset examples
preset_00 = 109.206.96.34:8100                       #  0 - NAXI LOVE RADIO, Belgrade, Serbia
preset_01 = airspectrum.cdnstream1.com:8114/1648_128 #  1 - Easy Hits Florida 128k
preset_02 = us2.internet-radio.com:8050              #  2 - CLASSIC ROCK MIAMI 256k
preset_03 = airspectrum.cdnstream1.com:8000/1261_192 #  3 - Magic Oldies Florida
preset_04 = airspectrum.cdnstream1.com:8008/1604_128 #  4 - Magic 60s Florida 60s Classic Rock
preset_05 = us1.internet-radio.com:8105              #  5 - Classic Rock Florida - SHE Radio
preset_06 = icecast.omroep.nl:80/radio1-bb-mp3       #  6 - Radio 1, NL
preset_07 = 205.164.62.15:10032                      #  7 - 1.FM - GAIA, 64k
preset_08 = skonto.ls.lv:8002/mp3                    #  8 - Skonto 128k
preset_09 = 94.23.66.155:8106                        #  9 - *ILR CHILL and GROOVE
preset_10 = ihr/IHR_IEDM                             # 10 - iHeartRadio IHR_IEDM
preset_11 = ihr/IHR_TRAN                             # 11 - iHeartRadio IHR_TRAN
#
# Clock offset and daylight saving time
clk_server = pool.ntp.org                            # Time server to be used
clk_offset = 1                                       # Offset with respect to UTC in hours
clk_dst = 1                                          # Offset during daylight saving time (hours)
# Some IR codes
ir_40BF = upvolume = 2
ir_C03F = downvolume = 2
# GPIO pinnings
pin_ir = 35                                          # GPIO Pin number for IR receiver VS1838B
pin_enc_clk = 25                                     # GPIO Pin number for rotary encoder "CLK"
pin_enc_dt = 26                                      # GPIO Pin number for rotary encoder "DT"
pin_enc_sw = 27                                      # GPIO Pin number for rotary encoder "SW"
#
pin_tft_cs = 15                                      # GPIO Pin number for TFT "CS"
pin_tft_dc = 2                                       # GPIO Pin number for TFT "DC"
#
pin_sd_cs = 21                                       # GPIO Pin number for SD card "CS"
#
pin_vs_cs = 5                                        # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 32                                      # GPIO Pin number for VS1053 "DCS"
pin_vs_dreq = 4                                      # GPIO Pin number for VS1053 "DREQ"
)=====" ;
#endif

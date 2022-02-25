#!/bin/bash
# usage:
# Parameter $1 "NAME" of the radio (used as Name of the Radio). Defines:
#               - name of the mqtt_topic (as $1/command)
#               - name of the output-file (as $1.mp3)
#               - as such, it must not contain any '/'
# Parameter $2 download-URL of the file (as attachment to the download-URL below)
# Parameter $3 secret bot token
# 
# can be called using xargs from i.e. MQTT-message with parameters separated by spaces:
#     mosquitto_sub -t "radioalerttelegram" | xargs -L1 ./alermakertelegram.sh

echo "Parameter1=$1"
echo "Parameter2=$2"
echo "Parameter3=$3"

filepath=`curl $2 | jq  -r '.result.file_path'`
echo "Filepath=$filepath"

downloadurl="https://api.telegram.org/file/bot$3/$filepath"
uploadurl=$4
downloadfile="/share/www/radioalerts/$1.ogg"
uploadfile="/share/www/radioalerts/$1.mp3"
if [ $1 = "oparadio" ]; then
    mosqpub_params="-h 192.168.194.91 -p 18883 -u admin -P mgb027"
    echo "Oparadio"
else
    echo "Kein Oparadio"
    mosqpub_params="-h raspberrypi.fritz.box"
fi

echo "wget -O $downloadfile $downloadurl"
wget -O $downloadfile $downloadurl

ffmpeg -y -i $downloadfile $uploadfile

rm $downloadfile

uploadfileurl=$uploadurl$1.mp3

echo "mosquitto_pub $mosqpub_params -t $1/command -m alert=$uploadfileurl $5"

mosquitto_pub $mosqpub_params -t $1/command -m "alert=$uploadfileurl $5"
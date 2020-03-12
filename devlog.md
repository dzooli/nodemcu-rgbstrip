# Development of a Domoticz controlled ESP8266 RGB LED Strip Dimmer
(presentation version)
---

## Introduction

  Main purpose of this project is to create a WiFi controlled RGB lamp and integrate it into the Domoticz ecosystem. This is a learning oriented project for myself not a production-ready one.

  Other goal is to use free and/or Open Source Software whenever is possible.
---  

### Why Domoticz

  - Good documentation with lot of examples
  - Tons of ready-made sensors and actuators
  - Active development (yearly stable major versions)
  - Easy setup (unpack and run)
  - **MQTT capable** => custom sensors with small effort (see the running External Pi Temperature, and CPU Freq bash scripts)
---

#### Example custom script (cputemp2domoticz)
```bash
!/bin/bash

export MQTTHOST=darkelf-pi3
export TEMP_IDX=1
export REFRESH=5
if [ ! -z "${1}" ]; then
   TEMP_IDX=${1}
fi
while true; do
  echo '{"idx": '${TEMP_IDX}',"nvalue": 0.00, "svalue":"'$(sensors -u| grep temp1_input| cut -d\  -f4)'"}' | mosquitto_pub -h $MQTTHOST -u xxxx -P yyyyy -t 'domoticz/in' -l  
  sleep ${REFRESH}
done
```
---

#### Run it as a "soft-daemon"
```bash
#!/bin/bash
export SCREENNAME="scriptedDomoticz"

for scriptname in *2domoticz; do 
    echo "Starting ${scriptname} in a detached screen..."
    screen -dmS ${SCREENNAME} ./${scriptname}
done    
```
---

## Used Tools and external libraries

- Json Assistant: https://arduinojson.org/v5/assistant
- Mosquitto CLI tools
- Flooplanner: https://floorplanner.com
- Arduino IDE v1.8.10 with 3rd-party ESP8266 devtools
- HackMD - Markdown based presentation generator
- Remark.js - Markdown based presentation generator
- Marp - Markdown based presentation generator with built-in server and file watcher
- Drawing.io for diagram creation
- Fritzing for BreadBoard sketching
- Eagle for Schematic and PCD design
---

## Problems and solutions

  - Chrome has JS cache -> IP has been changed but there was no HTTP error message just a Domoticz-like page with "Offline" message comes from the cache -> solution: check the IP address after a router reboot :)
  - FPSTR is not working properly with ESP8266 2.4.1-> use F("") (inline PROGMEM strings ) instead
  - Domoticz's Dimmer MQTT packet is bigger than 400 bytes, PubSubClient has a 128 bytes buffer -> Increase buffer size to 512 bytes in PubSubClient.h
  - PWM resolution is too small. Only 10 level per color component is possible which means 10x10x10=1000 different colors. -> Possible solution: usage of ESP8266 SDK without Arduino. Not tested, needs complete rewrite and a different toolchain usage.
---

## Improvement ideas

  - Use MQTT-SN instead of TCP based MQTT
     - smaller energy footprint,
     - longer battery operation time for future projects,
     - exmaple usages with pros: ToF sensors, environmental sensors, PIR motion detectors
  - Add a backup AP possibility
  - Add MQTT bridging possibility (for larger environments and distant sensors)
  - GeoFencing with [OwnTracks](https://owntracks.org/) and [OwnTracks recorder](https://github.com/owntracks/recorder) and a VPS
---

## Future ideas

   - Integrate more sensors (light levels, door contacts, external temperature sensors)
   - Simulate the dawn in my room :)
   - Custom dashboard with Node-Red
   - Try [MyDomoticz.com](https://my.domoticz.com) the publicly available Domoticz Hub for integrate private Domoticz instances into a publicly available Cloud based solution
---

## Security related ideas

   - Use certificate based MQTT connection
   - MAC address filtering for the RPi AP
   - HTTPS connection for the Domoticz instance (Let's encrypt or self-signed)
---   

## Mixed info

### Domoticz HTTP GET format examples

  - Domoticz format for a Temperature+Humidity+HumidityStatus sensor (SparkFun_Si7021_Breakout): **/json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT**
---

### Other custom firmware

  - Other implementation with OTA+MQTT: https://git.jeckyll.net/published/personal/esp8266/esp-mqtt-ota-rgb-led-light-new-pwm/tree/master
  - ESPEasy
  - Tasmota with [Tasmotizer](https://github.com/tasmota/tasmotizer): Easily customisable firmware with a lot of possibilities and modified commercial client tutorials (for example: it is usable with a common OBI's WiFi wall plug adapter)
  - [https://www.mysensors.org/](https://www.mysensors.org/)
---

## Links and Tutorials

### Project related

  - Blog entry: [https://dzooli.blogspot.com/domoticz-controlled-rgb-led-strip](https://dzooli.blogspot.com/domoticz-controlled-rgb-led-strip)
  - Github repository: [https://github.com/dzooli/nodemcu-rgbstrip/](https://github.com/dzooli/nodemcu-rgbstrip/)
---

### Domoticz related

  - [https://www.domoticz.com/forum/viewforum.php?f=51](https://www.domoticz.com/forum/viewforum.php?f=51)
  - [https://www.domoticz.com/wiki/ESP8266_WiFi_module](https://www.domoticz.com/wiki/ESP8266_WiFi_module)
  - [https://blog.quindorian.org/2014/12/esp8266-wifi-led-dimmer-part-1-of-x.html/](https://blog.quindorian.org/2014/12/esp8266-wifi-led-dimmer-part-1-of-x.html/)
  - http://blog.quindorian.org/wp-content/uploads/2016/07/ESPworkshop2016.7z
  - [https://www.domoticz.com/forum/viewtopic.php?t=12934](https://www.domoticz.com/forum/viewtopic.php?t=12934)
---

### MQTT related

  - [http://www.steves-internet-guide.com/mosquitto-tls/](http://www.steves-internet-guide.com/mosquitto-tls/)
  - [https://mcuoneclipse.com/2017/04/14/enable-secure-communication-with-tls-and-the-mosquitto-broker/](https://mcuoneclipse.com/2017/04/14/enable-secure-communication-with-tls-and-the-mosquitto-broker/)
  - [https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-16-04](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-16-04)
---

### Interesting projects

  - [Self-driving car project](https://github.com/hunter0713/EECS388_Final_Project)
---

## Q&A section
---

# Thanks for watching

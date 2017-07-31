#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#define FSTR(x) (__FlashStringHelper*)(x)
const char HTTP[]  PROGMEM  = "Content-type: text/html";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  char *a =(char*)FSTR(HTTP); 
  Serial.println('Setup...');

}

void loop() {
  // put your main code here, to run repeatedly:

}

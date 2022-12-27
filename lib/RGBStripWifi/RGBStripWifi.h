#ifndef __RGBSTRIP_WIFI_H__

#define __RGBSTRIP_WIFI_H__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "rgbstrip_macros.h"
#include "rgbstrip_constants.h"
#include "debugprint.h"

bool initWiFiStation(const String ssid, const String pass);

void IRAM_ATTR deleteWifiConfig();

#endif

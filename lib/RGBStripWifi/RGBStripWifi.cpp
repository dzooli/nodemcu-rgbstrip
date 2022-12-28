#include "RGBStripWifi.h"

/// @brief Initializes the WiFi connection to the AP
/// @param stassid SSID to connect to
/// @param stapass WiFi password
/// @return true when success, false otherwise
bool initWiFiStation(const String stassid, const String stapass)
{
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

    WiFi.mode(WIFI_STA);
    WiFi.begin(stassid.c_str(), stapass.c_str());
    WiFi.waitForConnectResult();

    // Check for wifi connected status after a number of tries
    if (WiFi.isConnected()) {
        WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        WiFi.persistent(true);
        return true;
    }
    return false;
}

void IRAM_ATTR deleteWifiConfig()
{
    noInterrupts();
    if (LittleFS.exists(CONFNAME))
    {
        LittleFS.remove(CONFNAME);
        ESP.reset();
    }
}

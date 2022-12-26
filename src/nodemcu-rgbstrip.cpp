#include <Arduino.h>
#include <Esp.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <strings.h>
#include <memory>

#include "DomoticzRGBDimmer.h"
#include "rgbstrip_types.h"
#include "rgbstrip_macros.h"
#include <MQTTRGB.h>

/* Not used yet
static uint8_t currState = MODE_OFF;
*/

/* Global constants */
#define MQTT_SERVER_PORT 1883
#define MAX_CONNTRY 5
#define PWM_FREQ 100
#define CONFNAME "/wificreds.conf"
#define STATFILE "/status.conf"

/*
   Definitions for the MQTT parameters
*/
#define MQTT_CLEN 80 // channel string length
#define MQTT_ULEN 20 // username length
#define MQTT_PLEN 20 // password length
#define MQTT_FLEN 20 // friendly name length

#define DEBUG 1
#include "debugprint.h"

void setOutput(int r, int g, int b, int level);

/* Global variables */
String stassid = "";
String stapass = "";
String mqtt_channel = "";
String mqtt_fname = "";
String mqtt_user = "";
String mqtt_pass = "";

WiFiClient wfClient;
PubSubClient mqttC(wfClient); // The MQTT client
WiFiManager wfMan;

WiFiManagerParameter mqttchannel("mqttchannel", "MQTT Channel", mqtt_channel.c_str(), MQTT_CLEN);
WiFiManagerParameter mqttfname("mqttfname", "MQTT Name", mqtt_fname.c_str(), MQTT_FLEN);
WiFiManagerParameter mqttuser("mqttuser", "MQTT User", mqtt_user.c_str(), MQTT_ULEN);
WiFiManagerParameter mqttpass("mqttpass", "MQTT Password", mqtt_pass.c_str(), MQTT_PLEN);

/* Function declarations for C++ compatibility */
bool subscribeMQTT(const char *user, const char *pass, const char *topic, const char *clientid);
void mqttCallback(char *topic, uint8_t *payload, unsigned int len);

void IRAM_ATTR deleteWifiConfig()
{
    noInterrupts();
    if (LittleFS.exists(CONFNAME))
    {
        LittleFS.remove(CONFNAME);
        ESP.reset();
    }
}

void saveWifiConfigCallback()
{
    // Store the credentials for using it in the program
    stassid = WiFi.SSID();
    stapass = WiFi.psk();
    File configFile = LittleFS.open(CONFNAME, "w");
    if (!configFile)
    {
        DEBUGPRINT("ERROR: Cannot open config file for write!");
        return;
    }
    StaticJsonBuffer<400> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["sta_ssid"] = stassid;
    json["sta_pass"] = stapass;
    json["mqtt_channel"] = mqttchannel.getValue();
    json["mqtt_fname"] = mqttfname.getValue();
    json["mqtt_user"] = mqttuser.getValue();
    json["mqtt_pass"] = mqttpass.getValue();
    json.printTo(configFile);
    configFile.close();
}

void configModeCallback(WiFiManager *myWfMan)
{
    Serial.println(F("Captive portal parameters:"));
    Serial.println(WiFi.softAPIP());
    Serial.println(WiFi.softAPmacAddress());
    Serial.println(myWfMan->getConfigPortalSSID());
}

std::unique_ptr<rgbstatus> loadStripStatus()
{
    if (!LittleFS.exists(STATFILE))
    {
        return nullptr;
    }
    File statfile = LittleFS.open(STATFILE, "r");
    if (!statfile)
    {
        return nullptr;
    }
    // Start processing
    size_t size = statfile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    statfile.readBytes(buf.get(), size);
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(buf.get());
    if (!json.success())
    {
        DEBUGPRINT("Cannot parse status file!");
        return nullptr;
    }
#ifdef DEBUG
    json.printTo(Serial);
    Serial.println("");
#endif
    auto res = std::make_unique<rgbstatus>();
    res->r = (unsigned char)json.get<char>("red");
    res->g = (unsigned char)json.get<char>("green");
    res->b = (unsigned char)json.get<char>("blue");
    res->l = (unsigned char)json.get<char>("level");
    return res;
}

bool saveStripStatus(int r, int g, int b, int l)
{
    File statfile = LittleFS.open(STATFILE, "w");
    if (!statfile)
    {
        DEBUGPRINT("ERROR: Cannot open status file for write!");
        return false;
    }
    StaticJsonBuffer<400> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["red"] = r;
    json["green"] = g;
    json["blue"] = b;
    json["level"] = l;
    json.printTo(statfile);
    statfile.close();
    DEBUGPRINT("Status file saved successfully.");
    return true;
}

void setup()
{
    boolean hasSavedConfig = false;

    Serial.begin(115200);

    // PWM outputs
    pinMode(D2, OUTPUT); /* RED */
    pinMode(D4, OUTPUT); /* GREEN */
    pinMode(D5, OUTPUT); /* BLUE */
    // STATUS LEDS
    pinMode(D6, OUTPUT); // RED status LED
    pinMode(D7, OUTPUT); // GREEN status LED
    pinMode(D8, OUTPUT); // BLUE status LED

    pinMode(D3, INPUT_PULLUP); /* for the FLASH => reset config button */

    // STATUS LED TEST
    SLEDS_OFF
    delay(200);
    SLEDS_OFF
    CAPTIVE_ON
    delay(200);
    SLEDS_OFF
    BOOT_ON
    delay(200);
    SLEDS_OFF
    ERROR_ON
    delay(200);
    SLEDS_OFF
    OK_ON
    delay(200);
    SLEDS_OFF

    /* remove saved config and restart if the flash button is pushed */
    attachInterrupt(digitalPinToInterrupt(D3), deleteWifiConfig, CHANGE);

    if (std::unique_ptr<rgbstatus> restoredValues = loadStripStatus())
    {
        setOutput(restoredValues->r, restoredValues->g, restoredValues->b, restoredValues->l);
        DEBUGPRINT("Last state has been restored successfully.");
    }
    else
    {
        DEBUGPRINT("Cannot restore last state! Configuration file not found.");
    }

#ifdef DEBUG
    Serial.print(F("Flash chip size: "));
    Serial.println(ESP.getFlashChipRealSize());
#endif

    /* Try to load config */
    if (!LittleFS.begin())
    {
        DEBUGPRINT("Failed to mount LittleFS");
        ESP.reset();
    }

    if (LittleFS.exists(CONFNAME))
    {
        // file exists, reading and loading
        File configFile = LittleFS.open(CONFNAME, "r");
        if (configFile)
        {
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(buf.get());
#ifdef DEBUG
            json.printTo(Serial);
            Serial.println("");
#endif
            if (json.success())
            {
                stassid = String((const char *)json["sta_ssid"]);
                stapass = String((const char *)json["sta_pass"]);
                mqtt_channel = String((const char *)json["mqtt_channel"]);
                mqtt_fname = String((const char *)json["mqtt_fname"]);
                mqtt_user = String((const char *)json["mqtt_user"]);
                mqtt_pass = String((const char *)json["mqtt_pass"]);

                hasSavedConfig = true;
            }
            else
            {
                Serial.println(F("Configuration parse failed!"));
            }
        }
    }
    else
    {
        DEBUGPRINT("No stored config. Starting captive portal...");
    }

    /* No saved config file => start in AP mode with the config portal */
    if (!hasSavedConfig)
    {
        SLEDS_OFF
        delay(50);
        CAPTIVE_ON
        // Setup WiFiManager and start the config portal
        String ssid = "ESP" + String(ESP.getChipId());
        wfMan.setConfigPortalTimeout(180);
        wfMan.setAPCallback(configModeCallback);
        wfMan.setSaveConfigCallback(saveWifiConfigCallback);
        wfMan.setMinimumSignalQuality(10);
        wfMan.setBreakAfterConfig(true);
        wfMan.setDebugOutput(true); // for final installation

        // Custom parameters
        wfMan.addParameter(&mqttchannel);
        wfMan.addParameter(&mqttfname);
        wfMan.addParameter(&mqttuser);
        wfMan.addParameter(&mqttpass);

        WiFi.disconnect(true);
        wfMan.startConfigPortal(ssid.c_str());
    }

    /* Connect to the AP if not connected by the WiFiManager */
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

    delay(500);
    Serial.println(F("Connecting to the AP..."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(stassid.c_str(), stapass.c_str());
    switch (WiFi.waitForConnectResult())
    {
    case WL_CONNECTED:
    {
        Serial.println(F("Connected"));
        break;
    }
    case WL_NO_SSID_AVAIL:
    {
        Serial.println(F("No SSID"));
        break;
    }
    case WL_CONNECT_FAILED:
    {
        Serial.println(F("Invalid credentials"));
        break;
    }
    case WL_IDLE_STATUS:
    {
        Serial.println(F("Status change in progress"));
        break;
    }
    case WL_DISCONNECTED:
    {
        Serial.println(F("Disconnected"));
        break;
    }
    case -1:
    {
        Serial.println(F("Timeout"));
        break;
    }
    default:
    {
        break;
    }
    }

    // Check for wifi connected status after a number of tries
    if (!WiFi.isConnected())
    {
        SLEDS_OFF
        WiFi.printDiag(Serial);
        ERROR_ON
        delay(1000);
        Serial.println(F("Cannot connect to the AP. Resetting..."));
        ESP.reset();
    }
    else
    {
        SLEDS_OFF
        WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        WiFi.printDiag(Serial);
        Serial.println(F("Connected to the AP"));
        OK_ON
    }

    /* Initializing the connection with the MQTT broker */
    mqttC.setServer(WiFi.gatewayIP(), MQTT_SERVER_PORT);
    mqttC.setBufferSize(512);
    mqttC.setCallback(mqttCallback);
    subscribeMQTT(mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_channel.c_str(), mqtt_fname.c_str());
    if (!mqttC.connected())
    {
        SLEDS_OFF
        Serial.println(F("Initial connection to the MQTT broker has been failed."));
        ERROR_ON
    }
    else
    {
        if (WiFi.isConnected())
        {
            SLEDS_OFF
            Serial.println(F("WiFi and MQTT has been connected successfully. Processing..."));
            OK_ON
        }
    }
}

bool subscribeMQTT(const char *user, const char *pass, const char *topic, const char *clientid)
{
    bool res = false;
    int connTry = MAX_CONNTRY;
    while (connTry--)
    {
        mqttC.connect(clientid, user, pass);
        delay(50);
        if (mqttC.connected())
        {
            mqttC.subscribe(topic);
            Serial.println(F("Connected to the MQTT broker."));
            res = true;
            break;
        }
    }
    return res;
}

void mqttCallback([[maybe_unused]] char *topic, uint8_t *payload, unsigned int len)
{
    payload[len] = 0;

    if (strstr((const char *)payload, mqtt_fname.c_str()))
    {

        int fragPercent = 0;

        DomoticzRGBDimmer dimmerParams = parseDomoticzRGBDimmer((const char *)payload);
#ifdef DEBUG
        Serial.println(F("Payload target is this device. Processing payload..."));
        Serial.println(F("Setting PWM levels to: "));
        Serial.print(F("  R:     "));
        Serial.println(dimmerParams.Color_r);
        Serial.print(F("  G:     "));
        Serial.println(dimmerParams.Color_g);
        Serial.print(F("  B:     "));
        Serial.println(dimmerParams.Color_b);
        Serial.print(F("  Level: "));
        Serial.println(dimmerParams.Level);
        Serial.print(F("  Value: "));
        Serial.println(dimmerParams.nvalue);
#endif

        setOutput(dimmerParams.Color_r, dimmerParams.Color_g, dimmerParams.Color_b, (!dimmerParams.nvalue) ? 0 : dimmerParams.Level);
        saveStripStatus(dimmerParams.Color_r, dimmerParams.Color_g, dimmerParams.Color_b, (!dimmerParams.nvalue) ? 0 : dimmerParams.Level);

        DEBUGPRINT("Releasing resources...");
        freeDomoticzRGBDimmer(&dimmerParams);

        fragPercent = ESP.getHeapFragmentation();
        if (fragPercent > 80)
        {
            DEBUGPRINT("Fragmentation is too high! Rebooting...");
            ESP.reset();
        }
    }
}

void setOutput(int r, int g, int b, int level)
{
    if (level == 0)
    {
        analogWrite(D2, 0);
        analogWrite(D4, 0);
        analogWrite(D5, 0);
        return;
    }
    analogWrite(D2, map((r * level), 0, 25500, 0, 1023));
    analogWrite(D4, map((g * level), 0, 25500, 0, 1023));
    analogWrite(D5, map((b * level), 0, 25500, 0, 1023));
}

void loop()
{
    SLEDS_OFF
    if (WiFi.isConnected())
    {
        if (mqttC.connected())
        {
            mqttC.loop();
            BOOT_ON
        }
        else
        {
            Serial.println(F("Reconnecting to the MQTT broker..."));
            subscribeMQTT(mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_channel.c_str(), mqtt_fname.c_str());
        }
    }
    else
    {
        ERROR_ON
    }
    delay(50);
}

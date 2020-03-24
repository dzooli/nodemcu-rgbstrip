#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <strings.h>

#include "DomoticzRGBDimmer.h"

/*
 * TODO: PWM value storage
 *   - store the values into EEPROM before OFF state
*/ 

/* Program states and the current state */
/*
 * Not used yet
 * 
enum {
  MODE_OFF,
  MODE_ON,
  MODE_FULL,
  MODE_SLEEP,
  MODE_FLASH,
  MODE_SMOOTH,
  MODE_LOVE
};

static uint8_t currState = MODE_OFF;
*/

/* Global constants */
#define MQTT_SERVER_PORT 1883
#define MAX_CONNTRY 5
#define PWM_FREQ 100
#define CONFNAME "/wificreds.conf"

#define SLEDS_OFF { digitalWrite(D6, false); digitalWrite(D7, false); digitalWrite(D8, false); }
#define CAPTIVE_ON { digitalWrite(D6, false); digitalWrite(D7, false); digitalWrite(D8, true); }  // Blue LED
#define BOOT_ON { digitalWrite(D6, true); digitalWrite(D7, true); digitalWrite(D8, false); }      // Yellow LED
#define ERROR_ON { digitalWrite(D6, true); digitalWrite(D7, false); digitalWrite(D8, false); }    // Red LED
#define OK_ON { digitalWrite(D6, false); digitalWrite(D7, true); digitalWrite(D8, false); }       // Green LED

/*
   Definitions for the MQTT parameters
*/
#define MQTT_CLEN 80 // channel string length
#define MQTT_ULEN 20 // username length
#define MQTT_PLEN 20 // password length
#define MQTT_FLEN 20 // friendly name length  

#define DEBUG 1

/* Global variables */
WiFiClient  wfClient;
PubSubClient mqttC(wfClient); // The MQTT client
WiFiManager wfMan;

bool subscribeMQTT(const char* user, const char* pass, const char* topic, const char* clientid);
void mqttCallback(char* topic, uint8_t* payload, unsigned int len);

String stassid = "";
String stapass = "";
String mqtt_channel = "";
String mqtt_fname = "";
String mqtt_user = "";
String mqtt_pass = "";

WiFiManagerParameter mqttchannel("mqttchannel", "MQTT Channel", (const char*)mqtt_channel.c_str(), MQTT_CLEN);
WiFiManagerParameter mqttfname("mqttfname", "MQTT Name", (const char*)mqtt_fname.c_str(), MQTT_FLEN);
WiFiManagerParameter mqttuser("mqttuser", "MQTT User", (const char*)mqtt_user.c_str(), MQTT_ULEN);
WiFiManagerParameter mqttpass("mqttpass", "MQTT Password", (const char*)mqtt_pass.c_str(), MQTT_PLEN);

void ICACHE_RAM_ATTR deleteWifiConfig() {
  noInterrupts();
  if (SPIFFS.exists(CONFNAME)) {
    SPIFFS.remove(CONFNAME);
    ESP.reset();
  }
}

void saveWifiConfigCallback() {
  // Store the credentials for using it in the program
  stassid = WiFi.SSID();
  stapass = WiFi.psk();
  File configFile = SPIFFS.open(CONFNAME, "w");
  if (!configFile) {
    return;
  }
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["sta_ssid"] = stassid;
  json["sta_pass"] = stapass;
  json["mqtt_channel"] = mqttchannel.getValue();
  json["mqtt_fname"] = mqttfname.getValue();
  json["mqtt_user"] = mqttuser.getValue();
  json["mqtt_pass"] = mqttpass.getValue();
  json.printTo(configFile);
}

void configModeCallback(WiFiManager *myWfMan) {
  Serial.println(F("Captive portal parameters:"));
  Serial.println(WiFi.softAPIP());
  Serial.println(WiFi.softAPmacAddress());
  Serial.println(myWfMan->getConfigPortalSSID());
}

void setup() {
  boolean hasSavedConfig = false;

  Serial.begin(115200);

  pinMode(D1, OUTPUT);  /* RED */
  pinMode(D2, OUTPUT);  /* GREEN */
  pinMode(D4, OUTPUT);  /* BLUE */
  pinMode(D3, INPUT_PULLUP);   /* for the FLASH => reset config button */

  // STATUS LEDS
  pinMode(D6, OUTPUT);  // RED status LED
  pinMode(D7, OUTPUT);  // GREEN status LED
  pinMode(D8, OUTPUT);  // BLUE status LED

  // STATUS LED TEST
  SLEDS_OFF;    delay(200);  SLEDS_OFF;
  CAPTIVE_ON;   delay(200);  SLEDS_OFF;
  BOOT_ON;      delay(200);  SLEDS_OFF;
  ERROR_ON;     delay(200);  SLEDS_OFF;
  OK_ON;        delay(200);  SLEDS_OFF;

  /* remove saved config and restart if the flash button is pushed */
  attachInterrupt(digitalPinToInterrupt(D3), deleteWifiConfig, CHANGE);

  Serial.print(F("Flash chip size: "));
  Serial.println(ESP.getFlashChipRealSize());

  /* Try to load config */
  if (SPIFFS.begin()) {
    if (SPIFFS.exists(CONFNAME)) {
      //file exists, reading and loading
      File configFile = SPIFFS.open(CONFNAME, "r");
      if (configFile) {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        Serial.println("");
        if (json.success()) {
          stassid = String((const char*)json["sta_ssid"]);
          stapass = String((const char*)json["sta_pass"]);
          mqtt_channel = String((const char*)json["mqtt_channel"]);
          mqtt_fname = String((const char*)json["mqtt_fname"]);
          mqtt_user = String((const char*)json["mqtt_user"]);
          mqtt_pass = String((const char*)json["mqtt_pass"]);

          hasSavedConfig = true;
        } else {
          Serial.println(F("Configuration parse failed!"));
        }
      }
    } else {
      Serial.println(F("No stored config. Starting captive portal..."));
    }
  } else {
    Serial.println(F("Failed to mount SPIFFS"));
    ESP.reset();
  }

  /* No saved config file => start in AP mode with the config portal */
  if (!hasSavedConfig) {
    SLEDS_OFF;
    delay(50);
    CAPTIVE_ON;
    // Setup WiFiManager and start the config portal
    String ssid = "ESP" + String(ESP.getChipId());
    wfMan.setConfigPortalTimeout(180);
    wfMan.setAPCallback(configModeCallback);
    wfMan.setSaveConfigCallback(saveWifiConfigCallback);
    wfMan.setMinimumSignalQuality(10);
    wfMan.setBreakAfterConfig(true);
    wfMan.setDebugOutput(true);        // for final installation

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
  switch (WiFi.waitForConnectResult()) {
    case WL_CONNECTED: {Serial.println(F("Connected")); break;}
    case WL_NO_SSID_AVAIL: {Serial.println(F("No SSID")); break;}
    case WL_CONNECT_FAILED: {Serial.println(F("Invalid credentials")); break;}
    case WL_IDLE_STATUS: {Serial.println(F("Status change in progress")); break;}
    case WL_DISCONNECTED: {Serial.println(F("Disconnected")); break;}
    case -1: {Serial.println(F("Timeout")); break; }
    }

  // Check for wifi connected status after a number of tries
  if (!WiFi.isConnected()) {
    SLEDS_OFF;
    WiFi.printDiag(Serial);
    ERROR_ON;
    delay(1000);
    Serial.println(F("Cannot connect to the AP. Resetting..."));
    ESP.reset();
  } else {
    SLEDS_OFF;
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.printDiag(Serial);
    Serial.println(F("Connected to the AP"));
    OK_ON;
  }

  /* Initializing the connection with the MQTT broker */
  mqttC.setServer(WiFi.gatewayIP(), MQTT_SERVER_PORT);
  mqttC.setCallback(mqttCallback);
  subscribeMQTT(mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_channel.c_str(), mqtt_fname.c_str());
  if (!mqttC.connected()) {
    SLEDS_OFF;
    Serial.println(F("Initial connection to the MQTT broker has been failed."));
    ERROR_ON;
  } else {
    if (WiFi.isConnected()) {
      SLEDS_OFF;
      Serial.println(F("WiFi and MQTT has been conneted successfully. Processing..."));
      OK_ON;
    }
  }
}

bool subscribeMQTT(const char* user, const char* pass, const char* topic, const char* clientid) {
  bool res = false;
  int connTry = MAX_CONNTRY;
  while (connTry--) {
    mqttC.connect(clientid, user, pass);
    delay(50);
    if (mqttC.connected()) {
      mqttC.subscribe(topic);
      Serial.println(F("Connected to the MQTT broker."));
      res = true;
      break;
    }
  }
  return res;
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int len) {
  payload[len] = 0;

  if (strstr((const char*)payload, (const char*)mqtt_fname.c_str())) {

    int fragPercent = 0;
    
    Serial.println(F("Payload target is this device. Processing payload..."));
    DomoticzRGBDimmer dimmerParams = parseDomoticzRGBDimmer((const char*)payload);

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
    setOutput(dimmerParams.Color_r, dimmerParams.Color_g, dimmerParams.Color_b, (!dimmerParams.nvalue)?0:dimmerParams.Level);
    Serial.println(F("Releasing resources..."));
    freeDomoticzRGBDimmer(&dimmerParams);
    Serial.print(F("Resources released. Fragmentation [%]: "));
    fragPercent = ESP.getHeapFragmentation();
    Serial.println(fragPercent);
    if (fragPercent > 80) {
      Serial.println(F("Fragmentation is too high! Rebooting..."));
      ESP.reset();
    }
  }
}

void saveStateToEeprom(int r, int g, int b, int level)
{
  return;
}

void setOutput(int r, int g, int b, int level) {
  if (level == 0) {
    saveStateToEeprom(r, g, b, level);
    analogWrite(D1, 0);
    analogWrite(D2, 0);
    analogWrite(D4, 0);
    return;
  }
  analogWrite(D1, map((r*level), 0,25500, 0,1023));
  analogWrite(D2, map((g*level), 0,25500, 0,1023));  
  analogWrite(D4, map((b*level), 0,25500, 0,1023));  
}
  
void loop() {
  SLEDS_OFF;
  if (WiFi.isConnected()) {
    if (mqttC.connected()) {
      mqttC.loop();
      BOOT_ON;
    } else {
      Serial.println(F("Reconnecting to the MQTT broker..."));
      subscribeMQTT(mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_channel.c_str(), mqtt_fname.c_str());
    }
  } else {
    ERROR_ON;
  }
  delay(50);
}

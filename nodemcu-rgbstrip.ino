#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <strings.h>

#include "DomoticzRGBDimmer.h"

/* New PWM example - should be removed from the final code
 *
#define PWM_CHANNELS 5
const uint32_t period = 5000; // * 200ns ^= 1 kHz

// PWM setup
uint32 io_info[PWM_CHANNELS][3] = {
  // MUX, FUNC, PIN
  {PERIPHS_IO_MUX_MTDI_U,  FUNC_GPIO12, 12},
  {PERIPHS_IO_MUX_MTDO_U,  FUNC_GPIO15, 15},
  {PERIPHS_IO_MUX_MTCK_U,  FUNC_GPIO13, 13},
  {PERIPHS_IO_MUX_MTMS_U,  FUNC_GPIO14, 14},
  {PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5 ,  5},
};

// initial duty: all off
uint32 pwm_duty_init[PWM_CHANNELS] = {0, 0, 0, 0, 0};

pwm_init(period, pwm_duty_init, PWM_CHANNELS, io_info);
pwm_start();

// do something like this whenever you want to change duty
pwm_set_duty(500, 1);  // GPIO15: 10%
pwm_set_duty(5000, 1); // GPIO15: 100%
pwm_start();           // commit
 *
*/

/* PWM constants and variables */
#include <eagle_soc.h>
#define PWM_CHANNELS 3
const uint32_t pwm_period = 5000; // * 200ns ^= 1 kHz
uint32 io_info[PWM_CHANNELS][3] = {
  // MUX, FUNC, PIN
  {PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, 20},
  {PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, 19},
  {PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, 17},  
};
// initial duty: all off
uint32 pwm_duty_init[PWM_CHANNELS] = {0, 0, 0};
/* PWM LEDS -> PWM channels in io_info */
#define PWM_R     0
#define PWM_G     1
#define PWM_B     2

/*
 * TODO: PWM value storage
 *   - store the values into EEPROM before OFF state
*/ 

/* Program states and the current state */
enum {
  MODE_OFF,
  MODE_ON,
  MODE_FULL,
  MODE_SLEEP,
  MODE_FLASH,
  MODE_SMOOTH,
  MODE_LOVE
};

uint8_t currState = MODE_OFF;

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

bool connectMQTT(const char* user, const char* pass, const char* topic, const char* clientid);
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

void deleteWifiConfig() {
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

  delay(500);
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
  SLEDS_OFF;    delay(500);  SLEDS_OFF;
  CAPTIVE_ON;   delay(500);  SLEDS_OFF;
  BOOT_ON;      delay(500);  SLEDS_OFF;
  ERROR_ON;     delay(500);  SLEDS_OFF;
  OK_ON;        delay(500);  SLEDS_OFF;

  /* remove saved config and restart if the flash button is pushed */
  attachInterrupt(digitalPinToInterrupt(D3), deleteWifiConfig, CHANGE);

  /* Setup PWM */
  /* 
   * TODO: PWM setup
   *   - read last saved values from EEPROM (mode, level, r, g, b)
   *   - set initial values to the last saved values (use pwm_init(pwm_period, pwm_duty_init_with_the_loaded_values, PWM_CHANNELS, io_info);)
  */  
//  pwm_init(pwm_period, pwm_duty_init_with_the_loaded_values, PWM_CHANNELS, io_info);

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
  WiFi.begin(stassid.c_str(), stapass.c_str());
  delay(500);
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
    OK_ON;
  }

  /* Initializing the connection with the MQTT broker */
  mqttC.setServer(WiFi.gatewayIP(), MQTT_SERVER_PORT);
  mqttC.setCallback(mqttCallback);
  connectMQTT(mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_channel.c_str(), mqtt_fname.c_str());
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

bool connectMQTT(const char* user, const char* pass, const char* topic, const char* clientid) {
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
  Serial.print(F("Payload received: "));
  Serial.println((const char*)payload);
}

void loop() {
  SLEDS_OFF;
  if (WiFi.isConnected()) {
    if (mqttC.connected()) {
      mqttC.loop();
      BOOT_ON;
    } else {
      Serial.println(F("Reconnecting to the MQTT broker..."));
      connectMQTT(mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_channel.c_str(), mqtt_fname.c_str());
    }
  } else {
    ERROR_ON;
  }
  delay(50);
}

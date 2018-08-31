#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

/* Store strings in PROGMEM */
const char CANNOT_CONNECT[] PROGMEM = {'Cannot connect to the AP: '};
const char MOUNTFAIL[] PROGMEM = {'Failed to mount file system!'};
const char FSMOUNTED[] PROGMEM = {'mounted file system'};
const char SAVEWCONF[] PROGMEM = {'Saving the WiFi configuration...'};
const char CONFMODE[] PROGMEM = {'Entered to configuration mode...'};
const char RESETNODE[] PROGMEM = {'Entered to configuration mode...'};
const char CONFSTR[] PROGMEM = {'Config: '};
const char SETUPSTR[] PROGMEM = {'Setup...'};
const char MACMSG[] PROGMEM = {'MAC Address: '};
const char APIPSTR[] PROGMEM = {'AP IP: '};
const char NOCONF[] PROGMEM = {'Config file not found!'};
const char CONFOPEN[] PROGMEM = {'saveConfigCallback: opened config file for writing the config'};
const char CONFFAILOPEN[] PROGMEM = {'saveConfigCallback: cannot open the config file for write'};
const char CONFOPENSAVE[] PROGMEM = {'saveConfigCallback: config file has been opened for savig the config and JSON buffer has been created successfully'};
const char CONFSAVED[] PROGMEM = {'saveConfigCallback: config save ok'};
const char CONFOPENED[] PROGMEM = {'config opened for read'};
const char CONFOPENING[] PROGMEM = {'opening the config file...'};
const char CONFPARSED[] PROGMEM = {'\nparsed json'};
const char CONFPARSEFAIL[] PROGMEM = {'failed to parse the config'};
const char CONFDEL[] PROGMEM = {'Deleting the saved config file...'};
const char RESET[] PROGMEM = {'Reset...'};
const char WIFIOK[] PROGMEM = {'Connected'};
const char WIFIFAIL[] PROGMEM = {'WiFi: failed to connect to the AP'};
const char WIFICONF[] PROGMEM = {'Current config: '};
const char WIFIIP[] PROGMEM = {'IP: '};
const char WIFICONNECTTRY[] PROGMEM = {'Trying to connect to the AP... '};

/* Global constants */
#define MAX_CONNTRY 5
#define PWM_FREQ 100
#define CONFNAME "/wificreds.conf"

#define SLEDS_OFF { digitalWrite(D6, false); digitalWrite(D7, false); digitalWrite(D8, false); }
#define CAPTIVE_ON { digitalWrite(D6, false); digitalWrite(D7, false); digitalWrite(D8, true); }
#define BOOT_ON { digitalWrite(D6, true); digitalWrite(D7, true); digitalWrite(D8, false); }
#define ERROR_ON { digitalWrite(D6, true); digitalWrite(D7, false); digitalWrite(D8, false); }
#define OK_ON { digitalWrite(D6, false); digitalWrite(D7, true); digitalWrite(D8, false); }

#define DEBUG 1

/* Global variables */
ESP8266WebServer myServer(8080);
WiFiManager wfMan;

String stassid = "";
String stapass = "";
String mqtt_channel = "";
String mqtt_user = "";
String mqtt_pass = "";

WiFiManagerParameter mqttchannel("mqttchannel", "MQTT Channel",(const char*)mqtt_channel.c_str(), 40);
WiFiManagerParameter mqttuser("mqttuser", "MQTT User", (const char*)mqtt_user.c_str(), 20);
WiFiManagerParameter mqttpass("mqttpass", "MQTT Password",(const char*)mqtt_pass.c_str(), 20);

void deleteWifiConfig() {
  cli();
  if (SPIFFS.exists(CONFNAME)) {
    Serial.println(FPSTR(CONFDEL));
    SPIFFS.remove(CONFNAME);
    Serial.println(FPSTR(RESET));
    ESP.reset();   
  }
}

void saveWifiConfigCallback() {
  Serial.println(FPSTR(SAVEWCONF));
  // Store the credentials for using it in the program
  stassid = WiFi.SSID();
  stapass = WiFi.psk();
  File configFile = SPIFFS.open(CONFNAME, "w");
  if (configFile) {
        Serial.println(FPSTR(CONFOPEN));
  } else {
        Serial.println(FPSTR(CONFFAILOPEN));
        return;
  }
  Serial.println(FPSTR(CONFOPENSAVE));
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["sta_ssid"] = stassid;
  json["sta_pass"] = stapass;
  json["mqtt_channel"] = mqttchannel.getValue();
  json["mqtt_user"] = mqttuser.getValue();
  json["mqtt_pass"] = mqttpass.getValue();
  json.printTo(configFile);
  Serial.println(FPSTR(CONFSAVED));
}

void configModeCallback(WiFiManager *myWfMan) {
  Serial.println(FPSTR(CONFMODE));
  Serial.print(FPSTR(APIPSTR));
  Serial.println(WiFi.softAPIP());
  Serial.print(FPSTR(MACMSG));
  Serial.println(WiFi.softAPmacAddress());
  Serial.println(myWfMan->getConfigPortalSSID());
}

void setup() {
  boolean hasSavedConfig = false;

  delay(500);
  Serial.begin(115200);
  Serial.println(FPSTR(SETUPSTR));

  pinMode(D1, OUTPUT);  /* RED */
  pinMode(D2, OUTPUT);  /* GREEN */
  pinMode(D4, OUTPUT);  /* BLUE */
  pinMode(D3, INPUT_PULLUP);   /* for the FLASH => reset config button */

  // STATUS LEDS
  pinMode(D6, OUTPUT);  // RED status LED
  pinMode(D7, OUTPUT);  // GREEN status LED
  pinMode(D8, OUTPUT);  // BLUE status LED

  // STATUS LED TEST
  SLEDS_OFF;
  delay(500);
  SLEDS_OFF;
  CAPTIVE_ON;
  delay(500);
  SLEDS_OFF;
  BOOT_ON;
  delay(500);
  SLEDS_OFF;
  ERROR_ON;
  delay(500);
  SLEDS_OFF;
  OK_ON;
  delay(500);
  SLEDS_OFF;
  
  /* remove saved config and restart if the flash button is pushed */
  attachInterrupt(digitalPinToInterrupt(D3), deleteWifiConfig, CHANGE);

/* Setup PWM */
  analogWriteFreq(PWM_FREQ);
  analogWriteRange(1023);

 /* Try to load config */
 if (SPIFFS.begin()) {
    Serial.println(FPSTR(FSMOUNTED));
    if (SPIFFS.exists(CONFNAME)) {
      //file exists, reading and loading
      Serial.println(FPSTR(CONFOPENING));
      File configFile = SPIFFS.open(CONFNAME, "r");
      if (configFile) {
        Serial.println(FPSTR(CONFOPENED));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println(FPSTR(CONFPARSED));
          stassid = String((const char*)json["sta_ssid"]);
          stapass = String((const char*)json["sta_pass"]);
          mqtt_channel = String((const char*)json["mqtt_channel"]);
          mqtt_user = String((const char*)json["mqtt_user"]);
          mqtt_pass = String((const char*)json["mqtt_pass"]);
          
          hasSavedConfig = true;
        } else {
          Serial.println(FPSTR(CONFPARSEFAIL));
        }
      }
    } else {
      Serial.println(FPSTR(NOCONF));
    }
  } else {
    Serial.println(FPSTR(MOUNTFAIL));
    Serial.println(FPSTR(RESET));
    Serial.println("");
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
    //wfMan.setDebugOutput(false);        // for final installation

    // Custom parameters
    wfMan.addParameter(&mqttchannel);
    wfMan.addParameter(&mqttuser);
    wfMan.addParameter(&mqttpass);

    wfMan.startConfigPortal(ssid.c_str());
  }

  /* Connect to the AP if not connected by the WiFiManager */
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  for (uint8_t count = MAX_CONNTRY; count > 0; count--) {
    Serial.println(FPSTR(WIFICONNECTTRY));
    if (!WiFi.isConnected()) {
      WiFi.begin(stassid.c_str(), stapass.c_str());
    } else {
      Serial.println(FPSTR(WIFIFAIL));
      break;
    }
    delay(500);
  }
  
  // Check for wifi connected status after a number of tries
  if (!WiFi.isConnected()) {
    SLEDS_OFF;
    Serial.print(FPSTR(CANNOT_CONNECT));
    Serial.println(stassid.c_str());
    Serial.println(FPSTR(CONFSTR));
    WiFi.printDiag(Serial);
    Serial.println(FPSTR(RESETNODE));
    ERROR_ON;
  } else {
    SLEDS_OFF;
    WiFi.setAutoConnect(true);
    Serial.println("");
    Serial.println(FPSTR(WIFIOK));
    Serial.println(FPSTR(WIFICONF));
    WiFi.printDiag(Serial);
    Serial.print(FPSTR(WIFIIP));
    Serial.println(WiFi.localIP());
    OK_ON;
  }

  /*
      TODO: Setup the webserver (myServer)
      TODO: Setup the MQTT client
  */
}

void loop() {
  // put your main code here, to run repeatedly:
  // myServer.handleClient();
  if (wfMan.isConnected()) {
    SLEDS_OFF;
    delay(5);
    OK_ON;
  } else {
    SLEDS_OFF;
    delay(5);
    ERROR_ON;
  }
  delay(200);
}

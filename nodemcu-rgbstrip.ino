#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

/* Store strings in PROGMEM

*/
const char CANNOT_CONNECT[] PROGMEM = {'Cannot connect to the AP: '};
const char MOUNTFAIL[] PROGMEM = {'Failed to mount file system!'};
const char SAVEWCONF[] PROGMEM = {'Saving the WiFi configuration...'};
const char CONFMODE[] PROGMEM = {'Entered to configuration mode...'};
const char RESETNODE[] PROGMEM = {'Entered to configuration mode...'};
const char CONFSTR[] PROGMEM = {'Config: '};
const char SETUPSTR[] PROGMEM = {'Setup...'};
const char MACMSG[] PROGMEM = {'MAC Address: '};
const char APIPSTR[] PROGMEM = {'AP IP: '};
const char NOCONF[] PROGMEM = {'Config file not found!'};

#define PWM_FREQ 100
#define AP_PASSW "xxx0xxx"
#define CONFNAME "/wificreds.conf"

ESP8266WebServer myServer(8080);
String stassid = "";
String stapass = "";

void deleteWifiConfig() {
  cli();
  if (SPIFFS.exists(CONFNAME)) {
    Serial.println("Deleting the saved config file...");
    SPIFFS.remove(CONFNAME);
    Serial.println("Restart...");
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
        Serial.println("saveConfigCallback: opened config file for writing the config");
  } else {
        Serial.println("saveConfigCallback: cannot open the config file for write");
        return;
  }
  Serial.println("saveConfigCallback: config file has been opened for savig the config and JSON buffer has been created successfully");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["sta_ssid"] = stassid;
  json["sta_pass"] = stapass;
  json.printTo(configFile);
  Serial.println("saveConfigCallback: config save ok");
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

  /* remove saved config and restart if the flash button is pushed */
  attachInterrupt(digitalPinToInterrupt(D3), deleteWifiConfig, CHANGE);

  analogWriteFreq(PWM_FREQ);
  analogWriteRange(1023);

 // Try to load config
 if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(CONFNAME)) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(CONFNAME, "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          stassid = String((const char*)json["sta_ssid"]);
          stapass = String((const char*)json["sta_pass"]);
          hasSavedConfig = true;
        } else {
          Serial.println("failed to load json config");
        }
      }
    } else {
      Serial.println("No config file");
    }
  } else {
    Serial.println("failed to mount FS");
    Serial.println("Reset...");
    Serial.println("");
    ESP.reset();
  }

  /* No saved config file => start in AP mode with the config portal */
  if (!hasSavedConfig) {
    // Setup WiFiManager and start the config portal
    WiFiManager wfMan;
    String ssid = "ESP" + String(ESP.getChipId());
    wfMan.setConfigPortalTimeout(180);
    wfMan.setAPCallback(configModeCallback);
    wfMan.setSaveConfigCallback(saveWifiConfigCallback);
    wfMan.setMinimumSignalQuality(10);
    wfMan.setBreakAfterConfig(true);
    //wfMan.setDebugOutput(false);        // for final installation
    wfMan.startConfigPortal(ssid.c_str());
  }

  /* Connect to the AP if not connected by the WiFiManager */
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  for (uint8_t count = 5; count > 0; count--) {
    if (!WiFi.isConnected()) {
      WiFi.begin(stassid.c_str(), stapass.c_str());
    } else {
      break;
    }
    delay(1000);
  }
  if (!WiFi.isConnected()) {
    Serial.print(FPSTR(CANNOT_CONNECT));
    Serial.println(stassid.c_str());
    Serial.println(FPSTR(CONFSTR));
    WiFi.printDiag(Serial);
    Serial.println(FPSTR(RESETNODE));
    delay(1000);
    ESP.reset();
  } else {
    WiFi.setAutoConnect(true);
    Serial.println("");
    Serial.println("Connected");
    Serial.println("Current config: ");
    WiFi.printDiag(Serial);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }

  /*
      TODO: Setup the webserver (myServer)
  */
}

void loop() {
  // put your main code here, to run repeatedly:
  // myServer.handleClient();
  delay(200);
}

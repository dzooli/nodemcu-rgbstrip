#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

/* Store strings in PROGMEM
 *  
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

#define PWM_FREQ 100
#define AP_PASSW "xxx0xxx"

ESP8266WebServer myServer(80);
String stassid = "";
String stapass = "";

void deleteSavedConfig() {
  // TODO: reset the WiFi connection parameters
}

void saveWifiConfigCallback() {
  // TODO: Save the WiFiManager configuration
  Serial.println(FPSTR(SAVEWCONF));
  // Store the credentials for using it in the program
  stassid = WiFi.SSID();
  stapass = WiFi.psk();
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
  
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  analogWriteFreq(PWM_FREQ);
  analogWriteRange(1023);

  // Start the SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println(FPSTR(MOUNTFAIL));
    Serial.println(FPSTR(RESETNODE));
    delay(1000);
    ESP.reset();
    return;
  }

  /*
   * TODO: Open and parse the stored credentialsfile 
   */

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
      wfMan.startConfigPortal(ssid.c_str(), AP_PASSW);
  } 
  
  // Connect to the AP if not connected by the WiFiManager
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  for (uint8_t count = 5; count>0; count--) {
    if (!WiFi.isConnected()){
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
  }

  /* 
   *  TODO: Setup the webserver (myServer)
   */
}

void loop() {
  // put your main code here, to run repeatedly:
  // myServer.handleClient();
  delay(2000);
}

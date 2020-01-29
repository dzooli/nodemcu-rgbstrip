#include <ArduinoJson.h>
#include <string.h>

#include "DomoticzRGBDimmer.h"

/*
   ArduinoJson Assistant based parser code for MQTT messages of the Domoticz
*/

struct DomoticzRGBDimmer parseDomoticzRGBDimmer(const char* payload) {
  const size_t capacity = JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(14) + 220;
  DynamicJsonBuffer jsonBuffer(capacity);
  DomoticzRGBDimmer res;

  //const char* json = "{\"Battery\":255,\"Color\":{\"b\":255,\"cw\":0,\"g\":189,\"m\":3,\"r\":227,\"t\":0,\"ww\":0},\"Level\":80,\"RSSI\":12,\"description\":\"\",\"dtype\":\"Color Switch\",\"id\":\"00082010\",\"idx\":10,\"name\":\"RoomLampMain\",\"nvalue\":10,\"stype\":\"RGB\",\"svalue1\":\"80\",\"switchType\":\"Dimmer\",\"unit\":1}";

  JsonObject& root = jsonBuffer.parseObject(payload);

  JsonObject& Color = root["Color"];

  //  const char* stype = root["stype"]; // "RGB"
  res.stype = (char*)calloc(strlen(root["stype"]) + 1, sizeof(char));
  strncpy(res.stype, root["stype"], strlen(root["stype"]));

  if (!strstr(res.stype, "RGB")) {
    res.Battery = -1;
    return res;
  }

  //  const char* switchType = root["switchType"]; // "Dimmer"
  res.switchType = (char*)calloc(strlen(root["switchType"]) + 1, sizeof(char));
  strncpy(res.switchType, root["switchType"], strlen(root["switchType"]));

  if (!strstr(res.switchType, "Dimmer")) {
    res.Battery = -1;
    return res;
  }

  res.Color_b = Color["b"]; // 255
  res.Color_cw = Color["cw"]; // 0
  res.Color_g = Color["g"]; // 189
  res.Color_m = Color["m"]; // 3
  res.Color_r = Color["r"]; // 227
  res.Color_t = Color["t"]; // 0
  res.Color_ww = Color["ww"]; // 0
  res.Battery = root["Battery"]; // 255
  res.idx = root["idx"]; // 10
  res.Level = root["Level"]; // 80
  res.RSSI = root["RSSI"]; // 12
  res.nvalue = root["nvalue"]; // 10
  res.unit = root["unit"]; // 1

  //  const char* description = root["description"]; // ""
  res.description = (char*)calloc(strlen(root["description"]) + 1, sizeof(char));
  strncpy(res.description, root["description"], strlen(root["description"]));

  //  const char* dtype = root["dtype"]; // "Color Switch"
  res.dtype = (char*)calloc(strlen(root["dtype"]) + 1, sizeof(char));
  strncpy(res.dtype, root["dtype"], strlen(root["dtype"]));

  //  const char* id = root["id"]; // "00082010"
  res.id = (char*)calloc(strlen(root["id"]) + 1, sizeof(char));
  strncpy(res.id, root["id"], strlen(root["id"]));

  //  const char* name = root["name"]; // "RoomLampMain"
  res.name = (char*)calloc(strlen(root["name"]) + 1, sizeof(char));
  strncpy(res.name, root["name"], strlen(root["name"]));

  //  const char* svalue1 = root["svalue1"]; // "80"
  res.svalue1 = (char*)calloc(strlen(root["svalue1"]) + 1, sizeof(char));
  strncpy(res.svalue1, root["svalue1"], strlen(root["svalue1"]));

  return res;
}

void freeDomoticzRGBDimmer(DomoticzRGBDimmer* storage)
{
  if (!storage) return;
  if (storage->description) {
    free(storage->description);
  }
  if (storage->dtype) {
    free(storage->dtype);
  }
  if (storage->id) {
    free(storage->id);
  }
  if (storage->name) {
    free(storage->name);
  }
  if (storage->stype) {
    free(storage->stype);
  }
  if (storage->svalue1) {
    free(storage->svalue1);
  }
  if (storage->switchType) {
    free(storage->switchType);
  }
  return;
}

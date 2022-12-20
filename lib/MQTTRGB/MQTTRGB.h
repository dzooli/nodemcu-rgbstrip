#ifndef __MQTTRGB_H__

#define __MQTTRGB_H__

#include "../../include/DomoticzRGBDimmer.h"

struct DomoticzRGBDimmer parseDomoticzRGBDimmer(const char *payload);
void freeDomoticzRGBDimmer(DomoticzRGBDimmer *storage);

#endif // __MQTTRGB_H__


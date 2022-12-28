#ifndef __RGBSTRIP_MACROS_H__

#define __RGBSTRIP_MACROS_H__

#include <Arduino.h>

#define SLEDS_OFF                \
    {                            \
        digitalWrite(D2, false); \
        digitalWrite(D5, false); \
        digitalWrite(D4, false); \
    }
#define CAPTIVE_ON               \
    {                            \
        digitalWrite(D2, false); \
        digitalWrite(D5, false); \
        digitalWrite(D4, true);  \
    } // Blue LED
#define BOOT_ON                  \
    {                            \
        digitalWrite(D2, true);  \
        digitalWrite(D5, true);  \
        digitalWrite(D4, false); \
    } // Yellow LED
#define ERROR_ON                 \
    {                            \
        digitalWrite(D2, true);  \
        digitalWrite(D5, false); \
        digitalWrite(D4, false); \
    } // Red LED
#define OK_ON                    \
    {                            \
        digitalWrite(D2, false); \
        digitalWrite(D5, true);  \
        digitalWrite(D4, false); \
    } // Green LED

inline void selfTest()
{
    // STATUS LED TEST
    SLEDS_OFF
    delay(100);
    SLEDS_OFF
    CAPTIVE_ON
    delay(100);
    SLEDS_OFF
    BOOT_ON
    delay(100);
    SLEDS_OFF
    ERROR_ON
    delay(100);
    SLEDS_OFF
    OK_ON
    delay(100);
    SLEDS_OFF
}


#endif

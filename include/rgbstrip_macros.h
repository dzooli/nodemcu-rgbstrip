#ifndef __RGBSTRIP_MACROS_H__

#define __RGBSTRIP_MACROS_H__

#define SLEDS_OFF                \
    {                            \
        digitalWrite(D6, false); \
        digitalWrite(D7, false); \
        digitalWrite(D8, false); \
    }
#define CAPTIVE_ON               \
    {                            \
        digitalWrite(D6, false); \
        digitalWrite(D7, false); \
        digitalWrite(D8, true);  \
    } // Blue LED
#define BOOT_ON                  \
    {                            \
        digitalWrite(D6, true);  \
        digitalWrite(D7, true);  \
        digitalWrite(D8, false); \
    } // Yellow LED
#define ERROR_ON                 \
    {                            \
        digitalWrite(D6, true);  \
        digitalWrite(D7, false); \
        digitalWrite(D8, false); \
    } // Red LED
#define OK_ON                    \
    {                            \
        digitalWrite(D6, false); \
        digitalWrite(D7, true);  \
        digitalWrite(D8, false); \
    } // Green LED

#endif

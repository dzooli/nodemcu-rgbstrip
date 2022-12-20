#ifndef __DOMOTICZRGB_H__

#define __DOMOTICZRGB_H__

typedef struct DomoticzRGBDimmer {
  int Battery, 
      Color_b, Color_g, Color_r,
      Color_cw, Color_ww, Color_t, Color_m, 
      Level,
      RSSI,
      idx,
      nvalue,
      unit;
  char *description,
       *dtype, 
       *id, 
       *name, 
       *stype, 
       *svalue1, 
       *switchType;
};

#endif // __DOMOTICZRGB_H__


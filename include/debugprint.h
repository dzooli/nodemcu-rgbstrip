#ifndef __DEBUG_PRINT_H__

#define __DEBUG_PRINT_H__

#ifdef DEBUG
#define DEBUGPRINT(s)   Serial.println(F(s))
#else
#define DEBUGPRINT(s) ;
#endif

#endif

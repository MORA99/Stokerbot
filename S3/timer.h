#ifndef timer_h
 #define timer_h
 #include <stdint.h>
 #include <avr/interrupt.h>
 uint16_t tickDiff(uint16_t oldtick);
 uint16_t tickDiffS(uint16_t oldtick);
 void initTimer(void);
#endif

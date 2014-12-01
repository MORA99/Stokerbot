#include "timer.h"
#include "twi.h"

uint16_t seconds, tick;
uint16_t tickS = 1;
uint16_t timerCounter;
extern void updateCounters();

inline uint16_t tickDiff(uint16_t oldtick)
{
	return tick - oldtick;
}

inline uint16_t tickDiffS(uint16_t oldtick)
{
	return tickS - oldtick;
}

void initTimer(void)
{
   TCCR1B |= (1 << WGM12) | (1 << CS12); // Configure timer 1 for CTC mode
   OCR1A   = 125; // Compare value 125
   TCCR1A |= (1 << CS12); // Start timer at Fcpu/256
   
   timerCounter = 0;
	
   TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt
}


ISR(TIMER1_COMPA_vect)
{
  tick++;
  timerCounter++;
  
  if (tickDiff(seconds) >= 500)
  {
  	tickS++;
	seconds = tick;
  }

  if (tick % 10 == 0)
  	twiDecTo();
	  
  updateCounters();
}

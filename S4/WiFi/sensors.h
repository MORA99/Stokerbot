#ifndef sensors_h
#define sensors_h

#include "Energia.h"
#define MAXSENSORS 100

struct sensor {
	char name[20];
        float value;
        uint16_t lastChange;        
        uint16_t lastUpdate;
};


class Sensors
{
private:
	sensor _sensors[MAXSENSORS];
        uint16_t _currentTick=0;
        uint8_t _nextSpot=0;        

public:
  void add(char* name, float value);
  float get(char* name);
  sensor getSensor(uint8_t i);
  void remove(char* name);
  uint8_t getNextSpot();
  void tick();
  uint16_t getCurrentTick();
};

#endif

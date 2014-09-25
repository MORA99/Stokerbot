/*
  Library for DHT11 and DHT22/RHT03/AM2302
  https://github.com/MORA99/Stokerbot/tree/master/Libraries/dht
  Released into the public domain - http://unlicense.org
*/

#ifndef DHT_H_
#define DHT_H_
#include "Energia.h"
#define DHT_TIMEOUT 250000

class dht
{
  public:
  static int8_t readRawData(unsigned char pin, int16_t *temperature, int16_t *humidity, boolean dht22);
  static int8_t readFloatData(unsigned char pin, float *temperature, float *humidity, boolean dht22);
  static float convertTemperature(int16_t temperaure);
  static float convertHumidity(int16_t humidity);  
};

#endif

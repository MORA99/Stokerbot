#ifndef DHT_H_
#define DHT_H_

#include "Energia.h"
#define DHT_TIMEOUT 250000
int8_t dht_getrawdata(unsigned char pin, short *temperature, short *humidity, boolean dht22);
int8_t dht_getfloatdata(unsigned char pin, float *temperature, float *humidity, boolean dht22);
#endif


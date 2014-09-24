/*
DHT Library 0x03

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.

References:
  - DHT-11 Library, by Charalampos Andrianakis on 18/12/11
*/


#ifndef DHT_H_
#define DHT_H_

#include <stdio.h>
#include <avr/io.h>
#include <stdbool.h>

//setup port
#define DHT_DDR DDRC
#define DHT_PORT PORTC
#define DHT_PIN PINC

//sensor type
#define DHT_DHT11 1
#define DHT_DHT22 2
#define DHT_RHT03 2
#define DHT_TYPE DHT_DHT22

//enable decimal precision (float)
#if DHT_TYPE == DHT_DHT11
#define DHT_FLOAT 0
#elif DHT_TYPE == DHT_DHT22
#define DHT_FLOAT 1
#endif

//timeout retries
#define DHT_TIMEOUT 200

extern int8_t dht_getdata(uint8_t pin, int16_t *temperature, int16_t *humidity, bool dht22);

#endif

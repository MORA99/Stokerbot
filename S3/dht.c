/*
DHT Library 0x03

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

#include "dht.h"

uint16_t rawhumidity;
uint16_t rawtemperature;

/*
 * get data from sensor
 */
int8_t dht_getdata(uint8_t syspin, int16_t *temperature, int16_t *humidity, bool dht22) {
	uint8_t bits[5];
	uint8_t i,j = 0;

	memset(bits, 0, sizeof(bits));

	//reset port
	DHT_DDR |= (1<<syspin); //output
	DHT_PORT |= (1<<syspin); //high
	_delay_ms(100);

	//send request
	DHT_PORT &= ~(1<<syspin); //low
	if (!dht22)
		_delay_ms(18);
	else
		_delay_us(500);

	DHT_PORT |= (1<<syspin); //high
	DHT_DDR &= ~(1<<syspin); //input
	_delay_us(40);

	//check start condition 1
	if((DHT_PIN & (1<<syspin))) {
		return -1;
	}
	_delay_us(80);
	//check start condition 2
	if(!(DHT_PIN & (1<<syspin))) {
		return -2;
	}
	_delay_us(80);

	//read the data
	uint16_t timeoutcounter = 0;
	for (j=0; j<5; j++) { //read 5 byte
		uint8_t result=0;
		for(i=0; i<8; i++) {//read every bit
			timeoutcounter = 0;
			while(!(DHT_PIN & (1<<syspin))) { //wait for an high input (non blocking)
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -3; //timeout
				}
			}
			_delay_us(30);
			if(DHT_PIN & (1<<syspin)) //if input is high after 30 us, get result
				result |= (1<<(7-i));
			timeoutcounter = 0;
			while(DHT_PIN & (1<<syspin)) { //wait until input get low (non blocking)
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -4; //timeout
				}
			}
		}
		bits[j] = result;
	}

	//reset port
	DHT_DDR |= (1<<syspin); //output
	DHT_PORT |= (1<<syspin); //low
	_delay_ms(100);

	//check checksum
	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		printf("CRC OK %d %d %d %d", bits[0], bits[1], bits[2], bits[3]);
		//return temperature and humidity
		if (dht22)
		{
			*humidity = bits[0]<<8 | bits[1];
			*temperature = bits[2]<<8 | bits[3];
		} else {
			*humidity = bits[0];
			*temperature = bits[2];			
		}
		return 0;
	}

	return -5;
}

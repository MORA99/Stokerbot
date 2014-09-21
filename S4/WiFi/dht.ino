#include "Energia.h"
#include "dht.h"

/*
DHT Library 0x03

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

/*
 * get data from sensor
 */
int8_t dht_getdata(unsigned char pin, short *temperature, short *humidity) {
	uint8_t bits[5];
	uint8_t i,j = 0;

	memset(bits, 0, sizeof(bits));

	//reset port
	pinMode(pin, OUTPUT);
	digitalWrite(pin, HIGH);
	delay(100);

	//send request
	digitalWrite(pin, LOW);
	#if DHT_TYPE == DHT_DHT11
	delay(18);
	#elif DHT_TYPE == DHT_DHT22
	delayMicroseconds(500);
	#endif
	digitalWrite(pin, HIGH);
	pinMode(pin, INPUT);
	delayMicroseconds(40);

	//check start condition 1
	if(digitalRead(pin)==HIGH) {
		return -1;
	}
	delayMicroseconds(80);
	//check start condition 2
	if(digitalRead(pin)==LOW) {
		return -2;
	}
	delayMicroseconds(80);

	//read the data
	uint16_t timeoutcounter = 0;
	for (j=0; j<5; j++) { //read 5 byte
		uint8_t result=0;
		for(i=0; i<8; i++) {//read every bit
			timeoutcounter = 0;
			while(digitalRead(pin)==LOW) { //wait for an high input
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -3; //timeout
				}
			}
			delayMicroseconds(30);
			if(digitalRead(pin)==HIGH) //if input is high after 30 us, get result
				result |= (1<<(7-i));
			timeoutcounter = 0;
			while(digitalRead(pin)==LOW) { //wait until input get low
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -4; //timeout
				}
			}
		}
		bits[j] = result;
	}

	//reset port
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
	delay(100);

	//check checksum
	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		//return temperature and humidity
		*humidity = bits[0]<<8 | bits[1];
		*temperature = bits[2]<<8 | bits[3];
		return 0;
	}

	return -5;
}


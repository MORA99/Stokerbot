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
pinMode(3, OUTPUT);
	uint8_t bits[5];
	uint8_t i,j = 0;

	memset(bits, 0, sizeof(bits));
/*
	//reset port
	pinMode(pin, OUTPUT);
	digitalWrite(pin, HIGH);
	delay(100);
*/
	//send request

	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
	#if DHT_TYPE == DHT_DHT11
	delay(18);
	#elif DHT_TYPE == DHT_DHT22
	delayMicroseconds(500);
	#endif

	pinMode(pin, INPUT);
	digitalWrite(pin, HIGH);

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
//	delayMicroseconds(40);

      //Sensor init ok, now read 5 bytes ...
      for (j=0; j<5; j++)
      {
        for (int8_t i=7; i>=0; i--)
        {
          if (pulseIn(pin, HIGH, 1000) > 30)
            bitSet(bits[j], i);
        }
      }
      
	//reset port
	pinMode(pin, INPUT);

	//check checksum
        Serial.println(bits[0],BIN);
        Serial.println(bits[1],BIN);
        Serial.println(bits[2],BIN);
        Serial.println(bits[3],BIN);
        Serial.println(bits[4],BIN);
        Serial.println(bits[0]+bits[1]+bits[2]+bits[3],BIN);

        Serial.print(bits[0]);Serial.print(" ");        
        Serial.print(bits[1]);Serial.print(" ");        
        Serial.print(bits[2]);Serial.print(" ");        
        Serial.print(bits[3]);Serial.println("");                

	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		//return temperature and humidity
		*humidity = bits[0]<<8 | bits[1];
		*temperature = bits[2]<<8 | bits[3];
		return 0;
	}

	return -5;
}


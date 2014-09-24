#include "Energia.h"
#include "dht.h"

int8_t dht_getrawdata(unsigned char pin, short *temperature, short *humidity, boolean dht22) {
	uint8_t bits[5];
	uint8_t i,j = 0;

	memset(bits, 0, sizeof(bits));

	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
	if (dht22)
	  delayMicroseconds(500);
        else
  	  delay(18);

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

	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		//return temperature and humidity
                if (dht22) {
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

int8_t dht_getfloatdata(unsigned char pin, float *temperature, float *humidity, boolean dht22)
{
  short rawtemperature, rawhumidity;
  int8_t res = dht_getrawdata(pin, &rawtemperature, &rawhumidity, dht22);
  if (res != 0)
  {
    return res;
  }
  if (dht22) {
  if(rawtemperature & 0x8000) {
    *temperature = (float)((rawtemperature & 0x7FFF) / 10.0) * -1.0;
  } else {
    *temperature = (float)rawtemperature/10.0;
  }
  *humidity = (float)rawhumidity/10.0;  
  } else {
    *temperature = (float)rawtemperature;
    *humidity = (float)rawhumidity;
  }
  return 0;
}


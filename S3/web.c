#include <avr/wdt.h>
#include "web.h"
#include "webFiles.h"
#include "I2CEEPROM.h"
#include "SPILCD.h"

#define TRANS_NUM_GWMAC 1
#define TRANS_NUM_WWWMAC 2

uint8_t gw_arp_state = 0;
uint8_t wwwip[4] = {144,76,16,173}; //Stokerlog.dk as of Oktober 2014, fallback if DNS fails

uint32_t last_rio_event = 0;
uint32_t uptime = 0;
uint16_t coapid = 0;
uint16_t timed_events[12*4];

static int8_t dns_state=0;
uint8_t start_web_client=0;
static uint32_t web_client_attempts=0;
static uint32_t web_client_sendok=0;
uint8_t web_client_monitor=0;
uint16_t dnsTimer;

char lines[6][4][21];
uint8_t lcd_timeouts[6];
uint8_t lcd_current_screen = 255;
uint8_t lcd_current_timeleft = 0;
uint8_t lcd_network_timer = 0;

char host[100];
char url[125];
static uint8_t gwmac[6]; 
static uint8_t wwwmac[6]; 

uint8_t sendBoxData=0;
uint8_t httpType = 0;

uint8_t coapclient = 1;

extern void reScheduleTasks();
extern void timedSaveEeprom();

#if SBNG_TARGET == 50
extern bool alarmDetected;
extern uint8_t alarmTimeout;
#endif

void lcd_write_screen(uint8_t screen)
{
	for (uint8_t i=0; i<4; i++)
	{
		LCDsetCursor(i, 0);
		LCDwrite(lines[screen][i], 0);
	}
}

//called every second
void lcd_update()
{
	if (lcd_current_screen == 255)
	{
		printf("First call to lcd_update, starting web client \r\n");
		//first screen update
		if (start_web_client == 0)
		{
			start_web_client = 10;
			lcd_current_screen = 254;
			//waiting for LCD data, callback function sets it to 250 when theres valid data
		}
	} else if (lcd_current_screen == 254)
	{
		if (lcd_network_timer++ > 60)
		{
			printf("Retry LCD get\r\n");
			start_web_client = 10;
			lcd_network_timer=0;
		}
	} else if (lcd_current_screen == 250)
	{
		printf("Web client set screen to 250 \r\n");
		lcd_current_screen = 0;
		lcd_current_timeleft = lcd_timeouts[0];
		lcd_write_screen(0);
	}

	if (lcd_current_screen >= 0 && lcd_current_screen <= 6)
	{
		if (lcd_current_timeleft > 0)
			lcd_current_timeleft--;
		if (lcd_network_timer > 0)
			lcd_network_timer--;

		//regular run
		uint8_t screensDefined = 0;
		for (uint8_t i=0; i<6; i++)
		{
			if (lcd_timeouts[i] > 0)
				screensDefined++;
		}

		printf("Timeout left for %u is %u \r\n", lcd_current_screen, lcd_current_timeleft);

		if (lcd_network_timer == 0
			&&
			(
				(screensDefined == 1 && lcd_current_timeleft < 20)
				||
				(screensDefined == lcd_current_screen+1)
			)
		   )
		   {
				if (start_web_client == 0)
				{
					printf("Requesting new LCD data\r\n");
					start_web_client = 10;
					lcd_network_timer = 30;
				}
		   }

		   if (lcd_current_timeleft == 0)
		   {
				printf("Time to change screen ... \r\n");
				lcd_current_screen++;
				if (lcd_current_screen >= screensDefined) //screens are zero indexed
					lcd_current_screen = 0;
				lcd_write_screen(lcd_current_screen);
				lcd_current_timeleft = lcd_timeouts[lcd_current_screen];
				printf("Changed to screen %u ... \r\n",lcd_current_screen);
		   }
	}
}


void initTimedEvents()
{
	coapclient = eepromReadByte(1500);
	printf("Init timed_events \r\n");
	for (uint16_t i=0; i<(12*4); i++)
	{
		timed_events[i] = 0;
	}
}

//called once per second
void checkTimedEvents()
{
	uptime++;

	for (uint8_t i=0; i<12; i++)
	{
		if (timed_events[i*4] != 0)
		{
			printf("Found timed event %u %u %u %u %u... \r\n", i, timed_events[(i*4)+0], timed_events[(i*4)+1], timed_events[(i*4)+2], timed_events[(i*4)+3]);

			#if SBNG_TARGET == 50
			if (timed_events[(i*4)+1] >= 4 && (timed_events[(i*4)+1] < 20 || timed_events[(i*4)+1] > 29))
			{
				printf("Pin not in acceptable range (%u)\r\n",timed_events[(i*4)+1]);
				break;
			}
			#else
			if (timed_events[(i*4)+1] >= 4)
			{
				printf("Pin not in acceptable range (%u)\r\n",timed_events[(i*4)+1]);
				break;
			}
			#endif

			timed_events[(i*4)+3] = timed_events[(i*4)+3]-1;

			if (timed_events[(i*4)+3] == 0)
			{
				printf("TIMEOUT\r\n");
				//reverse the pin
				#if SBNG_TARGET == 50
					if (timed_events[(i*4)+1] > 20)
					{
						if (timed_events[(i*4)+2] == 1)
							simpleSensorValues[timed_events[(i*4)+1]] = 0;
						else
							simpleSensorValues[timed_events[(i*4)+1]] = 1;
					} else {
				#endif
				if (timed_events[(i*4)+2] == 1)
					CLEARBIT(PORTC, (2+timed_events[(i*4)+1]));
				else
					SETBIT(PORTC, (2+timed_events[(i*4)+1]));
				#if SBNG_TARGET == 50
				}
				#endif
				//reset the array
				timed_events[(i*4)+0] = 0;
				timed_events[(i*4)+1] = 0;
				timed_events[(i*4)+2] = 0;
				timed_events[(i*4)+3] = 0;
			}
		}
	}
}

void addTimedEvent(uint8_t id, uint8_t pin, uint8_t state, uint8_t delay)
{
	printf("addTimedEvent(%d, %d, %d, %d)\r\n",id,pin,state,delay);

	for (uint8_t i=0; i<12; i++)
	{
		if (timed_events[i*4] == 0)
		{
			timed_events[(i*4)+0] = id;
			timed_events[(i*4)+1] = pin;
			timed_events[(i*4)+2] = state;
			timed_events[(i*4)+3] = delay;
			break;
		}
	}
}


uint16_t http200ok(void)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t http200gzok(void)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Encoding: gzip\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t https401(void)
{
	return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Authorization Required\r\nWWW-Authenticate: Basic realm=\"Stokerbot admin\"\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t http400(void)
{
	return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 400 Bad request\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}


uint16_t print_export_htm(uint8_t *buf)
{
	uint16_t plen;

	plen=http200ok();
	
	sprintf(tempbuf, "Uptime %lu \r\n", uptime);
	plen = fill_tcp_data(buf,plen,tempbuf);

	for (uint8_t i=0; i<MAXSENSORS; i++)
	{
		if (sensorValues[(i*SENSORSIZE)+FAMILY] != 0)
		{
			int frac = sensorValues[(i*SENSORSIZE)+VALUE2]*DS18X20_FRACCONV;  //Ganger de sidste par bits, med det step DS18B20 bruger
			
			sprintf_P(tempbuf, PSTR("%02X%02X%02X%02X%02X%02X%02X%02X %c%d.%04d\r\n"),
			sensorValues[(i*SENSORSIZE)+FAMILY],
			sensorValues[(i*SENSORSIZE)+ID1],
			sensorValues[(i*SENSORSIZE)+ID2],
			sensorValues[(i*SENSORSIZE)+ID3],
			sensorValues[(i*SENSORSIZE)+ID4],
			sensorValues[(i*SENSORSIZE)+ID5],
			sensorValues[(i*SENSORSIZE)+ID6],
			sensorValues[(i*SENSORSIZE)+CRC],
			sensorValues[(i*SENSORSIZE)+SIGN],
			sensorValues[(i*SENSORSIZE)+VALUE1],
			frac
			);

			plen = fill_tcp_data(buf,plen,tempbuf);
		}
	}
	
	//Analog pins
	for (uint8_t i=0; i<=7; i++)
	{
		sprintf_P(tempbuf, PSTR("A%u %lu\r\n"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}

	//Digital pins
	for (uint8_t i=0; i<=3; i++)
	{
		sprintf_P(tempbuf, PSTR("D%u %lu\r\n"),i,simpleSensorValues[i+8]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}
	#if SBNG_TARGET == 50
	//Relay states
	for (uint8_t i=24; i<=27; i++)
	{
		sprintf_P(tempbuf, PSTR("RL%u %lu\r\n"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}

	//Opto states
	for (uint8_t i=20; i<=23; i++)
	{
		sprintf_P(tempbuf, PSTR("OP%u %lu\r\n"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}
	#endif
	for (uint8_t i=0; i<4; i++)
	{
		if (simpleSensorTypes[i+8] == TYPE_DHT22)
		{
			float temperature=0;
			float humidity=0;
			if(simpleSensorValues[50] & 0x8000) {
				temperature = (float)((simpleSensorValues[50+(i*2)] & 0x7FFF) / 10.0) * -1.0;
				} else {
				temperature = (float)(simpleSensorValues[50+(i*2)])/10.0;
			}
			humidity = (float)(simpleSensorValues[50+(i*2)+1])/10.0;
			sprintf_P(tempbuf, PSTR("D%dT %g\r\nD%dH %g\r\n"),i,temperature,i,humidity);
			plen = fill_tcp_data(buf,plen,tempbuf);
		}
		if (simpleSensorTypes[i+8] == TYPE_DHT11)
		{
			float temperature=simpleSensorValues[50+(i*2)];
			float humidity=simpleSensorValues[50+(i*2)+1];
			sprintf_P(tempbuf, PSTR("D%dT %g\r\nD%dH %g\r\n"),i,temperature,i,humidity);
			plen = fill_tcp_data(buf,plen,tempbuf);
		}
	}

	return plen;
}

uint16_t print_export_json(uint8_t *buf)
{
	uint16_t plen;

	plen=http200ok();

	sprintf(tempbuf, "{\r\n");
	plen = fill_tcp_data(buf,plen,tempbuf);
	
	for (uint8_t i=0; i<MAXSENSORS; i++)
	{
		if (sensorValues[(i*SENSORSIZE)+FAMILY] != 0)
		{
			int frac = sensorValues[(i*SENSORSIZE)+VALUE2]*DS18X20_FRACCONV;  //Ganger de sidste par bits, med det step DS18B20 bruger
			if (sensorValues[(i*SENSORSIZE)+SIGN] == '+')
			{
				sprintf_P(tempbuf, PSTR("\"%02X%02X%02X%02X%02X%02X%02X%02X\":%d.%04d,\r\n"),
				sensorValues[(i*SENSORSIZE)+FAMILY],
				sensorValues[(i*SENSORSIZE)+ID1],
				sensorValues[(i*SENSORSIZE)+ID2],
				sensorValues[(i*SENSORSIZE)+ID3],
				sensorValues[(i*SENSORSIZE)+ID4],
				sensorValues[(i*SENSORSIZE)+ID5],
				sensorValues[(i*SENSORSIZE)+ID6],
				sensorValues[(i*SENSORSIZE)+CRC],
				sensorValues[(i*SENSORSIZE)+VALUE1],
				frac
				);
			} else {
				sprintf_P(tempbuf, PSTR("\"%02X%02X%02X%02X%02X%02X%02X%02X\":%c%d.%04d,\r\n"),
				sensorValues[(i*SENSORSIZE)+FAMILY],
				sensorValues[(i*SENSORSIZE)+ID1],
				sensorValues[(i*SENSORSIZE)+ID2],
				sensorValues[(i*SENSORSIZE)+ID3],
				sensorValues[(i*SENSORSIZE)+ID4],
				sensorValues[(i*SENSORSIZE)+ID5],
				sensorValues[(i*SENSORSIZE)+ID6],
				sensorValues[(i*SENSORSIZE)+CRC],
				sensorValues[(i*SENSORSIZE)+SIGN],
				sensorValues[(i*SENSORSIZE)+VALUE1],
				frac
				);				
			}

			plen = fill_tcp_data(buf,plen,tempbuf);
		}
	}
	
	//Analog pins
	for (uint8_t i=0; i<=7; i++)
	{
		sprintf_P(tempbuf, PSTR("\"A%u\":%lu,\r\n"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}

	//Digital pins
	for (uint8_t i=0; i<=3; i++)
	{
		sprintf_P(tempbuf, PSTR("\"D%u\":%lu,\r\n"),i,simpleSensorValues[i+8]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}
	#if SBNG_TARGET == 50
	//Relay states
	for (uint8_t i=24; i<=27; i++)
	{
		sprintf_P(tempbuf, PSTR("\"RL%u\":%lu,\r\n"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}

	//Opto states
	for (uint8_t i=20; i<=23; i++)
	{
		sprintf_P(tempbuf, PSTR("\"OP%u\":%lu,\r\n"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}
	#endif
	for (uint8_t i=0; i<4; i++)
	{
		if (simpleSensorTypes[i+8] == TYPE_DHT22)
		{
			float temperature=0;
			float humidity=0;
			if(simpleSensorValues[50] & 0x8000) {
				temperature = (float)((simpleSensorValues[50+(i*2)] & 0x7FFF) / 10.0) * -1.0;
				} else {
				temperature = (float)(simpleSensorValues[50+(i*2)])/10.0;
			}
			humidity = (float)(simpleSensorValues[50+(i*2)+1])/10.0;
			sprintf_P(tempbuf, PSTR("\"D%dT\":%g,\r\n\"D%dH\":%g,\r\n"),i,temperature,i,humidity);
			plen = fill_tcp_data(buf,plen,tempbuf);
		}
		if (simpleSensorTypes[i+8] == TYPE_DHT11)
		{
			float temperature=simpleSensorValues[50+(i*2)];
			float humidity=simpleSensorValues[50+(i*2)+1];
			sprintf_P(tempbuf, PSTR("\"D%dT\":%g,\r\n\"D%dH\":%g\r\n,"),i,temperature,i,humidity);
			plen = fill_tcp_data(buf,plen,tempbuf);
		}
	}

	sprintf(tempbuf, "\"Uptime\":%lu \r\n}", uptime);
	plen = fill_tcp_data(buf,plen,tempbuf);
	
	return plen;
}

void send_udp_data(char *data)
{
	if (route_via_gw(wwwip)==0)
		send_udp((uint8_t*)tempbuf,data,strlen(data),2313, wwwip, 54321,wwwmac);
	else
		send_udp((uint8_t*)tempbuf,data,strlen(data),2313, wwwip, 54321,gwmac);
}

uint16_t coap_prepare()
{
	if (route_via_gw(wwwip) == 0)
	send_udp_prepare((uint8_t*)tempbuf,5683, wwwip, 5683,wwwmac);
	else
	send_udp_prepare((uint8_t*)tempbuf,5683, wwwip, 5683,gwmac);

	coapid++;
	uint8_t p = UDP_DATA_P;
	//          VVTTLLLL
	tempbuf[p++] = 0b01010000; //V=01 T=01 (No confirm) 0 token length
	tempbuf[p++] = 0b00000010; //0.2=POST req
	tempbuf[p++] = (coapid & 0xFF00) >> 8; //Message id
	tempbuf[p++] = (coapid & 0xff); //Message id 1
	//tempbuf[p++] = 0xff; //no options
	
	eepromReadStr(1000,host,99);
	//Option 3 - delta 3 - URI_Host
	//3C 73 74 6f 6b 65 72 6c 6f 67 2e 64 6b
	uint8_t hostlen = strlen(host);

	if (hostlen > 12) //Hostname is limited to 99chars, so we dont need to support type 14
	{
		tempbuf[p++] = 0b00110000 | 13; //1101 = 13
		tempbuf[p++] = hostlen - 13;
		} else {
		tempbuf[p++] = 0b00110000 | hostlen;
	}
	
	strncpy(&tempbuf[p], host, hostlen);
	p += hostlen;

	//Option 11 - delta 8 - URI_PATH - /incoming maps to incoming.php
	//88 69 6e 63 6f 6d 69 6e 67
	tempbuf[p++] = 0x88;
	tempbuf[p++] = 0x69;
	tempbuf[p++] = 0x6e;
	tempbuf[p++] = 0x63;
	tempbuf[p++] = 0x6f;
	tempbuf[p++] = 0x6d;
	tempbuf[p++] = 0x69;
	tempbuf[p++] = 0x6e;
	tempbuf[p++] = 0x67;
	
	//Option 12 - delta 1 - format json
	//11 32
	tempbuf[p++] = 0x11;
	tempbuf[p++] = 0x32;
	
	tempbuf[p++] = 0xff; //end of options	
	return p;
}

void coap_send_digital()
{
	if (coapclient != 1) return;
	uint16_t p = coap_prepare();
	
	p += sprintf(&tempbuf[p], "{\"id\":\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"v\":1,\"sensors\":[{\"name\":\"D0\",\"value\":\"%lu\"},{\"name\":\"D1\",\"value\":\"%lu\"},{\"name\":\"D2\",\"value\":\"%lu\"},{\"name\":\"D3\",\"value\":\"%lu\"}]}",
		systemID[0],systemID[1],systemID[2],systemID[3],systemID[4],systemID[5],systemID[6],systemID[7],
		simpleSensorValues[8],simpleSensorValues[9],simpleSensorValues[10],simpleSensorValues[11]
		);	
	
	send_udp_transmit((uint8_t*)tempbuf,p-UDP_DATA_P);
}

void coap_send_analog()
{
	if (coapclient != 1) return;
	uint16_t p = coap_prepare();
	
	p += sprintf(&tempbuf[p], "{\"id\":\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"v\":1,\"sensors\":[{\"name\":\"ADC0\",\"value\":\"%lu\"},{\"name\":\"ADC1\",\"value\":\"%lu\"},{\"name\":\"ADC2\",\"value\":\"%lu\"},{\"name\":\"ADC3\",\"value\":\"%lu\"},{\"name\":\"ADC4\",\"value\":\"%lu\"},{\"name\":\"ADC5\",\"value\":\"%lu\"},{\"name\":\"ADC6\",\"value\":\"%lu\"},{\"name\":\"ADC7\",\"value\":\"%lu\"}]}",
		systemID[0],systemID[1],systemID[2],systemID[3],systemID[4],systemID[5],systemID[6],systemID[7],
		simpleSensorValues[0],simpleSensorValues[1],simpleSensorValues[2],simpleSensorValues[3],simpleSensorValues[4],simpleSensorValues[5],simpleSensorValues[6],simpleSensorValues[7]
		);
	
	send_udp_transmit((uint8_t*)tempbuf,p-UDP_DATA_P);
}

void coap_send_OW(uint8_t pos)
{
	if (coapclient != 1) return;
	uint16_t p = coap_prepare();
	
	int frac = sensorValues[(pos*SENSORSIZE)+VALUE2]*DS18X20_FRACCONV;  //Ganger de sidste par bits, med det step DS18B20 bruger
	
	p+=sprintf(&tempbuf[p], "{\"id\":\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"v\":1,\"sensors\":[{\"name\":\"%02X%02X%02X%02X%02X%02X%02X%02X\",\"value\":\"%c%d.%04d\"}]}",
	systemID[0],systemID[1],systemID[2],systemID[3],systemID[4],systemID[5],systemID[6],systemID[7],
	sensorValues[(pos*SENSORSIZE)+FAMILY],
	sensorValues[(pos*SENSORSIZE)+ID1],
	sensorValues[(pos*SENSORSIZE)+ID2],
	sensorValues[(pos*SENSORSIZE)+ID3],
	sensorValues[(pos*SENSORSIZE)+ID4],
	sensorValues[(pos*SENSORSIZE)+ID5],
	sensorValues[(pos*SENSORSIZE)+ID6],
	sensorValues[(pos*SENSORSIZE)+CRC],
	sensorValues[(pos*SENSORSIZE)+SIGN],
	sensorValues[(pos*SENSORSIZE)+VALUE1],
	frac
	);	
	
	send_udp_transmit((uint8_t*)tempbuf,p-UDP_DATA_P);
}

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf, bool ajax)
{
        uint16_t plen;

		plen=http200ok();
		if (!ajax) {
			sprintf_P(tempbuf, PSTR("<SCRIPT SRC='/loader.js?%u'></SCRIPT><script src='/microajax.js'></script>"),tickS);
			plen=fill_tcp_data(buf,plen,tempbuf);

			sprintf_P(tempbuf, PSTR("<SCRIPT>menu(2,1);</SCRIPT>"));
			plen=fill_tcp_data(buf,plen,tempbuf);

			sprintf_P(tempbuf, PSTR("SystemID: %02X%02X%02X%02X%02X%02X%02X%02X<br><br>"),systemID[0],systemID[1],systemID[2],systemID[3],systemID[4],systemID[5],systemID[6],systemID[7]);
			plen=fill_tcp_data(buf,plen,tempbuf);
		}
	
        if (gw_arp_state != 2){
                plen=fill_tcp_data_p(buf,plen,PSTR("<br>waiting for GW IP to answer arp.<br>"));
                return(plen);
        }
        if (dns_state==1){
                plen=fill_tcp_data_p(buf,plen,PSTR("<br>waiting for DNS answer.<br>"));
                return(plen);
        }

		plen=fill_tcp_data_p(buf,plen,PSTR("<div id='index'>"));

		uint8_t days = 0;
		uint8_t hours = 0;
		uint8_t minutes = 0;
		uint8_t seconds = 0;
		uint32_t up = uptime;
		while (up >= 86400)
		{
			days++;
			up -= 86400;
		}
		while (up >= 3600)
		{
			hours++;
			up -= 3600;
		}
		while (up >= 60)
		{
			minutes++;
			up -= 60;
		}
		seconds = up;

		sprintf_P(tempbuf, PSTR("Uptime: %udays %uhours %uminutes %useconds<br><br>"),days, hours, minutes, seconds);
		plen=fill_tcp_data(buf,plen,tempbuf);

		uint32_t pct = 0;	
		if (web_client_attempts > 0 && web_client_sendok > 0) pct = ((web_client_sendok *100) / web_client_attempts);
		
		sprintf_P(tempbuf, PSTR("Webclient uploads %lu/%lu (%lu%%)<br>"),web_client_sendok,web_client_attempts,pct);
		plen=fill_tcp_data(buf,plen,tempbuf);

		plen=fill_tcp_data_p(buf,plen,PSTR("<br><table border=0><thead><tr><th colspan=2>Sensors</th></tr>"));
	
	//Analog pins
	for (uint8_t i=0; i<=3; i++)
	{
		sprintf_P(tempbuf, PSTR("<tr><td>A%u</td><td>%lu</td></tr>"),i,simpleSensorValues[i]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}

	//Digital pins
	for (uint8_t i=0; i<=3; i++)
	{
		sprintf_P(tempbuf, PSTR("<tr><td>D%u</td><td>%lu</td></tr>"),i,simpleSensorValues[i+8]);
		plen = fill_tcp_data(buf,plen,tempbuf);
	}		

	for (uint8_t i=0; i<4; i++)
	{
		if (simpleSensorTypes[i+8] == TYPE_DHT22)
		{
			float temperature=0;
			float humidity=0;
			if(simpleSensorValues[50] & 0x8000) {
				temperature = (float)((simpleSensorValues[50+(i*2)] & 0x7FFF) / 10.0) * -1.0;
				} else {
				temperature = (float)(simpleSensorValues[50+(i*2)])/10.0;
			}
			humidity = (float)(simpleSensorValues[50+(i*2)+1])/10.0;
			sprintf_P(tempbuf, PSTR("<tr><td>D%dT</td><td>%g</td></tr><tr><td>D%dH</td><td>%g</td></tr>"),i,temperature,i,humidity);
			plen = fill_tcp_data(buf,plen,tempbuf);
		}
		if (simpleSensorTypes[i+8] == TYPE_DHT11)
		{
			float temperature=simpleSensorValues[50+(i*2)];
			float humidity=simpleSensorValues[50+(i*2)+1];
			sprintf_P(tempbuf, PSTR("<tr><td>D%dT</td><td>%g</td><td>D%dH</td><td>%g</td></tr>"),i,temperature,i,humidity);
			plen = fill_tcp_data(buf,plen,tempbuf);
		}
	}

	for (uint8_t i=0; i<MAXSENSORS; i++)
	{
		if (sensorValues[(i*SENSORSIZE)+FAMILY] != 0)
		{
			int frac = sensorValues[(i*SENSORSIZE)+VALUE2]*DS18X20_FRACCONV;  //Ganger de sidste par bits, med det step DS18B20 bruger
			
			sprintf_P(tempbuf, PSTR("<tr><td>%02X%02X%02X%02X%02X%02X%02X%02X</td><td>%c%d.%04d</td></tr>"),
			sensorValues[(i*SENSORSIZE)+FAMILY],
			sensorValues[(i*SENSORSIZE)+ID1],
			sensorValues[(i*SENSORSIZE)+ID2],
			sensorValues[(i*SENSORSIZE)+ID3],
			sensorValues[(i*SENSORSIZE)+ID4],
			sensorValues[(i*SENSORSIZE)+ID5],
			sensorValues[(i*SENSORSIZE)+ID6],
			sensorValues[(i*SENSORSIZE)+CRC],
			sensorValues[(i*SENSORSIZE)+SIGN],
			sensorValues[(i*SENSORSIZE)+VALUE1],
			frac
			);

			plen = fill_tcp_data(buf,plen,tempbuf);
		}
	}
		
		plen=fill_tcp_data_p(buf,plen,PSTR("</table>"));
		
		if (!ajax)
		{		
			plen=fill_tcp_data_p(buf,plen,PSTR("<script>function upd() { microAjax('/index.ajax', function (res) {document.getElementById('index').innerHTML= res;setTimeout(upd, 5000);});};setTimeout(upd(),5000);</script>"));
				
			printf_P(PSTR("Index : Plen is %d \r\n"), plen);				
		}
		       
		return(plen);
}


// prepare the webpage by writing the data to the tcp send buffer

uint16_t print_sensorlist(uint8_t *buf)
{
        uint16_t plen;
		bool first = true;

		plen=http200ok();
    
		sprintf_P(tempbuf, PSTR("var sensors=["));
		plen = fill_tcp_data(buf,plen,tempbuf);

	    for (uint8_t i=0; i<MAXSENSORS; i++)
	    {  
	      if (sensorValues[(i*SENSORSIZE)+FAMILY] != 0)
	      {
		  	if (!first)
				plen = fill_tcp_data(buf,plen,",");
			else
				first=false;

	        int frac = sensorValues[(i*SENSORSIZE)+VALUE2]*DS18X20_FRACCONV;  //Ganger de sidste par bits, med det step DS18B20 bruger
    
	        sprintf_P(tempbuf, PSTR("[%u, '%02X%02X%02X%02X%02X%02X%02X%02X', '%c%d.%04d']"),i,
	              sensorValues[(i*SENSORSIZE)+FAMILY],
	              sensorValues[(i*SENSORSIZE)+ID1],
	              sensorValues[(i*SENSORSIZE)+ID2],
	              sensorValues[(i*SENSORSIZE)+ID3],
	              sensorValues[(i*SENSORSIZE)+ID4],
	              sensorValues[(i*SENSORSIZE)+ID5],
	              sensorValues[(i*SENSORSIZE)+ID6],
	              sensorValues[(i*SENSORSIZE)+CRC],
	              sensorValues[(i*SENSORSIZE)+SIGN],
	              sensorValues[(i*SENSORSIZE)+VALUE1],
	              frac
	            );      

	            plen = fill_tcp_data(buf,plen,tempbuf);
	      }
		}
	
		plen=fill_tcp_data_p(buf,plen,PSTR("];"));

        return plen;
}

uint16_t print_alarmlist(uint8_t *buf)
{
/*

400 Alarm 1 enabled?
401 Alarm 1 address 1
402 Alarm 1 address 2
403 Alarm 1 address 3
404 Alarm 1 address 4
405 Alarm 1 address 5
406 Alarm 1 address 6
407 Alarm 1 address 7
408 Alarm 1 address 8
409 Alarm 1 type
410 Alarm 1 value
411 Alarm 1 target (0-3)
412 Alarm 1 state (during alarm) (0/1)
413 Alarm 1 reverse?

1 400
2 415
3 430
4 445
5 460
6 475
7 490
8 505

*/
        uint16_t plen;

		plen=http200ok();

		plen=fill_tcp_data_p(buf,plen,PSTR("var alarms = new Array();"));

		for (uint8_t alarm=1; alarm<=NUMALARMS; alarm++)
		{
			uint16_t pos = 400 + ((alarm-1)*15); //400 415 430 445 ...
			uint16_t sensorid  = 255;
			if (eepromReadByte(pos+1) != 0 && eepromReadByte(pos+1) != 0xFF)
			{
					sensorid = findSensor(
					eepromReadByte(pos+1), eepromReadByte(pos+2), eepromReadByte(pos+3), eepromReadByte(pos+4),
					eepromReadByte(pos+5), eepromReadByte(pos+6), eepromReadByte(pos+7), eepromReadByte(pos+8)
				);
			}

			sprintf_P(tempbuf, PSTR("alarms[%u] = [%u, '%u','%i','%u','%u','%u','%u'];"), alarm, sensorid,
			eepromReadByte(pos+9), eepromReadByteSigned(pos+10),eepromReadByte(pos+11),eepromReadByte(pos+12),
			eepromReadByte(pos+13),eepromReadByte(pos+0));
			
			plen=fill_tcp_data(buf,plen,tempbuf);
		}
    	
        return plen;
}

uint16_t print_webclient_webpage(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok();

	uint8_t interval = eepromReadByte(1200);

	char orgurl[100];
	eepromReadStr(1101,orgurl,99);
	eepromReadStr(1000,host,99);
	uint8_t coap = eepromReadByte(1500);
	sprintf_P(tempbuf, PSTR("<SCRIPT>var interval = %u;var host = '%s';var url='%s';var coap = %i;</SCRIPT>"), interval, host, orgurl, coap);
	plen=fill_tcp_data(buf,plen,tempbuf);

	plen=print_flash_webpage_only(webclient_htm, buf, plen);

	//Debug for at se hvor tæt vi er på grænsen (buffer size - lidt)
	printf_P(PSTR("Plen is %d \r\n"), plen);

	return(plen);
}

uint16_t print_settings_general_webpage(uint8_t *buf)
{
	uint16_t plen;

	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("<SCRIPT SRC='/loader.js'></SCRIPT><script>menu(2,1);formstart('/settings/general');"));

	char realpassword[11];
	eepromReadStr(200, realpassword, 10);
	sprintf_P(tempbuf, PSTR("dw('Password : ');tf('PASS', '%s', 10);dw('<br>');"), realpassword);
	plen=fill_tcp_data(buf,plen,tempbuf);

	uint8_t hasLcd = eepromReadByte(50);
	sprintf_P(tempbuf, PSTR("dw('LCD : ');cb('LCD', '%u', 1);dw('<br>');"), hasLcd);
	plen=fill_tcp_data(buf,plen,tempbuf);
	
	/*
	60		Analog interval
	61		Digital interval
	62		Onewire interval
	63		DHT interval
	*/	
	
	uint8_t digital = eepromReadByte(60);
	sprintf_P(tempbuf, PSTR("dw('Digital update interval : ');tf('digital', '%u');dw('<br>');"), digital);
	plen=fill_tcp_data(buf,plen,tempbuf);
	
	uint8_t analog  = eepromReadByte(61);
	sprintf_P(tempbuf, PSTR("dw('Analog update interval : ');tf('analog', '%u');dw('<br>');"), analog);
	plen=fill_tcp_data(buf,plen,tempbuf);

	uint8_t onewire = eepromReadByte(62);
	sprintf_P(tempbuf, PSTR("dw('1-wire update interval : ');tf('onewire', '%u');dw('<br>');"), onewire);
	plen=fill_tcp_data(buf,plen,tempbuf);
	
	uint8_t dht = eepromReadByte(63);
	sprintf_P(tempbuf, PSTR("dw('DHT update interval : ');tf('dht', '%u');dw('<br>');"), dht);
	plen=fill_tcp_data(buf,plen,tempbuf);		

	sprintf_P(tempbuf, PSTR("formend();</script><br>"));
	plen=fill_tcp_data(buf,plen,tempbuf);

	//Debug for at se hvor tæt vi er på grænsen (buffer size - lidt)
	printf_P(PSTR("Plen is %d \r\n"), plen);

	return(plen);
}

uint16_t print_loader(uint8_t *buf)
{
	uint16_t plen;

	plen=http200ok();
	plen=fill_tcp_data_p(buf,plen,PSTR("function require(jspath) { document.write('<script type=\"text/javascript\" src=\"'+jspath+'\"></script>'); }"));
	plen=fill_tcp_data_p(buf,plen,PSTR("require(\"/fmu.js\");require(\"/alist.js\");require(\"/alarms.js\");require(\"/slist.js\");"));
	plen=fill_tcp_data_p(buf,plen,PSTR("document.write('<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\"/>');"));
	
	sprintf_P(tempbuf, PSTR("function menu(major,minor) {dw('<div id=\"header\">Stokerbot S3 - Firmware %u.%u<br><a href=\"/\">Home</a>&nbsp;&nbsp;&nbsp;');"), SBNG_VERSION_MAJOR,SBNG_VERSION_MINOR);
	plen=fill_tcp_data(buf,plen,tempbuf);
	
	plen=fill_tcp_data_p(buf,plen,PSTR("dw('<a href=\"/settings/general/\">Settings<a>&nbsp;&nbsp;&nbsp;<a href=\"/settings/net/\">Network</a>&nbsp;&nbsp;&nbsp;<a href=\"/settings/webclient/\">Uploader</a>');"));
	plen=fill_tcp_data_p(buf,plen,PSTR("dw('&nbsp;&nbsp;&nbsp;<a href=\"/settings/io/\">I/O</a>&nbsp;&nbsp;&nbsp;<a href=\"/settings/alarm/\">Alarms</a></div><br><center>');}"));	
	
printf_P(PSTR("Plen is %d \r\n"), plen);
	return(plen);
}

uint16_t print_settings_webpage(uint8_t *buf)
{
        uint16_t plen;

		plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<SCRIPT SRC='/loader.js'></SCRIPT><script>menu(2,1);formstart('/settings/netsave');"));
      
		sprintf_P(tempbuf, PSTR("dw('IP : ');tf('IP0', %d, 2);tf('IP1', %d, 2);tf('IP2', %d, 2);tf('IP3', %d, 2);dw('<br>');"), myip[0], myip[1], myip[2], myip[3]);
		plen=fill_tcp_data(buf,plen,tempbuf);

		sprintf_P(tempbuf, PSTR("dw('GW : ');tf('GW0', %d, 2);tf('GW1', %d, 2);tf('GW2', %d, 2);tf('GW3', %d, 2);dw('<br>');"), gwip[0], gwip[1], gwip[2], gwip[3]);
		plen=fill_tcp_data(buf,plen,tempbuf);

		sprintf_P(tempbuf, PSTR("dw('DNS: ');tf('DNS0', %d, 2);tf('DNS1', %d, 2);tf('DNS2', %d, 2);tf('DNS3', %d, 2);dw('<br>');"), dnsip[0], dnsip[1], dnsip[2], dnsip[3]);
		plen=fill_tcp_data(buf,plen,tempbuf);

		sprintf_P(tempbuf, PSTR("dw('MASK: ');tf('MASK0', %d, 2);tf('MASK1', %d, 2);tf('MASK2', %d, 2);tf('MASK3', %d, 2);dw('<br>');"), netmask[0], netmask[1], netmask[2], netmask[3]);
		plen=fill_tcp_data(buf,plen,tempbuf);
	
		uint8_t bcast = eepromReadByte(25);
		sprintf_P(tempbuf, PSTR("dw('<a href=\"http://wiki.stokerlog.dk/index.php/Broadcast\" target=\"_blank\">Disable broadcast: </a>');cb('bcast',%u,1);dw('<br>');"), bcast);
		plen=fill_tcp_data(buf,plen,tempbuf);
		
		uint8_t dhcp = eepromReadByte(10);
		sprintf_P(tempbuf, PSTR("dw('DHCP: </a>');cb('dhcp',%u,1);dw('<br>');"), dhcp);
		plen=fill_tcp_data(buf,plen,tempbuf);
		
		uint16_t webport = eepromReadWord(23);
		sprintf_P(tempbuf, PSTR("dw('Webport: ');tf('webport',%u,4);dw('<br>');"), webport);
		plen=fill_tcp_data(buf,plen,tempbuf);		

		sprintf_P(tempbuf, PSTR("formend();</script>"));
		plen=fill_tcp_data(buf,plen,tempbuf);

		//Debug for at se hvor tæt vi er på grænsen (buffer size - lidt)
		printf_P(PSTR("Plen is %d \r\n"), plen);

        return(plen);
}

uint16_t print_redirect(uint8_t *buf, char* path)
{
	uint16_t plen;

	plen=http200ok();
	sprintf_P(tempbuf, PSTR("<h2>Settings saved, restarting, please wait ...</h2><meta http-equiv='refresh' content='15;url=%s'>"), path);
	plen=fill_tcp_data(buf,plen,tempbuf);

	return(plen);
}

uint16_t print_settings_io_webpage(uint8_t *buf)
{
        uint16_t plen;

		plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<SCRIPT SRC='/loader.js'></SCRIPT><script>menu(2,1);formstart('/settings/io');tfh('io','1',0);dw('<table border=0><thead><tr><th>Pin</th><th>Type</th><th>Pullup?</th></tr></thead>');"));
    
	  	for (uint8_t i=0; i<8; i++)
		{
			uint8_t val = eepromReadByte(300 + i);
			sprintf_P(tempbuf, PSTR("adcset(%d, 'ADC%d', %d, 'PUA%d', %d);"),i,i,simpleSensorTypes[i],i,val);
			plen=fill_tcp_data(buf,plen,tempbuf);
		}

	  	for (uint8_t i=0; i<4; i++)
		{
			uint8_t val = eepromReadByte(310 + i);
			sprintf_P(tempbuf, PSTR("dport(%d, 'D%d', %d , 'PUD%d', %d);"),i,i,simpleSensorTypes[i+8],i,val);
			plen=fill_tcp_data(buf,plen,tempbuf);
		}

		plen=fill_tcp_data_p(buf,plen,PSTR("dw('</table>');formend();</script>"));

		//Debug for at se hvor tæt vi er på grænsen (buffer size - lidt)
		printf_P(PSTR("Plen is %d \r\n"), plen);

        return(plen);
}


uint16_t print_flash_webpage(bool gzip, const char * pos, uint8_t *buf)
{
        uint16_t plen;

		if (gzip) 
			plen=http200gzok();
		else
			plen=http200ok();

		return print_flash_webpage_only(pos, buf, plen);
}

uint16_t print_flash_webpage_only(const char * pos, uint8_t* buf, uint16_t plen)
{
	uint8_t c;
	
		while (true)
		{
			if (
			(c = pgm_read_byte(pos)) == '%' &&
			pgm_read_byte(pos+1) == 'E' &&
			pgm_read_byte(pos+2) == 'N' &&
			pgm_read_byte(pos+3) == 'D'
			)
			{
				break;
			} else {
				plen = fill_tcp_data_len(buf, plen, &c, 1);
				pos++;
			}
		}

		return(plen);	
}

bool user_is_auth(char* buffer)
{
	char prev;
	char* pos = strstr_P(buffer, PSTR("Authorization"));
	pos+= 21; //Authorization: Basic
	if (pos != NULL)
	{
		char* ptr2 = (char*)pos;
		while (*ptr2 != '\r' && *ptr2 != '\n') ptr2++;
		prev=*ptr2;
		*ptr2 = '\0';
	
		memset(tempbuf, '\0', 25);
		base64dec(tempbuf, pos, 1);
		*ptr2 = prev;
		
		char* password = strstr(tempbuf, ":");

		if (password != NULL)
		{
			password++;

			char realpassword[11];
			eepromReadStr(200, realpassword, 10);

			if (strcmp(password, realpassword)==0)
			{
				return true;
			}
		}

		return false;
	}
	return false;
}

//Bliver kaldt når vi er forbundet til serveren og skal sende GET url...
uint16_t fill_custom_client_data(uint8_t *bufptr,uint16_t len)
{
	if (httpType != 1)
		return len;

#if SBNG_TARGET == 50
		sprintf_P(tempbuf, PSTR("&ALARM=%u"), alarmTimeout);
		len = fill_tcp_data(bufptr,len,tempbuf);
#endif

	for (uint8_t i=0; i<8; i++)
	{
		sprintf_P(tempbuf, PSTR("&ADC%u=%lu"), i, simpleSensorValues[i]);
		len = fill_tcp_data(bufptr,len,tempbuf);
	}

	for (uint8_t i=0; i<4; i++)
	{
		sprintf_P(tempbuf, PSTR("&D%u=%lu"), i, simpleSensorValues[i+8]);
		len = fill_tcp_data(bufptr,len,tempbuf);
	}

#if SBNG_TARGET == 50
	for (uint8_t i=20; i<24; i++)
	{
		sprintf_P(tempbuf, PSTR("&O%u=%lu"), i, simpleSensorValues[i]);
		len = fill_tcp_data(bufptr,len,tempbuf);
	}

	for (uint8_t i=24; i<28; i++)
	{
		sprintf_P(tempbuf, PSTR("&R%u=%lu"), i, simpleSensorValues[i]);
		len = fill_tcp_data(bufptr,len,tempbuf);
	}
#endif

    for (uint8_t i=0; i<MAXSENSORS; i++)
    {  
      if (sensorValues[(i*SENSORSIZE)+FAMILY] != 0)
      {
        int frac = sensorValues[(i*SENSORSIZE)+VALUE2]*DS18X20_FRACCONV;  //Ganger de sidste par bits, med det step DS18B20 bruger
    
        sprintf_P(tempbuf, PSTR("&%02X%02X%02X%02X%02X%02X%02X%02X=%c%d.%04d"),
              sensorValues[(i*SENSORSIZE)+FAMILY],
              sensorValues[(i*SENSORSIZE)+ID1],
              sensorValues[(i*SENSORSIZE)+ID2],
              sensorValues[(i*SENSORSIZE)+ID3],
              sensorValues[(i*SENSORSIZE)+ID4],
              sensorValues[(i*SENSORSIZE)+ID5],
              sensorValues[(i*SENSORSIZE)+ID6],
              sensorValues[(i*SENSORSIZE)+CRC],
              sensorValues[(i*SENSORSIZE)+SIGN],
              sensorValues[(i*SENSORSIZE)+VALUE1],
              frac
            );      

            len = fill_tcp_data(bufptr,len,tempbuf);
      }
	}


	for (uint8_t i=0; i<4; i++)
	{
		if (simpleSensorTypes[i+8] == TYPE_DHT22)
		{
			float temperature=0;
			float humidity=0;
			if(simpleSensorValues[50] & 0x8000) {
				temperature = (float)((simpleSensorValues[50+(i*2)] & 0x7FFF) / 10.0) * -1.0;
				} else {
				temperature = (float)(simpleSensorValues[50+(i*2)])/10.0;
			}
			humidity = (float)(simpleSensorValues[50+(i*2)+1])/10.0;
			sprintf_P(tempbuf, PSTR("&D%dT=%g&D%dH=%g"),i,temperature,i,humidity);
			len = fill_tcp_data(bufptr,len,tempbuf);
		}
		if (simpleSensorTypes[i+8] == TYPE_DHT11)
		{
			float temperature=simpleSensorValues[50+(i*2)];
			float humidity=simpleSensorValues[50+(i*2)+1];
			sprintf_P(tempbuf, PSTR("&D%dT=%g&D%dH=%g"),i,temperature,i,humidity);
			len = fill_tcp_data(bufptr,len,tempbuf);
		}		
	}
		

	return len;
}

void arpresolver_result_callback(uint8_t *ip,uint8_t transaction_number,uint8_t *mac)
{
	printf("Got ARP reply for %u.%u.%u.%u (%u:%u:%u:%u:%u:%u) \r\n", ip[0],ip[1],ip[2],ip[3], mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	uint8_t i=0;
	if (transaction_number==TRANS_NUM_GWMAC){
		// copy mac address over:
		while(i<6){gwmac[i]=mac[i];i++;}
	}
	if (transaction_number==TRANS_NUM_WWWMAC){
		// copy mac address over:
		while(i<6){wwwmac[i]=mac[i];i++;}
	}	
}

void handle_net(void)
{
	uint16_t dat_p,plen;

    // handle ping and wait for a tcp packet
    plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
	dat_p=packetloop_arp_icmp_tcp(buf,plen);

    if (gw_arp_state==0){
		printf("Requested ARP for gateway IP %u.%u.%u.%u\r\n",gwip[0],gwip[1],gwip[2],gwip[3]);
	    get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
	    gw_arp_state=1;
    }

    if (get_mac_with_arp_wait()==0 && gw_arp_state==1){
	    // done we have the mac address of the GW
	    gw_arp_state=2;
    }


    if(plen==0 && gw_arp_state==2){
            //Hvis der ikke er nogle indgående pakker, køres en tur i web client state machine, hvor dns lookup mv. håndteres		

            if (dns_state==0){
					if (web_client_monitor >= 60)
					{
						printf("Reset due to missing webclient");
						while(true);
					}

					web_client_monitor++;
					eepromReadStr(1000, host, 99);
					eepromReadStr(1100, url, 99);

					if (parse_ip(wwwip,host)!=0)
					{
						printf_P(PSTR("Request DNS lookup for %s\r\n"),host);
						dnsTimer=tickS;
						dns_state=1;
						dnslkup_request(buf,host,gwmac);
					} else {
						printf_P(PSTR("WWW host is an IP : %u.%u.%u.%u\r\n"),wwwip[0],wwwip[1],wwwip[2],wwwip[3]);
						if (route_via_gw(wwwip)==0)
						{
							printf("IP is local, requesting mac address \r\n");
							get_mac_with_arp(wwwip,TRANS_NUM_WWWMAC,&arpresolver_result_callback);
						}						
						dns_state=2;
					}
                    return;
            }

            if (dns_state==1 && dnslkup_haveanswer()){
					dnslkup_get_ip(wwwip);
					printf_P(PSTR("Got DNS reply : %d.%d.%d.%d \r\n"),wwwip[0],wwwip[1],wwwip[2],wwwip[3]);
                    dns_state=2;
					if (route_via_gw(wwwip)==0)
					{
						printf("DNS resolved to local IP, requesting mac address \r\n");
						get_mac_with_arp(wwwip,TRANS_NUM_WWWMAC,&arpresolver_result_callback);						
					}
            }

            if (dns_state!=2){
                    // retry every minute if dns-lookup failed:
                    if (tickDiffS(dnsTimer) > 20){
						printf_P(PSTR("DNS lookup failed, using fallback IP ... \r\n"));
                            dns_state=2;
                    }
                    // don't try to use web client before
                    // we have a result of dns-lookup
                    return;
            }

            //----------
            if (start_web_client==1){
				if (sendBoxData == 0)
				{
					eepromReadStr(1000, host, 99);				
					start_web_client=2;
					httpType = 2;
					
					//Add the static ID to the url, rest of data is added in callback
					sprintf_P(url, PSTR("/boxdata.php?id=%02X%02X%02X%02X%02X%02X%02X%02X&ip=%u.%u.%u.%u"), systemID[0], systemID[1], systemID[2], systemID[3], systemID[4], systemID[5], systemID[6], systemID[7], myip[0], myip[1], myip[2], myip[3]);
					//void client_browse_url(const char *urlbuf_p,char *urlbuf_varpart,const char *hoststr,void (*callback)(uint16_t,uint16_t,uint16_t),uint8_t *dstip,uint8_t *dstmac)
					if (route_via_gw(wwwip)==0) //LAN
						client_browse_url(url,NULL,host,&boxcallback,wwwip,wwwmac);
					else //WAN
						client_browse_url(url,NULL,host,&boxcallback,wwwip,gwmac);					
					sendBoxData=30;
				} else {
					sendBoxData--;
					printf_P(PSTR("Starting webclient request \r\n"));
						eepromReadStr(1000, host, 99);
						eepromReadStr(1100, url, 99);				
					
						dnsTimer=tickS;
						start_web_client=2;
						httpType = 1;
						web_client_attempts++;
						web_client_monitor++;
						if (web_client_monitor >= 5)
						{
							printf("Reset due to missing webclient");
							while(true);
						}
	
				//Add the static ID to the url, rest of data is added in callback				
				sprintf_P(tempbuf, PSTR("&id=%02X%02X%02X%02X%02X%02X%02X%02X"), systemID[0], systemID[1], systemID[2], systemID[3], systemID[4], systemID[5], systemID[6], systemID[7]);
				strcat(url, tempbuf);
				//void client_browse_url(const char *urlbuf_p,char *urlbuf_varpart,const char *hoststr,void (*callback)(uint16_t,uint16_t,uint16_t),uint8_t *dstip,uint8_t *dstmac)
				if (route_via_gw(wwwip)==0) //LAN
					client_browse_url(url,NULL,host,&browserresult_callback,wwwip,wwwmac);
				else //WAN
					client_browse_url(url,NULL,host,&browserresult_callback,wwwip,gwmac);
			}
	} else if (start_web_client == 10)
	{
		dnsTimer=tickS;
        start_web_client=2;
		httpType = 3;

		printf_P(PSTR("Starting lcd request \r\n"));
		char *ptr = (char*)tempbuf;
		sprintf_P(ptr, PSTR("?id=%02X%02X%02X%02X%02X%02X%02X%02X"), systemID[0], systemID[1], systemID[2], systemID[3], systemID[4], systemID[5], systemID[6], systemID[7]);
		ptr += strlen(ptr);
		client_browse_url("/getLCD.php",tempbuf,host,&lcd_callback,wwwip,gwmac);
	}

    	// reset after a delay to prevent permanent bouncing
        if (tickDiffS(dnsTimer)>10 && start_web_client==2){
        	start_web_client=0;
        }
        return;
    }
    if(dat_p==0){ // plen!=0
            // check for incomming messages not processed
            // as part of packetloop_icmp_tcp, e.g udp messages
            udp_client_check_for_dns_answer(buf,plen);
            return;
    }

    if (strncmp("POST /API",(char *)&(buf[dat_p]),9)==0){
		
		printf("POST /API :: \r\n *** \r\n%s\r\n***\r\n", &(buf[dat_p]));
			
		if (!user_is_auth((char *)&(buf[dat_p+5])))
		{
			dat_p=https401();
			goto SENDTCP;
		}
		
		char cgival[5];
		uint8_t pin,val,length;

		char* datapos = strstr((char *)&(buf[dat_p]), "\r\n\r\n");
		datapos += 3;
		*datapos ='?';
		//printf("Data : %s \r\n", datapos);
	
		length = find_key_val(datapos,cgival,3,"pin");
		if (length > 0 )
		{	
			pin = atoi(cgival);
			length = find_key_val(datapos,cgival,4,"val");
			if (length > 0 )
			{
				val = atoi(cgival);
		
				printf("API %u = %u\r\n", pin, val);
				
				if (pin >= 1 && pin <= 4 && (val == 0 || val == 1))
				{
					if (val == 1)
						SETBIT(PORTC, (pin+1));
					else
						CLEARBIT(PORTC, (pin+1));
				}
		
				dat_p=fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
				goto SENDTCP;
			}
		}
    }
            
    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
	    dat_p=fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
	    goto SENDTCP;
    }
	
    if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){	
            dat_p=print_webpage(buf,false);
            goto SENDTCP;
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/index.ajax"),11)==0){
            dat_p=print_webpage(buf,true);
            goto SENDTCP;		
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/general"),17)==0){	
			if (!user_is_auth((char *)&(buf[dat_p+4])))
			{
				dat_p=https401();
          		goto SENDTCP;
			}

			char cgival[11];
			uint8_t length = 0;
			length = find_key_val((char *)&(buf[dat_p+4]),cgival, 10,"PASS");
			//Changed from a loop here i beta2, revert if it fails -JKN
			if (length > 0 && length <= 10)
			{
				eepromSaveStr(200, cgival, length);
				save_checkbox_cgivalue((char*)&(buf[dat_p+4]), "LCD", 50);
				save_cgivalue_if_found((char*)&(buf[dat_p+4]), "digitlal", 60);
				save_cgivalue_if_found((char*)&(buf[dat_p+4]), "analog", 61);
				save_cgivalue_if_found((char*)&(buf[dat_p+4]), "onewire", 62);
				save_cgivalue_if_found((char*)&(buf[dat_p+4]), "dht", 63);
				reScheduleTasks();
			}

            dat_p=print_settings_general_webpage(buf);

          	goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/io"),12)==0){
			if (!user_is_auth((char *)&(buf[dat_p+4])))
			{
				dat_p=https401();
          		goto SENDTCP;
			}
			char cgival[3];
			if (find_key_val((char *)&(buf[dat_p+4]),cgival, 3,"io") > 0)
			{
				printf("IO SAVE\r\n");
				for (uint8_t i=0; i<8; i++)
				{
					sprintf_P(tempbuf, PSTR("ADC%u"), i);
					save_cgivalue_if_found((char*)&(buf[dat_p+4]), tempbuf, 100+i);
					sprintf_P(tempbuf, PSTR("PUA%u"), i);
					save_checkbox_cgivalue((char*)&(buf[dat_p+4]), tempbuf, 300+i);
				}

				for (uint8_t i=0; i<4; i++)
				{
					sprintf_P(tempbuf, PSTR("D%u"), i);
					save_cgivalue_if_found((char*)&(buf[dat_p+4]), tempbuf, 110+i);
					sprintf_P(tempbuf, PSTR("PUD%u"), i);
					save_checkbox_cgivalue((char*)&(buf[dat_p+4]), tempbuf, 310+i);					
				}

		        dat_p=print_redirect(buf, "/settings/io");
	            goto SENDTCPANDDIE;
			}

            dat_p=print_settings_io_webpage(buf);

          	goto SENDTCP;
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/netsave"),17)==0){	
			if (!user_is_auth((char *)&(buf[dat_p+4])))
			{
				dat_p=https401();
          		goto SENDTCP;
			}

			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "IP0", 11);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "IP1", 12);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "IP2", 13);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "IP3", 14);

			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "GW0", 15);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "GW1", 16);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "GW2", 17);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "GW3", 18);

			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "MASK0", 19);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "MASK1", 20);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "MASK2", 21);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "MASK3", 22);

			//23-28 MAC which is assigned dynamicly from DS2401... so not used

			word_save_cgivalue_if_found((char*)&(buf[dat_p+4]), "webport", 23);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "bcast", 25);
			save_checkbox_cgivalue((char*)&(buf[dat_p+4]), "dhcp", 10);
			
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "DNS0", 31);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "DNS1", 32);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "DNS2", 33);
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "DNS3", 34);

		    dat_p=print_redirect(buf, "/settings/net");
		    goto SENDTCPANDDIE;

		} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/net"),13)==0){
		if (!user_is_auth((char *)&(buf[dat_p+4])))
		{
			dat_p=https401();
			goto SENDTCP;
		}

		dat_p=print_settings_webpage(buf);

		goto SENDTCP;			  
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/alarm"),15)==0){	
			if (!user_is_auth((char *)&(buf[dat_p+4])))
			{
				dat_p=https401();
          		goto SENDTCP;
			}

			dat_p=print_flash_webpage(true, alarms_htm, buf);

          	goto SENDTCP;
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/savealarm"),19)==0) {
			char cgival[3];
			find_key_val((char *)&(buf[dat_p+4]),cgival,2,"alarmid");
			uint8_t i = atoi(cgival);
			i--; //Webpage shows from 1..8
				
			printf("Save alarm %u \r\n", i);

				//source = 401..408
				uint8_t length = find_key_val((char *)&(buf[dat_p+4]),cgival, 3,"S");
				uint8_t sensorid = atoi(cgival);
				uint8_t loc = 0;

				if (length > 0 && sensorid < MAXSENSORS)
				{
					loc = (15*i)+1;

					for (uint8_t pos=0; pos<8; pos++)
					{
						eepromWriteByte(400+loc, sensorValues[(SENSORSIZE * sensorid) + pos]);
						loc++;
					}
				}

				printf("Source %s = %u \r\n", cgival, loc);

				if (loc > 0)
				{
					save_cgivalue_if_found((char*)&(buf[dat_p+4]), "T", 400+(15*i)+9);
					signed_save_cgivalue_if_found((char*)&(buf[dat_p+4]), "V", 400+(15*i)+10);
					save_cgivalue_if_found((char*)&(buf[dat_p+4]), "D", 400+(15*i)+11);
					save_cgivalue_if_found((char*)&(buf[dat_p+4]), "P", 400+(15*i)+12);
					save_checkbox_cgivalue((char*)&(buf[dat_p+4]), "R", 400+(15*i)+13);
					save_checkbox_cgivalue((char*)&(buf[dat_p+4]), "E", 400+(15*i)+0);
					dat_p=http200ok();
					dat_p=fill_tcp_data_p(buf,dat_p,PSTR("SAVED"));
					printf("OK");
				} else {
					dat_p=http400();
					printf("ERR");
				}

				goto SENDTCP;			  
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/settings/webclient"), 19)==0){
		if (!user_is_auth((char *)&(buf[dat_p+4])))
		{
			dat_p=https401();
			goto SENDTCP;
		}
			char cgival[100];
		
			save_cgivalue_if_found((char*)&(buf[dat_p+4]), "interval", 1200);
		
			uint8_t length = find_key_val((char *)&(buf[dat_p+4]),cgival, 99,"host");
			if (length > 0 && length <= 99)
			{
				save_checkbox_cgivalue((char*)&(buf[dat_p+4]), "coap", 1500); //Placed here to make sure its not set to 0 when first requesting the page
				urldecode(cgival);
				cgival[strlen(cgival)] = '\0';
				eepromSaveStr(1000, cgival, strlen(cgival)+1);
				dns_state = 0;
				printf("Host changed to %s ", cgival);
			}

			length = find_key_val((char *)&(buf[dat_p+4]),cgival, 99,"url");
			if (length > 0 && length <= 99)
			{
				eepromSaveByte(1100, '/');
				urldecode(cgival);
				cgival[strlen(cgival)] = '\0';
				eepromSaveStr(1101, cgival, strlen(cgival)+1);
				printf("URL changed to %s ", cgival);
			}
			
			coapclient = eepromReadByte(1500);
			dat_p=print_webclient_webpage(buf);
			goto SENDTCP;			  
	} else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/fmu.js"), 7)==0){	
			dat_p=print_flash_webpage(true, fmu_js, buf);

          	goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/alarms.js"),10)==0){	
            dat_p=print_flash_webpage(true, alarms_js, buf);

          	goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/loader.js"),10)==0){	
            dat_p=print_loader(buf);
          	goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/slist.js"),9)==0){	
            dat_p=print_sensorlist(buf);

          	goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/alist.js"),9)==0){	
            dat_p=print_alarmlist(buf);

          	goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/style.css"),10)==0){	
            dat_p=print_flash_webpage(true, style_css, buf);
          	goto SENDTCP;		
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/export.htm"),11)==0){
		dat_p=print_export_htm(buf);
		goto SENDTCP;
    } else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/export.json"),12)==0){
		dat_p=print_export_json(buf);
		goto SENDTCP;
    }
	 else if (strncmp_P((char *)&(buf[dat_p+4]),PSTR("/microajax.js"),11)==0){
		dat_p=print_flash_webpage(true, microajax_minified_js, buf);
		goto SENDTCP;	
    } else{
        dat_p=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
        goto SENDTCP;
    }
    //

	SENDTCPANDDIE:
	timedSaveEeprom();
	www_server_reply(buf,dat_p); // send data
	while(true);

	SENDTCP:
    www_server_reply(buf,dat_p); // send data
}

bool save_checkbox_cgivalue(char* buffer, char* name, uint16_t eeprom_location)
{
	char cgival[11];
	uint8_t length = 0;
	length = find_key_val(buffer,cgival, 10,name);
	uint8_t value = 0;

	if (length > 0)
	{
		cgival[length+1] = '\0';
		value = atoi(cgival);
		eepromWriteByte(eeprom_location, value);
		printf("Write %u at %u \r\n", value, eeprom_location);
		return true;
	} else { 
		printf("Write 0 at %u \r\n", eeprom_location);
		eepromWriteByte(eeprom_location, 0);
		return false;
	}
}

bool word_save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location)
{
	char cgival[11];
	uint8_t length = 0;
	length = find_key_val(buffer,cgival, 10,name);
	uint16_t value = 0;

	if (length > 0)
	{
		cgival[length+1] = '\0';
		value = atoi(cgival);
		eepromWriteWord(eeprom_location, value);
		return true;
	}

	return false;
}

bool save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location)
{
	char cgival[11];
	uint8_t length = 0;
	length = find_key_val(buffer,cgival, 10,name);
	uint8_t value = 0;

	if (length > 0)
	{
		cgival[length+1] = '\0';
		value = atoi(cgival);
		eepromWriteByte(eeprom_location, value);
		return true;
	}

	return false;
}

bool signed_save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location)
{
	char cgival[11];
	uint8_t length = 0;
	length = find_key_val(buffer,cgival, 10,name);
	int8_t value = 0;

	if (length > 0)
	{
		cgival[length+1] = '\0';
		value = atoi(cgival);
		eepromWriteByte(eeprom_location, value);
		return true;
	}

	return false;
}

void lcd_callback(uint16_t statuscode,uint16_t datapos, uint16_t len){
		printf_P(PSTR("got lcd reply ...\r\n"));
        //datapos=0; // supress warning about unused paramter
		start_web_client=0;
        if (statuscode==0){
                //len=0; // avoid warning about unused variable
				char* buffer = (char *)&(buf[datapos]);
				char* datapos = strstr(buffer, "\r\n\r\n");
				datapos += 4;
				printf("Data : %s \r\n", datapos);

				while (datapos != NULL)
				{
					if (datapos[0] == '\n')
						datapos++;

					char crc[6];
					strncpy(crc, datapos+86, 5);
					crc[5] = '\0';
					printf("CRC IS %s \r\n", crc);

					uint16_t crci = atoi(crc);
					uint16_t crcc = 0;
					for (uint8_t i=0; i<84; i++)
					{
						crcc += datapos[i+1];
					}
					printf("Calculated crc is %u \r\n", crcc);
					if (crcc == crci)
					{
						uint8_t screenno = datapos[0] - 48;
						printf("LCD crc for screen %c (%u) is valid \r\n", datapos[0], screenno);
						

						char stimeout[5];
						strncpy(stimeout, (datapos+1), 4);
						stimeout[4] = '\0';
						printf("Timeout is %s \r\n", stimeout);
						lcd_timeouts[screenno] = atoi(stimeout);
						printf("Set timeout for %u to %u \r\n", screenno, lcd_timeouts[screenno]);

						uint16_t linepos = 5;
						for (uint8_t lineno=0; lineno<4; lineno++)
						{
							for (uint8_t c=0; c<20; c++)
							{
								lines[screenno][lineno][c] = datapos[linepos++];
							}
							lines[screenno][lineno][20] = '\0';
						}

						if (lcd_current_screen == 254)	//waiting for data
							lcd_current_screen = 250;
					}

					datapos++;
					datapos = strstr(datapos, "\n");
					printf("Strlen is %u \r\n", strlen(datapos));
					if (strlen(datapos) < 80) break;
				}
		}
}

void boxcallback(uint16_t statuscode,uint16_t datapos, uint16_t len) {
	
}

//Her skal vi håndtere svaret fra stokerlog, der kan komme SET PIN STATE, LCD LINE TEXT, TIME DDMMYY HHMMSS
void browserresult_callback(uint16_t statuscode,uint16_t datapos, uint16_t len){
		printf_P(PSTR("got reply %u ...\r\n"),statuscode);
		
        //datapos=0; // supress warning about unused paramter
		start_web_client=0;
        if (statuscode==200){
                //len=0; // avoid warning about unused variable
				printf("Datapos %u len %u \r\n", datapos, len);
				char* buffer = (char *)&(buf[datapos]);
				printf("%s \r\n\r\nEND\r\n", buffer);
				char* datapos = strstr(buffer, "\r\n\r\n");
				datapos += 4;
				printf("Data : %s \r\n", datapos);

/*
HTTP/1.0 200 OK
Date: Thu, 05 May 2011 18:04:16 GMT
Server: Apache
Connection: close
Content-Type: text/html; charset=utf-8

EOM 

END
Data : EOM 
*/
//For each line, parse contents

//This is a safety measure, to prevent the bot from running a bad event when rebooted, events time out after 5minutes
//if (uptime > 300) {
			char* line = strstr(buffer, "RIO");
			while (line != NULL)
			{
				unsigned int id, pin, state, delay;
				printf("RAW %s \r\n", line);
				sscanf(line, "RIO %u %u %u %u", &id, &pin, &state, &delay);
				printf("Found RIO line id:%u pin:%u state:%u delay:%u \r\n",id,pin,state,delay);
				printf("Last event is %lu \r\n", last_rio_event);
				if (id <= last_rio_event)
				{
					printf("Repeated RIO event detected \r\n");
				} else {
					#if SBNG_TARGET == 50
					if (pin > 20)
					{
						simpleSensorValues[pin] = state;
					} else {
					#endif
						if (state == 1)
							SETBIT(PORTC, (2+pin));
						else
							CLEARBIT(PORTC, (2+pin));
					#if SBNG_TARGET == 50
					}
					#endif
					if (delay != 0)
						addTimedEvent(id, pin, state, delay);

					last_rio_event=id;
				}

				line = strstr(line, "\n");
				line = strstr(line, "RIO");
			}
//}

                web_client_sendok++;
				web_client_monitor=0;
        }
}

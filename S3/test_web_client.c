/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * Chip type           : Atmega168 or Atmega328 with ENC28J60
 *********************************************/
#include "config.h"
#include "AVR035.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"
#include "enc28j60.h"
#include "timeout.h"
#include "net.h"
#include "dnslkup.h"
#include "dht.h"

#include "web.h"

#include "usart.h"

#include "onewire.h"
#include "ds18x20.h"

#include "analog.h"

#include "eeprom.h"
#include "base64_dec.h"
#include "timer.h"

#include "SPILCD.h"
#include "I2CEEPROM.h"
#include "twi.h"

#include "Queue.h"

#if SBNG_TARGET == 50
	#include "MCP23008.h"
#endif


void loadSimpleSensorData(void);
static void updateSimpleSensors(void);
void updateCounters(void);
static void timedSaveEeprom(void);
void timedAlarmCheck(void);
bool save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location);
void getDHTData();
void updateLCD();
void sendData();
void remoteIO();
void ARPbroadcast();

static uint8_t mymac[6] = {0x56,0x55,0x58,0x10,0x00,0x29};

uint8_t gwip[4] = {192,168,1,1};
uint8_t myip[4] = {192,168,1,65};
uint8_t netmask[]={255,255,255,0};
uint8_t dnsip[]={8,8,8,8}; // the google public DNS, don't change unless there is a real need

uint8_t buf[BUFFER_SIZE+1];
	char printbuff[100];
extern uint8_t *sensorScan;

extern uint8_t start_web_client;
static uint8_t hasLcd;

bool DS18B20Conv = false;

uint8_t systemID[8];
bool lcd_start_update = false;

#if SBNG_TARGET == 50
	bool alarmDetected = false;
	uint8_t alarmTimeout  = 0;
#endif

static void mywdt_reset()
{
	#if SBNG_TARGET == 50
		FLIPBIT(PORTB, 2);
	#endif
	wdt_reset();
}

static void mywdt_init()
{
	#if SBNG_TARGET == 50
		SETBIT(DDRB, 2); //hardware watchdog
		FLIPBIT(PORTB, 2);
		_delay_ms(1);
		FLIPBIT(PORTB, 2);	
	#else
		wdt_disable();
		wdt_reset();
		wdt_enable(WDTO_4S); 
		wdt_reset();
	#endif
	
	wdt_reset();
}

static void mywdt_sleep(uint16_t ms)
{
	while (ms > 100)
	{
		ms -= 100;
		mywdt_reset();
		_delay_ms(100);
	}
	while (ms > 0)
	{
		_delay_ms(1);
		ms--;
	}
	mywdt_reset();
}

void read_ip_addresses(void)
{
	myip[0] = eepromReadByte(11);
	myip[1] = eepromReadByte(12);
	myip[2] = eepromReadByte(13);
	myip[3] = eepromReadByte(14);

	gwip[0] = eepromReadByte(15);
	gwip[1] = eepromReadByte(16);
	gwip[2] = eepromReadByte(17);
	gwip[3] = eepromReadByte(18);

	netmask[0] = eepromReadByte(19);
	netmask[1] = eepromReadByte(20);
	netmask[2] = eepromReadByte(21);
	netmask[3] = eepromReadByte(22);

	dnsip[0] = eepromReadByte(31);
	dnsip[1] = eepromReadByte(32);
	dnsip[2] = eepromReadByte(33);
	dnsip[3] = eepromReadByte(34);
}


void save_ip_addresses(void)
{
	eepromWriteByte(11, myip[0]);
	eepromWriteByte(12, myip[1]);
	eepromWriteByte(13, myip[2]);
	eepromWriteByte(14, myip[3]);

	eepromWriteByte(15, gwip[0]);
	eepromWriteByte(16, gwip[1]);
	eepromWriteByte(17, gwip[2]);
	eepromWriteByte(18, gwip[3]);

	eepromWriteByte(19, netmask[0]);
	eepromWriteByte(20, netmask[1]);
	eepromWriteByte(21, netmask[2]);
	eepromWriteByte(22, netmask[3]);

	eepromWriteByte(31, dnsip[0]);
	eepromWriteByte(32, dnsip[1]);
	eepromWriteByte(33, dnsip[2]);
	eepromWriteByte(34, dnsip[3]);
}

void loadSimpleSensorData()    
{
	for (uint16_t i=0; i<MAXSENSORS*SENSORSIZE; i++)
		sensorValues[i] = 0;

	for (uint16_t i=0; i<MAXSENSORS*8; i++)
		sensorScan[i] = 0;

	//To load initial type values from eeprom
	for (int i=0; i<=7; i++)
	{
		uint8_t pos = 100+i;
		simpleSensorTypes[i] = eepromReadByte(pos);
		simpleSensorDebounce[i] = 0;

		if (simpleSensorTypes[i] == 3) //counter
		{
			uint8_t eepos = 140 + (i*2);
			simpleSensorValues[i] = eepromReadWord(eepos);
		} else {
			simpleSensorValues[i] = 0;
		}
	}

	//To load initial type values from eeprom
	for (int i=0; i<=3; i++)
	{
		uint8_t pos = 110+i;
		simpleSensorTypes[i+8] = eepromReadByte(pos);
		simpleSensorDebounce[i+8] = 0;

		if (simpleSensorTypes[i+8] == 3) //counter
		{
			uint8_t eepos = 160 + (i*2);
			simpleSensorValues[i+8] = eepromReadWord(eepos);
		} else {
			simpleSensorValues[i+8] = 0;
		}
	}
}

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);   

int main(void){      
	mywdt_init();
	mywdt_sleep(100);

#if SBNG_TARGET == 50
	SETBIT(PORTB, 3); //not selected
	SETBIT(DDRB, 3); //SD card CS
	
	CLEARBIT(DDRC, 6); //Alarm indgang
	CLEARBIT(PORTC, 7); //Lav
	SETBIT(DDRC, 7); //Alarm udgang
#endif

		//usart_init(9600);
		usart_init(250000);
		stdout = &uart_str;
		stderr = &uart_str;
		stdin  = &uart_str;
#if SBNG_TARGET == 50
		printf_P(PSTR("tolog.dk - Firmware %u.%u booting\r\n"),SBNG_VERSION_MAJOR,SBNG_VERSION_MINOR);
#else
		printf_P(PSTR("Stokerbot S3 - Firmware %u.%u booting\r\n"),SBNG_VERSION_MAJOR,SBNG_VERSION_MINOR);
#endif

 		mywdt_sleep(2500);

		spilcd_init();
		LCDclr();
		mywdt_sleep(100);

		hasLcd = eepromReadByte(50);
		if (hasLcd > 0 && hasLcd != 255)
		{
			LCDclr();
			LCDsetCursor(0, 0);
			#if SBNG_TARGET == 50
				LCDwrite(" tolog.dk v2.11 ", 0);
			#else
				LCDwrite(" Stokerbot S3S v2.11 ", 0);
			#endif
		}

		mywdt_reset();
		mywdt_sleep(2500);

 		sensorScan = (uint8_t*)&tempbuf;

		#if SBNG_TARGET == 50
			printf_P(PSTR("tolog.dk - Firmware %u.%u ready\r\n"),SBNG_VERSION_MAJOR,SBNG_VERSION_MINOR);
		#else
			printf_P(PSTR("Stokerbot S3 - Firmware %u.%u ready\r\n"),SBNG_VERSION_MAJOR,SBNG_VERSION_MINOR);
		#endif

	if (eepromReadByte(0) == 255 || eepromReadByte(11) == 255)
	{
		printf_P(PSTR("Setting default values\r\n"));
		//Set defaults
		eepromWriteByte(0, 0); //init

		myip[0] = 192;
		myip[1] = 168;
		myip[2] = 1;
		myip[3] = 47;

		netmask[0] = 255;
		netmask[1] = 255;
		netmask[2] = 255;
		netmask[3] = 0;

		gwip[0] = 192;
		gwip[1] = 168;
		gwip[2] = 1;
		gwip[3] = 1;

		dnsip[0] = 8;
		dnsip[1] = 8;
		dnsip[2] = 8;
		dnsip[3] = 8;

		eepromWriteWord(23, 80);  //web port
		eepromWriteByte(25, 0); //Done Disable Broadcast
		eepromWriteByte(10, 0);  //dhcp off
		eepromWriteByte(50, 0);  //no LCD

		save_ip_addresses();

		eepromWriteStr(200, "SBNG", 4);  //default password
		eepromWriteByte(204, '\0');
		eepromWriteByte(205, '\0');
		eepromWriteByte(206, '\0');
		eepromWriteByte(207, '\0');
		eepromWriteByte(208, '\0');
		eepromWriteByte(209, '\0');

		eepromWriteByte(100, 1); //Analog port 0 = ADC
		eepromWriteByte(101, 1); //Analog port 1 = ADC
		eepromWriteByte(102, 1); //Analog port 2 = ADC
		eepromWriteByte(103, 1); //Analog port 3 = ADC
		eepromWriteByte(104, 1); //Analog port 4 = ADC
		eepromWriteByte(105, 1); //Analog port 5 = ADC
		eepromWriteByte(106, 1); //Analog port 6 = ADC
		eepromWriteByte(107, 1); //Analog port 7 = ADC

		eepromWriteByte(110, 0); //Digital port 0 = OUT
		eepromWriteByte(111, 0); //Digital port 1 = OUT
		eepromWriteByte(112, 0); //Digital port 2 = OUT
		eepromWriteByte(113, 0); //Digital port 3 = OUT	

		for (uint8_t alarm=1; alarm<=NUMALARMS; alarm++)
		{
			uint16_t pos = 400 + ((alarm-1)*15); //400 415 430 445

			eepromWriteByte(pos+0, 0); //enabled
			eepromWriteByte(pos+1, 0); //sensorid
			eepromWriteByte(pos+2, 0); //sensorid
			eepromWriteByte(pos+3, 0); //sensorid
			eepromWriteByte(pos+4, 0); //sensorid
			eepromWriteByte(pos+5, 0); //sensorid
			eepromWriteByte(pos+6, 0); //sensorid
			eepromWriteByte(pos+7, 0); //sensorid
			eepromWriteByte(pos+8, 0); //sensorid
			eepromWriteByte(pos+9, '<'); //type
			eepromWriteByte(pos+10, 0); //value
			eepromWriteByte(pos+11, 0); //target
			eepromWriteByte(pos+12, 0); //state
			eepromWriteByte(pos+13, 0); //reverse
			eepromWriteByte(pos+14, 0); //not-used
		}
    	
		eepromWriteByte(1, EEPROM_VERSION);
		
		eepromWriteByte(1200, 3); //Interval 3=30sec
		eepromWriteStr(1000, "stokerlog.dk", 13); //host
		eepromWriteStr(1100, "/incoming.php?v=1", 18); //url
	}

	if (dnsip[0] == 255)
	{
		dnsip[0] = 8;
		dnsip[1] = 8;
		dnsip[2] = 8;
		dnsip[3] = 8;
		save_ip_addresses();
	}

	if (eepromReadByte(2) < 3)
	{
		eepromWriteByte(1, EEPROM_VERSION);	
		eepromWriteWord(29, 80);  //web port
		eepromWriteByte(25, 0); //Dont disable broadcast
		eepromWriteByte(1200, 3); //Interval 3=30sec
		eepromWriteStr(1000, "stokerlog.dk", 13); //host
		eepromWriteStr(1100, "/incoming.php?v=1", 18); //url	
	}

		read_ip_addresses();
		uint16_t webport = eepromReadWord(23);  //web port

		OW_selectPort(1);
        int nSensors = search_sensors(MAXSENSORS);  //Finder alle sensore (op til max)
		mywdt_reset();

		for ( int i=0; i<nSensors; i++ ) {
			if (sensorScan[i*OW_ROMCODE_SIZE+0] == 0x01)
			{
				for (uint8_t o=0; o<OW_ROMCODE_SIZE; o++)
				{
					systemID[o] = sensorScan[i*OW_ROMCODE_SIZE+o];
				}

				printf_P(PSTR("Found system id %02X%02X%02X%02X%02X%02X%02X%02X\r\n"),
				systemID[0],
				systemID[1],
				systemID[2],
				systemID[3],
				systemID[4],
				systemID[5],
				systemID[6],
				systemID[7]
				);

				//Example system id : 01 51 99 36 14 00 00 F5
			}
		}
		mywdt_reset();
	

	if (systemID[0] == 0)
	{
		printf_P(PSTR("No system id found, add a DS2401 or use old software ***"));

				systemID[0] = 21;
				systemID[1] = 22;
				systemID[2] = 23;
				systemID[3] = 24;
				systemID[4] = 25;
				systemID[5] = 26;
				systemID[6] = 27;
				systemID[7] = 28;
	}

	for (uint8_t o=1; o<=5; o++)
	{
		mymac[o] = systemID[o];
	}
	printf("IP: %u.%u.%u.%u\r\n", myip[0],myip[1],myip[2],myip[3]);
	printf("MAC: %02X%02X%02X%02X%02X%02X\r\n", mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);
	printf("Webport: %u\r\n", webport);
	

	if (hasLcd > 0 && hasLcd != 255)
	{
		LCDsetCursor(1, 0);
		char str[25];
		sprintf(str, "ID: %02X%02X%02X%02X%02X%02X%02X%02X",
				systemID[0],
				systemID[1],
				systemID[2],
				systemID[3],
				systemID[4],
				systemID[5],
				systemID[6],
				systemID[7]);

		LCDwrite(str, 0);

		LCDsetCursor(2, 0);
		sprintf(str, "IP: %u.%u.%u.%u", myip[0],myip[1],myip[2],myip[3]);
		LCDwrite(str, 0);
	}
		
		mywdt_reset();
		
		/*
		//Hard reset ENC.
		SETBIT(DDRB, 3); //set B3 as output
		CLEARBIT(PORTB, 3);
		_delay_ms(250);
		CLEARBIT(DDRB, 3); //set B3 as 3-state
		wdt_reset();
		_delay_ms(250);
*/

		mywdt_reset();

		bool bcast = false;
		if (eepromReadByte(25) == 1) bcast=true;
			
        enc28j60Init(mymac, bcast);
        enc28j60clkout(0);
        _delay_loop_1(0); // 60us

		mywdt_reset();

        initTimer();
	  	ADC_Init();
		twiInit(10); //200ms timeout
		loadSimpleSensorData();

		//Set digital pins based on selections...
		for (uint8_t i=8; i<=11; i++)
		{
			if (simpleSensorTypes[i] == 0)
			{
				//output
				SETBIT(DDRC, (i-6));
			} else {
				//input
				CLEARBIT(DDRC, (i-6));
			}
		}

       sei();

        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
       
        //init the web server ethernet/ip layer:
        init_ip_arp_udp_tcp(mymac,myip,webport);
        // init the web client:
        client_set_gwip(gwip);  // e.g internal IP of dsl router
		mywdt_reset();
		printf("ENC version %u\r\n",enc28j60getrev());
		mywdt_reset();
		initTimedEvents();

		mywdt_sleep(500);

		if (bcast)
		{
			while (!enc28j60linkup())
			{
				printf("Waiting for link \r\n");
				mywdt_sleep(100);
			}
			make_arp_broadcast(mymac, myip);
			scheduleFunction(ARPbroadcast, "ARPbroadcast", 5);
		}

		#if SBNG_TARGET == 50
			IOexpInit();
		#endif

		uint8_t interval = eepromReadByte(1200) * 10;

		//Queue.h handles slow tasks, each tick is 1second	
		scheduleFunction(updateSimpleSensors, "SimpleSensors", 2);
		scheduleFunction(updateOWSensors, "updateOWSensors", 2);
		scheduleFunction(timedAlarmCheck, "timedAlarmCheck", 5);
		scheduleFunction(getDHTData, "getDHTData", 5);
		scheduleFunction(remoteIO, "remoteIO", 1);
		scheduleFunction(sendData, "sendData", interval);
		scheduleFunction(updateLCD, "updateLCD", 1);
		scheduleFunction(timedSaveEeprom, "timedSaveEeprom", 1800);

		   uint16_t lastTick = tickS;
	       while(1)
		   {
			if (lastTick != tickS)
			{
				scheduleRun(tickS);
				lastTick = tickS;
			}
			handle_net();
			mywdt_reset();
           }
        return (0);
}

void ARPbroadcast()
{
	make_arp_broadcast(mymac, myip);	
}

void updateLCD()
{
	if (hasLcd > 0 && hasLcd != 255)
		lcd_update();
}

void sendData()
{
	uint16_t interval = eepromReadByte(1200) * 10;
	if (interval != 0)
	{
		printf("Starting webclient main\r\n");
		start_web_client = 1;
	}
	hasLcd = eepromReadByte(50);
	scheduleChangeFunction("sendData", tickS+interval, interval);
}

void remoteIO()
{
	#if SBNG_TARGET == 50
	if (alarmTimeout > 0)
	alarmTimeout--;
	#endif

	checkTimedEvents();	
}

void getDHTData()
{
	int16_t rawtemperature = 0;
	int16_t rawhumidity = 0;

	//Digital pins
	for (uint8_t i=0; i<=3; i++)
	{
		uint8_t eepos = 110 + i;
		simpleSensorTypes[i+8] = eepromReadByte(eepos);

		if (simpleSensorTypes[i+8] == TYPE_DHT11)
		{
			if(dht_getdata((i+2), &rawtemperature, &rawhumidity, false) == 0) {
				simpleSensorValues[50+(i*2)] = rawtemperature;
				simpleSensorValues[50+(i*2)+1] = rawhumidity;
			}
		}
		if (simpleSensorTypes[i+8] == TYPE_DHT22)
		{
			if(dht_getdata((i+2), &rawtemperature, &rawhumidity, true) == 0) {
				simpleSensorValues[50+(i*2)] = rawtemperature;
				simpleSensorValues[50+(i*2)+1] = rawhumidity;
			}
		}		
	}
}

void timedAlarmCheck(void)
{
	for (uint8_t alarm=1; alarm<=NUMALARMS; alarm++)
	{
		uint16_t pos = 400 + ((alarm-1)*15); //400 415 430 445

		if (eepromReadByte(pos+0) == 1)
		{
				uint8_t sensorpos = findSensor(
				eepromReadByte(pos+1), 
				eepromReadByte(pos+2), 
				eepromReadByte(pos+3), 
				eepromReadByte(pos+4), 
				eepromReadByte(pos+5), 
				eepromReadByte(pos+6), 
				eepromReadByte(pos+7), 
				eepromReadByte(pos+8));

				int8_t value = (int8_t)sensorValues[(sensorpos*SENSORSIZE)+VALUE1];
				char sign = sensorValues[(sensorpos*SENSORSIZE)+SIGN];				
				if (sign == '-') value *= -1;

				int8_t target = eepromReadByteSigned(pos+10);

				if (
					(eepromReadByte(pos+9) == 1 && value < target)
					||
					(eepromReadByte(pos+9) == 2 && value == target)
					||
					(eepromReadByte(pos+9) == 3 && value > target)
				)
				{
					//ALARM
					//DDR=in/out PIN=value/pullup PORT=state
					uint8_t pin = eepromReadByte(pos+11);

					if (pin >= 1 && pin <= 4)
					{
						if (eepromReadByte(pos+12) == 1)
						{
							SETBIT(PORTC, (1+pin));
						}
						else
							CLEARBIT(PORTC, (1+pin));
					}
					#if SBNG_TARGET == 50
					else if (pin >= 24 && pin <= 27) {
						//MCP driven relay
						simpleSensorValues[pin] = 1;
					}
					#endif
 				} else if (eepromReadByte(pos+13) == 1) {
 					//REVERSE
					uint8_t pin = eepromReadByte(pos+11);
					if (pin >= 1 && pin <= 4)
					{
						if (eepromReadByte(pos+12) == 1)
							CLEARBIT(PORTC, (1+pin));
						else
							SETBIT(PORTC, (1+pin));
					}
					#if SBNG_TARGET == 50
					else if (pin >= 24 && pin <= 27)
					{
						//MCP driven relay
						simpleSensorValues[pin] = 0;
					}
					#endif
 				}
		}
	}
}

//Called every 30minutes and only saved if theres any changes (100000 / 30minutes worst case = 5years)
static void timedSaveEeprom(void)
{
	for (uint8_t i=0; i<8; i++)
	{
		if (simpleSensorTypes[i] == 3) //counter
		{
			uint8_t pos = 110 + (i*2);
			//save tjekker selv om det er nødvendigt
			eepromSaveWord(pos, simpleSensorValues[i]);
		}
	}
}

//Called every 20ms
void updateCounters(void)
{
	#if SBNG_TARGET == 50
		if (GETBIT(PINC,6)>0)
		{
			if (alarmDetected == false)
			{
				alarmDetected = true;
				alarmTimeout = 60;		
			}
		} else {
			alarmDetected = false;
		}

		if (alarmTimeout > 0)
			SETBIT(PORTC, 7);
		else
			CLEARBIT(PORTC, 7);
	#endif

	for (uint8_t i=0; i<12; i++)
	{
		if (simpleSensorTypes[i] == 3)
		{
			//Check if pin is high
			if ( (i >= 8 && GETBIT(PINC,(i-6))>0) || (i < 8 && GETBIT(PINA,i)>0))
			{
				//and debounce is not set
				if (simpleSensorDebounce[i] == 0)
				{
					//Update counter
					simpleSensorValues[i]++;
					simpleSensorDebounce[i] = 1;
//					printf("Counter for %u incremented to %u\r\n",i,simpleSensorValues[i]);
				}
			} else {
				simpleSensorDebounce[i] = 0;
			}
		}
	}
}

static void updateSimpleSensors(void)
{
	//Send ADC+digital pins
	//ADC pins
	for (uint8_t i=0; i<=7; i++)
	{
		uint8_t eepos = 100 + i;
		simpleSensorTypes[i] = eepromReadByte(eepos);

		if (simpleSensorTypes[i] == TYPE_ADC)  //ADC
		{
			simpleSensorValues[i] = readOversampledAdc(i);
		}

		if (simpleSensorTypes[i] == TYPE_DIGIN)  //DIGITAL
		{
			if (GETBIT(PINA, i) > 0)
				simpleSensorValues[i] = 1;
			else
				simpleSensorValues[i] = 0;
		}
	}

	//Digital pins
	for (uint8_t i=0; i<=3; i++)
	{
		uint8_t eepos = 110 + i;
		simpleSensorTypes[i+8] = eepromReadByte(eepos);

		if (simpleSensorTypes[i+8] == TYPE_DIGIN || simpleSensorTypes[i+8] == TYPE_DIGOUT)  //DIGITAL
		{
		  	//Digital pins is C2-5
			if (GETBIT(PINC, (i+2)) > 0)
				simpleSensorValues[i+8] = 1;
			else
				simpleSensorValues[i+8] = 0;
		}
	}
	//3=COUNTER or DHT, not handled here, 0=digital out


	#if SBNG_TARGET == 50
		//Get data from MCP23008
		uint8_t mcp = IOexpReadInput();
		if (mcp != 0xFF)
		{
			if(mcp & 0x08) 
				simpleSensorValues[20] = 1;
			else
				simpleSensorValues[20] = 0;
			
			if(mcp & 0x04) 
				simpleSensorValues[21] = 1;
			else
				simpleSensorValues[21] = 0;
			
			if(mcp & 0x02) 
				simpleSensorValues[22] = 1;
			else
				simpleSensorValues[22] = 0;
			
			if(mcp & 0x01) 
				simpleSensorValues[23] = 1;
			else
				simpleSensorValues[23] = 0;
		}

		//set MCP outputs
		uint8_t mcpo = 0;
		if (simpleSensorValues[24] > 0)
			SETBIT(mcpo, 7);
		else
			CLEARBIT(mcpo, 7);
		
		if (simpleSensorValues[25] > 0)
			SETBIT(mcpo, 6);
		else
			CLEARBIT(mcpo, 6);
		
		if (simpleSensorValues[26] > 0)
			SETBIT(mcpo, 5);
		else
			CLEARBIT(mcpo, 5);
		
		if (simpleSensorValues[27] > 0)
			SETBIT(mcpo, 4);
		else
			CLEARBIT(mcpo, 4);

		IOexpSetOutput(mcpo);
	#endif
}

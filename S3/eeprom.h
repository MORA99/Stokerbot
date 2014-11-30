#include <avr/eeprom.h> 
#include <stdint.h>

/*
	1		55 if eeprom contains settings, FF if new eeprom, anything else if foeing/corrupted eeprom
	10		dhcp
	11-14 	my ip
 	15-18	gateway
	19-22	netmask
	23(-24)	web-port (unused)
	25		Disable broadcast
	50		LCD enabled (51-59 reserved for future)
	31-34	dns server

	100-107	analog sensor types
	110-113	digital sensor types, may expand in future
		
	200-209	password
	
	300-307	analog pullup
	310-313	digital pullup
		
	400-520	Alarm data (8*15)	
	
	1000-1100 HTTP URL
	1101	HTTP client enabled
	1102	HTTP client send ?id=xyz
	
	2000	Analog sensor values (32bit)
	2500	Digital sensor values (32bit)
*/
#ifndef sbeeprom
	#define sbeeprom

	extern void eepromSaveByte(uint16_t pos, uint8_t value);
	extern void eepromSaveWord(uint16_t pos, uint16_t value);
	extern void eepromSaveDword(uint16_t pos, uint32_t value);
	extern uint8_t eepromReadByte(uint16_t pos);
	extern int8_t eepromReadByteSigned(uint16_t pos);
	extern uint16_t eepromReadWord(uint16_t pos);
	extern uint32_t eepromReadDword(uint16_t pos);
	extern void eepromSaveStr(uint16_t pos, char* target, uint8_t limit);
	extern void eepromReadStr(uint16_t pos, char* target, uint8_t limit);

	#define eepromWriteByte eepromSaveByte
	#define eepromWriteWord eepromSaveWord
	#define eepromWriteStr  eepromSaveStr
#endif

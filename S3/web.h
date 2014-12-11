#ifndef web_h
 #define web_h
 #include <stdint.h>
 #include <stdbool.h>
 #include <avr/pgmspace.h>
 #include <stdlib.h>
 #include <string.h>

 #include "websrv_help_functions.h"
 #include "config.h"

 #include "ip_arp_udp_tcp.h"
 #include "enc28j60.h"
 #include "timeout.h"
 #include "net.h"
 #include "dnslkup.h"
 #include "eeprom.h"
 #include "base64_dec.h"

 #include "ds18x20.h"

 #include "timer.h"

uint16_t fill_custom_client_data(uint8_t *bufptr,uint16_t len);
 extern uint8_t buf[BUFFER_SIZE+1];
 extern uint8_t systemID[8];
 extern uint8_t gwip[4];
 extern uint8_t myip[4];
 extern uint8_t netmask[8];
 extern uint8_t dnsip[4];
 extern uint16_t tick;
 extern uint16_t tickS;
 extern uint8_t sendSimple;

 void lcd_update(void);
 uint16_t http200ok(void);
 uint16_t https401(void);
 uint16_t print_webpage(uint8_t *buf, bool ajax);
 uint16_t print_flash_webpage_only(const char * pos, uint8_t* buf, uint16_t plen);
 uint16_t print_settings_general_webpage(uint8_t *buf);
 uint16_t print_settings_alarms_webpage(uint8_t *buf);
 uint16_t print_settings_webpage(uint8_t *buf);
 uint16_t print_settings_io_webpage(uint8_t *buf);
 uint16_t print_fmu_webpage(uint8_t *buf);
 uint16_t print_loader(uint8_t *buf);

 void handle_net(void);
 bool save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location);
 bool signed_save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location);
 bool save_checkbox_cgivalue(char* buffer, char* name, uint16_t eeprom_location);
 bool word_save_cgivalue_if_found(char* buffer, char* name, uint16_t eeprom_location);
 void browserresult_callback(uint16_t statuscode,uint16_t datapos, uint16_t len);
 void lcd_callback(uint16_t statuscode,uint16_t datapos, uint16_t len);
 void boxcallback(uint16_t statuscode,uint16_t datapos, uint16_t len);
 bool user_is_auth(char* buffer);

 void checkTimedEvents();
 void initTimedEvents();

 #define WEBSERVER_VHOST "stokerlog.dk"


#endif

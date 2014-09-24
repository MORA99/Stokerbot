#include <avr/io.h>
#include <avr/interrupt.h>  
#include <util/delay.h>     

#define uchar unsigned char
#define CMD_ID 						0x11

#define CMD_LINE_0 					0x00
#define CMD_LINE_1 					0x01
#define CMD_LINE_2 					0x02
#define CMD_LINE_3 					0x03

#define CMD_BACKLIGHT 				0x65
#define CMD_CONTRAST 				0x66
#define CMD_NB_LINE 				0x67
#define CMD_NB_COLUMN				0x68
#define CMD_CLR_SCRN				0x69
#define CMD_PWR_SV_MD				0x6B
#define CMD_DRCT_MD_ON			 	0x30
#define CMD_DRCT_MD_OFF			 	0x31
#define CMD_DSPL_CURSOR				0x6C
#define CMD_RS_SET 					0x6D
#define CMD_RS_CLEAR 				0x6E
	
#define CMD_SV_SPLSCRN				0x72
#define CMD_SV_PRMTR				0x73
#define CMD_CHNG_BR	 				0x74

#define CMD_MOV_RIGHT 				0xC0
#define CMD_MOV_LEFT 				0xC1
#define CMD_MOV_UP 					0xC2
#define CMD_MOV_DOWN 				0xC3

#define CHAR_BATTERY_NULL			0x80
#define CHAR_BATTERY_HALF			0x81
#define CHAR_BATTERY_FULL			0x82
#define CHAR_BELL					0x83
#define CHAR_KEY					0x84
#define CHAR_SMILEY_HAPPY			0x85
#define CHAR_SMILEY_SAD				0x86
#define CHAR_SMILEY_NEUTRAL			0x87
#define CHAR_RIGHT					0x88
#define CHAR_WRONG					0x89
#define CHAR_ARROW_UP				0x8A
#define CHAR_ARROW_DOWN				0x8B
#define CHAR_ARROW_UP_RIGHT			0x90
#define CHAR_ARROW_UP_LEFT			0x91
#define CHAR_ARROW_DOWN_RIGHT		0x8C
#define CHAR_ARROW_DOWN_LEFT		0x92
#define CHAR_PLUS_MINUS				0x93
#define CHAR_EMPTY_SET				0x94
#define CHAR_NOT_EQUAL				0x95
#define CHAR_BARGRAPH_HORIZONTAL	0x96
#define CHAR_BARGRAPH_VERTICAL		0x97

#define PAR_DEFAULT 				0x00

#define PAR_LINE_0 				0x00
#define PAR_LINE_1 				0x01
#define PAR_LINE_2 				0x02
#define PAR_LINE_3 				0x03

#define PAR_COLUMN_0 			0x00
#define PAR_COLUMN_1 			0x01
#define PAR_COLUMN_2 			0x02
#define PAR_COLUMN_3 			0x03
#define PAR_COLUMN_4 			0x04
#define PAR_COLUMN_5 			0x05
#define PAR_COLUMN_6 			0x06
#define PAR_COLUMN_7 			0x07
#define PAR_COLUMN_8 			0x08
#define PAR_COLUMN_9 			0x09
#define PAR_COLUMN_10			0x0A
#define PAR_COLUMN_11			0x0B
#define PAR_COLUMN_12			0x0C
#define PAR_COLUMN_13			0x0D
#define PAR_COLUMN_14 			0x0E
#define PAR_COLUMN_15 			0x0F
#define PAR_COLUMN_16 			0x10
#define PAR_COLUMN_17 			0x11
#define PAR_COLUMN_18 			0x12
#define PAR_COLUMN_19 			0x13
#define PAR_COLUMN_20 			0x14

#define PAR_BR_2400				0x01
#define PAR_BR_4800				0x02
#define PAR_BR_9600				0x03
#define PAR_BR_14K4				0x04
#define PAR_BR_19K2				0x05
#define PAR_BR_28K8				0x06
#define PAR_BR_38K4				0x07

#define PAR_BL_OFF				0x00
#define PAR_BL_25				0x40
#define PAR_BL_50				0x80
#define PAR_BL_75				0xC0
#define PAR_BL_FULL				0xFF


void setup_uart(unsigned int ubrr);
void complete_table(void);
void lcd_logic_send(uchar data);



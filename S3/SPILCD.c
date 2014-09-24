#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "AVR035.h"
#include "SPILCD.h"
#include "dbg_putchar.h"
#include "lcd_logic.h"

//software serial 9600baud for now
//portd.7

void spilcd_init()
{
    SETBIT(DDRD, 7); //output
    LCDclr();

	dbg_putchar(CMD_ID);
	dbg_putchar(CMD_NB_LINE);
	dbg_putchar(4);
_delay_ms(5);
	dbg_putchar(CMD_ID);
	dbg_putchar(CMD_NB_COLUMN);
	dbg_putchar(20);
_delay_ms(5);
	dbg_putchar(CMD_ID);
	dbg_putchar(CMD_CONTRAST);
	dbg_putchar(255);
_delay_ms(5);
	dbg_putchar(CMD_ID);
	dbg_putchar(CMD_BACKLIGHT);
	dbg_putchar(255);
_delay_ms(5);
}

void LCDclr()
{
	dbg_putchar(CMD_ID);
	dbg_putchar(CMD_CLR_SCRN);
	dbg_putchar(0);
}

void LCDsetCursor(uint8_t x, uint8_t y)
{
	dbg_putchar(CMD_ID);
	if (x == 0) dbg_putchar(CMD_LINE_0);
	if (x == 1) dbg_putchar(CMD_LINE_1);
	if (x == 2) dbg_putchar(CMD_LINE_2);
	if (x == 3) dbg_putchar(CMD_LINE_3);
	dbg_putchar(y);
}

void LCDwrite(char* str, uint8_t length)
{
	dbg_puts(str);
}

void LCDbacklight(uint8_t level)
{
	dbg_putchar(CMD_BACKLIGHT);
	dbg_putchar(level);
}

void LCDcontrast(uint8_t level)
{
	dbg_putchar(CMD_CONTRAST);
	dbg_putchar(level);
}

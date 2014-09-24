#ifndef __spilcd_H
    #define __spilcd_H
    void shiftOut(char c);
    void shiftOutString(char* str, uint8_t limit);
    void LCDsetCursor(uint8_t x, uint8_t y);
    void LCDwrite(char* str, uint8_t length);
    void LCDclr(void);
    void LCDwriteChar(char c);
    void spilcd_init(void);
	void LCDbacklight(uint8_t level);
	void LCDcontrast(uint8_t level);
#endif

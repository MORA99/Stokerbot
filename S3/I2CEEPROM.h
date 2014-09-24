#define EEPROM_ADDRESS 0x50

uint8_t IEP_readbyte(uint16_t address);
uint8_t IEP_writebyte(uint16_t address, uint8_t value);

#include "twi.h"
#include <stdbool.h>

#define MCP23008_ADDRESS 0x20

bool IOexpInit(void);
bool IOexpSetOutput(uint8_t GP0);
uint8_t IOexpReadInput();

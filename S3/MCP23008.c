#include "MCP23008.h"

//Commands
#define CMD_GP     0x09   //I/O pins
#define CMD_IPOL   0x01   //Polarity of input pins (0=no invert 1=invert)
#define CMD_IODIR  0x00   //Direction of pins (0=out 1=in)
#define CMD_IOCON  0x05

bool IOexpInit(void)
{
  uint8_t ioBuffer[ 2 ];
  ioBuffer[0] = CMD_IODIR;
  ioBuffer[1] = 0b00001111; //OOOOIIII = 1=Input

  int res = twiWrite( MCP23008_ADDRESS, ioBuffer, 2 );

  if (res != 0)
  {
	return false;
  }


  ioBuffer[0] = CMD_IPOL;
  ioBuffer[1] = 0b00001111; //OOOOIIII = 1=Invert
  res = twiWrite( MCP23008_ADDRESS, ioBuffer, 2 );

  if (res != 0)
  {
	return false;
  }

  return true;
}

bool IOexpSetOutput(uint8_t GP0) //first 4 bits are outputs
{
	printf("IOexpSetOutput(%02X)\r\n",GP0);
  uint8_t ioBuffer[ 2 ];
  ioBuffer[0] = CMD_GP;
  ioBuffer[1] = GP0;
  
  int res = twiWrite( MCP23008_ADDRESS, ioBuffer, 2 );

  if (res != 0)
  {
	return false;
  }

  return true;
}

uint8_t IOexpReadInput()
{
  uint8_t ioBuffer[ 1 ];
  ioBuffer[0] = CMD_GP;
  twiWrite(MCP23008_ADDRESS, ioBuffer, 1);
  ioBuffer[0] = 0;
  
  uint8_t reply = twiRead(MCP23008_ADDRESS,ioBuffer,1);
  
  if (reply == 0)
  	return ioBuffer[0];
  else 
    return 0xFF; //only last 4 are input, so largest acceptable value is 0b00001111
}

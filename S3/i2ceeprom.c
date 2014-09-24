#include "config.h"
#include "twi.h"
#include "I2CEEPROM.h"
#include <avr/pgmspace.h>

uint8_t IEP_readbyte(uint16_t address)
{
  uint8_t ioBuffer[ 2 ];
  uint8_t rc;

  ioBuffer[ 0 ] = (address & 0xFF00) >> 8;
  ioBuffer[ 1 ] = (address & 0x00FF);

  rc = twiWrite( EEPROM_ADDRESS, ioBuffer, 2 );

  if ( rc != 0 )
  {
    //fprintf(stdout, twiWriteErrorFmt);
    return -1;
  }

  rc = twiRead( EEPROM_ADDRESS, (uint8_t *) &ioBuffer, 2 );  //2 bytes returned MSB first, D11-D4 + D3-D0 (padded with 0000)
  if ( rc != 0 )
  {
    printf_P(PSTR("Error read byte"));
    return -1;
  } // end if

  //Parse result
  return ioBuffer[0];
}


uint8_t IEP_writebyte(uint16_t address, uint8_t value)
{
  uint8_t ioBuffer[ 3 ];
  uint8_t rc;

  ioBuffer[ 0 ] = (address & 0xFF00) >> 8;
  ioBuffer[ 1 ] = (address & 0x00FF);
  ioBuffer[ 2 ] = value;
  rc = twiWrite( EEPROM_ADDRESS, ioBuffer, 3 );

  if ( rc != 0 )
  {
    printf_P(PSTR("Error write byte"));
    return -1;
  }
  return 0;
}

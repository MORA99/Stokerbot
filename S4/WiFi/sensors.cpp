#include "sensors.h"

void Sensors::add(char* name, float value)
{
  for (uint8_t i=0; i<MAXSENSORS; i++)
  {
   if (strlen(_sensors[i].name) > 0)
   {
     if (strstr(_sensors[i].name, name))
     {
       if (_sensors[i].value != value) _sensors[i].lastChange = _currentTick;
       _sensors[i].value = value;
       _sensors[i].lastUpdate = _currentTick;
       return;
     }
   }
  }
  strcpy(_sensors[_nextSpot].name, name);
  _sensors[_nextSpot].value = value;
  _sensors[_nextSpot].lastUpdate = _currentTick;
  _nextSpot++;
}

float Sensors::get(char* name)
{
  for (uint8_t i=0; i<MAXSENSORS; i++)
  {
   if (strstr(_sensors[i].name, name))
     return _sensors[i].value;
  }
  return NULL;
}

sensor Sensors::getSensor(uint8_t i)
{
  return _sensors[i];
}

void Sensors::remove(char* name)
{
  for (uint8_t i=0; i<MAXSENSORS; i++)
  {
   if (strstr(_sensors[i].name, name)) memset(_sensors[i].name, NULL, sizeof(_sensors[i].name));
  }
}

void Sensors::tick()
{
  _currentTick++;
}

uint8_t Sensors::getNextSpot() { return _nextSpot; }

uint16_t Sensors::getCurrentTick() { return _currentTick; }

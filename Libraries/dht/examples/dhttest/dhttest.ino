#include <dht.h>

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  float temperature, humidity;
 
 //Being a timed single wire protocol, errors do happen.
 //But by comparing the result of getData to 0 you can discard bad reads using the CRC system.
  if (dht::getFloatData(2, &temperature, &humidity, true) == 0)
  {
    Serial.print("T: ");
    Serial.print(temperature);
    Serial.print(" H: ");
    Serial.println(humidity);    
  }  
  
  
  delay(5000);  
  //This way is mostly useful if you dont want to load float support
  //or if you need to store the values before transmitting.
  
  int16_t rawtemperature, rawhumidity;
  if (dht::getRawData(2, &rawtemperature, &rawhumidity, true) == 0)
  {
    Serial.print("(raw) T: ");
    Serial.print(rawtemperature);
    Serial.print(" H: ");
    Serial.print(rawhumidity);    
    temperature = dht::convertTemperature(rawtemperature);
    humidity = dht::convertTemperature(rawhumidity);    
    Serial.print(">> T: ");
    Serial.print(temperature);
    Serial.print(" H: ");
    Serial.println(humidity);        
  }  
  delay(5000);    
}

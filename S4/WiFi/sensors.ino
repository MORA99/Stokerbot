
int exportSlow(long unsigned int now)
{
  exports(true);
}

void exports(boolean all)
{
   uint8_t next = sensors.getNextSpot();
   uint16_t current = sensors.getCurrentTick();
   uint8_t limit = 0;
   uint16_t diff;
     
   for (uint8_t i=0; i<next; i++)
   {
     sensor s = sensors.getSensor(i);
     if (strlen(s.name) > 0)
     {
       /*
       Serial.print("Sensor #");Serial.println(i);
       Serial.print("Name: ");Serial.println(s.name);     
       Serial.print("Value: ");Serial.println(s.value);     
       Serial.print("Update: ");Serial.println(s.lastUpdate);
       */
       if (all) //Send all sensors that were updated in the last 60 ticks
       {
         diff = current - s.lastUpdate;
         if (current < s.lastUpdate) diff = (65535 - s.lastUpdate) + current; //Rollover
         limit = 60;
       } else { //Send all sensors that were CHANGED in the last 2 ticks
         diff = current - s.lastChange;
         if (current < s.lastChange) diff = (65535 - s.lastChange) + current; //Rollover
       }
       
       if (diff <= limit)
       {
         //{"cmd":"data","data":[{s.name: s.value}]}
         char buffer[100];
         Generator::JsonObject<4> root; 
         
         Generator::JsonObject<2> sensorObj;
         sensorObj[s.name] = s.value;
         
         Generator::JsonArray<1> data;
         data.add(sensorObj);
         
         root["cmd"] = "data";
         root["data"] = data;
         root.printTo(buffer, sizeof(buffer));           

         wsc.sendMessage(buffer, strlen(buffer));
         
//         sprintf(buffer, "{\"cmd\":\"data\",\"data\":[{\"%s\": \"%f\"}]}", s.name, s.value);
//          Serial.println(buffer);
       }
     }
   }
}

int temp(unsigned long now)
{
  sensors.add("TMP006-O", tmp006.readObjTempC());
  sensors.add("TMP006-D", tmp006.readDieTempC());  
}

int accel(long unsigned int now)
{
  sensors.add("Accel.x", accelerometer.readXData());
  sensors.add("Accel.y", accelerometer.readYData());
  sensors.add("Accel.z", accelerometer.readZData());  
}

int second(long unsigned int now)
{
 wsc.run();
 sys();
 readOW();
}

int sensorTick(long unsigned int now)
{
 exports(false);
 sensors.tick();  
}

int dht(long unsigned int now)
{
  float humidity;
  float temperature;
  int8_t res = dht::readFloatData(2, &temperature, &humidity, true);
  if (res == 0) {
    sensors.add("D0H", humidity);  
    sensors.add("D0T", temperature);    
  }
}

int sys()
{
  sensors.add("RSSI", WiFi.RSSI());
}

boolean dsconv = false;
void readOW() {
  if (!dsconv)
  {
      ds.CmdT();
      dsconv=true;
  } else {
    dsconv=false;
    byte rom[8];
    char buffer[20];
    while (ds.search(rom)) {
      float temp = (float)ds.GetTemperature(rom) / 100;    
      sprintf(buffer, "%02X%02X%02X%02X%02X%02X%02X%02X", rom[0],rom[1],rom[2],rom[3],rom[4],rom[5],rom[6],rom[7]);
      sensors.add(buffer, temp);
    }
    ds.reset_search();
  }
}

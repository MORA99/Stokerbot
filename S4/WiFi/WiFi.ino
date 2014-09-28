#include "Energia.h"
#include "config.h"

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <JsonGenerator.h>
#include <JsonParser.h>
#include <Wire.h>
#include <BMA222.h>
#include <Adafruit_TMP006.h>
#include <StellarisDS18B20.h>

#include "Queue.h"
#include "WebClient.h"
#include "dht.h"
#include "sensors.h"

using namespace ArduinoJson;

Queue tasks;
BMA222 accelerometer;
Adafruit_TMP006 tmp006(0x41);
//WebsocketClient wsc("echo.websocket.org", 80, "/");
WebsocketClient wsc("iotpool.com", 4000, "/");
Sensors sensors;

DS18B20 ds(3);

WiFiServer server(80);

void setup() {

  Serial.begin(115200);      // initialize serial communication
  pinMode(RED_LED, OUTPUT);      // set the LED pin mode

  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to Network named: ");
  // print the network name (SSID);
  Serial.println(ssid); 
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED) {
    // print dots while we wait to connect
    Serial.print(".");
    delay(300);
  }
  
  Serial.println("\nYou're connected to the network");
  Serial.println("Waiting for an ip address");
  
  while (WiFi.localIP() == INADDR_NONE) {
    // print dots while we wait for an ip addresss
    Serial.print(".");
    delay(300);
  }

  // you're connected now, so print out the status  
  printWifiStatus();
  
  Serial.println("Starting webserver on port 80");
  server.begin();                           // start the web server on port 80
  Serial.println("Webserver started!");

  //randomSeed(analogRead(14));

  accelerometer.begin();
  wsc.connect();
  tmp006.begin();

  tasks.scheduleFunction(exportSlow, "ES", 60000, 60000);
  tasks.scheduleFunction(sensorTick, "ST", 250, 250);
  tasks.scheduleFunction(second, "Second", 0, 1000);

  tasks.scheduleFunction(accel, "accel", 1000, 1000);
  tasks.scheduleFunction(dht, "dht", 5000, 5000);   
  tasks.scheduleFunction(temp, "temp", 5000, 2000);
}

int temp(unsigned long now)
{
  sensors.add("TMP006-O", tmp006.readObjTempC());
  sensors.add("TMP006-D", tmp006.readDieTempC());  
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    clientid++;
    Serial.print("new client ");
    Serial.println(clientid);
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          delay(25);
          client.println("Content-Type: text/html");
          delay(25);
          client.println("Connection: close");  // the connection will be closed after completion of the response
          delay(25);
//          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          delay(25);
          client.println();
          delay(25);
          client.println("<!DOCTYPE HTML>");
          delay(25);
          client.println("<html>");

          for (uint16_t i=0; i<100; i++)
          {
            client.println("Dummy");
            delay(2);
          }
          /*
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogChannel; //analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          */
          client.println("</html>");
          
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }

    // close the connection:
    client.stop();
    Serial.print("client ");
    Serial.print(clientid);
    Serial.println(" disonnected");
  }
  
  tasks.Run(millis());
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("Network Name: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

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
//         Serial.println(buffer);
       }
     }
   }
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


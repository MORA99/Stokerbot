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
void wscMessage(char* msg);

Queue tasks;
BMA222 accelerometer;
Adafruit_TMP006 tmp006(0x41);
//WebsocketClient wsc("echo.websocket.org", 80, "/", false, test);
//WebsocketClient wsc("echo.websocket.org", 443, "/", true, test);
WebsocketClient wsc("iotpool.com", 4000, "/", wscMessage);
Sensors sensors;

DS18B20 ds(3);

WiFiServer server(80);

void wscMessage(char* msg)
{
  Serial.print("Got msg : ");
  Serial.println(msg);
  if (strcmp(msg, "{\"cmd\":\"identify\"}")==0)
  {
    Serial.println("IDENTIFY");
    char* reg = "{\"cmd\":\"identify\", \"data\":\"CC3200\"}";
    wsc.sendMessage(reg, strlen(reg));    
  }
}

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
//  wsc.sslconnect();
  wsc.connect();
  tmp006.begin();

  tasks.scheduleFunction(exportSlow, "ES", 60000, 60000);
  tasks.scheduleFunction(sensorTick, "ST", 250, 250);
  tasks.scheduleFunction(second, "Second", 0, 1000);

  tasks.scheduleFunction(accel, "accel", 1000, 1000);
  tasks.scheduleFunction(dht, "dht", 5000, 5000);   
  tasks.scheduleFunction(temp, "temp", 5000, 2000);
}

void loop() {
  runServer();
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




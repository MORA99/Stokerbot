#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "WebClient.h"

// your network name also called SSID
char ssid[] = "---";
// your network password
char password[] = "---";
// your network key Index number (needed only for WEP)
int keyIndex = 0;
int clientid = 0;



void newMessage(char* msg); //This function will be called when the websocket gets new data
WebsocketClient wsc("echo.websocket.org", 80, "/", newMessage); //Public loopback service


void newMessage(char* msg)
{
  Serial.print("Got msg : ");
  Serial.println(msg);
}


void setup() {
  Serial.begin(115200);      // initialize serial communication
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
  
  wsc.connect(); //Start websocket connection
}

uint8_t timer = 0;

void loop() {
  wsc.run(); //This function needs to be called regularly to keep the websocket in sync, somewhere in the area of 1-100times a second depending on your usage.
  
  delay(100);
  if (timer == 50 || timer==100 || timer == 150 || timer == 200 || timer == 250)
  {
    char str[50];
    sprintf(str, "Hello from timer %u\r\n", timer);
    wsc.sendMessage(str, strlen(str));
  }
  timer++;
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


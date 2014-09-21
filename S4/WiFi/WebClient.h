#ifndef WebsocketClient_h
#define WebsocketClient_h

#include <WiFi.h>
#include <WiFiClient.h>
//#include <EthernetClient>

class WebsocketClient
{
  private:
  WiFiClient client;
//  EthernetClient client;
  uint16_t _port;
  char* _host;
  char* _path;
  boolean sendPong();
  char _key[25];
  
  public:
  WebsocketClient(char* host, uint16_t port, char* path);
  void connect();
  int run();
  boolean sendPing();
  boolean sendMessage(char* msg, uint16_t length);
};
#endif


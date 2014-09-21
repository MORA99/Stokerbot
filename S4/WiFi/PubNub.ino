WiFiClient pubnubclient;
WiFiClient pubnubclientpub;
uint8_t state = 0;
char ts[20] = "0";
uint8_t connectThrottle = 0;
char* channel = "test";

char* sub = "demo";
char* pub = "demo";

/*
0: Unconnected
1: Connected
2: Requested file
*/

boolean PubNubPub(char* message)
{
  char dst[sizeof(message)+100];
  
  if (pubnubclientpub.connected())
  {
    return false;
  }
  
  pubnubclientpub.connect("pubsub.pubnub.com", 80);
  Serial.println("Connecting to pubnub to publish");

  uint8_t wait = 200; //2seconds
  while (!pubnubclientpub.connected() && wait > 0)
  {
    wait--;
    delay(10);
  }

  Serial.println("Connected, Sending data");
    pubnubclientpub.print("GET /publish/");
    pubnubclientpub.print(pub);
    pubnubclientpub.print('/');
    pubnubclientpub.print(sub);
    pubnubclientpub.print("/0/");
    pubnubclientpub.print(channel);
    pubnubclientpub.print("/0/");
    pubnubclientpub.print(urlencode(dst,message));
    pubnubclientpub.println(" HTTP/1.1");
    pubnubclientpub.println("Host: pubsub.pubnub.com");
    pubnubclientpub.println("User-Agent: Energia/1.1");
    pubnubclientpub.println("Connection: close");
    pubnubclientpub.println();  
    
    delay(100);
    while (pubnubclientpub.available())
    {
      char c = pubnubclientpub.read();
      Serial.print(c);
    }
    
    Serial.println("");
    pubnubclientpub.stop();
}

void PubNubRun()
{
  if (connectThrottle > 0) connectThrottle--;
  if (!pubnubclient.connected() && connectThrottle == 0)
  {
    pubnubclient.connect("pubsub.pubnub.com", 80);
    Serial.println("Connecting to pubnub");
    state = 0;
    connectThrottle = 100;
    return;
  }
  if (pubnubclient.connected() && state == 0)
  {
    connectThrottle = 0;
    Serial.println("Connected, Requesting data");
    state = 1;
    pubnubclient.print("GET /subscribe/");
    pubnubclient.print(sub);
    pubnubclient.print("/");
    pubnubclient.print(channel);
    pubnubclient.print("/0/");
    pubnubclient.print(ts);
    pubnubclient.println(" HTTP/1.1");
    pubnubclient.println("Host: pubsub.pubnub.com");
    pubnubclient.println("User-Agent: Energia/1.1");
    pubnubclient.println("Connection: close");
    pubnubclient.println();  
    state = 2;
    return;
  }
  if (state == 2)
  {
    if (pubnubclient.connected() && pubnubclient.available()==0)
    {
//      Serial.println("Still connected, no news");
      return;
    }
    if (pubnubclient.available()==0) return;
    
    char buf[512];
    uint16_t i = 0;
    
    while (pubnubclient.available() || pubnubclient.connected()) //Keep reading till server closes connection
    {
      if (pubnubclient.available())
      {
        if (pubnubclient.read() == '\r' && pubnubclient.read() == '\n' && pubnubclient.read() == '\r' && pubnubclient.read() == '\n')
        {
          while(pubnubclient.connected())
          {
            if (pubnubclient.available())
            {
              buf[i++] = pubnubclient.read();
            }
          }
        }
      }
    }
    buf[i] = NULL;
    Serial.println("Reply read ... ");
    Serial.println(buf);    
    Parser::JsonParser<50> parser;
    Parser::JsonArray root = parser.parse(buf);

    if (!root.success())
    {
        Serial.println("JsonParser.parse() failed");
        //[[{"text":"hey"},{"text":"hey"},{"text":"hey"},{"text":"hey"},{"text":"hey"},{"text":"hey"},{"text":"hey"},{"text":"hey"},{"text":"hey"}],"14109827507205081"]
        return;
    }

    memset(ts, '\0', sizeof(ts));
    strcpy(ts, (char*)root[1]);
    Serial.print("Timestamp is ");
    Serial.println(ts);
    
    Serial.println("Messenges");
    Parser::JsonArray msgs = root[0];
    
    for (uint8_t i=0; i<msgs.size(); i++)
    {
      //Handle mesenges, must be a wellknown json format
      Serial.println((char*)msgs[i]["text"]);
    }    
    Serial.println();
  }
}


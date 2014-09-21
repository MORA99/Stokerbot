WiFiClient IOTclient;
uint8_t IOTconnectDelay = 0;

void IOTconnect()
{
  if (IOTconnectDelay-- == 0)
  {
    IOTconnectDelay = 100;
    char* host = "iotpool.com";
    int port = 2600;  
    
    if (IOTclient.connect(host, port)) {
      Serial.println("Connected");
    } else {
      Serial.println("Connection failed.");
      return;
    }    
  }
}

void IOTInit() {
  IOTconnect();
}

int IOTRun(unsigned long now)
{
  if (!IOTclient.connected())
  {
    Serial.println("Disconnected...");
    IOTconnect();
    return 0;
  }

  while (IOTclient.available())
  {
   char c = IOTclient.read();
   Serial.print(c); 
  }  
}

void sendMessage(char* msg)
{
  IOTclient.println(msg);
  Serial.println();
  Serial.print("---");
  Serial.print(msg);
  Serial.print("---");
  Serial.println();
}



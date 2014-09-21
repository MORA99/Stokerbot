WiFiClient client;

void sendReport()
{
  if (!client.connected())
  {
    client.connect("stokerlog.dk", 4000);
    Serial.println("Connecting to stokerlog");
    return;
  }
  if (client.connected())
  {
    client.println("Bla bla bla");
  }
  
  
  /*
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect("stokerlog.dk", 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("GET /incoming.php?id=cc3200 HTTP/1.1");    
    client.println("Host: stokerlog.dk");
    client.println("User-Agent: Energia/1.1");
    client.println("Connection: close");
    client.println();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }  
  */
}


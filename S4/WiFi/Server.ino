void runServer()
{
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
          client.println("Refresh: 1");  // refresh the page automatically every 5 sec
          delay(25);
          client.println();
          delay(25);
          client.println("<!DOCTYPE HTML>");
          delay(25);
          client.println("<html>[");

         uint8_t next = sensors.getNextSpot();
         uint8_t y = 0;
         for (uint8_t i=0; i<next; i++)
         {
           sensor s = sensors.getSensor(i);
           if (strlen(s.name) > 0)
           {
               if (y++ > 0) client.print(',');
               Generator::JsonObject<2> sensorObj;
               sensorObj[s.name] = s.value;     
               client.print(sensorObj);
             }
           }
          
          client.println("]</html>");
          
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
}

WiFiClient WSclient;

void WSInit() {
  char* path = "/";
  char* host = "echo.websocket.org";
//  char* host = "iotpool.com";  
  uint16_t port = 80;
//  uint16_t port = 2600;  
  char* key = "x3JJHMbDL1EzLkh9GBhXDw==";
  char* protocol = "chat";
  
  if (WSclient.connect(host, port)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    return;
  }

    WSclient.print("GET ");
    WSclient.print(path);
    WSclient.print(" HTTP/1.1\r\n");
    WSclient.print("Upgrade: websocket\r\n");
    WSclient.print("Connection: Upgrade\r\n");
    WSclient.print("Host: ");
    WSclient.print(host);
    WSclient.println(); 
    WSclient.print("Sec-WebSocket-Key: ");
    WSclient.print(key);
    WSclient.println();
    WSclient.print("Sec-WebSocket-Protocol: ");
    WSclient.print(protocol);
    WSclient.println();
    WSclient.print("Sec-WebSocket-Version: 13\r\n");
    WSclient.println();
delay(1000);

while (WSclient.available())
{
 char c = WSclient.read();
 Serial.print(c); 
}
Serial.println("Handshake untested ..., websocket from here on out...");
delay(5000);
sendMessage("hello",5);
/*
sendMessage("ADC1:120",8);

sendMessage("ADC2:244",8);

sendMessage("ADC3:111",8);

sendMessage("ADC4:15",7);
*/
/*
HTTP/1.1 101 Web Socket Protocol Handshake
Upgrade: WebSocket
Connection: Upgrade
Sec-WebSocket-Accept: fWDprEwtL2ZZ4DcUCCkymXFwZQM=
Server: Kaazing Gateway
Date: Thu, 18 Sep 2014 15:38:26 GMT
*/

/*
  // Handshake with the server
  webSocketClient.path = "/";
  webSocketClient.host = "echo.websocket.org";
  
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");
    return;
  }
  */
}

int WSRun(unsigned long now)
{
  if (!WSclient.connected())
  {
    Serial.println("Disconnected...");
  }
  Serial.print("Data : ");
  Serial.println(WSclient.available());
  if (WSclient.available() > 0)
  {
    char c = WSclient.read();
    uint8_t op = c & 0b00001111;
    uint8_t fin = c & 0b10000000;
    Serial.print("Opcode : ");
    Serial.println(op,BIN);
    Serial.print("Fin : ");
    Serial.println(fin,BIN);
    /*
      0x00: this frame continues the payload from the last.
      0x01: this frame includes utf-8 text data.
      0x02: this frame includes binary data.
      0x08: this frame terminates the connection.
      0x09: this frame is a ping.
      0x10: this frame is a pong.    
    */
    if (op == 0x00 || op == 0x01 || op==0x02) //Data
    {
      Serial.println("WS Got opcode packet");
      if (fin > 0)
      {
        Serial.println("Single frame message");
        c = WSclient.read();
        char masked = c & 0b10000000;
        uint16_t len = c & 0b01111111;
        
        if (len == 126)
        {
          //next 2 bytes are length
          len = WSclient.read();
          len << 8;
          len = len & WSclient.read();
        }
        if (len == 127)
        {
          //next 8 bytes are length
          Serial.println("64bit outgoing messenges not supported");
        }
        
        Serial.print("Message is ");
        Serial.print(len);
        Serial.println(" chars long");        
        
        //Generally server replies are not masked, but RFC does not forbid it
        char mask[4];
        if (masked > 0)
        {
          mask[0] = WSclient.read();
          mask[1] = WSclient.read();
          mask[2] = WSclient.read();
          mask[3] = WSclient.read();
        }
        
        char data[len+1];
        for (uint8_t i=0; i<len; i++)
        {
          data[i] = WSclient.read();
          if (masked > 0) data[i] = data[i] ^ mask[i % 4];
        }
        data[len] = NULL;
        Serial.println("Frame contents : ");
        Serial.println(data);
      }
    } else if (op == 0x08)
    {
      Serial.println("WS Disconnect opcode");
      WSclient.stop();
    } else if (op == 0x09)
    {
      Serial.println("Got ping ...");      
      sendPong();
    } else if (op = 0x10)
    {
      Serial.println("Got pong ...");
    } else {
      Serial.print("Unknown opcode ");
      Serial.println(op);      
    }
  }
  //10000001 101 1101000 1100101 1101100 1101100 1101111
  /*
  fin=1, opcode=1
  no mask, len=5
  h e l l o
  */
  while (WSclient.available())
  {
   char c = WSclient.read();
   Serial.print(c,BIN); 
   Serial.print(" ");    
  }  
}

void sendPong()
{
  
}

void sendPing()
{
  
}

void sendMessage(char* msg, uint16_t length)
{
  Serial.println("Sending packet");
/*
+-+-+-+-+-------+-+-------------+-------------------------------+
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               | Masking-key, if MASK set to 1 |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+

Opcodes
0x00: this frame continues the payload from the last.
0x01: this frame includes utf-8 text data.
0x02: this frame includes binary data.
0x08: this frame terminates the connection.
0x09: this frame is a ping.
0x10: this frame is a pong.
*/
  
WSclient.write(0b10000001);
Serial.print(0b10000001,HEX);
/*
payload_len (7 bits): the length of the payload.

0-125 mean the payload is that long. 
126 means that the following two bytes indicate the length.
127 means the next 8 bytes indicate the length. 

So it comes in ~7bit, 16bit and 64bit.
*/
if (length <= 125) 
{
  WSclient.write(0b10000000 | length);
  Serial.print(0b10000000 | length,HEX);
}
else
{
  WSclient.write(0b11111110); //mask+126
  WSclient.write((uint8_t)(length >> 8));
  WSclient.write((uint8_t)(length & 0xFF));
}
//64bit outgoing messenges not supported

byte mask[4];
/*
mask[0] = random(0, 255);
mask[1] = random(0, 255);
mask[2] = random(0, 255);
mask[3] = random(0, 255);
*/

mask[0] = 0b00000000;
mask[1] = 0b11111111;
mask[2] = 0b00000000;
mask[3] = 0b11111111;
WSclient.write(mask[0]);
WSclient.write(mask[1]);
WSclient.write(mask[2]);
WSclient.write(mask[3]);


Serial.print(mask[0],HEX); 
Serial.print(mask[1],HEX); 
Serial.print(mask[2],HEX); 
Serial.print(mask[3],HEX); 

for (uint16_t i=0; i<length; i++) {
  WSclient.write(msg[i] ^ mask[i % 4]);
  Serial.print(msg[i] ^ mask[i % 4], HEX);
}
Serial.println();
Serial.println("Packet sent ..."); 
}

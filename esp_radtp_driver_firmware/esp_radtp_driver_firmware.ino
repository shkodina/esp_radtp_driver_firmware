#include <avr/pgmspace.h>
#include "ESP8266WiFi.h"

#define WAIT_DELAY_FOR_RE_CONNECT_ms 1000
const char *ssid     = "Microel5250";
const char *password = "92021044";

const char* host = "192.168.0.100";
uint16_t port0 = 7000;
uint16_t port1 = 7001;
uint32_t id = 1;

bool is_connected = false;
uint8_t keep_alive_counter = 128;

void setup (){
  Serial.begin(115200);

  Serial.println(F("Going to connect..."));
  WiFi.begin(ssid, password);
  while (! WiFi.isConnected()){
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
    Serial.println(F("Fucked WiFi!"));
  }
  Serial.println(WiFi.localIP());
}

void agent_handshake(WiFiClient &client, uint32_t &id){
    for (char i = 0; i < 4; i++)
      client.read();
      
    void * id_p = &id;  
    for (char i = 0; i < 4; i++)
      client.write(((char*)id_p)[i]);
    
    for (char i = 0; i < 4; i++)
      client.read();
    for (char i = 0; i < 4; i++)
      client.write(0);  
}

void loop(){
  WiFiClient client;

/*
  while (true){
    Serial.println(F("Fuck You!"));
    Serial.println(WiFi.localIP());
    delay(1000); 
  } 
*/  
  if (!is_connected && !client.connect(host, port1)){
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
  }
  
  if(client.connected() && !is_connected){
    agent_handshake(client, id);
    is_connected = true;
  }
  
  while (client.connected()) {
    if (client.available()){
      
      uint32_t cnt;
      for (uint8_t i = 0; i < 4; i++)
        ((char*)(&cnt))[i] = client.read();

      Serial.print("AWAITING:");
      Serial.print(cnt);
      Serial.println();

      while (cnt != 0){
        for (uint8_t i = 0; i = client.available(); i++){
          Serial.print(client.read());
          Serial.print(" ");
          cnt--;
        }
      }
      Serial.println();
    }
  delay(200);
 //   if(!keep_alive_counter++)client.write(0);
  }
  is_connected = false;
}

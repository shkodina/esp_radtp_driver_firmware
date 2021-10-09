#include <avr/pgmspace.h>
#include "ESP8266WiFi.h"

#define DEBUG
#define WAIT_DELAY_FOR_RE_CONNECT_ms 1000

const char *ssid     = "Microel5250";
const char *password = "92021044";

const char* host      = "192.168.0.100";
uint16_t    data_port = 7000;  //  port0
uint16_t    cmd_port  = 7001;  //  port1
uint32_t    id        = 1;

uint8_t keep_alive_counter = 128;

//=================================================================

void setup (){
  Serial.begin(115200);

  #ifdef DEBUG 
    Serial.println(F("Going to connect..."));
  #endif
  
  WiFi.begin(ssid, password);
  
  while (! WiFi.isConnected()){
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
    #ifdef DEBUG
      Serial.println(F("Waiting Fucking WiFi!"));
    #endif
  }
  
  Serial.println(WiFi.localIP());
}

//=================================================================

void agent_handshake(WiFiClient &drv, uint32_t &id){
    #ifdef DEBUG
      Serial.println(F("Handshaking..."));
    #endif
    for (char i = 0; i < 4; i++)
      drv.read();
      
    void * id_p = &id;  
    for (char i = 0; i < 4; i++)
      drv.write(((char*)id_p)[i]);
    
    for (char i = 0; i < 4; i++)
      drv.read();
    for (char i = 0; i < 4; i++)
      drv.write(0);  
}

//=================================================================
#define CMD_BUF_LEN        64

#define TO_AGENT_CONNECTED 1  //  1
#define TIME_TO_HEAD       2  //  1 << 1
#define TIME_TO_BODY       4  //  1 << 2

//=================================================================

WiFiClient drv_cmd;
uint8_t cmd_buf [CMD_BUF_LEN];
void process_drv_cmd (){
  #ifdef DEBUG
    static uint32_t entrance = 0;
    entrance++;
  #endif
  static uint32_t is = 0;
  if (!drv_cmd.connected()) {
    drv_cmd.stop();
    is &= ~TO_AGENT_CONNECTED;
  }

  while (!drv_cmd.connected()) {
    drv_cmd.connect(host, cmd_port);
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
  }

   if (drv_cmd.connected() && !(is & TO_AGENT_CONNECTED)) {
    agent_handshake(drv_cmd, id);
    is |= TO_AGENT_CONNECTED;
    is |= TIME_TO_HEAD;
  }  

  if (drv_cmd.connected() && (is & TO_AGENT_CONNECTED)) {
    uint32_t c = 0;
    while (drv_cmd.available()) {
      
      cmd_buf[c++] = drv_cmd.read();

      if (!drv_cmd.available()){

        #ifdef DEBUG
          for (uint32_t i = 0; i < c; i++){
            Serial.print(cmd_buf[i]);
            Serial.print(" ");            
          }
        #endif
        
        if ( is & TIME_TO_HEAD ) {
          uint32_t wc = 0;
          for (char i = 0; i < 4; i++){
            wc += (cmd_buf[i] << (8 * i));
          }
          if (wc > 0){
            is &= ~TIME_TO_HEAD;
            is |= TIME_TO_BODY;
          }
            
          #ifdef DEBUG
            Serial.print("Awaiting : ");
            Serial.println(wc);
          #endif
        }else if ( is & TIME_TO_BODY ) {    
          #ifdef DEBUG
            Serial.print("It was body by len: ");
            Serial.println(c);
          #endif
          is |= TIME_TO_HEAD;
          is &= ~TIME_TO_BODY;
        }

        #ifdef DEBUG
          Serial.print("Entarnce to process_drv_cmd = ");
          Serial.println(entrance);
        #endif
      }
    }
/*    if (drv_cmd.available()) {
      uint32_t cnt;
      for (uint8_t i = 0; i < 4; i++)
        ((char*)(&cnt))[i] = drv_cmd.read();

      Serial.print("AWAITING:");
      Serial.print(cnt);
      Serial.println();

      while (cnt != 0){
        Serial.print(drv_cmd.read());
        Serial.print(" ");
        cnt--;
      }
      Serial.println();       
     }
     */

  }  

  
}

//=================================================================

WiFiClient drv_data;
void process_drv_data (){
  
}  



void loop(){
  process_drv_cmd();
  process_drv_data();
}

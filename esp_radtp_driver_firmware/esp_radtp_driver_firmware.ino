#include <avr/pgmspace.h>
#include "radtp_packet_defines.h"
#include "ESP8266WiFi.h"

#define DEBUG
#define WAIT_DELAY_FOR_RE_CONNECT_ms 1000

const char *ssid     = "Microel5250";
const char *password = "92021044";

const char* host      = "192.168.0.101";
uint16_t    data_port = 7000;  //  port0
uint16_t    cmd_port  = 7001;  //  port1
uint32_t    id        = 1;

uint8_t keep_alive_counter = 128;

uint32_t g_is = 0;  //  global flags holder
#define KEEP_ALIVE_CAME 1  //  1 << 0

//=================================================================


bool check_wifi () {
  while (! WiFi.isConnected()){
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
    #ifdef DEBUG
      Serial.println(F("Waiting Fucking WiFi!"));
    #endif
    if (WiFi.isConnected())
      Serial.println(WiFi.localIP());
  }

  return WiFi.isConnected();
}


//=================================================================

void setup (){
  Serial.begin(115200);

  #ifdef DEBUG
    Serial.println(F("Going to connect..."));
  #endif

  WiFi.begin(ssid, password);
}

//=================================================================

void agent_handshake(WiFiClient &drv, uint32_t &id){
    #ifdef DEBUG
      Serial.println(F("Handshaking..."));
    #endif
    while (!drv.available());
    for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
      drv.read();

    void * id_p = &id;
    for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
      drv.write(((char*)id_p)[i]);

    while (!drv.available());
    for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
      drv.read();
    for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
      drv.write(0);
}

//=================================================================
#define CMD_BUF_LEN        64

#define TO_AGENT_CONNECTED 1  //  1
#define HEAD_CAME_BEFORE   2  //  1 << 1


//=================================================================

WiFiClient drv_cmd;
uint8_t cmd_buf [CMD_BUF_LEN];

uint32_t get_wc_from_cmd_buf(){
  uint32_t wc = 0;
  for (uint8_t i = 0; i < PKT_HEAD_LEN; i++){
    wc += (cmd_buf[i] << (8 * i));
  }
  return wc;
}

void process_drv_cmd (){
  #ifdef DEBUG
    static uint32_t entrance = 0;
    entrance++;
  #endif
  static uint32_t is = 0;
  static uint32_t wc = 0;
  if (!drv_cmd.connected()) {
    drv_cmd.stop();
    is &= ~TO_AGENT_CONNECTED;
  }

  while (!drv_cmd.connected()) {
    #ifdef DEBUG
      Serial.println(F("Connecting to agent..."));
    #endif
    drv_cmd.connect(host, cmd_port);
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
  }

   if (drv_cmd.connected() && !(is & TO_AGENT_CONNECTED)) {
    agent_handshake(drv_cmd, id);
    is |= TO_AGENT_CONNECTED;
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

        if ( c == PKT_HEAD_LEN ) {
          wc = get_wc_from_cmd_buf();
          is |= HEAD_CAME_BEFORE;
          #ifdef DEBUG
            Serial.print(F("HEAD ONLY : Awaiting : "));
            Serial.println(wc);
          #endif
        }else if ( c > PKT_HEAD_LEN) {

          if (is & HEAD_CAME_BEFORE) {
            c += PKT_HEAD_LEN;
          }else{
            wc = get_wc_from_cmd_buf();
          }

          if ( c != wc + PKT_HEAD_LEN ) {
            #ifdef DEBUG
              Serial.println("Bad Packet!!!");
            #endif
            wc = 0;
            is &= ~HEAD_CAME_BEFORE;
            return;
          }

          #ifdef DEBUG
            Serial.print("FULL PACKET. Len : ");
            Serial.println(c);
          #endif

          uint32_t pos = 0;
          if ( ! (is & HEAD_CAME_BEFORE) ){
              pos = PKT_TYPE_POSITION;
          }
          is &= ~HEAD_CAME_BEFORE;

          if (cmd_buf[pos] == 254){
            #ifdef DEBUG
              Serial.println("It is KEEP_ALIVE");
            #endif
            g_is |= KEEP_ALIVE_CAME;
            return;
          }
        }

        #ifdef DEBUG
          Serial.print("Entarnce to process_drv_cmd = ");
          Serial.println(entrance);
        #endif
      }
    }
  }
}

//=================================================================

uint8_t keep_alive_pkt [28] = {24, 0, 0, 0, 254, 1, 0, 24, 0, 1, 2, 65, 66, 2, 210, 31, 75, 97, 7, 209, 7, 0, 0, 7, 1, 0, 0, 0};
void send_keep_alive_pkt (WiFiClient &drv){
  for ( uint8_t i = 0; i < 28; i++){
      drv.write(keep_alive_pkt[i]);
  }
}

//=================================================================

WiFiClient drv_data;
void process_drv_data (){
  #ifdef DEBUG
    static uint32_t entrance = 0;
    entrance++;
  #endif
  static uint32_t is = 0;
  if (!drv_data.connected()) {
    drv_data.stop();
    is &= ~TO_AGENT_CONNECTED;
  }

  while (!drv_data.connected()) {
    #ifdef DEBUG
      Serial.println(F("Connecting to agent..."));
    #endif
    drv_data.connect(host, data_port);
    delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
  }

   if (drv_data.connected() && !(is & TO_AGENT_CONNECTED)) {
    agent_handshake(drv_data, id);
    is |= TO_AGENT_CONNECTED;
  }

  if (drv_data.connected() && (is & TO_AGENT_CONNECTED)) {

    if ( g_is & KEEP_ALIVE_CAME ) {
      g_is &= ~KEEP_ALIVE_CAME;
      send_keep_alive_pkt(drv_data);
      #ifdef DEBUG
          Serial.print("Entarnce to process_drv_data send KEEP_ALIVE = ");
          Serial.println(entrance);
      #endif
    }

    uint32_t c = 0;
    while (drv_data.available()) {

      cmd_buf[c++] = drv_data.read();

      if (!drv_data.available()){
        #ifdef DEBUG
          for (uint32_t i = 0; i < c; i++){
            Serial.print(cmd_buf[i]);
            Serial.print(" ");
          }
        #endif


        #ifdef DEBUG
          Serial.print("Entarnce to process_drv_data = ");
          Serial.println(entrance);
        #endif
      }
    }
  }
}



void loop(){
  check_wifi();
  process_drv_cmd();
  process_drv_data();

}

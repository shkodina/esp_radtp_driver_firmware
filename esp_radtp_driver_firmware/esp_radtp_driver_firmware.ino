#include <avr/pgmspace.h>

#include "radtp_packet_defines.h"
#include "radtp_packet_utils.h"

#include "ESP8266WiFi.h"

#define DEBUG

const char *ssid     = "Microel5250";
const char *password = "92021044";
#define WAIT_DELAY_FOR_RE_CONNECT_ms 1000


const char* host          = "192.168.0.101";
uint16_t        data_port = 7000;    //    port0
uint16_t        cmd_port  = 7001;    //    port1
uint32_t        id        = 1;


pkt_t pkt;
WiFiClient drv_cmd;
WiFiClient drv_data;
#define CMD_BUF_LEN                64
uint8_t cmd_buf [CMD_BUF_LEN];


uint32_t g_is = 0;           //    global flags holder
#define KEEP_ALIVE_CAME 1    //    1 << 0


#define TO_AGENT_CONNECTED 1    //    1

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

bool parse_buf_to_pkt (uint8_t buf, uint32_t buf_len, pkt_t & pkt) {
    uint32_t p = 0;
	
	uint32_t  wc = from_buf_to_uint32(buf, p);
	if ( wc == c - PKT_HEAD_LEN) {  // good packet with head
		p = PKT_TYPE_POSITION;
	}else if( wc == pkt.wc ) {  // good packet, but head came before
		p = 0;
	} else { // something wrong
		return false;
	}
	
	pkt.type = buf[p];
	p += PKT_POSITION_L;
	p += PKT_TYPE_L; 
    p += PKT_RESERVE_L;
   	
	uint16_t second_pkt_len = from_buf_to_uint16(buf, p);
	p += PKT_LEN2_L;
	
	switch ( p )
      {
         case 'A':
            uppercase_A++;
            break;
         case 'a':
            lowercase_a++;
            break;
         default:
            other++;
      }
    return true;
}

//=================================================================

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
                    Serial.print(F("Entarnce to process_drv_cmd = "));
                    Serial.println(entrance);
					Serial.print(F("RAW Data : "));
                    for (uint32_t i = 0; i < c; i++){
                        Serial.print(cmd_buf[i]);
                        Serial.print(" ");
                    }
					Serial.println();
                #endif

                if ( c == PKT_HEAD_LEN ) 
				{
					pkt.wc = from_buf_to_uint32(cmd_buf, 0);
                    #ifdef DEBUG
                        Serial.print(F("HEAD ONLY : Awaiting : "));
                        Serial.println(pkt.wc);
                    #endif
                }
				else if ( c > PKT_HEAD_LEN) 
				{
			
					if ( ! parse_buf_to_pkt(cmd_buf, c, pkt) ) {
						#ifdef DEBUG
							Serial.println(F("Bad Packet!!!"));
						#endif
						pkt_clean_up(pkt);
						return;			
					}

                    #ifdef DEBUG
                        Serial.print(F("FULL PACKET. Len : "));
                        Serial.println(c);
                    #endif

                    if ( pkt.type == PKT_TYPE_KEEP_ALIVE){
                        #ifdef DEBUG
                            Serial.println(F("It is KEEP_ALIVE"));
                        #endif
                        g_is |= KEEP_ALIVE_CAME;
                    }else if ( pkt.type == PKT_TYPE_CMD ) {
                        #ifdef DEBUG
                            Serial.println(F("It is CMD"));
							Serial.print(F("WC = "));
							Serial.println(pkt.wc);
 							Serial.print(F("CODE = "));
							Serial.println(pkt.cmd_event);
							Serial.print(F("KKS = "));
							for (uint8_t i = 0; i < pkt.kks_len; i++)
								Serial.print(pkt.kks[i]);
                            Serial.println();
                        #endif		
					}
					pkt_clean_up(pkt);
                }
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

#include <avr/pgmspace.h>

#define IMITATION

#include "radtp_packet_defines.hh"
#include "radtp_packet_utils.hh"

#include "ESP8266WiFi.h"

const char *ssid     = "Microel5250";
const char *password = "92021044";
#define WAIT_DELAY_FOR_RE_CONNECT_ms 1000

//har this_kks[5] = {'E','S','P','0','1'};
char this_kks[] = "ESP01";
uint8_t this_kks_len = 5;
uint32_t this_timestamp = 0;

const char* host          = "192.168.0.101";
uint16_t        data_port = 7000;    //    port0
uint16_t        cmd_port  = 7001;    //    port1
uint32_t        id        = 1;

WiFiClient drv_cmd;
WiFiClient drv_data;

pkt_t pkt;
#define CMD_BUF_LEN                  64
uint8_t cmd_buf [CMD_BUF_LEN];
uint8_t cmd_buf_clen = EMPTY;

pkt_t pkt_reply;
#define REPLY_BUF_LEN                32
uint8_t reply_buf [CMD_BUF_LEN];
uint8_t reply_buf_clen = EMPTY;

pkt_t pkt_event;
#define EVENT_BUF_LEN                32
uint8_t event_buf [EVENT_BUF_LEN];
uint8_t event_buf_clen = EMPTY;

pkt_t pkt_mea;
#define MEA_BUF_LEN                32
uint8_t mea_buf [MEA_BUF_LEN];
uint8_t mea_buf_clen = EMPTY;

uint32_t g_is = EMPTY;          //    global flags holder
#define KEEP_ALIVE_CAME    1    //    1 << 0
#define TIME_TO_SEND_REPLY 2    //    1 << 1
#define TIME_TO_SEND_EVENT 4    //    1 << 2
#define TIME_TO_SEND_MEA   8    //    1 << 3


// FLAGS for local usage is "drv_cmd" and "drv_data"
#define TO_AGENT_CONNECTED 1    //    1
#define MULTY_PACKET       2    //    1 << 1

//=================================================================

bool check_wifi () {
    while (! WiFi.isConnected()){
        delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
        #ifdef DEBUG_1
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

    #ifdef DEBUG_1
	Serial.println(F("Going to connect..."));
    #endif

    WiFi.begin(ssid, password);
}

//=================================================================

bool agent_handshake(WiFiClient &drv, uint32_t &id){
	uint16_t wait = 65535;
	#if defined DEBUG_1 || defined DEBUG_2
	Serial.println(F("Handshaking..."));
	#endif

	while (!drv.available() && wait) wait--;
	if (! wait ) return false;
	
	for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
		drv.read();

	void * id_p = &id;
	for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
		drv.write(((char*)id_p)[i]);

	wait = 65535;
	while (!drv.available() && wait) wait--;
	if (! wait ) return false;

	for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
		drv.read();
	for (uint8_t i = 0; i < PKT_HEAD_LEN; i++)
		drv.write(0);
	
	return true;
}

//=================================================================

void check_pin_event () {
	#ifdef IMITATION
	static uint32_t c = 0;
	static uint16_t evt = 10;
	
	if ( c++ == 400000) {
		from_str_buf_to_str_buf(this_kks, this_kks_len, pkt_event.kks);
		pkt_event.kks_len = this_kks_len;
		pkt_event.timestamp = this_timestamp;
		pkt_event.cmd_event = evt++;
		
		g_is |= TIME_TO_SEND_EVENT;
		c = 0;
	}
	#endif
	return;
}

//=================================================================

void check_pin_mea () {
	#ifdef IMITATION
	static uint32_t c = 0;
	static uint16_t mea = 10;
	
	if ( c++ == 200000) {
		from_str_buf_to_str_buf(this_kks, this_kks_len, pkt_mea.kks);
		pkt_mea.kks_len = this_kks_len;
		pkt_mea.timestamp = this_timestamp;
		pkt_mea.mea = mea++;
		
		g_is |= TIME_TO_SEND_MEA;
		c = 0;
	}
	#endif
	return;
}

//=================================================================

void process_cmd (pkt_t * pkt, pkt_t * pkt_reply){
	#ifdef DEBUG_2
	Serial.println(F("It is CMD"));
	Serial.print(F("WC = "));
	Serial.println(pkt->wc);
	Serial.print(F("CODE = "));
	Serial.println(pkt->cmd_event);
	Serial.print(F("KKS = "));
	for (uint8_t i = 0; i < pkt->kks_len; i++) 
		Serial.print(pkt->kks[i]);
	Serial.println();
	for (uint8_t i = 0; i < pkt->cmd_params_count; i++){
		Serial.print(F("Parameter ["));
		Serial.print(i+1);
		Serial.print(F("] = "));
		Serial.println(pkt->cmd_params[i]);
	}
	#endif	

    pkt_clean_up(pkt_reply);
	from_str_buf_to_str_buf(pkt->kks,  pkt->kks_len, pkt_reply->kks);
	pkt_reply->kks_len = pkt->kks_len;
	pkt_reply->cmd_event = pkt->cmd_event + 1;
	pkt_reply->timestamp = pkt->timestamp;
	g_is |= TIME_TO_SEND_REPLY;
	
	return;
}

//=================================================================

void pkt_ready_checker() {
	if ( g_is & TIME_TO_SEND_REPLY) {
		reply_buf_clen = pkt_build_reply_buffer_for_send (&pkt_reply, reply_buf);	
	}
	
	if ( g_is & TIME_TO_SEND_EVENT) {
		event_buf_clen = pkt_build_event_buffer_for_send (&pkt_event, event_buf);	
	}
	
	if ( g_is & TIME_TO_SEND_MEA) {
		mea_buf_clen = pkt_build_mea_buffer_for_send (&pkt_mea, mea_buf);	
	}
}

//=================================================================

void process_drv_cmd_pkt (pkt_t * pkt, pkt_t * pkt_reply){
	if ( pkt->type == PKT_TYPE_KEEP_ALIVE){

		#ifdef DEBUG_1
		Serial.println(F("It is KEEP_ALIVE"));
		#endif
		
		this_timestamp = pkt->timestamp;

		g_is |= KEEP_ALIVE_CAME;
	}
	
	if ( pkt->type == PKT_TYPE_CMD ) {
		process_cmd(pkt, pkt_reply);
	}

}

//=================================================================

void process_drv_cmd (){
	
    #ifdef DEBUG_1
	static uint32_t entrance = EMPTY;
	entrance++;
    #endif
	
    static uint32_t is = EMPTY;

    if (!drv_cmd.connected()) {
        drv_cmd.stop();
        is &= ~TO_AGENT_CONNECTED;
    }

    while (!drv_cmd.connected()) {
        #ifdef DEBUG_2
		Serial.println(F("Connecting to agent..."));
        #endif
        drv_cmd.connect(host, cmd_port);
        delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
    }

     if (drv_cmd.connected() && !(is & TO_AGENT_CONNECTED)) {
        if (!agent_handshake(drv_cmd, id)){
			drv_cmd.stop();
			return;
		}
        is |= TO_AGENT_CONNECTED;
    }

    if (drv_cmd.connected() && (is & TO_AGENT_CONNECTED)) {
        uint32_t c = EMPTY;
		
		if ( is & MULTY_PACKET ) {
			c = cmd_buf_clen;
		}
		
        while (drv_cmd.available() && !(is & MULTY_PACKET)) {
            cmd_buf[c++] = drv_cmd.read();
			cmd_buf_clen = c;
        }


		#ifdef DEBUG_1
		if ( c > 0 ) {
    		Serial.print(F("c = "));
			Serial.println(c);
			Serial.print(F("Entarnce to process_drv_cmd = "));
			Serial.println(entrance);
			Serial.print(F("RAW Data : "));
			for (uint32_t i = 0; i < c; i++){
				Serial.print(cmd_buf[i]);
				Serial.print(" ");
			}
			Serial.println();
		}
		#endif

        is &= ~MULTY_PACKET;  //  reset multy bufer.. if need it will up again in a future
		
		if ( c == PKT_HEAD_LEN ) 
		{
			pkt.wc = from_buf_to_uint32(cmd_buf, 0);
			#ifdef DEBUG_2
			Serial.print(F("HEAD ONLY : Awaiting : "));
			Serial.println(pkt.wc);
			#endif
		}
		else if ( c > PKT_HEAD_LEN) 
		{
			uint8_t parser_result = parse_buf_to_pkt(cmd_buf, c, &pkt);
			
			if ( parser_result == PKT_PARSER_RESULT_ERROR ) 
			{	
				#ifdef DEBUG_2
				Serial.println(F("Bad Packet!!!"));
				#endif
				
				pkt_clean_up(&pkt);
				return;			
			}
			
			if ( parser_result == PKT_PARSER_RESULT_MULTY_PKT ){
				is |= MULTY_PACKET;
				shift_buffer(cmd_buf, pkt.wc, CMD_BUF_LEN);
				cmd_buf_clen -= pkt.wc;
			}

			#ifdef DEBUG_1
			Serial.print(F("FULL PACKET. Len : "));
			Serial.println(c);
			#endif

			process_drv_cmd_pkt (&pkt, &pkt_reply);
			pkt_clean_up(&pkt);
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

void send_reply_pkt (WiFiClient &drv){
    for ( uint8_t i = 0; i < reply_buf_clen; i++){
            drv.write(reply_buf[i]);
    }
}

//=================================================================

void send_event_pkt (WiFiClient &drv){
    for ( uint8_t i = 0; i < event_buf_clen; i++){
            drv.write(event_buf[i]);
    }
}

//=================================================================

void send_mea_pkt (WiFiClient &drv){
    for ( uint8_t i = 0; i < mea_buf_clen; i++){
            drv.write(mea_buf[i]);
    }
}

//=================================================================

void process_drv_data (){
    #ifdef DEBUG_1
        static uint32_t entrance = 0;
        entrance++;
    #endif
    static uint32_t is = 0;
    if (!drv_data.connected()) {
        drv_data.stop();
        is &= ~TO_AGENT_CONNECTED;
    }

    while (!drv_data.connected()) {
        #ifdef DEBUG_2
            Serial.println(F("Connecting to agent..."));
        #endif
        drv_data.connect(host, data_port);
        delay(WAIT_DELAY_FOR_RE_CONNECT_ms);
    }

     if (drv_data.connected() && !(is & TO_AGENT_CONNECTED)) {
        if (!agent_handshake(drv_data, id)){
			drv_data.stop();
			return;
		}
        is |= TO_AGENT_CONNECTED;
    }

    if (drv_data.connected() && (is & TO_AGENT_CONNECTED)) {

        if ( g_is & KEEP_ALIVE_CAME ) {
            g_is &= ~KEEP_ALIVE_CAME;
            send_keep_alive_pkt(drv_data);
            #ifdef DEBUG_1
            Serial.print("Entarnce to process_drv_data send KEEP_ALIVE = ");
            Serial.println(entrance);
            #endif
        }
		
		if ( g_is & TIME_TO_SEND_REPLY) {
			g_is &= ~TIME_TO_SEND_REPLY;
            send_reply_pkt(drv_data);
            #ifdef DEBUG_1
            Serial.print("Entarnce to process_drv_data send REPLY = ");
            Serial.println(entrance);
            #endif			
		}

		if ( g_is & TIME_TO_SEND_EVENT) {
			g_is &= ~TIME_TO_SEND_EVENT;
            send_event_pkt(drv_data);
            #ifdef DEBUG_1
            Serial.print("Entarnce to process_drv_data send EVENT = ");
            Serial.println(entrance);
            #endif			
		}

		if ( g_is & TIME_TO_SEND_MEA) {
			g_is &= ~TIME_TO_SEND_MEA;
            send_mea_pkt(drv_data);
            #ifdef DEBUG_1
            Serial.print("Entarnce to process_drv_data send MEASUREMENT = ");
            Serial.println(entrance);
            #endif			
		}

        uint32_t c = 0;
        while (drv_data.available()) {

            cmd_buf[c++] = drv_data.read();

            if (!drv_data.available()){
                #ifdef DEBUG_1
                    for (uint32_t i = 0; i < c; i++){
                        Serial.print(cmd_buf[i]);
                        Serial.print(" ");
                    }
                #endif


                #ifdef DEBUG_1
                    Serial.print("Entarnce to process_drv_data = ");
                    Serial.println(entrance);
                #endif
            }
        }
    }
}

//=================================================================

void loop()
{
    check_wifi();
	
    process_drv_cmd();
	
	check_pin_event();
	check_pin_mea();
	
	pkt_ready_checker();
	
    process_drv_data();
}

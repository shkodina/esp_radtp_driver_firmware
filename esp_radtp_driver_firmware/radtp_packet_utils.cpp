#include <stdint.h>

#include "radtp_packet_defines.hh"
#include "radtp_packet_utils.hh"

//==================================================================

#ifdef DEBUG
	#include <Arduino.h>
#endif

//==================================================================

uint32_t from_buf_to_uint32 (uint8_t buf[], uint32_t pos) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < 4; i++) {
      v += (buf[pos + i] << (8 * i));
    }
    return v;
}

//=================================================================

uint16_t from_buf_to_uint16 (uint8_t buf[], uint32_t pos) {
    uint16_t v = 0;
    for (uint8_t i = 0; i < 2; i++) {
      v += (buf[pos + i] << (8 * i));
    }
    return v;
}

//=================================================================

uint8_t from_buf_to_str_buf (uint8_t buf[], uint32_t pos, char str[]) {
	uint8_t str_len = buf[pos];
	pos++;
	for ( uint8_t i = 0; i < str_len; i++){
		str[i] = buf[pos + i];
	}
	return str_len;
}

//=================================================================

void pkt_clean_up (pkt_t * pkt){
	pkt->wc = 0;
	pkt->type = 0;
    pkt->timestamp = 0;
	for (uint8_t i = 0; i < PKT_KKS_MAX_LEN; i++)
		pkt->kks[i] = 0;
	pkt->kks_len = 0;
    pkt->cmd_event = 0;
	pkt->mea = 0;
	
	for (uint8_t i = 0; i < CMD_MAX_PARAM_COUNT; i++)
		pkt->cmd_params[i] = 0;
	pkt->cmd_params_count = 0;
}

//=================================================================

uint8_t parse_buf_to_pkt (uint8_t buf[], uint32_t buf_len, pkt_t * pkt) {  // return количество пакетов в буфере ( 0, 1 или больше 1) 
    uint32_t p = 0;
	
	uint32_t  wc = from_buf_to_uint32(buf, p);
	if ( wc == buf_len - PKT_HEAD_LEN) {  // good packet with head
	    pkt->wc = wc;
		p = PKT_TYPE_POSITION;
	}else if( buf_len == pkt->wc ) {  // good packet, but head came before
		p = 0;
	} else { // something wrong
		#ifdef DEBUG
			Serial.println(F("...something wrong..."));
		#endif	
		// TODO оказывается может прийти несколько подряд пакетов и тогда их нужно раздерабанить и обработать последовательно
		return 0;
	}
	
	pkt->type = buf[p];
	p += PKT_TYPE_L; 
    p += PKT_RESERVE_L;
   	
	uint16_t second_pkt_len = from_buf_to_uint16(buf, p);
	p += PKT_LEN2_L;
	#ifdef DEBUG
		Serial.print(F("PKT_LEN2 : "));
		Serial.println(second_pkt_len);
		Serial.print(F("buf_pos : "));
		Serial.println(p);
		Serial.print(F("buf[pos] : "));
		Serial.println(buf[p]);
	#endif

    while (p < second_pkt_len) {
		switch ( buf[p] )
		{
			case PKT_ATTR_CODE_KKS:
				p += PKT_POSITION_L;
				pkt->kks_len = from_buf_to_str_buf(buf, p, pkt->kks);
				p += pkt->kks_len;
				p += 1;
				break;
			case PKT_ATTR_CODE_TIMESTAMP:
			    p += PKT_POSITION_L;
				pkt->timestamp = from_buf_to_uint32(buf, p);
				p += PKT_ATTR_CODE_TIMESTAMP_L;
				break;
			case PKT_ATTR_CODE_FValue:  //  ignore float param
			    p += PKT_POSITION_L;
				p += PKT_ATTR_CODE_FValue_L;
				break;
			case PKT_ATTR_CODE_DValue:  //  ignore double param
			    p += PKT_POSITION_L;
				p += PKT_ATTR_CODE_DValue_L;
				break;
			case PKT_ATTR_CODE_BValue:
			    p += PKT_POSITION_L;
				if (pkt->cmd_params_count != CMD_MAX_PARAM_COUNT){
					pkt->cmd_params[pkt->cmd_params_count] = buf[p];
					pkt->cmd_params_count += 1;
				}
				p += PKT_ATTR_CODE_BValue_L;
				break;
			case PKT_ATTR_CODE_WValue:
			    p += PKT_POSITION_L;
				if (pkt->cmd_params_count != CMD_MAX_PARAM_COUNT){
					pkt->cmd_params[pkt->cmd_params_count] = from_buf_to_uint16(buf, p);
					pkt->cmd_params_count += 1;
				}
				p += PKT_ATTR_CODE_WValue_L;
				break;
			case PKT_ATTR_CODE_LValue:
			    p += PKT_POSITION_L;
				if (pkt->cmd_params_count != CMD_MAX_PARAM_COUNT){
					pkt->cmd_params[pkt->cmd_params_count] = from_buf_to_uint32(buf, p);
					pkt->cmd_params_count += 1;
				}
				p += PKT_ATTR_CODE_LValue_L;
				break;
			case PKT_ATTR_CODE_SValue:  //  ignore string param
			    p += PKT_POSITION_L;
				p += buf[p];
				p += 1;
				break;
			case PKT_ATTR_CODE_BLBValue:  //  ignore string param
			    p += PKT_POSITION_L;
				p += from_buf_to_uint16(buf, p);
				p += 2;
				break;
			case PKT_ATTR_CODE_ALARM:  //  ignore alarm param
			    p += PKT_POSITION_L;
				p += PKT_ATTR_CODE_ALARM_L;
				break;
			case PKT_ATTR_CODE_EVENT:
			    p += PKT_POSITION_L;
				pkt->cmd_event = from_buf_to_uint16(buf, p);
				p += PKT_ATTR_CODE_EVENT_L;
				break;
			case PKT_ATTR_CODE_QUALITY:  //  ignore quality param
			    p += PKT_POSITION_L;
				p += PKT_ATTR_CODE_QUALITY_L;
				break;
			default:
				#ifdef DEBUG
					Serial.print(F("Wrong Attr code : "));
					Serial.println(buf[p]);
				#endif
				return 0;
		}
	}
    return 1;
}

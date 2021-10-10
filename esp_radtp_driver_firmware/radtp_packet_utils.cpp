#include <stdint.h>

#include "radtp_packet_defines.hh"
#include "radtp_packet_utils.hh"

//==================================================================

#if defined DEBUG || defined DEBIG_1 || defined DEBUG_2
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

uint8_t from_str_buf_to_str_buf (char buf[], uint32_t count, char str[]) {
	for ( uint8_t i = 0; i < count; i++){
		str[i] = buf[i];
	}
	return count;
}

//=================================================================

void pkt_clean_up (pkt_t * pkt){
	pkt->wc = EMPTY;
	pkt->type = EMPTY;
    pkt->timestamp = EMPTY;
	for (uint8_t i = 0; i < PKT_KKS_MAX_LEN; i++)
		pkt->kks[i] = EMPTY;
	pkt->kks_len = EMPTY;
    pkt->cmd_event = EMPTY;
	pkt->mea = EMPTY;
	
	for (uint8_t i = 0; i < CMD_MAX_PARAM_COUNT; i++)
		pkt->cmd_params[i] = EMPTY;
	pkt->cmd_params_count = EMPTY;
}

//=================================================================

void shift_buffer(uint8_t buf[], uint8_t shift, uint8_t buf_full_len) {
	for (uint8_t i = 0; i < shift; i++)
		buf[i] = buf[i+shift];
	for (uint8_t i = shift; i < buf_full_len; i++)
		buf[i] = EMPTY;
}

//=================================================================

uint8_t parse_buf_to_pkt (uint8_t buf[], uint32_t buf_len, pkt_t * pkt) {  // return количество пакетов в буфере ( 0, 1 или больше 1) 
    uint32_t current_position = 0;
	uint8_t return_result = PKT_PARSER_RESULT_ERROR;
	
	uint32_t  wc = from_buf_to_uint32(buf, current_position);
	if ( wc == buf_len - PKT_HEAD_LEN) {  // good packet with head
	    pkt->wc = wc;
		current_position = PKT_TYPE_POSITION;
		return_result = PKT_PARSER_RESULT_SINGLE_PKT;
	}else if( buf_len == pkt->wc ) {  // good packet, but head came before
		current_position = 0;
		return_result = PKT_PARSER_RESULT_SINGLE_PKT;
	} else { // something wrong
		#ifdef DEBUG_2
		Serial.println(F("Multi packet may be..."));
		#endif	
		return_result = PKT_PARSER_RESULT_MULTY_PKT;
	}
	
	pkt->type = buf[current_position];
	current_position += PKT_TYPE_L; 
    current_position += PKT_RESERVE_L;
   	
	uint16_t second_pkt_len = from_buf_to_uint16(buf, current_position);
	current_position += PKT_LEN2_L;
	#ifdef DEBUG_1
		Serial.print(F("PKT_LEN2 : "));
		Serial.println(second_pkt_len);
		Serial.print(F("buf_pos : "));
		Serial.println(current_position);
		Serial.print(F("buf[pos] : "));
		Serial.println(buf[current_position]);
	#endif

    while (current_position < second_pkt_len) {
		switch ( buf[current_position] )
		{
			case PKT_ATTR_CODE_KKS:
				current_position += PKT_POSITION_L;
				pkt->kks_len = from_buf_to_str_buf(buf, current_position, pkt->kks);
				current_position += pkt->kks_len;
				current_position += 1;
				break;
				
			case PKT_ATTR_CODE_TIMESTAMP:
			    current_position += PKT_POSITION_L;
				pkt->timestamp = from_buf_to_uint32(buf, current_position);
				current_position += PKT_ATTR_CODE_TIMESTAMP_L;
				break;
				
			case PKT_ATTR_CODE_FValue:  //  ignore float param
			    current_position += PKT_POSITION_L;
				current_position += PKT_ATTR_CODE_FValue_L;
				break;
				
			case PKT_ATTR_CODE_DValue:  //  ignore double param
			    current_position += PKT_POSITION_L;
				current_position += PKT_ATTR_CODE_DValue_L;
				break;
				
			case PKT_ATTR_CODE_BValue:
			    current_position += PKT_POSITION_L;
				if (pkt->cmd_params_count != CMD_MAX_PARAM_COUNT){
					pkt->cmd_params[pkt->cmd_params_count] = buf[current_position];
					pkt->cmd_params_count += 1;
				}
				current_position += PKT_ATTR_CODE_BValue_L;
				break;
				
			case PKT_ATTR_CODE_WValue:
			    current_position += PKT_POSITION_L;
				if (pkt->cmd_params_count != CMD_MAX_PARAM_COUNT){
					pkt->cmd_params[pkt->cmd_params_count] = from_buf_to_uint16(buf, current_position);
					pkt->cmd_params_count += 1;
				}
				current_position += PKT_ATTR_CODE_WValue_L;
				break;
				
			case PKT_ATTR_CODE_LValue:
			    current_position += PKT_POSITION_L;
				if (pkt->cmd_params_count != CMD_MAX_PARAM_COUNT){
					pkt->cmd_params[pkt->cmd_params_count] = from_buf_to_uint32(buf, current_position);
					pkt->cmd_params_count += 1;
				}
				current_position += PKT_ATTR_CODE_LValue_L;
				break;
				
			case PKT_ATTR_CODE_SValue:  //  ignore string param
			    current_position += PKT_POSITION_L;
				current_position += buf[current_position];
				current_position += 1;
				break;
				
			case PKT_ATTR_CODE_BLBValue:  //  ignore string param
			    current_position += PKT_POSITION_L;
				current_position += from_buf_to_uint16(buf, current_position);
				current_position += 2;
				break;
				
			case PKT_ATTR_CODE_ALARM:  //  ignore alarm param
			    current_position += PKT_POSITION_L;
				current_position += PKT_ATTR_CODE_ALARM_L;
				break;
				
			case PKT_ATTR_CODE_EVENT:
			    current_position += PKT_POSITION_L;
				pkt->cmd_event = from_buf_to_uint16(buf, current_position);
				current_position += PKT_ATTR_CODE_EVENT_L;
				break;
				
			case PKT_ATTR_CODE_QUALITY:  //  ignore quality param
			    current_position += PKT_POSITION_L;
				current_position += PKT_ATTR_CODE_QUALITY_L;
				break;
				
			default:
				#ifdef DEBUG_2
					Serial.print(F("Wrong Attr code : "));
					Serial.println(buf[current_position]);
				#endif
				return PKT_PARSER_RESULT_ERROR;
		}
	}
    return return_result;
}

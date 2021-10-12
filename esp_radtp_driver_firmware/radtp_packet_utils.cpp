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
    for (uint8_t i = FROM_BEGINING; i < 4; i++) {
      v += (buf[pos + i] << (8 * i));
    }
    return v;
}

//=================================================================

uint16_t from_buf_to_uint16 (uint8_t buf[], uint32_t pos) {
    uint16_t v = 0;
    for (uint8_t i = FROM_BEGINING; i < 2; i++) {
      v += (buf[pos + i] << (8 * i));
    }
    return v;
}

//=================================================================

uint32_t from_uint16_to_buf  (uint16_t value, uint8_t buf[], uint32_t pos) {                   //  return new position for buffer pointer
  for ( uint8_t i = 0; i < 2; i++){
    buf[pos + i] = value;
    value = value >> 8;
  }
  return pos + 2;
}

//================================================================

uint32_t from_uint32_to_buf  (uint32_t value, uint8_t buf[], uint32_t pos) {                   //  return new position for buffer pointer
  for ( uint8_t i = 0; i < 4; i++){
    buf[pos + i] = value;
    value = value >> 8;
  }
  return pos + 4;
}

//=================================================================

uint8_t from_buf_to_str (uint8_t buf[], uint32_t pos, char str[]) {
	uint8_t str_len = buf[pos];
	pos++;
	for ( uint8_t i = FROM_BEGINING; i < str_len; i++){
		str[i] = buf[pos + i];
	}
	return str_len;
}

//=================================================================

uint32_t  from_str_to_buf (char str[], uint8_t str_len, uint8_t buf[], uint32_t buf_pos){
	buf[buf_pos++] = str_len;
	for (uint8_t i = FROM_BEGINING; i < str_len; i++){
		buf[buf_pos + i] = str[i];
	}
	return buf_pos + str_len;
}

//=================================================================

uint8_t from_str_buf_to_str_buf (char buf[], uint32_t count, char str[]) {
	for ( uint8_t i = FROM_BEGINING; i < count; i++){
		str[i] = buf[i];
	}
	return count;
}

//=================================================================

void pkt_clean_up (pkt_t * pkt){
	pkt->wc = EMPTY;
	pkt->type = EMPTY;
    pkt->timestamp = EMPTY;
	for (uint8_t i = FROM_BEGINING; i < PKT_KKS_MAX_LEN; i++)
		pkt->kks[i] = EMPTY;
	pkt->kks_len = EMPTY;
    pkt->cmd_event = EMPTY;
	pkt->mea = EMPTY;
	
	for (uint8_t i = FROM_BEGINING; i < CMD_MAX_PARAM_COUNT; i++)
		pkt->cmd_params[i] = EMPTY;
	pkt->cmd_params_count = EMPTY;
}

//=================================================================

void shift_buffer(uint8_t buf[], uint8_t shift, uint8_t buf_full_len) {
	for (uint8_t i = FROM_BEGINING; i < shift; i++)
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
				pkt->kks_len = from_buf_to_str(buf, current_position, pkt->kks);
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

//=============================================================================

uint32_t pkt_build_reply_buffer_for_send(pkt_t * pkt_reply, uint8_t reply_buf[]) {
	uint8_t p = PKT_TYPE_POSITION;
	
	reply_buf[p] = PKT_TYPE_CMD_REPLY;
	p += PKT_TYPE_L;
	
	p = from_uint16_to_buf(EMPTY, reply_buf, p);
	
	uint8_t p_len_2 = p;
	p += PKT_LEN2_L;
	
	// add kks
	reply_buf[p] = PKT_ATTR_CODE_KKS;
	p += PKT_POSITION_L;
	p = from_str_to_buf(pkt_reply->kks, pkt_reply->kks_len, reply_buf, p);
	
	// add timestamp
	reply_buf[p] = PKT_ATTR_CODE_TIMESTAMP;
	p += PKT_POSITION_L;
	p = from_uint32_to_buf(pkt_reply->timestamp, reply_buf, p);
	
	// add cmd_event
	reply_buf[p] = PKT_ATTR_CODE_EVENT;
	p += PKT_POSITION_L;
	p = from_uint16_to_buf(pkt_reply->cmd_event, reply_buf, p);
	
	// add PKT_LEN
	from_uint32_to_buf(p - PKT_HEAD_LEN, reply_buf, FROM_BEGINING);
	// add PKT_LEN 2
	from_uint16_to_buf(p - PKT_HEAD_LEN, reply_buf, p_len_2);
	
	#if defined DEBUG_1 
	Serial.print(F("RAW CMD_REPLY PKT : "));
	for (uint32_t i = 0; i < p; i++){
		Serial.print(reply_buf[i]);
		Serial.print(" ");
	}
	Serial.println();
	#endif		
	
	return p;
}

//=============================================================================

uint32_t pkt_build_event_buffer_for_send(pkt_t * pkt_event, uint8_t event_buf[]) {
	uint8_t p = PKT_TYPE_POSITION;
	
	event_buf[p] = PKT_TYPE_EVENT;
	p += PKT_TYPE_L;
	
	p = from_uint16_to_buf(EMPTY, event_buf, p);
	
	uint8_t p_len_2 = p;
	p += PKT_LEN2_L;
	
	// add kks
	event_buf[p] = PKT_ATTR_CODE_KKS;
	p += PKT_POSITION_L;
	p = from_str_to_buf(pkt_event->kks, pkt_event->kks_len, event_buf, p);
	
	// add timestamp
	event_buf[p] = PKT_ATTR_CODE_TIMESTAMP;
	p += PKT_POSITION_L;
	p = from_uint32_to_buf(pkt_event->timestamp, event_buf, p);
	
	// add cmd_event
	event_buf[p] = PKT_ATTR_CODE_EVENT;
	p += PKT_POSITION_L;
	p = from_uint16_to_buf(pkt_event->cmd_event, event_buf, p);
	
	// add PKT_LEN
	from_uint32_to_buf(p - PKT_HEAD_LEN, event_buf, FROM_BEGINING);
	// add PKT_LEN 2
	from_uint16_to_buf(p - PKT_HEAD_LEN, event_buf, p_len_2);
	
	#if defined DEBUG_1 
	Serial.print(F("RAW EVENT PKT : "));
	for (uint32_t i = 0; i < p; i++){
		Serial.print(event_buf[i]);
		Serial.print(" ");
	}
	Serial.println();
	#endif		
	
	return p;
}

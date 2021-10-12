#ifndef RADTP_PACKET_UTILS_FOR_ESP_PIPER
#define RADTP_PACKET_UTILS_FOR_ESP_PIPER

uint32_t from_buf_to_uint32  (uint8_t buf[], uint32_t pos);
uint16_t from_buf_to_uint16  (uint8_t buf[], uint32_t pos);
uint8_t  from_buf_to_str     (uint8_t buf[], uint32_t pos, char str[]);

uint32_t from_str_to_buf     (char str[], uint8_t str_len, uint8_t buf[], uint32_t buf_pos);  //  return new position for buffer pointer
uint32_t from_uint16_to_buf  (uint16_t value, uint8_t buf[], uint32_t pos);                   //  return new position for buffer pointer
uint32_t from_uint32_to_buf  (uint32_t value, uint8_t buf[], uint32_t pos);                   //  return new position for buffer pointer

uint8_t  from_str_buf_to_str_buf (char buf[], uint32_t pos, char str[]);


void     pkt_clean_up (pkt_t * pkt);
uint8_t  parse_buf_to_pkt (uint8_t buf[], uint32_t buf_len, pkt_t * pkt);  // return количество пакетов в буфере ( 0, 1 или больше 1) 
void     shift_buffer (uint8_t buf[], uint8_t shift, uint8_t buf_full_len);

uint32_t pkt_build_reply_buffer_for_send(pkt_t * pkt_reply, uint8_t reply_buf[]);
#endif
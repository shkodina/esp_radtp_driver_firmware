#ifndef RADTP_PACKET_UTILS_FOR_ESP_PIPER
#define RADTP_PACKET_UTILS_FOR_ESP_PIPER

uint32_t from_buf_to_uint32  (uint8_t buf[], uint32_t pos);
uint16_t from_buf_to_uint16  (uint8_t buf[], uint32_t pos);
uint8_t  from_buf_to_str_buf (uint8_t buf[], uint32_t pos, char str[]);

void     pkt_clean_up (pkt_t * pkt);
uint8_t  parse_buf_to_pkt (uint8_t buf[], uint32_t buf_len, pkt_t * pkt);  // return количество пакетов в буфере ( 0, 1 или больше 1) 
void shift_buffer(uint8_t buf[], uint8_t shift);
#endif
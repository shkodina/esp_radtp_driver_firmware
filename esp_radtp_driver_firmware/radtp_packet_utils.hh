#ifndef RADTP_PACKET_UTILS_FOR_ESP_PIPER
#define RADTP_PACKET_UTILS_FOR_ESP_PIPER

uint32_t from_buf_to_uint32  (uint8_t buf[], uint32_t pos);
uint16_t from_buf_to_uint16  (uint8_t buf[], uint32_t pos);
uint8_t  from_buf_to_str_buf (uint8_t buf[], uint32_t pos, char str[]);

void     pkt_clean_up (pkt_t * pkt);
#endif
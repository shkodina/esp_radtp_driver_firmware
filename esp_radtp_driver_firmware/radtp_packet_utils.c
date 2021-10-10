#include "radtp_packet_defines.h"
#include "radtp_packet_utils.h"



uint32_t from_buf_to_uint32 (uint8_t * buf, uint32_t pos) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < 4; i++) {
      v += (cmd_buf[pos + i] << (8 * i));
    }
    return v;
}



uint16_t from_buf_to_uint16 (uint8_t * buf, uint32_t pos) {
    uint16_t v = 0;
    for (uint8_t i = 0; i < 2; i++) {
      v += (cmd_buf[pos + i] << (8 * i));
    }
    return v;
}



void pkt_clean_up (pkt_t & pkt){
	pkt.wc = 0;
	pkt.type = 0;
    pkt.timestamp = 0;
	for (uint8_t i = 0; i < PKT_KKS_MAX_LEN; i++)
		pkt.kks[i] = 0;
	pkt.kks_len = 0;
    pkt.cmd_event = 0;
	pkt.mea = 0;
}
#ifndef RADTP_PACKET_DEFINES_FOR_ESP_PIPER
#define RADTP_PACKET_DEFINES_FOR_ESP_PIPER


#define DEBUG
#define DEBUG_1
#define DEBUG_2

#define EMPTY            0
#define FROM_BEGINING    0

#define PKT_HEAD_LEN     4
#define PKT_KKS_MAX_LEN  64

#define PKT_POSITION_L              1
#define PKT_RESERVE_L               2
#define PKT_LEN2_L                  2

#define PKT_TYPE_POSITION         4
#define PKT_TYPE_L                  1

#define PKT_TYPE_CMD              5
#define PKT_TYPE_CMD_REPLY        9
#define PKT_TYPE_KEEP_ALIVE       254

#define PKT_ATTR_CODE_KKS         1    //  next byte contain the length of current KKS
#define PKT_ATTR_CODE_TIMESTAMP   2
#define PKT_ATTR_CODE_TIMESTAMP_L   4  //  length of timestamp
#define PKT_ATTR_CODE_FValue      3    //  float
#define PKT_ATTR_CODE_FValue_L      4  //  length of float
#define PKT_ATTR_CODE_DValue      4    //  double
#define PKT_ATTR_CODE_DValue_L      8  //  length of double
#define PKT_ATTR_CODE_BValue      5    //  byte
#define PKT_ATTR_CODE_BValue_L      1  //  length of byte
#define PKT_ATTR_CODE_WValue      6    //  short (word)
#define PKT_ATTR_CODE_WValue_L      2  //  length of short
#define PKT_ATTR_CODE_LValue      7    //  int 
#define PKT_ATTR_CODE_LValue_L      4  //  length of int
#define PKT_ATTR_CODE_SValue      8    //  string //  next byte contain the length of current string [ 1 - 255 ]
#define PKT_ATTR_CODE_BLBValue    9    //  blob   //  next byte contain the length of current blob   [ 1 - 65535 ]
#define PKT_ATTR_CODE_ALARM       10   //  alarm (number of excided threshold)
#define PKT_ATTR_CODE_ALARM_L       1  //  length of alarm
#define PKT_ATTR_CODE_EVENT       11   //  cmd or event code (status / state)
#define PKT_ATTR_CODE_EVENT_L       2  //  length of event
#define PKT_ATTR_CODE_QUALITY     12   //  quality
#define PKT_ATTR_CODE_QUALITY_L     1  //  length of quality


#define CMD_MAX_PARAM_COUNT 5
struct pkt_struct {
	uint32_t wc;
	uint8_t type;
    uint32_t timestamp;
    char kks[PKT_KKS_MAX_LEN];
	uint8_t kks_len;
    uint16_t cmd_event;
	uint32_t cmd_params[CMD_MAX_PARAM_COUNT];
	uint8_t cmd_params_count;
	float mea;
};
typedef struct pkt_struct pkt_t;


#define PKT_PARSER_RESULT_ERROR      0
#define PKT_PARSER_RESULT_SINGLE_PKT 1
#define PKT_PARSER_RESULT_MULTY_PKT  2


#endif
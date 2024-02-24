#ifndef RTP_H
#define RTP_H
#include <stdint.h>

#define RTP_VESION 2

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC 97

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PKT_SIZE 1400

struct RtpHeader {
    // byte 0
    uint8_t csrc_len : 4;  // contributing source identifiers count
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;

    // byte 1
    uint8_t payload_type : 7;
    uint8_t marker : 1;

    // bytes 2,3
    uint16_t seq;

    // bytes 4-7
    uint32_t timestamp;

    // bytes 8-11
    uint32_t ssrc;

    // data
    uint8_t payload[0];
};

struct RtcpHeader {
    // byte 0
    uint8_t rc : 5;  // reception report count
    uint8_t padding : 1;
    uint8_t version : 2;
    // byte 1
    uint8_t packet_type;

    // bytes 2,3
    uint16_t length;
};

class RtpPacket {
   public:
    RtpPacket();
    ~RtpPacket();

   public:
    uint8_t* buf;   // 4+rtp_header+rtp_body
    uint8_t* buf4;  // rtp_header+rtp_body
    RtpHeader* const rtp_header;
    int size;  // rtp_header+rtp_body
};

void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtp_header);
void parseRtcpHeader(uint8_t* buf, struct RtcpHeader* rtcp_header);

#endif  // RTP_H

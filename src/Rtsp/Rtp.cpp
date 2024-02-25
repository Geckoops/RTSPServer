#include "Rtp.h"

#include <string.h>

RtpPacket::RtpPacket()
    : buf(new uint8_t[4 + RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100]),
      buf4(buf + 4),
      rtp_header((RtpHeader*)buf4),
      size(0) {}

RtpPacket::~RtpPacket() {
    delete[] buf;
    buf = nullptr;
}

void parseRtpHeader(uint8_t* buf, struct RtpHeader* rtp_header) {
    memset(rtp_header, 0, sizeof(*rtp_header));

    // byte 0
    rtp_header->version = (buf[0] & 0xC0) >> 6;
    rtp_header->padding = (buf[0] & 0x20) >> 5;
    rtp_header->extension = (buf[0] & 0x10) >> 4;
    rtp_header->csrc_len = (buf[0] & 0x0F);
    // byte 1
    rtp_header->marker = (buf[1] & 0x80) >> 7;
    rtp_header->payload_type = (buf[1] & 0x7F);
    // bytes 2,3
    rtp_header->seq = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
    // bytes 4-7
    rtp_header->timestamp = ((buf[4] & 0xFF) << 24) | ((buf[5] & 0xFF) << 16) |
                            ((buf[6] & 0xFF) << 8) | ((buf[7] & 0xFF));
    // bytes 8-11
    rtp_header->ssrc = ((buf[8] & 0xFF) << 24) | ((buf[9] & 0xFF) << 16) |
                       ((buf[10] & 0xFF) << 8) | ((buf[11] & 0xFF));
}

void parseRtcpHeader(uint8_t* buf, struct RtcpHeader* rtcp_header) {
    memset(rtcp_header, 0, sizeof(*rtcp_header));
    // byte 0
    rtcp_header->version = (buf[0] & 0xC0) >> 6;
    rtcp_header->padding = (buf[0] & 0x20) >> 5;
    rtcp_header->rc = (buf[0] & 0x1F);
    // byte 1
    rtcp_header->packet_type = (buf[1] & 0xFF);
    // bytes 2,3
    rtcp_header->length = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
}
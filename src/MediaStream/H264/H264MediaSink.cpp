#include "H264MediaSink.h"

#include <stdio.h>
#include <string.h>

#include "Log.h"
#include "SocketsOps.h"

// 时钟频率
static const int kH264ClockRate = 90000;

H264MediaSink* H264MediaSink::createNew(UsageEnvironment* env,
                                        MediaSource* media_source, double fps) {
    return new H264MediaSink(env, media_source, fps);
}

H264MediaSink::H264MediaSink(UsageEnvironment* env, MediaSource* media_source,
                             double fps)
    : Sink(env, media_source, RTP_PAYLOAD_TYPE_H264),
      clock_rate_(kH264ClockRate),
      fps_(fps) {
    LOGI("H264MediaSink()");
    runEvery(1000 / fps);
}

H264MediaSink::~H264MediaSink() { LOGI("~H264MediaSink()"); }

std::string H264MediaSink::getMediaDescription(uint16_t port) {
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP %d", port, payload_type_);

    return buf;
}

std::string H264MediaSink::getAttribute() {
    char buf[200] = {0};
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", payload_type_, clock_rate_);
    sprintf(buf + strlen(buf), "a=framerate:%.3lf\r\n", fps_);
    sprintf(buf + strlen(buf), "a=fmtp:96 sprop-parameter-sets=%s,%s",
            getSPSInfo().c_str(), getPPSInfo().c_str());

    return buf;
}

void H264MediaSink::sendFrame(MediaFrame* frame) {
    // 发送RTP数据包
    RtpHeader* rtp_header = rtp_packet_.rtp_header;
    uint8_t nalu_type = frame->buf[0];

    if (frame->size <= RTP_MAX_PKT_SIZE) {
        memcpy(rtp_header->payload, frame->buf, frame->size);
        rtp_packet_.size = RTP_HEADER_SIZE + frame->size;
        sendRtpPacket(&rtp_packet_);
        seq_++;

        if ((nalu_type & 0x1F) == 7 ||
            (nalu_type & 0x1F) == 8) {  // 如果是SPS、PPS就不需要加时间戳
            return;
        }
    } else {
        // 有几个完整的包
        int pkt_num = frame->size / RTP_MAX_PKT_SIZE;
        // 剩余不完整包的大小
        int ramain_pkt_size = frame->size % RTP_MAX_PKT_SIZE;

        int i, pos = 1;

        /* 发送完整的包 */
        for (i = 0; i < pkt_num; i++) {
            /*
             *     FU Indicator
             *    0 1 2 3 4 5 6 7
             *   +-+-+-+-+-+-+-+-+
             *   |F|NRI|  Type   |
             *   +---------------+
             * */

            //(nalu_type & 0x60)表示nalu的重要性，28表示为分片
            rtp_header->payload[0] = (nalu_type & 0x60) | 28;

            /*
             *      FU Header
             *    0 1 2 3 4 5 6 7
             *   +-+-+-+-+-+-+-+-+
             *   |S|E|R|  Type   |
             *   +---------------+
             * */
            rtp_header->payload[1] = nalu_type & 0x1F;

            if (i == 0) {                        // 第一包数据
                rtp_header->payload[1] |= 0x80;  // start
            } else if (ramain_pkt_size == 0 &&
                       i == pkt_num - 1) {       // 最后一包数据
                rtp_header->payload[1] |= 0x40;  // end
            }

            memcpy(rtp_header->payload + 2, frame->buf + pos, RTP_MAX_PKT_SIZE);
            rtp_packet_.size = RTP_HEADER_SIZE + 2 + RTP_MAX_PKT_SIZE;
            sendRtpPacket(&rtp_packet_);

            seq_++;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (ramain_pkt_size > 0) {
            rtp_header->payload[0] = (nalu_type & 0x60) | 28;
            rtp_header->payload[1] = nalu_type & 0x1F;
            rtp_header->payload[1] |= 0x40;  // end

            memcpy(rtp_header->payload + 2, frame->buf + pos, ramain_pkt_size);
            rtp_packet_.size = RTP_HEADER_SIZE + 2 + ramain_pkt_size;
            sendRtpPacket(&rtp_packet_);

            seq_++;
        }
    }

    timestamp_ += clock_rate_ / fps_;
}

std::string H264MediaSink::getSPSInfo() {
    if (sps_info_.empty()) {
        return "Z2QAH6xyiQNg95//wKAAn9qAgICgAAB9IAAXcBHjBhRM";
    }
    return sps_info_;
}

std::string H264MediaSink::getPPSInfo() {
    if (pps_info_.empty()) {
        return "aOk7yw==";
    }
    return pps_info_;
}

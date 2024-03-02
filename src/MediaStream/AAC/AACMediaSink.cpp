#include "AACMediaSink.h"

#include <stdio.h>
#include <string.h>

#include <string>

#include "Log.h"

// 一帧aac数据的大小，默认是1024，可能是其他值
static const int kAACFrameSize = 1024;
// static const int kAACClockRate = 8000;
// 时钟频率
static const uint32_t kAACSampleRate[16] = {97000, 88200, 64000, 48000, 44100,
                                            32000, 24000, 22050, 16000, 12000,
                                            11025, 8000,  7350, /*reserved */
                                            0,     0,     0};

AACMediaSink* AACMediaSink::createNew(UsageEnvironment* env,
                                      MediaSource* media_source,
                                      int sample_rate, int channels) {
    return new AACMediaSink(env, media_source, RTP_PAYLOAD_TYPE_AAC,
                            sample_rate, channels);
}

AACMediaSink::AACMediaSink(UsageEnvironment* env, MediaSource* media_source,
                           int payload_type, int sample_rate, int channels)
    : Sink(env, media_source, payload_type),
      sample_rate_(sample_rate),
      channels_(channels),
      fps_(1.0 * sample_rate / kAACFrameSize) {
    LOGI("AACMediaSink()");
    marker_ = 1;
    runEvery(1000 / fps_);
}

AACMediaSink::~AACMediaSink() { LOGI("~AACMediaSink()"); }

std::string AACMediaSink::getMediaDescription(uint16_t port) {
    char buf[100] = {0};
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, payload_type_);

    return buf;
}

std::string AACMediaSink::getAttribute() {
    char buf[500] = {0};
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", sample_rate_,
            channels_);

    uint8_t index = 0;
    for (index = 0; index < 16; index++) {
        if (kAACSampleRate[index] == sample_rate_) {
            break;
        }
    }

    if (index == 16) {
        return "";
    }

    uint8_t profile = 1;
    char config_str[10] = {0};
    sprintf(config_str, "%02x%02x",
            (uint8_t)((profile + 1) << 3) | (index >> 1),
            (uint8_t)((index << 7) | (channels_ << 3)));

    sprintf(buf + strlen(buf),
            "a=fmtp:%d profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
            payload_type_, atoi(config_str));

    return buf;
}

void AACMediaSink::sendFrame(MediaFrame* frame) {
    RtpHeader* rtp_header = rtp_packet_.rtp_header;
    int frame_size = frame->size - 7;  // 去掉aac头部

    rtp_header->payload[0] = 0x00;
    rtp_header->payload[1] = 0x10;
    rtp_header->payload[2] = (frame_size & 0x1FE0) >> 5;  // 高8位
    rtp_header->payload[3] = (frame_size & 0x1F) << 3;    // 低5位

    /* 去掉aac的头部 */
    memcpy(rtp_header->payload + 4, frame->buf + 7, frame_size);
    rtp_packet_.size = RTP_HEADER_SIZE + 4 + frame_size;

    sendRtpPacket(&rtp_packet_);

    seq_++;

    timestamp_ += sample_rate_ / fps_;
}

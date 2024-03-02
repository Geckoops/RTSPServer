#ifndef AACMEDIASINK_H
#define AACMEDIASINK_H

#include "MediaSource.h"
#include "Sink.h"
#include "UsageEnvironment.h"


class AACMediaSink : public Sink {
   public:
    static AACMediaSink* createNew(UsageEnvironment* env,
                                   MediaSource* media_source, int sample_rate,
                                   int channels);

    AACMediaSink(UsageEnvironment* env, MediaSource* media_source,
                 int payload_type, int sample_rate, int channels);
    virtual ~AACMediaSink();

    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();

   protected:
    virtual void sendFrame(MediaFrame* frame);

   private:
    RtpPacket rtp_packet_;
    MediaSource* media_source_;
    uint32_t sample_rate_;  // 采样频率
    uint32_t channels_;     // 通道数
    double fps_;
};

#endif  // AACMEDIASINK_H
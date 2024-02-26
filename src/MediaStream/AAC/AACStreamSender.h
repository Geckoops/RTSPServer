#ifndef AACSTREAMSENDER_H
#define AACSTREAMSENDER_H

#include "UsageEnvironment.h"
#include "MediaFileReader.h"
#include "StreamSender.h"

class AACStreamSender : public StreamSender {
   public:
    static AACStreamSender* createNew(UsageEnvironment* env,
                                      MediaFileReader* media_reader,
                                      int sample_rate, int channels);

    AACStreamSender(UsageEnvironment* env, MediaFileReader* media_reader,
                    int payload_type, int sample_rate, int channels);
    virtual ~AACStreamSender();

    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();

   protected:
    virtual void sendFrame(MediaFrame* frame);

   private:
    RtpPacket rtp_packet_;
    MediaFileReader* media_reader_;
    uint32_t sample_rate_;  // 采样频率
    uint32_t channels_;     // 通道数
    double fps_;
};

#endif  // AACSTREAMSENDER_H
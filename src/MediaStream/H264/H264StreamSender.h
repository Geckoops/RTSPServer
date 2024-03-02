#ifndef H264STREAMSENDER_H
#define H264STREAMSENDER_H

#include <stdint.h>

#include "StreamSender.h"

class H264StreamSender : public StreamSender {
   public:
    static H264StreamSender* createNew(UsageEnvironment* env,
                                       MediaFileReader* media_reader,
                                       double fps);

    H264StreamSender(UsageEnvironment* env, MediaFileReader* media_reader,
                     double fps);
    virtual ~H264StreamSender();
    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();
    virtual void sendFrame(MediaFrame* frame);

   private:
    std::string getSPSInfo();
    std::string getPPSInfo();

   private:
    RtpPacket rtp_packet_;
    int clock_rate_;
    double fps_;
    std::string sps_info_;
    std::string pps_info_;
};

#endif  // H264STREAMSENDER_H
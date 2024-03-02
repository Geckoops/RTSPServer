#ifndef H264MEDIASINK_H
#define H264MEDIASINK_H

#include <stdint.h>

#include "Sink.h"

class H264MediaSink : public Sink {
   public:
    static H264MediaSink* createNew(UsageEnvironment* env,
                                    MediaSource* media_source, double fps);

    H264MediaSink(UsageEnvironment* env, MediaSource* media_source, double fps);
    virtual ~H264MediaSink();
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

#endif  // H264MEDIASINK_H
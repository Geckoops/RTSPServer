#ifndef SINK_H
#define SINK_H
#include <stdint.h>

#include <functional>
#include <string>

#include "Event.h"
#include "MediaFileReader.h"
#include "Rtp.h"
#include "UsageEnvironment.h"


class StreamSender {
   public:
    enum PacketType {
        UNKNOWN = -1,
        RTPPACKET = 0,
    };

    using SessionSendPacketCallback =
        std::function<void(RtpPacket*, PacketType)>;

    StreamSender(UsageEnvironment* env, MediaFileReader* media_reader,
                 int payload_type);
    virtual ~StreamSender();

    void stopTimerEvent();

    virtual std::string getMediaDescription(uint16_t port) = 0;
    virtual std::string getAttribute() = 0;

    void setSessionCb(SessionSendPacketCallback cb);

   protected:
    virtual void sendFrame(MediaFrame* frame) = 0;
    void sendRtpPacket(RtpPacket* packet);

    void runEvery(int interval);

   private:
    void handleTimeout();

   protected:
    UsageEnvironment* env_;
    MediaFileReader* media_reader_;
    SessionSendPacketCallback session_send_packet_;

    uint8_t csrc_len_;
    uint8_t extension_;
    uint8_t padding_;
    uint8_t version_;
    uint8_t payload_type_;
    uint8_t marker_;
    uint16_t seq_;
    uint32_t timestamp_;
    uint32_t ssrc_;

   private:
    TimerEvent* timer_event_;
    Timer::TimerId timer_id_;  // runEvery()之后获取
};

#endif  // SINK_H
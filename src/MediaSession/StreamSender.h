#ifndef STREAM_SENDER_H
#define STREAM_SENDER_H
#include <stdint.h>

#include <functional>
#include <string>

#include "Event.h"
#include "UsageEnvironment.h"
#include "MediaFileReader.h"
#include "Rtp.h"

class StreamSender {
   public:
    enum PacketType {
        UNKNOWN = -1,
        RTPPACKET = 0,
    };

    using SessionSendPacketCallback =
        std::function<void(RtpPacket*, PacketType)>;

    StreamSender(UsageEnvironment* env, MediaFileReader* media_reader, int payload_type);
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

    // RTP header
    uint8_t csrc_len_;      // CSRC标识符的个数
    uint8_t extension_;     // 是否有扩展
    uint8_t padding_;       // 是否有填充
    uint8_t version_;       // 版本号
    uint8_t payload_type_;  // 负载类型
    uint8_t marker_;        // 标记位
    uint16_t seq_;          // 序列号
    uint32_t timestamp_;    // 时间戳
    uint32_t ssrc_;         // 同步源标识符

   private:
    TimerEvent* timer_event_;
    Timer::TimerId timer_id_;  // runEvery()之后获取
};

#endif  // STREAM_SENDER_H
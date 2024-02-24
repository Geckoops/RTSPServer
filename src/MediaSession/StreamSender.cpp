#include "StreamSender.h"

#include "Log.h"
#include "SocketsOps.h"

StreamSender::StreamSender(UsageEnvironment* env, MediaFileReader* media_reader,
                           int payload_type)
    : env_(env),
      media_reader_(media_reader),
      session_send_packet_(NULL),
      csrc_len_(0),
      extension_(0),
      padding_(0),
      version_(RTP_VESION),
      payload_type_(payload_type),
      marker_(0),
      seq_(0),
      timestamp_(0),
      ssrc_(rand()),
      timer_id_(0) {
    LOGI("StreamSender()");
    timer_event_ = TimerEvent::createNew([this]() { this->handleTimeout(); });
}

StreamSender::~StreamSender() {
    LOGI("~StreamSender()");

    delete timer_event_;
    delete media_reader_;
}
void StreamSender::stopTimerEvent() { timer_event_->stop(); }

void StreamSender::setSessionCb(SessionSendPacketCallback cb) {
    session_send_packet_ = cb;
}

void StreamSender::sendRtpPacket(RtpPacket* packet) {
    RtpHeader* rtp_header = packet->rtp_header;
    rtp_header->csrc_len = csrc_len_;
    rtp_header->extension = extension_;
    rtp_header->padding = padding_;
    rtp_header->version = version_;
    rtp_header->payload_type = payload_type_;
    rtp_header->marker = marker_;
    rtp_header->seq = htons(seq_);
    rtp_header->timestamp = htonl(timestamp_);
    rtp_header->ssrc = htonl(ssrc_);

    if (session_send_packet_) {
        session_send_packet_(packet, PacketType::RTPPACKET);
    }
}

void StreamSender::handleTimeout() {
    MediaFrame* frame = media_reader_->getFrameFromOutputQueue();
    if (!frame) {
        return;
    }
    this->sendFrame(frame);  // 由具体子类实现发送逻辑

    // 将使用过的frame插入输入队列，插入输入队列以后，加入一个子线程task，从文件中读取数据再次将输入写入到frame队列中
    media_reader_->putFrameToInputQueue(frame);
}

void StreamSender::runEvery(int interval) {
    timer_id_ =
        env_->scheduler()->addTimedEventRunEvery(timer_event_, interval);
}

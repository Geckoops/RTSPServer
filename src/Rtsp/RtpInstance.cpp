#include "RtpInstance.h"

#include "SocketsOps.h"

RtpInstance* RtpInstance::createNewOverUdp(int local_sockfd,
                                           uint16_t local_port,
                                           std::string dest_ip,
                                           uint16_t dest_port) {
    return new RtpInstance(local_sockfd, local_port, dest_ip, dest_port);
}

RtpInstance* RtpInstance::createNewOverTcp(int sockfd, uint8_t rtp_channel) {
    return new RtpInstance(sockfd, rtp_channel);
}

RtpInstance::~RtpInstance() { sockets::close(sockfd_); }

uint16_t RtpInstance::getLocalPort() const { return local_port_; }

uint16_t RtpInstance::getPeerPort() { return dest_addr_.getPort(); }

int RtpInstance::send(RtpPacket* rtp_packet) {
    switch (rtp_type_) {
        case RtpInstance::RTP_OVER_UDP: {
            return sendOverUdp(rtp_packet->buf4, rtp_packet->size);
            break;
        }
        case RtpInstance::RTP_OVER_TCP: {
            rtp_packet->buf[0] = '$';
            rtp_packet->buf[1] = (uint8_t)rtp_channel_;
            rtp_packet->buf[2] = (uint8_t)(((rtp_packet->size) & 0xFF00) >> 8);
            rtp_packet->buf[3] = (uint8_t)((rtp_packet->size) & 0xFF);
            return sendOverTcp(rtp_packet->buf, 4 + rtp_packet->size);
            break;
        }

        default: {
            return -1;
            break;
        }
    }
}

bool RtpInstance::isAlive() const { return is_alive_; }

int RtpInstance::setAlive(bool is_alive) {
    is_alive_ = is_alive;
    return 0;
};

void RtpInstance::setSessionId(uint16_t session_id) {
    session_id_ = session_id;
}

uint16_t RtpInstance::sessionId() const { return session_id_; }

int RtpInstance::sendOverUdp(void* buf, int size) {
    return sockets::sendto(sockfd_, buf, size, dest_addr_.getAddr());
}

int RtpInstance::sendOverTcp(void* buf, int size) {
    return sockets::send(sockfd_, buf, size);
}

RtpInstance::RtpInstance(int local_sockfd, uint16_t local_port,
                         const std::string& dest_ip, uint16_t dest_port)
    : rtp_type_(RTP_OVER_UDP),
      sockfd_(local_sockfd),
      local_port_(local_port),
      dest_addr_(dest_ip, dest_port),
      is_alive_(false),
      session_id_(0),
      rtp_channel_(0) {}

RtpInstance::RtpInstance(int sockfd, uint8_t rtp_channel)
    : rtp_type_(RTP_OVER_TCP),
      sockfd_(sockfd),
      local_port_(0),
      is_alive_(false),
      session_id_(0),
      rtp_channel_(rtp_channel) {}

RtcpInstance* RtcpInstance::createNew(int local_sockfd, uint16_t local_port,
                                      std::string dest_ip, uint16_t dest_port) {
    return new RtcpInstance(local_sockfd, local_port, dest_ip, dest_port);
}

int RtcpInstance::send(void* buf, int size) {
    return sockets::sendto(local_sockfd_, buf, size, dest_addr_.getAddr());
}

int RtcpInstance::recv(void* buf, int size, Ipv4Address* addr) {
    // TODO
    return 0;
}

uint16_t RtcpInstance::getLocalPort() const { return local_port_; }

int RtcpInstance::isAlive() const { return is_alive_; }

int RtcpInstance::setAlive(bool is_alive) {
    is_alive_ = is_alive;
    return 0;
};

void RtcpInstance::setSessionId(uint16_t session_id) {
    session_id_ = session_id;
}

uint16_t RtcpInstance::sessionId() const { return session_id_; }

RtcpInstance::RtcpInstance(int local_sockfd, uint16_t local_port,
                           std::string dest_ip, uint16_t dest_port)
    : local_sockfd_(local_sockfd),
      local_port_(local_port),
      dest_addr_(dest_ip, dest_port),
      is_alive_(false),
      session_id_(0) {}

RtcpInstance::~RtcpInstance() { sockets::close(local_sockfd_); }

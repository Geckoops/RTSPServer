#ifndef RTPINSTANCE_H
#define RTPINSTANCE_H
#include <stdint.h>

#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif  //_WIN32
#include "InetAddress.h"
#include "Rtp.h"

class RtpInstance {
   public:
    enum RtpType { RTP_OVER_UDP, RTP_OVER_TCP };

    static RtpInstance* createNewOverUdp(int local_sockfd, uint16_t local_port,
                                         std::string dest_ip,
                                         uint16_t dest_port);

    static RtpInstance* createNewOverTcp(int sockfd, uint8_t rtp_channel);

    ~RtpInstance();
    uint16_t getLocalPort() const;
    uint16_t getPeerPort();

    int send(RtpPacket* rtp_packet);

    bool isAlive() const;
    int setAlive(bool is_alive);
    void setSessionId(uint16_t session_id);
    uint16_t sessionId() const;

   private:
    int sendOverUdp(void* buf, int size);

    int sendOverTcp(void* buf, int size);

   public:
    RtpInstance(int local_sockfd, uint16_t local_port,
                const std::string& dest_ip, uint16_t dest_port);

    RtpInstance(int sockfd, uint8_t rtp_channel);

   private:
    RtpType rtp_type_;
    int sockfd_;
    uint16_t local_port_;    // for udp
    Ipv4Address dest_addr_;  // for udp
    bool is_alive_;
    uint16_t session_id_;
    uint8_t rtp_channel_;
};

class RtcpInstance {
   public:
    static RtcpInstance* createNew(int local_sockfd, uint16_t local_port,
                                   std::string dest_ip, uint16_t dest_port);

    int send(void* buf, int size);

    int recv(void* buf, int size, Ipv4Address* addr) ;

    uint16_t getLocalPort() const;

    int isAlive() const;
    int setAlive(bool is_alive) ;
    void setSessionId(uint16_t session_id);
    uint16_t sessionId() const;

   public:
    RtcpInstance(int local_sockfd, uint16_t local_port, std::string dest_ip,
                 uint16_t dest_port);

    ~RtcpInstance();

   private:
    int local_sockfd_;
    uint16_t local_port_;
    Ipv4Address dest_addr_;
    bool is_alive_;
    uint16_t session_id_;
};

#endif  // RTPINSTANCE_H
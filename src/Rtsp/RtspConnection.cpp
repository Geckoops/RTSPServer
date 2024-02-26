#include "RtspConnection.h"

#include <stdio.h>
#include <string.h>

#include "Log.h"
#include "MediaSession.h"
#include "MediaSessionManager.h"
#include "Rtp.h"
#include "RtspServer.h"
#include "SocketsOps.h"
#include "Version.h"


RtspConnection* RtspConnection::createNew(RtspServer* rtsp_server,
                                          int client_fd) {
    return new RtspConnection(rtsp_server, client_fd);
}

RtspConnection::RtspConnection(RtspServer* rtsp_server, int client_fd)
    : TcpConnection(rtsp_server->env(), client_fd),
      rtsp_server_(rtsp_server),
      method_(RtspConnection::Method::NONE),
      stream_prefix_("track"),
      track_id_(MediaSession::TrackId::TrackIdNone),
      session_id_(rand()),
      is_rtp_over_tcp_(false) {
    LOGI("RtspConnection() client_fd_=%d", client_fd_);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        rtp_instances_[i] = nullptr;
        rtcp_instances_[i] = nullptr;
    }

    peer_ip_ = sockets::getPeerIp(client_fd);
}

RtspConnection::~RtspConnection() {
    LOGI("~RtspConnection() client_fd_=%d", client_fd_);
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (rtp_instances_[i]) {
            MediaSession* session = rtsp_server_->session_manger->getSession(suffix_);

            if (session == nullptr) {
                session->removeRtpInstance(rtp_instances_[i]);
            }
            delete rtp_instances_[i];
        }

        if (rtcp_instances_[i]) {
            delete rtcp_instances_[i];
        }
    }
}

void RtspConnection::handleReadBytes() {
    if (is_rtp_over_tcp_) {
        if (input_buffer_.peek()[0] == '$') {
            handleRtpOverTcp();
            return;
        }
    }

    if (!parseRequest()) {
        LOGE("parseRequest err");
        goto disconnect;
    }
    switch (method_) {
        case OPTIONS:
            if (!handleCmdOption()) {
                goto disconnect;
            }
            break;
        case DESCRIBE:
            if (!handleCmdDescribe()) {
                goto disconnect;
            }
            break;
        case SETUP:
            if (!handleCmdSetup()) {
                goto disconnect;
            }
            break;
        case PLAY:
            if (!handleCmdPlay()) {
                goto disconnect;
            }
            break;
        case TEARDOWN:
            if (!handleCmdTeardown()) {
                goto disconnect;
            }
            break;

        default:
            goto disconnect;
    }

    return;
disconnect:
    handleDisConnect();
}

bool RtspConnection::parseRequest() {
    // 解析第一行
    const char* crlf = input_buffer_.findCRLF();
    if (crlf == nullptr) {
        input_buffer_.retrieveAll();
        return false;
    }
    bool ret = parseRequest1(input_buffer_.peek(), crlf);
    if (ret == false) {
        input_buffer_.retrieveAll();
        return false;
    } else {
        input_buffer_.retrieveUntil(crlf + 2);
    }

    // 解析第一行之后的所有行
    crlf = input_buffer_.findLastCrlf();
    if (crlf == nullptr) {
        input_buffer_.retrieveAll();
        return false;
    }
    ret = parseRequest2(input_buffer_.peek(), crlf);

    if (ret == false) {
        input_buffer_.retrieveAll();
        return false;
    } else {
        input_buffer_.retrieveUntil(crlf + 2);
        return true;
    }
}

bool RtspConnection::parseRequest1(const char* begin, const char* end) {
    std::string message(begin, end);
    char method[64] = {0};
    char url[512] = {0};
    char version[64] = {0};

    if (sscanf(message.c_str(), "%s %s %s", method, url, version) != 3) {
        return false;
    }

    if (!strcmp(method, "OPTIONS")) {
        method_ = OPTIONS;
    } else if (!strcmp(method, "DESCRIBE")) {
        method_ = DESCRIBE;
    } else if (!strcmp(method, "SETUP")) {
        method_ = SETUP;
    } else if (!strcmp(method, "PLAY")) {
        method_ = PLAY;
    } else if (!strcmp(method, "TEARDOWN")) {
        method_ = TEARDOWN;
    } else {
        method_ = NONE;
        return false;
    }
    if (strncmp(url, "rtsp://", 7) != 0) {
        return false;
    }

    uint16_t port = 0;
    char ip[64] = {0};
    char suffix[64] = {0};

    if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3) {
    } else if (sscanf(url + 7, "%[^/]/%s", ip, suffix) == 2) {
        port = 554;  // 如果rtsp请求地址中无端口，默认获取的端口为：554
    } else {
        return false;
    }

    url_ = url;
    suffix_ = suffix;

    return true;
}

bool RtspConnection::parseRequest2(const char* begin, const char* end) {
    std::string message(begin, end);

    if (!parseCSeq(message)) {
        return false;
    }
    if (method_ == OPTIONS) {
        return true;
    } else if (method_ == DESCRIBE) {
        return parseDescribe(message);
    } else if (method_ == SETUP) {
        return parseSetup(message);
    } else if (method_ == PLAY) {
        return parsePlay(message);
    } else if (method_ == TEARDOWN) {
        return true;
    } else {
        return false;
    }
}

bool RtspConnection::parseCSeq(std::string& message) {
    std::size_t pos = message.find("CSeq");
    if (pos != std::string::npos) {
        uint32_t cseq = 0;
        sscanf(message.c_str() + pos, "%*[^:]: %u", &cseq);
        cseq_ = cseq;
        return true;
    }

    return false;
}

bool RtspConnection::parseDescribe(std::string& message) {
    if ((message.rfind("Accept") == std::string::npos) ||
        (message.rfind("sdp") == std::string::npos)) {
        return false;
    }

    return true;
}

bool RtspConnection::parseSetup(std::string& message) {
    track_id_ = MediaSession::TrackIdNone;
    std::size_t pos;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        pos = url_.find(stream_prefix_ + std::to_string(i));
        if (pos != std::string::npos) {
            if (i == 0) {
                track_id_ = MediaSession::TrackId0;
            } else if (i == 1) {
                track_id_ = MediaSession::TrackId1;
            }
        }
    }

    if (track_id_ == MediaSession::TrackIdNone) {
        return false;
    }

    pos = message.find("Transport");
    if (pos != std::string::npos) {
        if ((pos = message.find("RTP/AVP/TCP")) != std::string::npos) {
            uint8_t rtp_channel, rtcp_channel;
            is_rtp_over_tcp_ = true;

            if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hhu-%hhu",
                       &rtp_channel, &rtcp_channel) != 2) {
                return false;
            }

            rtp_channel_ = rtp_channel;

            return true;
        } else if ((pos = message.find("RTP/AVP")) != std::string::npos) {
            uint16_t rtp_port = 0, rtcp_port = 0;
            if (((message.find("unicast", pos)) != std::string::npos)) {
                if (sscanf(message.c_str() + pos,
                           "%*[^;];%*[^;];%*[^=]=%hu-%hu", &rtp_port,
                           &rtcp_port) != 2) {
                    return false;
                }
            } else if ((message.find("multicast", pos)) != std::string::npos) {
                return true;
            } else
                return false;

            peer_rtp_port_ = rtp_port;
            peer_rtcp_port_ = rtcp_port;
        } else {
            return false;
        }

        return true;
    }

    return false;
}

bool RtspConnection::parsePlay(std::string& message) {
    std::size_t pos = message.find("Session");
    if (pos != std::string::npos) {
        uint32_t session_id = 0;
        if (sscanf(message.c_str() + pos, "%*[^:]: %u", &session_id) != 1)
            return false;
        return true;
    }

    return false;
}

bool RtspConnection::handleCmdOption() {
    snprintf(buffer_, sizeof(buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Public: DESCRIBE, SETUP, PLAY, TEARDOWN\r\n"
             "Server: %s\r\n"
             "\r\n",
             cseq_, PROJECT_VERSION);

    if (sendMessage(buffer_, strlen(buffer_)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::handleCmdDescribe() {
    MediaSession* session = rtsp_server_->session_manger->getSession(suffix_);

    if (session == nullptr) {
        LOGE("can't find session:%s", suffix_.c_str());
        return false;
    }
    std::string sdp = session->generateSDPDescription();

    memset(buffer_, 0, sizeof(buffer_));
    snprintf(buffer_, sizeof(buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Content-Length: %u\r\n"
             "Content-Type: application/sdp\r\n"
             "\r\n"
             "%s",
             cseq_, (unsigned int)sdp.size(), sdp.c_str());

    if (sendMessage(buffer_, strlen(buffer_)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::handleCmdSetup() {
    char session_name[100];
    if (sscanf(suffix_.c_str(), "%[^/]/", session_name) != 1) {
        return false;
    }
    MediaSession* session = rtsp_server_->session_manger->getSession(session_name);
    if (session == nullptr) {
        LOGE("can't find session:%s", session_name);
        return false;
    }

    if (track_id_ >= MEDIA_MAX_TRACK_NUM ||
        rtp_instances_[track_id_] != nullptr ||
        rtcp_instances_[track_id_] != nullptr) {
        return false;
    }

    if (session->isStartMulticast()) {
        snprintf(buffer_, sizeof(buffer_),
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %d\r\n"
                 "Transport: RTP/AVP;multicast;"
                 "destination=%s;source=%s;port=%d-%d;ttl=255\r\n"
                 "Session: %08x\r\n"
                 "\r\n",
                 cseq_, session->getMulticastDestAddr().c_str(),
                 sockets::getLocalIp().c_str(),
                 session->getMulticastDestRtpPort(track_id_),
                 session->getMulticastDestRtpPort(track_id_) + 1, session_id_);
    } else {
        if (is_rtp_over_tcp_) {
            // 创建rtp over tcp
            createRtpOverTcp(track_id_, client_fd_, rtp_channel_);
            rtp_instances_[track_id_]->setSessionId(session_id_);

            session->addRtpInstance(track_id_, rtp_instances_[track_id_]);

            snprintf(buffer_, sizeof(buffer_),
                     "RTSP/1.0 200 OK\r\n"
                     "CSeq: %d\r\n"
                     "Server: %s\r\n"
                     "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
                     "Session: %08x\r\n"
                     "\r\n",
                     cseq_, PROJECT_VERSION, rtp_channel_, rtp_channel_ + 1,
                     session_id_);
        } else {
            // 创建 rtp over udp
            if (createRtpRtcpOverUdp(track_id_, peer_ip_, peer_rtp_port_,
                                     peer_rtcp_port_) != true) {
                LOGE("failed to createRtpRtcpOverUdp");
                return false;
            }

            rtp_instances_[track_id_]->setSessionId(session_id_);
            rtcp_instances_[track_id_]->setSessionId(session_id_);

            session->addRtpInstance(track_id_, rtp_instances_[track_id_]);

            snprintf(
                buffer_, sizeof(buffer_),
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %u\r\n"
                "Server: %s\r\n"
                "Transport: "
                "RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                "Session: %08x\r\n"
                "\r\n",
                cseq_, PROJECT_VERSION, peer_rtp_port_, peer_rtcp_port_,
                rtp_instances_[track_id_]->getLocalPort(),
                rtcp_instances_[track_id_]->getLocalPort(), session_id_);
        }
    }

    if (sendMessage(buffer_, strlen(buffer_)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::handleCmdPlay() {
    snprintf(buffer_, sizeof(buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "Server: %s\r\n"
             "Range: npt=0.000-\r\n"
             "Session: %08x; timeout=60\r\n"
             "\r\n",
             cseq_, PROJECT_VERSION, session_id_);

    if (sendMessage(buffer_, strlen(buffer_)) < 0) {
        return false;
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (rtp_instances_[i]) {
            rtp_instances_[i]->setAlive(true);
        }

        if (rtcp_instances_[i]) {
            rtcp_instances_[i]->setAlive(true);
        }
    }

    return true;
}

bool RtspConnection::handleCmdTeardown() {
    snprintf(buffer_, sizeof(buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "Server: %s\r\n"
             "\r\n",
             cseq_, PROJECT_VERSION);

    if (sendMessage(buffer_, strlen(buffer_)) < 0) {
        return false;
    }

    return true;
}

int RtspConnection::sendMessage(char* buf, int size) {
    LOGI("%s", buf);
    int ret;

    output_buffer_.append(buf, size);
    ret = output_buffer_.write(client_fd_);
    output_buffer_.retrieveAll();

    return ret;
}

int RtspConnection::sendMessage() {
    int ret = output_buffer_.write(client_fd_);
    output_buffer_.retrieveAll();
    return ret;
}

bool RtspConnection::createRtpRtcpOverUdp(MediaSession::TrackId track_id,
                                          std::string peer_ip,
                                          uint16_t peer_rtp_port,
                                          uint16_t peer_rtcp_port) {
    int rtp_sockfd, rtcp_sockfd;
    int16_t rtp_port, rtcp_port;
    bool ret;

    if (rtp_instances_[track_id] || rtcp_instances_[track_id]) {
        return false;
    }

    int i;
    for (i = 0; i < 10; ++i) {  // 重试10次
        rtp_sockfd = sockets::createUdpSock();
        if (rtp_sockfd < 0) {
            return false;
        }

        rtcp_sockfd = sockets::createUdpSock();
        if (rtcp_sockfd < 0) {
            sockets::close(rtp_sockfd);
            return false;
        }

        uint16_t port = rand() & 0xfffe;
        if (port < 10000) {
            port += 10000;
        }

        rtp_port = port;
        rtcp_port = port + 1;

        ret = sockets::bind(rtp_sockfd, "0.0.0.0", rtp_port);
        if (ret != true) {
            sockets::close(rtp_sockfd);
            sockets::close(rtcp_sockfd);
            continue;
        }

        ret = sockets::bind(rtcp_sockfd, "0.0.0.0", rtcp_port);
        if (ret != true) {
            sockets::close(rtp_sockfd);
            sockets::close(rtcp_sockfd);
            continue;
        }

        break;
    }

    if (i == 10) {
        return false;
    }

    rtp_instances_[track_id] = RtpInstance::createNewOverUdp(
        rtp_sockfd, rtp_port, peer_ip, peer_rtp_port);
    rtcp_instances_[track_id] = RtcpInstance::createNew(
        rtcp_sockfd, rtcp_port, peer_ip, peer_rtcp_port);

    return true;
}

bool RtspConnection::createRtpOverTcp(MediaSession::TrackId track_id,
                                      int sockfd, uint8_t rtp_channel) {
    rtp_instances_[track_id] =
        RtpInstance::createNewOverTcp(sockfd, rtp_channel);

    return true;
}

void RtspConnection::handleRtpOverTcp() {
    int num = 0;
    while (true) {
        num += 1;
        uint8_t* buf = (uint8_t*)input_buffer_.peek();
        uint8_t rtp_channel = buf[1];
        int16_t rtp_size = (buf[2] << 8) | buf[3];

        int16_t buf_size = 4 + rtp_size;

        if (input_buffer_.readableBytes() < buf_size) {
            // 缓存数据小于一个RTP数据包的长度
            return;
        } else {
            if (0x00 == rtp_channel) {
                RtpHeader rtp_header;
                parseRtpHeader(buf + 4, &rtp_header);
                LOGI("num=%d,rtp_size=%d", num, rtp_size);
            } else if (0x01 == rtp_channel) {
                RtcpHeader rtcp_header;
                parseRtcpHeader(buf + 4, &rtcp_header);

                LOGI("num=%d,rtcp_header.packet_type=%d,rtp_size=%d", num,
                     rtcp_header.packet_type, rtp_size);
            } else if (0x02 == rtp_channel) {
                RtpHeader rtp_header;
                parseRtpHeader(buf + 4, &rtp_header);
                LOGI("num=%d,rtp_size=%d", num, rtp_size);
            } else if (0x03 == rtp_channel) {
                RtcpHeader rtcp_header;
                parseRtcpHeader(buf + 4, &rtcp_header);

                LOGI("num=%d,rtcp_header.packet_type=%d,rtp_size=%d", num,
                     rtcp_header.packet_type, rtp_size);
            }

            input_buffer_.retrieve(buf_size);
        }
    }
}
#include "MediaSession.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <algorithm>

#include "Log.h"
#include "SocketsOps.h"

MediaSession* MediaSession::createNew(std::string session_name) {
    return new MediaSession(session_name);
}

MediaSession::MediaSession(const std::string& session_name)
    : session_name_(session_name), is_start_multicast_(false) {
    LOGI("MediaSession() name=%s", session_name.data());

    tracks_[0].track_id = TrackId0;
    tracks_[0].is_alive = false;

    tracks_[1].track_id = TrackId1;
    tracks_[1].is_alive = false;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        multicast_rtp_instances_[i] = nullptr;
        multicast_rtcp_instances_[i] = nullptr;
    }
}

MediaSession::~MediaSession() {
    LOGI("~MediaSession()");
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (multicast_rtp_instances_[i]) {
            this->removeRtpInstance(multicast_rtp_instances_[i]);
            delete multicast_rtp_instances_[i];
        }

        if (multicast_rtcp_instances_[i]) {
            delete multicast_rtcp_instances_[i];
        }
    }
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (tracks_[i].is_alive) {
            Sink* sink = tracks_[i].sink;
            delete sink;
        }
    }
}

std::string MediaSession::generateSDPDescription() {
    if (!sdp_.empty()) {
        return sdp_;
    }
    std::string ip = sockets::getLocalIp();
    char buf[2048] = {0};

    snprintf(buf, sizeof(buf),
             "v=0\r\n"
             "o=- 9%ld 1 IN IP4 %s\r\n"
             "t=0 0\r\n"
             "a=control:*\r\n"
             "a=type:broadcast\r\n",
             (long)time(nullptr), ip.c_str());

    if (isStartMulticast()) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "a=rtcp-unicast: reflection\r\n");
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        uint16_t port = 0;

        if (tracks_[i].is_alive != true) {
            continue;
        }

        if (isStartMulticast()) {
            port = getMulticastDestRtpPort((TrackId)i);
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\r\n",
                 tracks_[i].sink->getMediaDescription(port).c_str());

        if (isStartMulticast())
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "c=IN IP4 %s/255\r\n", getMulticastDestAddr().c_str());
        else
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "c=IN IP4 %s\r\n", ip.c_str());

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\r\n",
                 tracks_[i].sink->getAttribute().c_str());

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "a=control:track%d\r\n", tracks_[i].track_id);
    }

    sdp_ = buf;
    return sdp_;
}

MediaSession::Track* MediaSession::getTrack(MediaSession::TrackId track_id) {
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (tracks_[i].track_id == track_id) {
            return &tracks_[i];
        }
    }

    return nullptr;
}

bool MediaSession::addSink(MediaSession::TrackId track_id,
                                   Sink* sink) {
    Track* track = getTrack(track_id);

    if (track == nullptr) {
        return false;
    }

    track->sink = sink;
    track->is_alive = true;

    sink->setSessionCb(
        [this, track](RtpPacket* rtp_packet,
                      Sink::PacketType packet_type) {
            if (packet_type == Sink::PacketType::RTPPACKET) {
                this->handleSendRtpPacket(track, rtp_packet);
            }
        });
    return true;
}

bool MediaSession::addRtpInstance(MediaSession::TrackId track_id,
                                  RtpInstance* rtp_instance) {
    Track* track = getTrack(track_id);
    if (track == nullptr || track->is_alive != true) {
        return false;
    }

    track->rtp_instances.push_back(rtp_instance);

    return true;
}

bool MediaSession::removeRtpInstance(RtpInstance* rtp_instance) {
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (tracks_[i].is_alive == false) {
            continue;
        }

        auto it = std::find(tracks_[i].rtp_instances.begin(),
                            tracks_[i].rtp_instances.end(), rtp_instance);
        if (it == tracks_[i].rtp_instances.end()) {
            continue;
        }

        tracks_[i].rtp_instances.erase(it);
        return true;
    }

    return false;
}

void MediaSession::handleSendRtpPacket(MediaSession::Track* track,
                                       RtpPacket* rtp_packet) {
    for (auto& rtp_instance : track->rtp_instances) {
        if (rtp_instance->isAlive()) {
            rtp_instance->send(rtp_packet);
        }
    }
}

bool MediaSession::startMulticast() {
    // 随机生成多播地址
    struct sockaddr_in addr = {0};
    uint32_t range = 0xE8FFFFFF - 0xE8000100;
    addr.sin_addr.s_addr = htonl(0xE8000100 + (rand()) % range);
    multicast_addr_ = inet_ntoa(addr.sin_addr);

    int rtpSockfd1, rtcpSockfd1;
    int rtpSockfd2, rtcpSockfd2;
    uint16_t rtpPort1, rtcpPort1;
    uint16_t rtpPort2, rtcpPort2;

    rtpSockfd1 = sockets::createUdpSock();
    assert(rtpSockfd1 > 0);

    rtpSockfd2 = sockets::createUdpSock();
    assert(rtpSockfd2 > 0);

    rtcpSockfd1 = sockets::createUdpSock();
    assert(rtcpSockfd1 > 0);

    rtcpSockfd2 = sockets::createUdpSock();
    assert(rtcpSockfd2 > 0);

    uint16_t port = rand() & 0xfffe;
    if (port < 10000) {
        port += 10000;
    }

    rtpPort1 = port;
    rtcpPort1 = port + 1;
    rtpPort2 = rtcpPort1 + 1;
    rtcpPort2 = rtpPort2 + 1;

    multicast_rtp_instances_[TrackId0] =
        RtpInstance::createNewOverUdp(rtpSockfd1, 0, multicast_addr_, rtpPort1);
    multicast_rtp_instances_[TrackId1] =
        RtpInstance::createNewOverUdp(rtpSockfd2, 0, multicast_addr_, rtpPort2);
    multicast_rtcp_instances_[TrackId0] =
        RtcpInstance::createNew(rtcpSockfd1, 0, multicast_addr_, rtcpPort1);
    multicast_rtcp_instances_[TrackId1] =
        RtcpInstance::createNew(rtcpSockfd2, 0, multicast_addr_, rtcpPort2);

    this->addRtpInstance(TrackId0, multicast_rtp_instances_[TrackId0]);
    this->addRtpInstance(TrackId1, multicast_rtp_instances_[TrackId1]);
    multicast_rtp_instances_[TrackId0]->setAlive(true);
    multicast_rtp_instances_[TrackId1]->setAlive(true);

    is_start_multicast_ = true;

    return true;
}

bool MediaSession::isStartMulticast() { return is_start_multicast_; }

uint16_t MediaSession::getMulticastDestRtpPort(TrackId track_id) {
    if (track_id > TrackId1 || multicast_rtp_instances_[track_id] == nullptr) {
        return -1;
    }

    return multicast_rtp_instances_[track_id]->getPeerPort();
}
#ifndef MEDIASESSION_H
#define MEDIASESSION_H
#include <string>
#include <list>

#include "RtpInstance.h"
#include "Sink.h"

#define MEDIA_MAX_TRACK_NUM 2

class MediaSession
{
public:
    enum TrackId
    {
        TrackIdNone = -1,
        TrackId0    = 0,
        TrackId1    = 1,
    };

    static MediaSession* createNew(std::string session_name);
    explicit MediaSession(const std::string& session_name);
    ~MediaSession();

public:

    std::string name() const { return session_name_; }
    std::string generateSDPDescription();
    bool addSink(MediaSession::TrackId track_id, Sink* sink);

    bool addRtpInstance(MediaSession::TrackId track_id, RtpInstance* rtp_instance);
    bool removeRtpInstance(RtpInstance* rtp_instance);


    bool startMulticast();
    bool isStartMulticast();
    std::string getMulticastDestAddr() const { return multicast_addr_; }
    uint16_t getMulticastDestRtpPort(TrackId track_id);

private:
    class Track {
    public:
        Sink* sink;
        int track_id;
        bool is_alive;
        std::list<RtpInstance*> rtp_instances;
    };

    Track* getTrack(MediaSession::TrackId track_id);
   
    void handleSendRtpPacket(MediaSession::Track* tarck, RtpPacket* rtp_packet);

private:
    std::string session_name_;
    std::string sdp_;
    Track tracks_[MEDIA_MAX_TRACK_NUM];
    bool is_start_multicast_;
    std::string multicast_addr_;
    RtpInstance* multicast_rtp_instances_[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* multicast_rtcp_instances_[MEDIA_MAX_TRACK_NUM];
};
#endif //MEDIASESSION_H
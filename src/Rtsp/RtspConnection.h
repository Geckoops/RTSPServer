#ifndef RTSPCONNECTION_H
#define RTSPCONNECTION_H
#include <string>

#include "MediaSession.h"
#include "TcpConnection.h"

class RtspServer;
class RtspConnection : public TcpConnection {
   public:
    enum Method {
        OPTIONS,
        DESCRIBE,
        SETUP,
        PLAY,
        TEARDOWN,
        NONE,
    };

    static RtspConnection* createNew(RtspServer* rtsp_server, int client_fd);

    RtspConnection(RtspServer* rtsp_server, int client_fd);
    virtual ~RtspConnection();

   protected:
    virtual void handleReadBytes();

   private:
    bool parseRequest();
    bool parseRequest1(const char* begin, const char* end);
    bool parseRequest2(const char* begin, const char* end);

    bool parseCSeq(std::string& message);
    bool parseDescribe(std::string& message);
    bool parseSetup(std::string& message);
    bool parsePlay(std::string& message);

    bool handleCmdOption();
    bool handleCmdDescribe();
    bool handleCmdSetup();
    bool handleCmdPlay();
    bool handleCmdTeardown();

    int sendMessage(char* buf, int size);
    int sendMessage();

    bool createRtpRtcpOverUdp(MediaSession::TrackId track_id, std::string peer_ip,
                              uint16_t peer_rtp_port, uint16_t peer_rtcp_port);
    bool createRtpOverTcp(MediaSession::TrackId track_id, int sockfd,
                          uint8_t rtp_channel);

    void handleRtpOverTcp();

   private:
    RtspServer* rtsp_server_;
    std::string peer_ip_;
    Method method_;
    std::string url_;
    std::string suffix_;
    uint32_t cseq_;
    std::string stream_prefix_;  // 数据流名称（作为拉流服务默认是track）

    uint16_t peer_rtp_port_;
    uint16_t peer_rtcp_port_;

    MediaSession::TrackId track_id_;  // 拉流setup请求时，当前的trackId
    RtpInstance* rtp_instances_[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* rtcp_instances_[MEDIA_MAX_TRACK_NUM];

    int session_id_;
    bool is_rtp_over_tcp_;
    uint8_t rtp_channel_;
};
#endif  // RTSPCONNECTION_H
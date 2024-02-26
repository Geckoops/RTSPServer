#ifndef RTSPSERVER_H
#define RTSPSERVER_H
#include <mutex>

#include "Event.h"
#include "UsageEnvironment.h"
#include "InetAddress.h"
class MediaSessionManager;
class RtspConnection;
class RtspServer {
   public:
    static RtspServer* createNew(UsageEnvironment* env,
                                 MediaSessionManager* session_manger,
                                 Ipv4Address& addr);

    RtspServer(UsageEnvironment* env, MediaSessionManager* session_manger,
               Ipv4Address& addr);
    ~RtspServer();

   public:
    MediaSessionManager* session_manger;
    void start();
    UsageEnvironment* env() const { return env_; }

   private:
    void handleRead();
    void handleDisConnect(int client_fd);
    void handleCloseConnect();

   private:
    UsageEnvironment* env_;
    int fd_;
    Ipv4Address addr_;
    bool listen_;
    IOEvent* accept_io_event_;
    std::mutex mtx_;

    std::map<int, RtspConnection*>
        conn_map_;  // <client_fd,conn> 维护所有被创建的连接
    std::vector<int> disconn_list_;      // 所有被取消的连接 client_fd
    TriggerEvent* close_trigger_event_;  // 关闭连接的触发事件
};
#endif  // RTSPSERVER_H

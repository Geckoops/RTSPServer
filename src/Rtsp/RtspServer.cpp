#include "RtspServer.h"

#include "Log.h"
#include "RtspConnection.h"
#include "SocketsOps.h"
#include "assert.h"

RtspServer* RtspServer::createNew(UsageEnvironment* env,
                                  MediaSessionManager* session_manger,
                                  Ipv4Address& addr) {
    return new RtspServer(env, session_manger, addr);
}
RtspServer::RtspServer(UsageEnvironment* env,
                       MediaSessionManager* session_manger, Ipv4Address& addr)
    : session_manger(session_manger), env_(env), addr_(addr), listen_(false) {
    fd_ = sockets::createTcpSock();
    sockets::setReuseAddr(fd_, 1);
    if (!sockets::bind(fd_, addr.getIp(), addr_.getPort())) {
        return;
    }

    LOGI("start server rtsp://%s:%d fd=%d", addr.getIp().data(), addr.getPort(),
         fd_);

    accept_io_event_ = IOEvent::createNew(fd_);
    accept_io_event_->setReadCallback(
        [this]() { this->handleRead(); });  // 设置回调的socket可读 函数指针
    accept_io_event_->enableReadHandling();

    close_trigger_event_ =
        TriggerEvent::createNew([this]() { this->handleCloseConnect(); });
    // 设置回调的关闭连接 函数指针
}

RtspServer::~RtspServer() {
    if (listen_) {
        env_->scheduler()->removeIOEvent(accept_io_event_);
    }

    delete accept_io_event_;
    delete close_trigger_event_;

    sockets::close(fd_);
}

void RtspServer::start() {
    LOGI("RtspServer start");
    listen_ = true;
    sockets::listen(fd_, 60);
    env_->scheduler()->addIOEvent(accept_io_event_);
}

void RtspServer::handleRead() {
    int client_fd = sockets::accept(fd_);
    if (client_fd < 0) {
        LOGE("handleRead error,client_fd=%d", client_fd);
        return;
    }
    RtspConnection* conn = RtspConnection::createNew(this, client_fd);
    conn->setDisConnectCallback(
        [this](int client_fd) { this->handleDisConnect(client_fd); });
    conn_map_.insert({client_fd, conn});
}

void RtspServer::handleDisConnect(int client_fd) {
    std::lock_guard<std::mutex> lck(mtx_);
    disconn_list_.push_back(client_fd);

    env_->scheduler()->addTriggerEvent(close_trigger_event_);
}

void RtspServer::handleCloseConnect() {
    std::lock_guard<std::mutex> lck(mtx_);

    for (auto it : disconn_list_) {
        auto fd = conn_map_.find(it);
        assert(fd != conn_map_.end());
        delete fd->second;
        conn_map_.erase(fd);
    }

    disconn_list_.clear();
}
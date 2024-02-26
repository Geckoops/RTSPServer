#include "TcpConnection.h"

#include "Log.h"
#include "SocketsOps.h"

TcpConnection::TcpConnection(UsageEnvironment* env, int client_fd)
    : env_(env), client_fd_(client_fd) {
    client_io_event_ = IOEvent::createNew(client_fd);
    client_io_event_->setReadCallback([this]() { this->handleRead(); });
    client_io_event_->setWriteCallback([this]() { this->handleWrite(); });
    client_io_event_->setErrorCallback([this]() { this->handleError(); });
    client_io_event_->enableReadHandling();  // 默认只开启读

    //    client_io_event_->enableWriteHandling();
    //    client_io_event_->enableErrorHandling();

    env_->scheduler()->addIOEvent(client_io_event_);
}

TcpConnection::~TcpConnection() {
    env_->scheduler()->removeIOEvent(client_io_event_);
    delete client_io_event_;

    sockets::close(client_fd_);
}

void TcpConnection::setDisConnectCallback(DisConnectCallback cb) {
    disconnect_callback_ = cb;
}

void TcpConnection::enableReadHandling() {
    if (client_io_event_->isReadHandling()) {
        return;
    }

    client_io_event_->enableReadHandling();
    env_->scheduler()->updateIOEvent(client_io_event_);
}

void TcpConnection::enableWriteHandling() {
    if (client_io_event_->isWriteHandling()) {
        return;
    }

    client_io_event_->enableWriteHandling();
    env_->scheduler()->updateIOEvent(client_io_event_);
}

void TcpConnection::enableErrorHandling() {
    if (client_io_event_->isErrorHandling()) {
        return;
    }

    client_io_event_->enableErrorHandling();
    env_->scheduler()->updateIOEvent(client_io_event_);
}

void TcpConnection::disableReadeHandling() {
    if (!client_io_event_->isReadHandling()) {
        return;
    }

    client_io_event_->disableReadeHandling();
    env_->scheduler()->updateIOEvent(client_io_event_);
}

void TcpConnection::disableWriteHandling() {
    if (!client_io_event_->isWriteHandling()) {
        return;
    }

    client_io_event_->disableWriteHandling();
    env_->scheduler()->updateIOEvent(client_io_event_);
}

void TcpConnection::disableErrorHandling() {
    if (!client_io_event_->isErrorHandling()) {
        return;
    }

    client_io_event_->disableErrorHandling();
    env_->scheduler()->updateIOEvent(client_io_event_);
}

void TcpConnection::handleDisConnect() {
    if (disconnect_callback_) {
        disconnect_callback_(client_fd_);
    }
}

void TcpConnection::handleRead() {
    LOGI("handleRead, fd=%d", client_fd_);
    int ret = input_buffer_.read(client_fd_);

    if (ret <= 0) {
        LOGE("disconnecting fd=%d, ret=%d", client_fd_, ret);
        handleDisConnect();
        return;
    }

    handleReadBytes();  // 调用RtspConnecton对象的实现函数
}

void TcpConnection::handleReadBytes() { input_buffer_.retrieveAll(); }

void TcpConnection::handleWrite() {
    LOGI("handleWrite, fd=%d", client_fd_);
    output_buffer_.retrieveAll();
}

void TcpConnection::handleError() { LOGI("handleError fd=%d", client_fd_); }

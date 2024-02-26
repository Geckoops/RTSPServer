#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include <functional>

#include "Event.h"
#include "UsageEnvironment.h"
#include "Buffer.h"

class TcpConnection {
   public:
    using DisConnectCallback = std::function<void(int)>;

    TcpConnection(UsageEnvironment* env, int client_fd);
    virtual ~TcpConnection();

    void setDisConnectCallback(DisConnectCallback cb);

   protected:
    void enableReadHandling();
    void enableWriteHandling();
    void enableErrorHandling();
    void disableReadeHandling();
    void disableWriteHandling();
    void disableErrorHandling();

    void handleRead();
    virtual void handleReadBytes();
    virtual void handleWrite();
    virtual void handleError();

    void handleDisConnect();

   protected:
    UsageEnvironment* env_;
    int client_fd_;
    IOEvent* client_io_event_;
    DisConnectCallback disconnect_callback_;
    Buffer input_buffer_;
    Buffer output_buffer_;
    char buffer_[2048];
};

#endif  // TCPCONNECTION_H
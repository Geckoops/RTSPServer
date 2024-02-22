#ifndef SELECTPOLLER_H
#define SELECTPOLLER_H
#include <vector>

#include "Poller.h"

#ifndef _WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#else

#include <WS2tcpip.h>
#include <WinSock2.h>

#endif  //_WIN32

class SelectPoller : public Poller {
   public:
    SelectPoller();
    virtual ~SelectPoller();

    static SelectPoller* createNew();

    virtual bool addIOEvent(IOEvent* event);
    virtual bool updateIOEvent(IOEvent* event);
    virtual bool removeIOEvent(IOEvent* event);
    virtual void handleEvent();

   private:
    fd_set read_set_;
    fd_set write_set_;
    fd_set exception_set_;
    int max_num_sockets_;
    std::vector<IOEvent*> io_events_;  // 存储临时活跃的IO事件对象
};
#endif  // SELECTPOLLER_H
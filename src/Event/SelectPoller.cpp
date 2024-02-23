#include "SelectPoller.h"

#include "Log.h"

SelectPoller::SelectPoller() {
    FD_ZERO(&read_set_);
    FD_ZERO(&write_set_);
    FD_ZERO(&exception_set_);
}

SelectPoller::~SelectPoller() {}

SelectPoller* SelectPoller::createNew() {
    return new SelectPoller();
}

bool SelectPoller::addIOEvent(IOEvent* event) { return updateIOEvent(event); }

bool SelectPoller::updateIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        LOGE("fd=%d", fd);

        return false;
    }

    FD_CLR(fd, &read_set_);
    FD_CLR(fd, &write_set_);
    FD_CLR(fd, &exception_set_);

    auto it = event_map_.find(fd);
    if (it != event_map_.end())  // 先前已经添加则修改
    {
        if (event->isReadHandling()) {
            FD_SET(fd, &read_set_);
        }
        if (event->isWriteHandling()) {
            FD_SET(fd, &write_set_);
        }
        if (event->isErrorHandling()) {
            FD_SET(fd, &exception_set_);
        }
    } else  // 先前未添加则添加IO事件
    {
        if (event->isReadHandling()) {
            FD_SET(fd, &read_set_);
        }
        if (event->isWriteHandling()) {
            FD_SET(fd, &write_set_);
        }
        if (event->isErrorHandling()) {
            FD_SET(fd, &exception_set_);
        }

        event_map_.insert({fd, event});
    }

    if (event_map_.empty())
        max_num_sockets_ = 0;
    else
        max_num_sockets_ = event_map_.rbegin()->first + 1;
    // 更新最大文件描述符+1

    return true;
}

bool SelectPoller::removeIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        return false;
    }

    FD_CLR(fd, &read_set_);
    FD_CLR(fd, &write_set_);
    FD_CLR(fd, &exception_set_);

    auto it = event_map_.find(fd);
    if (it != event_map_.end()) {
        event_map_.erase(it);
    }

    if (event_map_.empty()) {
        max_num_sockets_ = 0;
    } else {
        max_num_sockets_ = event_map_.rbegin()->first + 1;
    }

    return true;
}

void SelectPoller::handleEvent() {
    fd_set read_set = read_set_;
    fd_set write_set = write_set_;
    fd_set exception_set = exception_set_;
    struct timeval timeout;
    int ret;
    int r_event;

    timeout.tv_sec = 1000;  // 秒
    timeout.tv_usec = 0;    // 微秒
    // LOGI("event_map_.size() = %d，select io start", event_map_.size());

    ret = select(max_num_sockets_, &read_set, &write_set, &exception_set, &timeout);

    if (ret < 0) {
        return;
    } else {
        // LOGI("检测到活跃的描述符 ret=%d", ret);
    }
    // LOGI("select io end");

    for (auto [it1, it2] : event_map_) {
        r_event = 0;
        if (FD_ISSET(it1, &read_set)) r_event |= IOEvent::EVENT_READ;

        if (FD_ISSET(it1, &write_set)) r_event |= IOEvent::EVENT_WRITE;

        if (FD_ISSET(it1, &exception_set)) r_event |= IOEvent::EVENT_ERROR;

        if (r_event != 0) {
            it2->setREvent(r_event);
            io_events_.push_back(it2);
        }
    }

    for (auto& io_event : io_events_) {
        io_event->handleEvent();
    }

    io_events_.clear();
}

#include "Event.h"

#include "Log.h"


TriggerEvent* TriggerEvent::createNew(EventCallback cb) {
    return new TriggerEvent(cb);
}

TriggerEvent::TriggerEvent(EventCallback cb) : trigger_callback_(cb) {
    LOGI("TriggerEvent()");
}

TriggerEvent::~TriggerEvent() { LOGI("~TriggerEvent()"); }

void TriggerEvent::handleEvent() {
    if (trigger_callback_) {
        trigger_callback_();
    }
}

TimerEvent* TimerEvent::createNew(EventCallback cb) {
    return new TimerEvent(cb);
}

TimerEvent::TimerEvent(EventCallback cb)
    : timeout_callback_(cb), is_stop_(false) {
    LOGI("TimerEvent()");
}

TimerEvent::~TimerEvent() { LOGI("~TimerEvent()"); }

bool TimerEvent::handleEvent() {
    if (is_stop_) {
        return is_stop_;
    }

    if (timeout_callback_) {
        timeout_callback_();
    }

    return is_stop_;
}

void TimerEvent::stop() { is_stop_ = true; }

IOEvent* IOEvent::createNew(int fd) {
    if (fd < 0) {
        return NULL;
    }

    return new IOEvent(fd);
}

IOEvent::IOEvent(int fd)
    : fd_(fd),
      event_(EVENT_NONE),
      r_event_(EVENT_NONE),
      read_callback_(NULL),
      write_callback_(NULL),
      error_callback_(NULL) {
    LOGI("IOEvent() fd=%d", fd_);
}

IOEvent::~IOEvent() { LOGI("~IOEvent() fd=%d", fd_); }

void IOEvent::handleEvent() {
    if (read_callback_ && (r_event_ & EVENT_READ)) {
        read_callback_();
    }

    if (write_callback_ && (r_event_ & EVENT_WRITE)) {
        write_callback_();
    }

    if (error_callback_ && (r_event_ & EVENT_ERROR)) {
        error_callback_();
    }
};

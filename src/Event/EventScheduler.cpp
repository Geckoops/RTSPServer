#include "EventScheduler.h"

#include <thread>

#include "Log.h"
#include "SelectPoller.h"

#ifndef _WIN32
#include <sys/eventfd.h>

#include "SocketsOps.h"
#endif  //_WIN32

EventScheduler* EventScheduler::createNew(PollerType type) {
    if (type != POLLER_SELECT && type != POLLER_POLL && type != POLLER_EPOLL) {
        return nullptr;
    }

    return new EventScheduler(type);
}

EventScheduler::EventScheduler(PollerType type) : quit_(false) {
    switch (type) {
        case POLLER_SELECT:
            poller_ = SelectPoller::createNew();
            break;

            // case POLLER_POLL:
            //     poller_ = PollPoller::createNew();
            //     break;

            // case POLLER_EPOLL:
            //     poller_ = EPollPoller::createNew();
            //     break;

        default:
            exit(-1);
            break;
    }

    timer_manager_ = TimerManager::createNew(this);
    // WIN系统的定时器回调由子线程托管，非WIN系统则通过select网络模型
}

EventScheduler::~EventScheduler() {
    delete timer_manager_;
    delete poller_;

#ifdef _WIN32
    WSACleanup();
#endif  // _WIN32
}

bool EventScheduler::addTriggerEvent(TriggerEvent* event) {
    trigger_events_.push_back(event);

    return true;
}

Timer::TimerId EventScheduler::addTimedEventRunAfter(
    TimerEvent* event, Timer::TimeInterval delay) {
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += delay;

    return timer_manager_->addTimer(event, timestamp, 0);
}

Timer::TimerId EventScheduler::addTimedEventRunAt(TimerEvent* event,
                                                  Timer::Timestamp when) {
    return timer_manager_->addTimer(event, when, 0);
}

Timer::TimerId EventScheduler::addTimedEventRunEvery(
    TimerEvent* event, Timer::TimeInterval interval) {
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += interval;

    return timer_manager_->addTimer(event, timestamp, interval);
}

bool EventScheduler::removeTimedEvent(Timer::TimerId timerId) {
    return timer_manager_->removeTimer(timerId);
}

bool EventScheduler::addIOEvent(IOEvent* event) {
    return poller_->addIOEvent(event);
}

bool EventScheduler::updateIOEvent(IOEvent* event) {
    return poller_->updateIOEvent(event);
}

bool EventScheduler::removeIOEvent(IOEvent* event) {
    return poller_->removeIOEvent(event);
}

void EventScheduler::loop() {
#ifdef _WIN32
    std::thread(
        [](EventScheduler* sch) {
            while (!sch->quit_) {
                if (sch->timer_manager_read_callback_) {
                    sch->timer_manager_read_callback_();
                }
            }
        },
        this)
        .detach();
#endif  // _WIN32

    while (!quit_) {
        handleTriggerEvents();
        poller_->handleEvent();
    }
}

void EventScheduler::handleTriggerEvents() {
    if (!trigger_events_.empty()) {
        for (auto trigger_event : trigger_events_) {
            trigger_event->handleEvent();
        }

        trigger_events_.clear();
    }
}

Poller* EventScheduler::poller() { return poller_; }

void EventScheduler::setTimerManagerReadCallback(EventCallback cb) {
    timer_manager_read_callback_ = cb;
}

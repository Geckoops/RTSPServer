#include "Timer.h"
#ifndef _WIN32
#include <sys/timerfd.h>

#include "Log.h"
#include "Poller.h"

#endif  //_WIN32
#include <time.h>

#include <chrono>

#include "Event.h"
#include "EventScheduler.h"

// struct timespec {
//     time_t tv_sec; //Seconds
//     long   tv_nsec;// Nanoseconds
// };
// struct itimerspec {
//     struct timespec it_interval;  //Interval for periodic timer
//     （定时间隔周期） struct timespec it_value;     //Initial expiration
//     (第一次超时时间)
// };
//     it_interval不为0 表示是周期性定时器
//     it_value和it_interval都为0 表示停止定时器

#ifndef _WIN32
static bool timerFdSetTime(int fd, Timer::Timestamp when,
                           Timer::TimeInterval period) {
    struct itimerspec new_val;

    new_val.it_value.tv_sec = when / 1000;                 // ms->s
    new_val.it_value.tv_nsec = when % 1000 * 1000 * 1000;  // ms->ns
    new_val.it_interval.tv_sec = period / 1000;
    new_val.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

    int old_value = timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_val, nullptr);
    if (old_value < 0) {
        return false;
    }
    return true;
}
#endif  //_WIN32

Timer::Timer(TimerEvent* event, Timestamp timestamp, TimeInterval time_interval,
             TimerId timer_id)
    : timer_event_(event),
      timestamp_(timestamp),
      timer_interval_(time_interval),
      timer_id_(timer_id) {
    if (time_interval > 0) {
        repeat_ = true;  // 循环定时器
    } else {
        repeat_ = false;  // 一次性定时器
    }
}

Timer::~Timer() {}

Timer::Timestamp Timer::getCurTime() {
#ifndef _WIN32
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1000 + now.tv_nsec / 1000000);
#else
    long long now = std::chrono::steady_clock::now().time_since_epoch().count();
    return now / 1000000;
#endif  //_WIN32
}

Timer::Timestamp Timer::getCurTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool Timer::handleEvent() {
    if (timer_event_ == nullptr) {
        return false;
    }
    return timer_event_->handleEvent();
}

TimerManager* TimerManager::createNew(EventScheduler* scheduler) {
    if (scheduler == nullptr) {
        return nullptr;
    }
    return new TimerManager(scheduler);
}

TimerManager::TimerManager(EventScheduler* scheduler)
    : poller_(scheduler->poller()), last_timer_id_(0) {
#ifndef _WIN32
    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (timer_fd_ < 0) {
        LOGE("create TimerFd error");
        return;
    } else {
        LOGI("fd=%d", timer_fd_);
    }
    timer_io_event_ = IOEvent::createNew(timer_fd_);
    timer_io_event_->setReadCallback([this] { this->handleRead(); });
    timer_io_event_->enableReadHandling();
    modifyTimeout();
    poller_->addIOEvent(timer_io_event_);
#else
    scheduler->setTimerManagerReadCallback([this]() { this->handleRead(); });

#endif  //_WIN32
}

TimerManager::~TimerManager() {
#ifndef _WIN32
    poller_->removeIOEvent(timer_io_event_);
    delete timer_io_event_;
#endif  //_WIN32
}

Timer::TimerId TimerManager::addTimer(TimerEvent* event,
                                      Timer::Timestamp timestamp,
                                      Timer::TimeInterval time_interval) {
    ++last_timer_id_;
    Timer timer(event, timestamp, time_interval, last_timer_id_);

    timers_.insert({last_timer_id_, timer});
    events_.insert({timestamp, timer});
    modifyTimeout();

    return last_timer_id_;
}

bool TimerManager::removeTimer(Timer::TimerId timer_id) {
    auto it = timers_.find(timer_id);
    if (it != timers_.end()) {
        timers_.erase(timer_id);
        // TODO 还需要删除events_的事件
    }

    modifyTimeout();

    return true;
}

void TimerManager::modifyTimeout() {
#ifndef _WIN32
    auto it = events_.begin();
    if (it != events_.end()) {  // 存在至少一个定时器
        Timer timer = it->second;
        timerFdSetTime(timer_fd_, timer.timestamp_, timer.timer_interval_);
    } else {
        timerFdSetTime(timer_fd_, 0, 0);
    }
#endif  // _WIN32
}

void TimerManager::handleRead() {
    // LOGI("timers_.size()=%d,events_.size()=%d",timers_.size(),events_.size());
    Timer::Timestamp timestamp = Timer::getCurTime();
    if (!timers_.empty() && !events_.empty()) {
        auto it = events_.begin();
        Timer timer = it->second;
        int expire = timer.timestamp_ - timestamp;

        // LOGI("timestamp=%d,timestamp_=%d,expire=%d,time_interval=%d",
        // timestamp, timer.timestamp_, expire, timer.timer_interval_);

        if (timestamp > timer.timestamp_ || expire == 0) {
            bool timerEventIsStop = timer.handleEvent();
            events_.erase(it);
            if (timer.repeat_) {
                if (timerEventIsStop) {
                    timers_.erase(timer.timer_id_);
                } else {
                    timer.timestamp_ = timestamp + timer.timer_interval_;
                    events_.insert({timer.timestamp_, timer});
                }
            } else {
                timers_.erase(timer.timer_id_);
            }
        }
    }
    modifyTimeout();
}

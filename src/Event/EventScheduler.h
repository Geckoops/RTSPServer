#ifndef EVENTSCHEDULER_H
#define EVENTSCHEDULER_H

#include <mutex>
#include <vector>

#include "Event.h"
#include "Timer.h"

class Poller;

class EventScheduler {
   public:
    enum PollerType { POLLER_SELECT, POLLER_POLL, POLLER_EPOLL };
    static EventScheduler* createNew(PollerType type);

    explicit EventScheduler(PollerType type);
    virtual ~EventScheduler();

   public:
    bool addTriggerEvent(TriggerEvent* event);
    Timer::TimerId addTimedEventRunAfter(TimerEvent* event,
                                          Timer::TimeInterval delay);
    Timer::TimerId addTimedEventRunAt(TimerEvent* event, Timer::Timestamp when);
    Timer::TimerId addTimedEventRunEvery(TimerEvent* event,
                                         Timer::TimeInterval interval);
    bool removeTimedEvent(Timer::TimerId timerId);
    bool addIOEvent(IOEvent* event);
    bool updateIOEvent(IOEvent* event);
    bool removeIOEvent(IOEvent* event);

    void loop();
    Poller* poller();
    void setTimerManagerReadCallback(EventCallback cb);

   private:
    void handleTriggerEvents();

   private:
    bool quit_;
    Poller* poller_;
    TimerManager* timer_manager_;
    std::vector<TriggerEvent*> trigger_events_;

    std::mutex mtx_;

    // WIN系统专用的定时器回调start
    EventCallback timer_manager_read_callback_;
    // WIN系统专用的定时器回调end
};

#endif  // EVENTSCHEDULER_H
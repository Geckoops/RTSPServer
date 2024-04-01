#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

#include <map>

class EventScheduler;
class Poller;
class TimerEvent;
class IOEvent;

class Timer {
   public:
    typedef uint32_t TimerId;
    typedef int64_t Timestamp;      // ms
    typedef uint32_t TimeInterval;  // ms

    ~Timer();

    static Timestamp getCurTime();  // 获取当前系统启动以来的毫秒数
    static Timestamp getCurTimestamp();  // 获取毫秒级时间戳（13位）

   private:
    friend class TimerManager;
    Timer(TimerEvent* event, Timestamp timestamp, TimeInterval time_interval,
          TimerId timer_id);

   private:
    bool handleEvent();

   private:
    TimerEvent* timer_event_;
    Timestamp timestamp_;
    TimeInterval timer_interval_;
    TimerId timer_id_;

    bool repeat_;
};

class TimerManager {
   public:
    static TimerManager* createNew(EventScheduler* scheduler);

    TimerManager(EventScheduler* scheduler);
    ~TimerManager();

    Timer::TimerId addTimer(TimerEvent* event, Timer::Timestamp timestamp,
                            Timer::TimeInterval time_interval);
    bool removeTimer(Timer::TimerId timer_id);

   private:
    void handleRead();
    void modifyTimeout();

   private:
    Poller* poller_;
    std::map<Timer::TimerId, Timer> timers_;
    std::multimap<Timer::Timestamp, Timer> events_;
    uint32_t last_timer_id_;
#ifndef _WIN32
    int timer_fd_;
    IOEvent* timer_io_event_;
#endif  //_WIN32
};

#endif  // TIMER_H
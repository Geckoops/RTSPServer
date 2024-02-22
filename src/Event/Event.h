#ifndef EVENT_H
#define EVENT_H

#include <functional>

using EventCallback = std::function<void()>;

class TriggerEvent {
   public:
    static TriggerEvent* createNew(EventCallback cb);

    TriggerEvent(EventCallback cb);
    ~TriggerEvent();

    void setTriggerCallback(EventCallback cb) { trigger_callback_ = cb; }
    void handleEvent();

   private:
    EventCallback trigger_callback_;
};

class TimerEvent {
   public:
    static TimerEvent* createNew(EventCallback cb);

    TimerEvent(EventCallback cb);
    ~TimerEvent();

    void setTimeoutCallback(EventCallback cb) { timeout_callback_ = cb; }
    bool handleEvent();

    void stop();

   private:
    EventCallback timeout_callback_;
    bool is_stop_;
};

class IOEvent {
   public:
    enum IOEventType {
        EVENT_NONE = 0,
        EVENT_READ = 1,
        EVENT_WRITE = 2,
        EVENT_ERROR = 4,
    };

    static IOEvent* createNew(int fd);

    IOEvent(int fd);
    ~IOEvent();

    int getFd() const { return fd_; }
    int getEvent() const { return event_; }
    void setREvent(int event) { r_event_ = event; }

    void setReadCallback(EventCallback cb) { read_callback_ = cb; };
    void setWriteCallback(EventCallback cb) { write_callback_ = cb; };
    void setErrorCallback(EventCallback cb) { error_callback_ = cb; };

    void enableReadHandling() { event_ |= EVENT_READ; }
    void enableWriteHandling() { event_ |= EVENT_WRITE; }
    void enableErrorHandling() { event_ |= EVENT_ERROR; }
    void disableReadeHandling() { event_ &= ~EVENT_READ; }
    void disableWriteHandling() { event_ &= ~EVENT_WRITE; }
    void disableErrorHandling() { event_ &= ~EVENT_ERROR; }

    bool isNoneHandling() const { return event_ == EVENT_NONE; }
    bool isReadHandling() const { return (event_ & EVENT_READ) != 0; }
    bool isWriteHandling() const { return (event_ & EVENT_WRITE) != 0; }
    bool isErrorHandling() const { return (event_ & EVENT_ERROR) != 0; };

    void handleEvent();

   private:
    int fd_;
    int event_;
    int r_event_;
    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback error_callback_;
};

#endif  // EVENT_H
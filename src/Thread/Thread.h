#ifndef THREAD_H
#define THREAD_H
#include <functional>
#include <thread>

class Thread {
   public:
    Thread();
    virtual ~Thread();
    using ThreadFunc = std::function<void()>;

    bool start(ThreadFunc func);
    bool detach();
    bool join();

   private:
    bool is_start_;
    bool is_detach_;
    std::thread th_;
};

#endif  // THREAD_H

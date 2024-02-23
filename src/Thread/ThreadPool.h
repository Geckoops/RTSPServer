#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "Thread.h"

class ThreadPool {
   public:
    class Task {
       public:
        using TaskCallback = std::function<void()>;
        Task(TaskCallback cb) : mTaskCallback(cb){};

        void setTaskCallback(TaskCallback cb) { mTaskCallback = cb; }

        void handle() { mTaskCallback(); }

        Task& operator=(const Task& task) {
            if (this != &task) {
                this->mTaskCallback = task.mTaskCallback;
            }
            return *this;
        }

       private:
        TaskCallback mTaskCallback;
    };

    static ThreadPool* createNew(int num);

    explicit ThreadPool(int num);
    ~ThreadPool();

    void addTask(Task& task);

   private:
    void createThreads();
    void cancelThreads();

   private:
    std::queue<Task> tasks_;
    std::mutex mtx_;               // 互斥锁.
    std::condition_variable cv_;  // 条件变量.

    std::vector<Thread> threads_;
    bool quit_;
};

#endif  // THREADPOOL_H

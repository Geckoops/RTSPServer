#include "ThreadPool.h"

ThreadPool* ThreadPool::createNew(int num) { return new ThreadPool(num); }

ThreadPool::ThreadPool(int num) : threads_(num), quit_(false) {
    createThreads();
}

ThreadPool::~ThreadPool() { cancelThreads(); }

void ThreadPool::addTask(ThreadPool::Task& task) {
    std::unique_lock<std::mutex> lck(mtx_);
    tasks_.push(task);
    cv_.notify_one();
}

void ThreadPool::createThreads() {
    std::unique_lock<std::mutex> lck(mtx_);
    for (auto& th : threads_) {
        th.start([this]() {
            while (!quit_) {
                std::unique_lock<std::mutex> lck(mtx_);
                if (tasks_.empty()) {
                    cv_.wait(lck);
                }

                if (tasks_.empty()) {
                    continue;
                }

                Task task = tasks_.front();
                tasks_.pop();

                task.handle();
            }
        });
    }
}

void ThreadPool::cancelThreads() {
    std::unique_lock<std::mutex> lck(mtx_);
    quit_ = false;
    cv_.notify_all();
    for (auto& th : threads_) {
        th.join();
    }

    threads_.clear();
}
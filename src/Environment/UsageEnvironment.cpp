#include "UsageEnvironment.h"

UsageEnvironment* UsageEnvironment::createNew(EventScheduler* scheduler,
                                              ThreadPool* threadPool) {
    return new UsageEnvironment(scheduler, threadPool);
}

UsageEnvironment::UsageEnvironment(EventScheduler* scheduler,
                                   ThreadPool* threadPool)
    : scheduler_(scheduler), thread_pool_(threadPool) {}

UsageEnvironment::~UsageEnvironment() {}

EventScheduler* UsageEnvironment::scheduler() { return scheduler_; }

ThreadPool* UsageEnvironment::threadPool() { return thread_pool_; }
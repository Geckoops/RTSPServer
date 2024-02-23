#ifndef USAGEENVIRONMENT_H
#define USAGEENVIRONMENT_H

#include "EventScheduler.h"
#include "ThreadPool.h"

class UsageEnvironment {
   public:
    static UsageEnvironment* createNew(EventScheduler* scheduler,
                                       ThreadPool* threadPool);

    UsageEnvironment(EventScheduler* scheduler, ThreadPool* threadPool);
    ~UsageEnvironment();

    EventScheduler* scheduler();
    ThreadPool* threadPool();

   private:
    EventScheduler* scheduler_;
    ThreadPool* thread_pool_;
};

#endif  // USAGEENVIRONMENT_H
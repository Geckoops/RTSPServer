#ifndef H264MEDIASOURCE_H
#define H264MEDIASOURCE_H

#include <string>

#include "MediaSource.h"

class H264MediaSource : public MediaSource {
   public:
    static H264MediaSource* createNew(UsageEnvironment* env,
                                      const std::string& file);

    H264MediaSource(UsageEnvironment* env, const std::string& file);
    virtual ~H264MediaSource();

   protected:
    virtual void handleTask();

   private:
    int getFrameFromH264File(uint8_t* frame, unsigned int size);

   private:
    FILE* file_;
};

#endif  // H264MEDIASOURCE_H
#ifndef H264MEDIASOURCE_H
#define H264MEDIASOURCE_H

#include <string>

#include "MediaSource.h"
#include "Encryptor.h"
#include <memory>

class H264MediaSource : public MediaSource {
   public:
    static H264MediaSource* createNew(UsageEnvironment* env,
                                      const std::string& file,
                                      const std::string& secret_key = "");

    virtual ~H264MediaSource();

   protected:
    H264MediaSource(UsageEnvironment* env, const std::string& file,
                    const std::string& secret_key);

    virtual void handleTask();

   private:
    int getFrameFromH264File(uint8_t* frame, unsigned int size);

   private:
    FILE* file_;
    bool encrypt_;
    std::shared_ptr<Encryptor> encryptor_;
};

#endif  // H264MEDIASOURCE_H
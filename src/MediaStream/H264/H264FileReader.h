#ifndef H264FILEREADER_H
#define H264FILEREADER_H

#include <string>

#include "MediaFileReader.h"


class H264FileReader : public MediaFileReader {
   public:
    static H264FileReader* createNew(UsageEnvironment* env,
                                     const std::string& file);

    H264FileReader(UsageEnvironment* env, const std::string& file);
    virtual ~H264FileReader();

   protected:
    virtual void handleTask();

   private:
    int getFrameFromH264File(uint8_t* frame, unsigned int size);

   private:
    FILE* file_;
};

#endif  // H264FILEREADER_H
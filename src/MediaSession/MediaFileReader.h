#ifndef MEDIAFILEREADER_H
#define MEDIAFILEREADER_H
#include <stdint.h>

#include <mutex>
#include <queue>
#include <string>

#include "ThreadPool.h"
#include "UsageEnvironment.h"

#define FRAME_MAX_SIZE (1024 * 200)
#define DEFAULT_FRAME_NUM 4

class MediaFrame {
   public:
    MediaFrame() : temp(new uint8_t[FRAME_MAX_SIZE]), buf(nullptr), size(0) {}
    ~MediaFrame() { delete[] temp; }

    uint8_t* temp;  // 容器
    uint8_t* buf;   // 引用容器
    int size;
};

class MediaFileReader {
   public:
    explicit MediaFileReader(UsageEnvironment* env);
    virtual ~MediaFileReader();

    MediaFrame* getFrameFromOutputQueue();         // 从输出队列获取帧
    void putFrameToInputQueue(MediaFrame* frame);  // 把帧送入输入队列
    std::string getFileName() { return file_name_; }

   protected:
    virtual void handleTask() = 0;

   protected:
    UsageEnvironment* env_;
    MediaFrame frames_[DEFAULT_FRAME_NUM];
    std::queue<MediaFrame*> frame_input_queue_;
    std::queue<MediaFrame*> frame_output_queue_;

    std::mutex mtx_;
    ThreadPool::Task task_;
    std::string file_name_;
};
#endif  // MEDIAFILEREADER_H
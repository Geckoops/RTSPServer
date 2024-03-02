#include "MediaSource.h"

#include "Log.h"
MediaSource::MediaSource(UsageEnvironment* env)
    : env_(env), task_([this]() { this->handleTask(); }) {
    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        frame_input_queue_.push(&frames_[i]);
    }
}

MediaSource::~MediaSource() { LOGI("~MediaSource()"); }

MediaFrame* MediaSource::getFrameFromOutputQueue() {
    std::lock_guard<std::mutex> lck(mtx_);
    if (frame_output_queue_.empty()) {
        return nullptr;
    }
    MediaFrame* frame = frame_output_queue_.front();
    frame_output_queue_.pop();

    return frame;
}

void MediaSource::putFrameToInputQueue(MediaFrame* frame) {
    std::lock_guard<std::mutex> lck(mtx_);

    frame_input_queue_.push(frame);

    env_->threadPool()->addTask(task_);
}
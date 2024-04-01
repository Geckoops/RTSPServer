#include "H264MediaSource.h"

#include <fcntl.h>

#include "Log.h"

static inline bool startCode3(uint8_t* buf);
static inline bool startCode4(uint8_t* buf);

H264MediaSource* H264MediaSource::createNew(UsageEnvironment* env,
                                            const std::string& file,
                                            const std::string& secret_key) {
    return new H264MediaSource(env, file, secret_key);
}

H264MediaSource::H264MediaSource(UsageEnvironment* env, const std::string& file,
                                 const std::string& secret_key)
    : MediaSource(env) {
    file_name_ = file;
    file_ = fopen(file.c_str(), "rb");
    if (file_ == nullptr) {
        LOGE("Open H264 file:%s error", file.c_str());
        exit(-1);
    }

    if (!secret_key.empty()) {
        encrypt_ = true;
        encryptor_.reset(Encryptor::createNew(secret_key));
    }

    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        env_->threadPool()->addTask(task_);
    }
}

H264MediaSource::~H264MediaSource() {
    LOGI("File closed: %s", file_name_.c_str());
    fclose(file_);
}

void H264MediaSource::handleTask() {
    std::lock_guard<std::mutex> lck(mtx_);

    if (frame_input_queue_.empty()) {
        return;
    }

    MediaFrame* frame = frame_input_queue_.front();
    int start_code_num = 0;

    while (true) {
        frame->size = getFrameFromH264File(frame->temp, FRAME_MAX_SIZE);
        if (frame->size < 0) {  // 可能是读到文件结尾了，试试再读一次
            frame->size = getFrameFromH264File(frame->temp, FRAME_MAX_SIZE);
            if (frame->size < 0) {  // 读文件错误，退出
                exit(0);
            }
        }
        if (startCode3(frame->temp)) {
            start_code_num = 3;
        } else {
            start_code_num = 4;
        }
        frame->buf = frame->temp + start_code_num;
        frame->size -= start_code_num;

        uint8_t nalu_type = frame->buf[0] & 0x1F;
        // LOGI("start_code_num=%d,nalu_type=%d,nalu_size=%d", start_code_num,
        // nalu_type, frame->size);

        if (nalu_type == 0x09) {  // 分隔符，没有数据
            continue;
        } else if (nalu_type == 0x07 || nalu_type == 0x08) {  // SPS和PPS
            break;
        } else {
            break;
        }
    }

    frame_input_queue_.pop();
    frame_output_queue_.push(frame);
}

static inline bool startCode3(uint8_t* buf) {
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static inline bool startCode4(uint8_t* buf) {
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static uint8_t* findNextStartCode(uint8_t* buf, int len) {
    if (len < 3) {
        return nullptr;
    }

    for (int i = 0; i < len - 3; ++i) {
        if (startCode3(buf) || startCode4(buf)) {
            return buf;
        }

        ++buf;
    }

    if (startCode3(buf)) {
        return buf;
    }

    return nullptr;
}

int H264MediaSource::getFrameFromH264File(uint8_t* frame, unsigned int size) {
    if (file_ == nullptr) {
        LOGE("Read %s error, file not open", file_name_.c_str());
        return -1;
    }

    int r, frame_size;
    uint8_t* next_start_code;

    r = fread(frame, 1, size, file_);
    if (!startCode3(frame) && !startCode4(frame)) {
        fseek(file_, 0, SEEK_SET);
        LOGE("Read %s error, no start_code found", file_name_.c_str());
        return -1;
    }

    next_start_code = findNextStartCode(frame + 3, r - 3);
    if (next_start_code == nullptr) {
        fseek(file_, 0, SEEK_SET);
        frame_size = -1;
        LOGE("Read %s error, no next_start_code, r=%d", file_name_.c_str(), r);
    } else {
        frame_size = (next_start_code - frame);
        if (encrypt_) {  // 从第二个字节开始加密
            encryptor_->xorBuffers(frame + 1, frame_size - 1);
        }
        fseek(file_, frame_size - r, SEEK_CUR);
    }
    return frame_size;
}
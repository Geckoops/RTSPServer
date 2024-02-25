#include "AACFileReader.h"

#include <string.h>

#include "Log.h"

AACFileReader* AACFileReader::createNew(UsageEnvironment* env,
                                        const std::string& file) {
    return new AACFileReader(env, file);
}

AACFileReader::AACFileReader(UsageEnvironment* env, const std::string& file)
    : MediaFileReader(env) {
    file_name_ = file;
    file_ = fopen(file.c_str(), "rb");
    if (file_ == nullptr) {
        LOGE("Open AAC file:%s error", file.c_str());
        exit(-1);
    }

    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        env_->threadPool()->addTask(task_);
    }
}

AACFileReader::~AACFileReader() {
    LOGI("File closed: %s", file_name_.c_str());
    fclose(file_);
}

void AACFileReader::handleTask() {
    std::lock_guard<std::mutex> lck(mtx_);

    if (frame_input_queue_.empty()) {
        return;
    }
    MediaFrame* frame = frame_input_queue_.front();

    frame->size = getFrameFromAACFile(frame->temp, FRAME_MAX_SIZE);
    if (frame->size < 0) {
        return;
    }
    frame->buf = frame->temp;

    frame_input_queue_.pop();
    frame_output_queue_.push(frame);
}

bool AACFileReader::parseAdtsHeader(uint8_t* in,
                                    struct AdtsHeader* adts_header) {
    memset(adts_header, 0, sizeof(*adts_header));

    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)) {
        adts_header->id = ((unsigned int)in[1] & 0x08) >> 3;
        adts_header->layer = ((unsigned int)in[1] & 0x06) >> 1;
        adts_header->protection_absent = (unsigned int)in[1] & 0x01;
        adts_header->profile = ((unsigned int)in[2] & 0xc0) >> 6;
        adts_header->sampling_freq_index = ((unsigned int)in[2] & 0x3c) >> 2;
        adts_header->private_bit = ((unsigned int)in[2] & 0x02) >> 1;
        adts_header->channel_cfg = ((((unsigned int)in[2] & 0x01) << 2) |
                                    (((unsigned int)in[3] & 0xc0) >> 6));
        adts_header->original_copy = ((unsigned int)in[3] & 0x20) >> 5;
        adts_header->home = ((unsigned int)in[3] & 0x10) >> 4;
        adts_header->copyright_identification_bit =
            ((unsigned int)in[3] & 0x08) >> 3;
        adts_header->copyright_identification_start =
            (unsigned int)in[3] & 0x04 >> 2;
        adts_header->aac_frame_length =
            (((((unsigned int)in[3]) & 0x03) << 11) |
             (((unsigned int)in[4] & 0xFF) << 3) |
             ((unsigned int)in[5] & 0xE0) >> 5);
        adts_header->adts_buffer_fullness = (((unsigned int)in[5] & 0x1f) << 6 |
                                             ((unsigned int)in[6] & 0xfc) >> 2);
        adts_header->number_of_raw_data_block_in_frame =
            ((unsigned int)in[6] & 0x03);

        return true;
    } else {
        LOGE("failed to parse adts header");
        return false;
    }
}

int AACFileReader::getFrameFromAACFile(uint8_t* buf, unsigned int size) {
    if (file_ == nullptr) {
        LOGE("Read %s error, file not open", file_name_.c_str());
        return -1;
    }

    uint8_t tmp_buf[7];
    int ret;
    ret = fread(tmp_buf, 1, 7, file_);
    if (ret <= 0) {
        fseek(file_, 0, SEEK_SET);
        ret = fread(tmp_buf, 1, 7, file_);
        if (ret <= 0) {
            return -1;
        }
    }

    if (!parseAdtsHeader(tmp_buf, &adts_header_)) {
        return -1;
    }

    if (adts_header_.aac_frame_length > size) {
        return -1;
    }

    memcpy(buf, tmp_buf, 7);
    ret = fread(buf + 7, 1, adts_header_.aac_frame_length - 7, file_);
    if (ret < 0) {
        LOGE("read error");
        return -1;
    }

    return adts_header_.aac_frame_length;
}
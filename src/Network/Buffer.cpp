#include "Buffer.h"

#include <assert.h>
#include <stdlib.h>

#include <algorithm>

#include "SocketsOps.h"

const int Buffer::kInitialSize = 1024;
const char* Buffer::kCRLF = "\r\n";

Buffer::Buffer() : buffer_size_(kInitialSize), read_index_(0), write_index_(0) {
    buffer_ = new char[buffer_size_];
}

Buffer::~Buffer() { delete[] buffer_; }

int Buffer::read(int fd) {
    char extrabuf[65536];
    const int writable = writableBytes();
    const int n = sockets::recv(fd, extrabuf, sizeof(extrabuf));
    if (n <= 0) {
        return -1;
    } else if (n <= writable) {
        std::copy(extrabuf, extrabuf + n, beginWrite());
        write_index_ += n;

    } else {
        std::copy(extrabuf, extrabuf + writable, beginWrite());
        write_index_ += writable;
        append(extrabuf + writable, n - writable);
    }
    return n;
}

int Buffer::write(int fd) { return sockets::send(fd, peek(), readableBytes()); }

int Buffer::readableBytes() const { return write_index_ - read_index_; }

int Buffer::writableBytes() const { return buffer_size_ - write_index_; }

int Buffer::prependableBytes() const { return read_index_; }

char* Buffer::peek() { return begin() + read_index_; }

const char* Buffer::peek() const { return begin() + read_index_; }

const char* Buffer::findCRLF() const {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
}

const char* Buffer::findCRLF(const char* start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
}

const char* Buffer::findLastCrlf() const {
    const char* crlf = std::find_end(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
}

void Buffer::retrieveReadZero() {  // 本次读取的数据不全，重新从头读取
    read_index_ = 0;
}

void Buffer::retrieve(int len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
        read_index_ += len;
    } else {
        retrieveAll();
    }
}

void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}

void Buffer::retrieveAll() {  // 恢复索引
    read_index_ = 0;
    write_index_ = 0;
}

char* Buffer::beginWrite() { return begin() + write_index_; }

const char* Buffer::beginWrite() const { return begin() + write_index_; }

void Buffer::unwrite(int len) {
    assert(len <= readableBytes());
    write_index_ -= len;
}

/* 确保有足够的空间 */
void Buffer::ensureWritableBytes(int len) {
    if (writableBytes() < len) {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}

void Buffer::makeSpace(int len) {
    if (writableBytes() + prependableBytes() < len) {  // 如果剩余空间不足
        /* 扩大空间 */
        buffer_size_ = write_index_ + len;
        buffer_ = (char*)realloc(buffer_, buffer_size_);
    } else {  // 剩余空间足够
        /* 移动内容 */
        int readable = readableBytes();
        std::copy(begin() + read_index_, begin() + write_index_, begin());
        read_index_ = 0;
        write_index_ = read_index_ + readable;
        assert(readable == readableBytes());
    }
}

void Buffer::append(const char* data, int len) {
    ensureWritableBytes(len);                   // 调整扩大的空间
    std::copy(data, data + len, beginWrite());  // 拷贝数据

    assert(len <= writableBytes());
    write_index_ += len;  // 重新调节写位置
}

void Buffer::append(const void* data, int len) {
    append((const char*)(data), len);
}

char* Buffer::begin() { return buffer_; }

const char* Buffer::begin() const { return buffer_; }
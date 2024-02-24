#ifndef BUFFER_H
#define BUFFER_H

class Buffer {
   public:
    static const int kInitialSize;

    explicit Buffer();
    ~Buffer();

    int readableBytes() const;

    int writableBytes() const;

    int prependableBytes() const;

    char* peek();

    const char* peek() const;

    const char* findCRLF() const;

    const char* findCRLF(const char* start) const;

    const char* findLastCrlf() const;

    void retrieveReadZero();

    void retrieve(int len);

    void retrieveUntil(const char* end);

    void retrieveAll();

    char* beginWrite();

    const char* beginWrite() const;

    void unwrite(int len);

    /* 确保有足够的空间 */
    void ensureWritableBytes(int len);

    void makeSpace(int len);

    void append(const char* data, int len);

    void append(const void* data, int len);

    int read(int fd);

    int write(int fd);

   private:
    char* begin();

    const char* begin() const;

   private:
    char* buffer_;
    int buffer_size_;
    int read_index_;   // 当前
    int write_index_;  // 当前从socket实际读取到的字节长度

    static const char* kCRLF;
};

#endif  // BUFFER_H
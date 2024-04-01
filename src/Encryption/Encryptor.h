#ifndef ENCRYPT_H
#define ENCRYPT_H

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>

class Encryptor {
   public:
    static Encryptor* createNew(
        const std::string& secret_key,
        std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func =
            nullptr);
    static Encryptor* createNew(
        const std::vector<uint8_t>& secret_key,
        std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func =
            nullptr);

    void xorBuffers(std::vector<uint8_t>& buffer);
    void xorBuffers(const std::vector<uint8_t>& buffer,
                    std::vector<uint8_t>& result);
    void xorBuffers(uint8_t* buffer, size_t size);

   private:
    Encryptor(
        const std::string& secret_key,
        std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func);
    Encryptor(
        const std::vector<uint8_t>& secret_key,
        std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func);
    std::vector<uint8_t> secret_key_hash_;
};  // namespace Encryptor

#endif  // ENCRYPT_H
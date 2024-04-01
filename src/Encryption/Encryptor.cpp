#include "Encryptor.h"

#include "Hash.h"

Encryptor* Encryptor::createNew(
    const std::string& secret_key,
    std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func) {
    if (!secret_key.empty()) {
        return new Encryptor(secret_key, hash_func);
    }
    return nullptr;
}

Encryptor* Encryptor::createNew(
    const std::vector<uint8_t>& secret_key,
    std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func) {
    if (!secret_key.empty()) {
        return new Encryptor(secret_key, hash_func);
    }
    return nullptr;
}

Encryptor::Encryptor(
    const std::string& secret_key,
    std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func) {
    std::vector<uint8_t> secret_key_vec(secret_key.begin(), secret_key.end());

    secret_key_hash_ = hash_func ? hash_func(secret_key_vec)
                                 : Hash::transformSecretKey(secret_key_vec);
}

Encryptor::Encryptor(
    const std::vector<uint8_t>& secret_key,
    std::function<std::vector<uint8_t>(std::vector<uint8_t>)> hash_func) {
    secret_key_hash_ = hash_func ? hash_func(secret_key)
                                 : Hash::transformSecretKey(secret_key);
}

void Encryptor::xorBuffers(std::vector<uint8_t>& buffer) {
    auto it = buffer.begin();
    while (it != buffer.end()) {
        for (size_t i = 0; i < secret_key_hash_.size(); ++i) {
            *it = *it ^ secret_key_hash_[i];
            ++it;
        }
    }
}

void Encryptor::xorBuffers(const std::vector<uint8_t>& buffer,
                           std::vector<uint8_t>& result) {
    result.resize(buffer.size());

    auto it = buffer.begin();
    auto it_result = result.begin();
    while (it != buffer.end()) {
        for (size_t i = 0; i < secret_key_hash_.size(); ++i) {
            *it_result = *it ^ secret_key_hash_[i];
            ++it;
            ++it_result;
        }
    }
}

void Encryptor::xorBuffers(uint8_t* buffer, size_t size) {
    auto buffer_end = buffer + size;
    while (buffer < buffer_end) {
        for (size_t i = 0; i < secret_key_hash_.size(); ++i) {
            *buffer = *buffer ^ secret_key_hash_[i];
            buffer++;
        }
    }
}
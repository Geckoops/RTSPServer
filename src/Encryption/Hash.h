#ifndef HASH_H
#define HASH_H

#include <stdint.h>

#include <string>
#include <vector>

static const int kKeyLength = 1024;

namespace Hash {
unsigned int murmurHash3(const std::vector<uint8_t>& input);

std::vector<uint8_t> transformSecretKey(const std::vector<uint8_t>& secret_key);

std::string transfromToBase64(const uint8_t* data, int size);

}  // namespace Hash

#endif  // HASH_H
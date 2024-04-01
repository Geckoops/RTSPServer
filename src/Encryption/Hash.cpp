#include "Hash.h"

#include <stdexcept>
#include <vector>


namespace Hash {

const int KHashSeed = 1024;

unsigned int murmurHash3(const std::vector<uint8_t>& input,
                         const unsigned int seed) {
    const unsigned int c1 = 0xcc9e2d51;
    const unsigned int c2 = 0x1b873593;
    const unsigned int r1 = 15;
    const unsigned int r2 = 13;
    const unsigned int m = 5;
    const unsigned int n = 0xe6546b64;

    unsigned int hash = seed;
    unsigned int len = static_cast<unsigned int>(input.size());
    const unsigned int* data =
        reinterpret_cast<const unsigned int*>(input.data());

    for (unsigned int i = 0; i < len / 4; ++i) {
        unsigned int k = data[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    const unsigned char* tail =
        reinterpret_cast<const unsigned char*>(&data[len / 4]);
    unsigned int k = 0;

    switch (len & 3) {
        case 3:
            k ^= tail[2] << 16;
        case 2:
            k ^= tail[1] << 8;
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            hash ^= k;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

std::vector<uint8_t> transformSecretKey(const std::vector<uint8_t>& secret_key) {
    if (secret_key.empty()) {
        throw std::invalid_argument("Secret key cannot be empty");
    }

    std::vector<uint8_t> result;
    result.reserve(kKeyLength + 3);

    while (result.size() < kKeyLength) {
        unsigned int hash = murmurHash3(secret_key, KHashSeed + result.size());
        result.push_back(static_cast<uint8_t>(hash & 0xff));
        result.push_back(static_cast<uint8_t>((hash >> 8) & 0xff));
        result.push_back(static_cast<uint8_t>((hash >> 16) & 0xff));
        result.push_back(static_cast<uint8_t>((hash >> 24) & 0xff));
    }
    result.resize(kKeyLength);

    return result;
}

std::string transfromToBase64(const uint8_t* data, int size) {
    std::string result;
    static const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (size--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                              ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                              ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                result += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] =
            ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] =
            ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            result += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            result += '=';
        }
    }

    return result;
}

}  // namespace Hash
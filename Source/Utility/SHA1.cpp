#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <filesystem>

#include "Utility/SHA1.h"

class SHA1 {
public:
    SHA1() { reset(); }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            block[blockByteIndex++] = data[i];
            byteCount++;
            if (blockByteIndex == 64) {
                processBlock();
                blockByteIndex = 0;
            }
        }
    }

    void finalize() {
        block[blockByteIndex++] = 0x80;
        if (blockByteIndex > 56) {
            while (blockByteIndex < 64)
                block[blockByteIndex++] = 0;
            processBlock();
            blockByteIndex = 0;
        }

        while (blockByteIndex < 56)
            block[blockByteIndex++] = 0;

        uint64_t bitCount = byteCount * 8;
        for (int i = 7; i >= 0; --i)
            block[blockByteIndex++] = static_cast<uint8_t>((bitCount >> (i * 8)) & 0xFF);

        processBlock();
    }

    std::string hexDigest() const {
        std::ostringstream result;
        for (int i = 0; i < 5; ++i)
            result << std::hex << std::setw(8) << std::setfill('0') << hash[i];
        return result.str();
    }

private:
    uint32_t hash[5];
    uint8_t block[64];
    size_t blockByteIndex = 0;
    uint64_t byteCount = 0;

    static uint32_t rotateLeft(uint32_t value, uint32_t bits) {
        return (value << bits) | (value >> (32 - bits));
    }

    void reset() {
        hash[0] = 0x67452301;
        hash[1] = 0xEFCDAB89;
        hash[2] = 0x98BADCFE;
        hash[3] = 0x10325476;
        hash[4] = 0xC3D2E1F0;
        blockByteIndex = 0;
        byteCount = 0;
    }

    void processBlock() {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(block[i * 4 + 3]));
        }

        for (int i = 16; i < 80; ++i)
            w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

        uint32_t a = hash[0];
        uint32_t b = hash[1];
        uint32_t c = hash[2];
        uint32_t d = hash[3];
        uint32_t e = hash[4];

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotateLeft(b, 30);
            b = a;
            a = temp;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
    }
};

std::string sha1_from_file(const std::filesystem::path& filename) {
    SHA1 sha1;
    std::ifstream file(filename, std::ios::binary);
    std::vector<uint8_t> buffer(65536);

    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        sha1.update(buffer.data(), file.gcount());
    }

    sha1.finalize();
    return sha1.hexDigest();
}

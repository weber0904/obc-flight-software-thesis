#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace OBC {

namespace {

constexpr FwSizeType SHA256_BLOCK_SIZE = 64U;

static_assert(!std::numeric_limits<FwSizeType>::is_signed,
              "Command auth crypto assumes FwSizeType remains unsigned per the F' platform contract");

class Sha256Accumulator final {
  public:
    Sha256Accumulator() : m_state{0x6A09E667U,
                                  0xBB67AE85U,
                                  0x3C6EF372U,
                                  0xA54FF53AU,
                                  0x510E527FU,
                                  0x9B05688CU,
                                  0x1F83D9ABU,
                                  0x5BE0CD19U},
                          m_totalBytes(0U),
                          m_bufferSize(0U) {}

    void update(const U8* data, FwSizeType size) {
        if (data == nullptr || size == 0U) {
            return;
        }

        this->m_totalBytes += static_cast<U64>(size);

        FwSizeType offset = 0U;
        while (offset < size) {
            const FwSizeType copySize =
                std::min(static_cast<FwSizeType>(SHA256_BLOCK_SIZE - this->m_bufferSize),
                         static_cast<FwSizeType>(size - offset));
            std::copy_n(data + offset, copySize, this->m_buffer.data() + this->m_bufferSize);
            this->m_bufferSize += copySize;
            offset += copySize;

            if (this->m_bufferSize == SHA256_BLOCK_SIZE) {
                this->transform_(this->m_buffer.data());
                this->m_bufferSize = 0U;
            }
        }
    }

    void final(U8 digest[COMMAND_AUTH_SHA256_DIGEST_SIZE]) {
        this->m_buffer[this->m_bufferSize++] = 0x80U;

        if (this->m_bufferSize > (SHA256_BLOCK_SIZE - 8U)) {
            std::fill(this->m_buffer.begin() + this->m_bufferSize, this->m_buffer.end(), 0U);
            this->transform_(this->m_buffer.data());
            this->m_bufferSize = 0U;
        }

        std::fill(this->m_buffer.begin() + this->m_bufferSize, this->m_buffer.begin() + (SHA256_BLOCK_SIZE - 8U), 0U);

        const U64 totalBits = this->m_totalBytes * 8U;
        for (FwSizeType i = 0U; i < 8U; i++) {
            this->m_buffer[SHA256_BLOCK_SIZE - 1U - i] = static_cast<U8>((totalBits >> (i * 8U)) & 0xFFU);
        }
        this->transform_(this->m_buffer.data());

        for (FwSizeType i = 0U; i < this->m_state.size(); i++) {
            const U32 value = this->m_state[i];
            digest[(i * 4U) + 0U] = static_cast<U8>((value >> 24U) & 0xFFU);
            digest[(i * 4U) + 1U] = static_cast<U8>((value >> 16U) & 0xFFU);
            digest[(i * 4U) + 2U] = static_cast<U8>((value >> 8U) & 0xFFU);
            digest[(i * 4U) + 3U] = static_cast<U8>(value & 0xFFU);
        }
    }

  private:
    static U32 rotateRight_(U32 value, U32 count) {
        return (value >> count) | (value << (32U - count));
    }

    static U32 choose_(U32 x, U32 y, U32 z) {
        return (x & y) ^ (~x & z);
    }

    static U32 majority_(U32 x, U32 y, U32 z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    static U32 bigSigma0_(U32 value) {
        return rotateRight_(value, 2U) ^ rotateRight_(value, 13U) ^ rotateRight_(value, 22U);
    }

    static U32 bigSigma1_(U32 value) {
        return rotateRight_(value, 6U) ^ rotateRight_(value, 11U) ^ rotateRight_(value, 25U);
    }

    static U32 smallSigma0_(U32 value) {
        return rotateRight_(value, 7U) ^ rotateRight_(value, 18U) ^ (value >> 3U);
    }

    static U32 smallSigma1_(U32 value) {
        return rotateRight_(value, 17U) ^ rotateRight_(value, 19U) ^ (value >> 10U);
    }

    static U32 loadBigEndian32_(const U8* data) {
        return (static_cast<U32>(data[0]) << 24U) | (static_cast<U32>(data[1]) << 16U) |
               (static_cast<U32>(data[2]) << 8U) | static_cast<U32>(data[3]);
    }

    void transform_(const U8 block[SHA256_BLOCK_SIZE]) {
        static const std::array<U32, 64> K = {0x428A2F98U,
                                              0x71374491U,
                                              0xB5C0FBCFU,
                                              0xE9B5DBA5U,
                                              0x3956C25BU,
                                              0x59F111F1U,
                                              0x923F82A4U,
                                              0xAB1C5ED5U,
                                              0xD807AA98U,
                                              0x12835B01U,
                                              0x243185BEU,
                                              0x550C7DC3U,
                                              0x72BE5D74U,
                                              0x80DEB1FEU,
                                              0x9BDC06A7U,
                                              0xC19BF174U,
                                              0xE49B69C1U,
                                              0xEFBE4786U,
                                              0x0FC19DC6U,
                                              0x240CA1CCU,
                                              0x2DE92C6FU,
                                              0x4A7484AAU,
                                              0x5CB0A9DCU,
                                              0x76F988DAU,
                                              0x983E5152U,
                                              0xA831C66DU,
                                              0xB00327C8U,
                                              0xBF597FC7U,
                                              0xC6E00BF3U,
                                              0xD5A79147U,
                                              0x06CA6351U,
                                              0x14292967U,
                                              0x27B70A85U,
                                              0x2E1B2138U,
                                              0x4D2C6DFCU,
                                              0x53380D13U,
                                              0x650A7354U,
                                              0x766A0ABBU,
                                              0x81C2C92EU,
                                              0x92722C85U,
                                              0xA2BFE8A1U,
                                              0xA81A664BU,
                                              0xC24B8B70U,
                                              0xC76C51A3U,
                                              0xD192E819U,
                                              0xD6990624U,
                                              0xF40E3585U,
                                              0x106AA070U,
                                              0x19A4C116U,
                                              0x1E376C08U,
                                              0x2748774CU,
                                              0x34B0BCB5U,
                                              0x391C0CB3U,
                                              0x4ED8AA4AU,
                                              0x5B9CCA4FU,
                                              0x682E6FF3U,
                                              0x748F82EEU,
                                              0x78A5636FU,
                                              0x84C87814U,
                                              0x8CC70208U,
                                              0x90BEFFFAU,
                                              0xA4506CEBU,
                                              0xBEF9A3F7U,
                                              0xC67178F2U};

        std::array<U32, 64> schedule = {};
        for (FwSizeType i = 0U; i < 16U; i++) {
            schedule[i] = loadBigEndian32_(block + (i * 4U));
        }
        for (FwSizeType i = 16U; i < schedule.size(); i++) {
            schedule[i] = smallSigma1_(schedule[i - 2U]) + schedule[i - 7U] + smallSigma0_(schedule[i - 15U]) +
                          schedule[i - 16U];
        }

        U32 a = this->m_state[0];
        U32 b = this->m_state[1];
        U32 c = this->m_state[2];
        U32 d = this->m_state[3];
        U32 e = this->m_state[4];
        U32 f = this->m_state[5];
        U32 g = this->m_state[6];
        U32 h = this->m_state[7];

        for (FwSizeType i = 0U; i < schedule.size(); i++) {
            const U32 temp1 = h + bigSigma1_(e) + choose_(e, f, g) + K[i] + schedule[i];
            const U32 temp2 = bigSigma0_(a) + majority_(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        this->m_state[0] += a;
        this->m_state[1] += b;
        this->m_state[2] += c;
        this->m_state[3] += d;
        this->m_state[4] += e;
        this->m_state[5] += f;
        this->m_state[6] += g;
        this->m_state[7] += h;
    }

  private:
    std::array<U32, 8> m_state;
    U64 m_totalBytes;
    std::array<U8, SHA256_BLOCK_SIZE> m_buffer = {};
    FwSizeType m_bufferSize;
};

}  // namespace

void sha256Bytes(const U8* data, FwSizeType dataLength, U8 digest[COMMAND_AUTH_SHA256_DIGEST_SIZE]) {
    Sha256Accumulator hash;
    hash.update(data, dataLength);
    hash.final(digest);
}

bool hmacSha256(const U8* key,
                FwSizeType keyLength,
                const U8* data,
                FwSizeType dataLength,
                U8 digest[COMMAND_AUTH_SHA256_DIGEST_SIZE]) {
    if (key == nullptr || keyLength == 0U || data == nullptr || digest == nullptr) {
        return false;
    }

    std::array<U8, SHA256_BLOCK_SIZE> normalizedKey = {};
    if (keyLength > SHA256_BLOCK_SIZE) {
        sha256Bytes(key, keyLength, normalizedKey.data());
    } else {
        std::copy_n(key, keyLength, normalizedKey.data());
    }

    std::array<U8, SHA256_BLOCK_SIZE> innerPad = {};
    std::array<U8, SHA256_BLOCK_SIZE> outerPad = {};
    for (FwSizeType i = 0U; i < SHA256_BLOCK_SIZE; i++) {
        innerPad[i] = static_cast<U8>(normalizedKey[i] ^ 0x36U);
        outerPad[i] = static_cast<U8>(normalizedKey[i] ^ 0x5CU);
    }

    U8 innerDigest[COMMAND_AUTH_SHA256_DIGEST_SIZE] = {};
    {
        Sha256Accumulator hash;
        hash.update(innerPad.data(), innerPad.size());
        hash.update(data, dataLength);
        hash.final(innerDigest);
    }

    {
        Sha256Accumulator hash;
        hash.update(outerPad.data(), outerPad.size());
        hash.update(innerDigest, sizeof(innerDigest));
        hash.final(digest);
    }

    return true;
}

bool constantTimeEqual(const U8* lhs, const U8* rhs, FwSizeType size) {
    if (lhs == nullptr || rhs == nullptr) {
        return false;
    }

    U8 diff = 0U;
    for (FwSizeType i = 0U; i < size; i++) {
        diff = static_cast<U8>(diff | (lhs[i] ^ rhs[i]));
    }
    return diff == 0U;
}

}  // namespace OBC

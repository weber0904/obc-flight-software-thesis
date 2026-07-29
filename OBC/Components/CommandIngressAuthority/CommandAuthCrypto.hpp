#ifndef OBC_CommandAuthCrypto_HPP
#define OBC_CommandAuthCrypto_HPP

#include <Fw/FPrimeBasicTypes.hpp>

namespace OBC {

constexpr FwSizeType COMMAND_AUTH_SHA256_DIGEST_SIZE = 32U;

void sha256Bytes(const U8* data, FwSizeType dataLength, U8 digest[COMMAND_AUTH_SHA256_DIGEST_SIZE]);

bool hmacSha256(const U8* key,
                FwSizeType keyLength,
                const U8* data,
                FwSizeType dataLength,
                U8 digest[COMMAND_AUTH_SHA256_DIGEST_SIZE]);

bool constantTimeEqual(const U8* lhs, const U8* rhs, FwSizeType size);

}  // namespace OBC

#endif

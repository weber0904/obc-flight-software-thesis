#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

#include <cstring>
#include <limits>

namespace OBC {

static_assert(!std::numeric_limits<FwSizeType>::is_signed,
              "Command authority auth configuration assumes FwSizeType remains unsigned per the F' platform contract");

AuthorityConfig authorityConfigFromProfile(const char* profile) {
    AuthorityConfig config;
    if (profile == nullptr) {
        return config;
    }
    if (std::strcmp(profile, "sband-primary") == 0) {
        config.identity = AuthorityLinkIdentity::SBAND;
        config.role = AuthorityLinkRole::PRIMARY;
        config.valid = true;
    } else if (std::strcmp(profile, "uhf-primary") == 0) {
        config.identity = AuthorityLinkIdentity::UHF;
        config.role = AuthorityLinkRole::PRIMARY_AFTER_FAILOVER;
        config.valid = true;
    } else if (std::strcmp(profile, "uhf-backup") == 0) {
        config.identity = AuthorityLinkIdentity::UHF;
        config.role = AuthorityLinkRole::BACKUP;
        config.valid = true;
    } else if (std::strcmp(profile, "dev-direct") == 0) {
        config.identity = AuthorityLinkIdentity::DEV_DIRECT;
        config.role = AuthorityLinkRole::DEV_FULL;
        config.valid = true;
    } else if (std::strcmp(profile, "internal") == 0) {
        config.identity = AuthorityLinkIdentity::INTERNAL;
        config.role = AuthorityLinkRole::INTERNAL;
        config.valid = true;
    }
    return config;
}

U32 expectedSourceIdForAuthorityIdentity(AuthorityLinkIdentity identity) {
    switch (identity) {
        case AuthorityLinkIdentity::UNKNOWN:
            return 0U;
        case AuthorityLinkIdentity::SBAND:
            return 1U;
        case AuthorityLinkIdentity::UHF:
            return 2U;
        case AuthorityLinkIdentity::DEV_DIRECT:
            return 3U;
        case AuthorityLinkIdentity::INTERNAL:
            return 4U;
    }
    return 0U;
}

U8 secureServiceIdForAuthorityIdentity(AuthorityLinkIdentity identity) {
    switch (identity) {
        case AuthorityLinkIdentity::SBAND:
            return 1U;
        case AuthorityLinkIdentity::UHF:
            return 2U;
        case AuthorityLinkIdentity::UNKNOWN:
        case AuthorityLinkIdentity::DEV_DIRECT:
        case AuthorityLinkIdentity::INTERNAL:
        default:
            return 0U;
    }
}

bool configureAuthorityAuth(AuthorityConfig& config,
                            U32 sourceId,
                            U16 keySlot,
                            const U8* keyBytes,
                            FwSizeType keyLength,
                            CommandAuthAlgorithm algorithm) {
    const U32 expectedSourceId = expectedSourceIdForAuthorityIdentity(config.identity);
    if (!config.valid || sourceId == 0U || keySlot == 0U || keyBytes == nullptr || keyLength == 0U ||
        keyLength > CommandAuthConfig::MAX_KEY_BYTES || algorithm != CommandAuthAlgorithm::HMAC_SHA256 ||
        expectedSourceId == 0U || sourceId != expectedSourceId) {
        config.auth = CommandAuthConfig();
        return false;
    }

    config.auth.enabled = true;
    config.auth.sourceId = sourceId;
    config.auth.keySlot = keySlot;
    config.auth.algorithm = algorithm;
    config.auth.keyLength = static_cast<U8>(keyLength);
    std::memcpy(config.auth.keyBytes, keyBytes, keyLength);
    if (keyLength < CommandAuthConfig::MAX_KEY_BYTES) {
        std::memset(config.auth.keyBytes + keyLength, 0, CommandAuthConfig::MAX_KEY_BYTES - keyLength);
    }
    return true;
}

bool isCommManagedAuthority(const AuthorityConfig& config) {
    if (!config.valid) {
        return true;
    }

    return config.identity == AuthorityLinkIdentity::SBAND || config.identity == AuthorityLinkIdentity::UHF;
}

bool isRestrictedAuthority(const AuthorityConfig& config) {
    if (!config.valid) {
        return true;
    }

    return isCommManagedAuthority(config) && config.role == AuthorityLinkRole::BACKUP;
}

}  // namespace OBC

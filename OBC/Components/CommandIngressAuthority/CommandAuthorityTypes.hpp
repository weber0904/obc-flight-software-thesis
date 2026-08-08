#ifndef OBC_CommandAuthorityTypes_HPP
#define OBC_CommandAuthorityTypes_HPP

#include <cstring>

#include <Fw/Cmd/CmdResponseEnumAc.hpp>
#include <Fw/FPrimeBasicTypes.hpp>

namespace OBC {

enum class AuthorityLinkIdentity : U32 {
    UNKNOWN = 0,
    SBAND = 1,
    UHF = 2,
    DEV_DIRECT = 3,
    INTERNAL = 4,
};

enum class AuthorityLinkRole : U32 {
    UNKNOWN = 0,
    PRIMARY = 1,
    BACKUP = 2,
    PRIMARY_AFTER_FAILOVER = 3,
    DEV_FULL = 4,
    INTERNAL = 5,
};

enum class AuthorityCommandClass : U32 {
    UNKNOWN = 0,
    READ_STATUS = 1,
    MODE_CHANGE = 2,
    SAFETY_EMERGENCY = 3,
    FILE_TRANSFER = 4,
    CONFIG_UPDATE = 5,
    RESET = 6,
    PAYLOAD_CONTROL = 7,
    ADCS_CONTROL = 8,
    POWER_CONTROL = 9,
    COMM_CONTROL = 10,
    CSP_TRAFFIC = 11,
    DEV_INTERNAL = 12,
    DATA_PRODUCT = 13,
};

enum class AuthorityResourceLabel : U32 {
    NONE = 0,
    MODE_STATE = 1,
    EPS = 2,
    ADCS = 3,
    GPS = 4,
    RADIO = 5,
    STORAGE = 6,
    BOOT = 7,
    HEALTH = 8,
    CSP = 9,
    COMM_LINK = 10,
    FILE_TRANSFER_SESSION = 11,
    CONFIG_STORE = 12,
    DATA_PRODUCTS = 13,
    EVENT_FILTER = 14,
    COMMAND_DISPATCH = 15,
    PAYLOAD = 16,
};

enum class AuthorityRejectReason : U32 {
    NONE = 0,
    POLICY_DENIED = 1,
    MALFORMED_RESTRICTED = 2,
    INVALID_CONFIG = 3,
    UNKNOWN_OPCODE_RESTRICTED = 4,
    ENVELOPE_REQUIRED = 5,
};

enum class CommandAuthAlgorithm : U32 {
    NONE = 0,
    HMAC_SHA256 = 1,
};

struct CommandAuthConfig {
    static constexpr FwSizeType MAX_KEY_BYTES = 64U;

    bool enabled = false;
    U32 sourceId = 0;
    U16 keySlot = 0;
    CommandAuthAlgorithm algorithm = CommandAuthAlgorithm::NONE;
    U8 keyLength = 0;
    U8 keyBytes[MAX_KEY_BYTES] = {};
};

struct AuthorityConfig {
    AuthorityLinkIdentity identity = AuthorityLinkIdentity::UNKNOWN;
    AuthorityLinkRole role = AuthorityLinkRole::UNKNOWN;
    bool valid = false;
    CommandAuthConfig auth = {};
};

struct AuthorityDecision {
    bool allow = false;
    AuthorityRejectReason reason = AuthorityRejectReason::POLICY_DENIED;
    Fw::CmdResponse response = Fw::CmdResponse::VALIDATION_ERROR;
    AuthorityCommandClass commandClass = AuthorityCommandClass::UNKNOWN;
    AuthorityResourceLabel resource = AuthorityResourceLabel::NONE;
};

AuthorityConfig authorityConfigFromProfile(const char* profile);

U32 expectedSourceIdForAuthorityIdentity(AuthorityLinkIdentity identity);

U8 secureServiceIdForAuthorityIdentity(AuthorityLinkIdentity identity);

bool configureAuthorityAuth(AuthorityConfig& config,
                            U32 sourceId,
                            U16 keySlot,
                            const U8* keyBytes,
                            FwSizeType keyLength,
                            CommandAuthAlgorithm algorithm = CommandAuthAlgorithm::HMAC_SHA256);

bool isCommManagedAuthority(const AuthorityConfig& config);

bool isRestrictedAuthority(const AuthorityConfig& config);

}  // namespace OBC

#endif

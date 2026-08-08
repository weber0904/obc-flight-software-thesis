#ifndef OBC_BootManifestVerifier_HPP
#define OBC_BootManifestVerifier_HPP

#include <string>

#include "Fw/FPrimeBasicTypes.hpp"
#include "OBC/Types/BootSlotEnumAc.hpp"

namespace OBC {

constexpr U32 BOOT_TRUST_REJECT_NONE = 0U;
constexpr U32 BOOT_TRUST_REJECT_MANIFEST_MISSING = 11U;
constexpr U32 BOOT_TRUST_REJECT_MANIFEST_MALFORMED = 12U;
constexpr U32 BOOT_TRUST_REJECT_SIGNER_UNKNOWN = 13U;
constexpr U32 BOOT_TRUST_REJECT_SIGNATURE_INVALID = 14U;
constexpr U32 BOOT_TRUST_REJECT_VERSION_DOWNGRADE = 15U;
constexpr U32 BOOT_TRUST_REJECT_TARGET_SLOT_MISMATCH = 16U;
constexpr U32 BOOT_TRUST_REJECT_CONFIG_INVALID = 17U;
constexpr U32 BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH = 18U;
constexpr U32 BOOT_TRUST_REJECT_MANIFEST_SIZE_MISMATCH = 19U;
constexpr U32 BOOT_TRUST_REJECT_IMAGE_PATH_MISMATCH = 20U;

constexpr U32 BOOT_TRUST_STATUS_UNVERIFIED = 0U;
constexpr U32 BOOT_TRUST_STATUS_TRUSTED = 1U;
constexpr U32 BOOT_TRUST_STATUS_REJECTED = 2U;
constexpr U32 BOOT_TRUST_STATUS_PENDING = 3U;
constexpr U32 BOOT_TRUST_STATUS_CONFIRMED = 4U;

struct BootTrustConfig {
    std::string algorithm = "hmac-sha256";
    std::string trustedSignerId = "repo-dev-boot-signer";
    U32 trustedKeySlot = 1U;
    std::string trustedKeyHex = "424f4f545f54525553545f434841494e5f56315f4445565f4b4559";
};

struct BootManifest {
    std::string imagePath;
    U32 imageSize = 0U;
    std::string imageDigest;
    OBC::BootSlot targetSlot = OBC::BootSlot::NONE;
    std::string imageId;
    U32 softwareVersion = 0U;
    std::string signerId;
    U32 keySlot = 0U;
    std::string signatureAlgorithm;
    std::string signature;
};

struct BootManifestDecision {
    bool accepted = false;
    U32 rejectReason = BOOT_TRUST_REJECT_NONE;
    BootManifest manifest = {};
};

bool computeBootManifestSignatureHex(const BootManifest& manifest,
                                     const BootTrustConfig& config,
                                     std::string& signatureHex);

BootManifestDecision verifyBootManifestFile(const std::string& manifestPath,
                                            const std::string& requestedImagePath,
                                            U32 actualImageSize,
                                            const std::string& actualImageDigest,
                                            OBC::BootSlot expectedSlot,
                                            U32 lastAcceptedVersion,
                                            const BootTrustConfig& config);

}  // namespace OBC

#endif

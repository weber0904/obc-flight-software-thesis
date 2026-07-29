#ifndef OBC_BootMetadataStore_HPP
#define OBC_BootMetadataStore_HPP

#include <string>

#include "Fw/FPrimeBasicTypes.hpp"
#include "OBC/Types/RecoveryIncidentSourceEnumAc.hpp"
#include "OBC/Types/RecoveryLevelEnumAc.hpp"
#include "OBC/Types/BootSlotEnumAc.hpp"
#include "OBC/Types/ResetCauseEnumAc.hpp"

namespace OBC {

struct BootMetadata {
    U32 schemaVersion = 4U;
    OBC::BootSlot activeSlot = OBC::BootSlot::SLOT_A;
    OBC::BootSlot pendingSlot = OBC::BootSlot::NONE;
    OBC::BootSlot lastKnownGoodSlot = OBC::BootSlot::SLOT_A;
    bool confirmed = true;
    std::string expectedDigest;
    U32 expectedSize = 0U;
    U32 lastBootAttemptTime = 0U;
    U32 lastErrorCode = 0U;
    bool stageVerified = false;
    std::string stagedPath;
    std::string manifestPath;
    std::string manifestImagePath;
    U32 trustStatus = 0U;
    U32 trustRejectReason = 0U;
    std::string imageId;
    U32 stagedSoftwareVersion = 0U;
    U32 activeSoftwareVersion = 0U;
    U32 pendingSoftwareVersion = 0U;
    U32 lastKnownGoodSoftwareVersion = 0U;
    U32 lastAcceptedVersion = 0U;
    std::string signerId;
    U32 keySlot = 0U;
    std::string signatureAlgorithm;
    OBC::ResetCause resetCause = OBC::ResetCause::UNKNOWN;
    U32 bootCount = 0U;
    U32 consecutiveResetCount = 0U;
    OBC::RecoveryIncidentSource lastRecoverySource = OBC::RecoveryIncidentSource::NONE;
    OBC::RecoveryLevel lastRecoveryLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
    bool bootSafeFallbackRequired = false;
    bool recoveryResetPending = false;
};

class BootMetadataStore final {
  public:
    enum class LoadStatus {
        OK,
        NOT_FOUND,
        INVALID,
        IO_ERROR,
    };

  public:
    BootMetadataStore();

    explicit BootMetadataStore(const std::string& rootDir);

    void setRootDir(const std::string& rootDir);

    const std::string& getRootDir() const;

    const std::string& getMetadataPath() const;

    bool ensureStorage() const;

    LoadStatus load(BootMetadata& metadata) const;

    bool save(const BootMetadata& metadata) const;

    static bool computeDigestHex(const std::string& filePath, std::string& digestHex);

  private:
    static bool parseSlot_(const std::string& value, OBC::BootSlot& slot);

    static const char* slotToString_(OBC::BootSlot slot);

    static bool parseBool_(const std::string& value, bool& result);

    static std::string trim_(const std::string& value);

    static bool readFile_(const std::string& path, std::string& contents);

    static bool writeFile_(const std::string& path, const std::string& contents);

    bool ensureDirectoryTree_() const;

    void refreshPaths_();

  private:
    std::string m_rootDir;
    std::string m_metadataPath;
};

}  // namespace OBC

#endif

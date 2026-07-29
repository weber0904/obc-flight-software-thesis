#ifndef OBC_BootManager_HPP
#define OBC_BootManager_HPP

#include <string>

#include "Fw/Cmd/CmdString.hpp"
#include "Fw/Tlm/TlmBuffer.hpp"
#include "Fw/Types/Assert.hpp"
#include "OBC/Components/BootManager/BootManifestVerifier.hpp"
#include "OBC/Components/BootManager/BootManagerComponentAc.hpp"
#include "OBC/Components/BootManager/BootMetadataStore.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"

namespace OBC {

class BootManager final : public BootManagerComponentBase, public OBC::IRecoveryBootControl {
  public:
    explicit BootManager(const char* const compName);

    ~BootManager() override;

    void configureRuntimeStorageRoots(const std::string& persistentRoot, const std::string& stagingRoot);

    void configureBootTrustForRuntime(const OBC::BootTrustConfig& config);

    void configurePersistentFaultRecorderForRuntime(OBC::IPersistentFaultRecorder* recorder);

    void configureStorageRootForTest(const std::string& rootDir);

    bool reloadMetadataForTest();

    bool simulateRuntimeBootForTest();

    bool retryRuntimeBootInitializationForTest();

    void overrideStorageRootForPersistenceForTest(const std::string& rootDir);

    void tickForTest();

    Fw::CmdResponse prepareUpdateForRuntime(U32 imageSize, const std::string& digest);

    Fw::CmdResponse verifyStagedImageForRuntime(const std::string& stagingPath);

    Fw::CmdResponse activateStagedImageForRuntime();

    Fw::CmdResponse confirmForRuntime();

    Fw::CmdResponse rollbackForRuntime();

    const OBC::BootMetadata& getMetadataForRuntime() const;

    OBC::ResetCause getResetCauseForRuntime() const;

    U32 getBootCountForRuntime() const override;

    U32 getConsecutiveResetCountForRuntime() const override;

    U32 getUptimeForRuntime() const override;

    bool isBootSafeFallbackRequiredForRuntime() const override;

    U32 getRemainingConfirmSecondsForRuntime() const;

    U8 getUpdateProgressForRuntime() const;

    bool recordRecoveryRestartIntentForRuntime(OBC::ResetCause cause,
                                               OBC::RecoveryIncidentSource source,
                                               OBC::RecoveryLevel level) override;

    bool recordRecoveryRebootIntentForRuntime(OBC::ResetCause cause,
                                              OBC::RecoveryIncidentSource source,
                                              OBC::RecoveryLevel level) override {
        return this->recordRecoveryRestartIntentForRuntime(cause, source, level);
    }

    bool acknowledgeRuntimeStableForRuntime() override;

  private:
    static constexpr U32 DEFAULT_CONFIRM_TIMEOUT_SEC = 60U;

    static constexpr U32 ERROR_NONE = 0U;
    static constexpr U32 ERROR_METADATA_INVALID = 1U;
    static constexpr U32 ERROR_DIGEST_INVALID = 2U;
    static constexpr U32 ERROR_PREPARE_REQUIRED = 3U;
    static constexpr U32 ERROR_STAGING_FILE = 4U;
    static constexpr U32 ERROR_STAGING_SIZE_MISMATCH = 5U;
    static constexpr U32 ERROR_STAGING_DIGEST_MISMATCH = 6U;
    static constexpr U32 ERROR_METADATA_WRITE = 7U;
    static constexpr U32 ERROR_STAGE_NOT_VERIFIED = 8U;
    static constexpr U32 ERROR_NO_PENDING_SLOT = 9U;
    static constexpr U32 ERROR_CONFIRM_TIMEOUT = 10U;
    static constexpr U32 ERROR_PENDING_CONFIRM_ACTIVE = 21U;

    static constexpr U8 PROGRESS_IDLE = 0U;
    static constexpr U8 PROGRESS_PREPARED = 10U;
    static constexpr U8 PROGRESS_VERIFIED = 60U;
    static constexpr U8 PROGRESS_ACTIVATED = 100U;

  private:
    void BOOT_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void BOOT_PREPARE_UPDATE_cmdHandler(FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        U32 imageSize,
                                        const Fw::CmdStringArg& digest) override;

    void BOOT_VERIFY_STAGED_IMAGE_cmdHandler(FwOpcodeType opCode,
                                             U32 cmdSeq,
                                             const Fw::CmdStringArg& stagingPath) override;

    void BOOT_ACTIVATE_STAGED_IMAGE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void BOOT_CONFIRM_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void BOOT_ROLLBACK_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void GET_RESET_CAUSE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void GET_BOOT_COUNT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void schedIn_handler(FwIndexType portNum, U32 context) override;

    bool loadOrInitializeMetadata_();

    bool initializeRuntimeBoot_();

    bool persistState_(U32 errorCodeOnFailure);

    void appendPersistentFaultRecord_(const OBC::PersistentFaultRecord& record) const;

    void publishState_();
    void publishExplicitRefreshTelemetry_();
    void emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag);

    template <typename T>
    void emitStatusRefreshTelemetryValue_(FwChanIdType channelId, const T& value, Fw::Time& timeTag) {
        Fw::TlmBuffer buffer;
        const Fw::SerializeStatus status = buffer.serializeFrom(value);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));
        this->emitStatusRefreshTelemetry_(channelId, buffer, timeTag);
    }

    void publishError_(U32 errorCode);

    void respond_(FwOpcodeType opCode, U32 cmdSeq, Fw::CmdResponse response);

    bool isDigestFormatValid_(const std::string& digest) const;

    std::string resolveStagingPath_(const char* stagingPath) const;

    std::string resolveManifestPath_(const std::string& resolvedStagingPath) const;

    static std::string joinPath_(const std::string& base, const char* child);

    OBC::BootSlot inactiveSlot_() const;

    bool validateStagedTrust_(OBC::BootManifestDecision& decision) const;

    bool isPendingTrustStateCoherent_() const;

    void clearStagedTrust_();

    void rejectTrust_(U32 reasonCode, const OBC::BootManifest& manifest);

    void confirmUpdate_();

    void rollback_(U32 reasonCode, bool emitEvent);

    static bool isRecoveryResetCause_(OBC::ResetCause cause);

  private:
    static constexpr U32 RECOVERY_SAFE_FALLBACK_THRESHOLD = 3U;
    OBC::BootMetadataStore m_store;
    OBC::BootMetadata m_metadata;
    bool m_loaded;
    bool m_runtimeBootInitialized;
    U32 m_elapsedSeconds;
    U32 m_remainingConfirmSeconds;
    U8 m_updateProgress;
    std::string m_stagingRoot;
    OBC::BootTrustConfig m_bootTrustConfig;
    OBC::IPersistentFaultRecorder* m_persistentFaultRecorder;
    Fw::CmdResponse m_lastCommandResponse;
};

}  // namespace OBC

#endif

#include "OBC/Components/BootManager/BootManager.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

#include "Os/FileSystem.hpp"

namespace OBC {

namespace {

constexpr char DEFAULT_STAGING_ROOT[] = "runtime/staging";

std::string normalizeDigest(const char* digest) {
    std::string value(digest == nullptr ? "" : digest);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void saturatingIncrement(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
}

}  // namespace

BootManager::BootManager(const char* const compName)
    : BootManagerComponentBase(compName),
      m_store(),
      m_metadata(),
      m_loaded(false),
      m_runtimeBootInitialized(false),
      m_elapsedSeconds(0U),
      m_remainingConfirmSeconds(0U),
      m_updateProgress(PROGRESS_IDLE),
      m_stagingRoot(DEFAULT_STAGING_ROOT),
      m_bootTrustConfig(),
      m_persistentFaultRecorder(nullptr),
      m_lastCommandResponse(Fw::CmdResponse::OK) {
    (void)this->loadOrInitializeMetadata_();
    this->publishState_();
}

BootManager::~BootManager() = default;

void BootManager::configureRuntimeStorageRoots(const std::string& persistentRoot, const std::string& stagingRoot) {
    const std::string normalizedPersistent = persistentRoot.empty() ? "runtime/persistent-data" : persistentRoot;
    const std::string normalizedStaging = stagingRoot.empty() ? DEFAULT_STAGING_ROOT : stagingRoot;

    this->m_store.setRootDir(joinPath_(normalizedPersistent, "boot"));
    this->m_stagingRoot = normalizedStaging;
    this->m_loaded = false;
    this->m_runtimeBootInitialized = false;
    (void)this->initializeRuntimeBoot_();
    this->publishState_();
}

void BootManager::configureBootTrustForRuntime(const OBC::BootTrustConfig& config) {
    this->m_bootTrustConfig = config;
}

void BootManager::configurePersistentFaultRecorderForRuntime(OBC::IPersistentFaultRecorder* recorder) {
    this->m_persistentFaultRecorder = recorder;
}

void BootManager::configureStorageRootForTest(const std::string& rootDir) {
    this->m_store.setRootDir(rootDir);
    this->m_stagingRoot = rootDir;
    this->m_loaded = false;
    this->m_runtimeBootInitialized = false;
    (void)this->initializeRuntimeBoot_();
    this->publishState_();
}

bool BootManager::reloadMetadataForTest() {
    this->m_loaded = false;
    const bool loaded = this->loadOrInitializeMetadata_();
    this->publishState_();
    return loaded;
}

bool BootManager::simulateRuntimeBootForTest() {
    this->m_loaded = false;
    this->m_runtimeBootInitialized = false;
    const bool loaded = this->initializeRuntimeBoot_();
    this->publishState_();
    return loaded;
}

bool BootManager::retryRuntimeBootInitializationForTest() {
    this->m_runtimeBootInitialized = false;
    const bool loaded = this->initializeRuntimeBoot_();
    this->publishState_();
    return loaded;
}

void BootManager::overrideStorageRootForPersistenceForTest(const std::string& rootDir) {
    this->m_store.setRootDir(rootDir);
    this->m_stagingRoot = rootDir;
}

void BootManager::tickForTest() {
    this->schedIn_handler(0, 0U);
}

Fw::CmdResponse BootManager::prepareUpdateForRuntime(U32 imageSize, const std::string& digest) {
    Fw::CmdStringArg digestArg(digest.c_str());
    this->BOOT_PREPARE_UPDATE_cmdHandler(0, 0U, imageSize, digestArg);
    return this->m_lastCommandResponse;
}

Fw::CmdResponse BootManager::verifyStagedImageForRuntime(const std::string& stagingPath) {
    Fw::CmdStringArg pathArg(stagingPath.c_str());
    this->BOOT_VERIFY_STAGED_IMAGE_cmdHandler(0, 0U, pathArg);
    return this->m_lastCommandResponse;
}

Fw::CmdResponse BootManager::activateStagedImageForRuntime() {
    this->BOOT_ACTIVATE_STAGED_IMAGE_cmdHandler(0, 0U);
    return this->m_lastCommandResponse;
}

Fw::CmdResponse BootManager::confirmForRuntime() {
    this->BOOT_CONFIRM_cmdHandler(0, 0U);
    return this->m_lastCommandResponse;
}

Fw::CmdResponse BootManager::rollbackForRuntime() {
    this->BOOT_ROLLBACK_cmdHandler(0, 0U);
    return this->m_lastCommandResponse;
}

const OBC::BootMetadata& BootManager::getMetadataForRuntime() const {
    return this->m_metadata;
}

OBC::ResetCause BootManager::getResetCauseForRuntime() const {
    return this->m_metadata.resetCause;
}

U32 BootManager::getBootCountForRuntime() const {
    return this->m_metadata.bootCount;
}

U32 BootManager::getConsecutiveResetCountForRuntime() const {
    return this->m_metadata.consecutiveResetCount;
}

U32 BootManager::getUptimeForRuntime() const {
    return this->m_elapsedSeconds;
}

bool BootManager::isBootSafeFallbackRequiredForRuntime() const {
    return this->m_metadata.bootSafeFallbackRequired;
}

U32 BootManager::getRemainingConfirmSecondsForRuntime() const {
    return this->m_remainingConfirmSeconds;
}

U8 BootManager::getUpdateProgressForRuntime() const {
    return this->m_updateProgress;
}

bool BootManager::recordRecoveryRestartIntentForRuntime(OBC::ResetCause cause,
                                                        OBC::RecoveryIncidentSource source,
                                                        OBC::RecoveryLevel level) {
    if (!this->loadOrInitializeMetadata_()) {
        return false;
    }

    const OBC::BootMetadata previous = this->m_metadata;
    this->m_metadata.resetCause = cause;
    this->m_metadata.lastRecoverySource = source;
    this->m_metadata.lastRecoveryLevel = level;
    this->m_metadata.recoveryResetPending = true;
    this->log_WARNING_HI_BOOT_RECOVERY_INTENT_RECORDED(cause, source, level);
    this->publishState_();
    if (this->persistState_(ERROR_METADATA_WRITE)) {
        return true;
    }
    this->m_metadata = previous;
    this->publishState_();
    return false;
}

bool BootManager::acknowledgeRuntimeStableForRuntime() {
    if (!this->loadOrInitializeMetadata_()) {
        return false;
    }

    const OBC::BootMetadata previous = this->m_metadata;
    this->m_metadata.consecutiveResetCount = 0U;
    this->m_metadata.bootSafeFallbackRequired = false;
    this->log_ACTIVITY_HI_BOOT_RECOVERY_STABLE_ACK(this->m_metadata.resetCause, this->m_metadata.bootCount);
    this->publishState_();
    if (this->persistState_(ERROR_METADATA_WRITE)) {
        OBC::PersistentFaultRecord record = {};
        record.kind = OBC::PersistentFaultRecordKind::RECOVERY_BOOT_ACK;
        record.source = this->m_metadata.lastRecoverySource;
        record.level = this->m_metadata.lastRecoveryLevel;
        record.resetCause = this->m_metadata.resetCause;
        record.timestampSec = OBC::persistentFaultTimestampSec(this->getTime());
        record.uptimeSec = this->m_elapsedSeconds;
        record.bootCount = this->m_metadata.bootCount;
        record.consecutiveResetCount = this->m_metadata.consecutiveResetCount;
        record.detail = previous.consecutiveResetCount;
        this->appendPersistentFaultRecord_(record);
        return true;
    }
    this->m_metadata = previous;
    this->publishState_();
    return false;
}

void BootManager::BOOT_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->publishExplicitRefreshTelemetry_();
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::BOOT_PREPARE_UPDATE_cmdHandler(FwOpcodeType opCode,
                                                 U32 cmdSeq,
                                                 U32 imageSize,
                                                 const Fw::CmdStringArg& digest) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (!this->m_metadata.confirmed && this->m_metadata.pendingSlot != OBC::BootSlot::NONE) {
        this->publishError_(ERROR_PENDING_CONFIRM_ACTIVE);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    const std::string normalizedDigest = normalizeDigest(digest.toChar());
    if (imageSize == 0U || !this->isDigestFormatValid_(normalizedDigest)) {
        this->publishError_(ERROR_DIGEST_INVALID);
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->m_metadata.expectedSize = imageSize;
    this->m_metadata.expectedDigest = normalizedDigest;
    this->clearStagedTrust_();
    this->m_updateProgress = PROGRESS_PREPARED;
    this->publishError_(ERROR_NONE);

    if (!this->persistState_(ERROR_METADATA_WRITE)) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->log_ACTIVITY_HI_BOOT_UPDATE_PREPARED(imageSize);
    this->publishState_();
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::BOOT_VERIFY_STAGED_IMAGE_cmdHandler(FwOpcodeType opCode,
                                                      U32 cmdSeq,
                                                      const Fw::CmdStringArg& stagingPath) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    const std::string requestedPath(stagingPath.toChar());
    const std::string path = this->resolveStagingPath_(requestedPath.c_str());
    if (this->m_metadata.expectedSize == 0U || this->m_metadata.expectedDigest.empty()) {
        this->publishError_(ERROR_PREPARE_REQUIRED);
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(ERROR_PREPARE_REQUIRED);
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (requestedPath.empty()) {
        this->publishError_(ERROR_STAGING_FILE);
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(ERROR_STAGING_FILE);
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    FwSizeType fileSize = 0U;
    if (Os::FileSystem::getFileSize(path.c_str(), fileSize) != Os::FileSystem::Status::OP_OK) {
        this->publishError_(ERROR_STAGING_FILE);
        this->clearStagedTrust_();
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(ERROR_STAGING_FILE);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (fileSize != this->m_metadata.expectedSize) {
        this->publishError_(ERROR_STAGING_SIZE_MISMATCH);
        this->clearStagedTrust_();
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(ERROR_STAGING_SIZE_MISMATCH);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    std::string computedDigest;
    if (!OBC::BootMetadataStore::computeDigestHex(path, computedDigest)) {
        this->publishError_(ERROR_STAGING_FILE);
        this->clearStagedTrust_();
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(ERROR_STAGING_FILE);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (computedDigest != this->m_metadata.expectedDigest) {
        this->publishError_(ERROR_STAGING_DIGEST_MISMATCH);
        this->clearStagedTrust_();
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(ERROR_STAGING_DIGEST_MISMATCH);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    const OBC::BootManifestDecision decision = OBC::verifyBootManifestFile(this->resolveManifestPath_(path),
                                                                           requestedPath,
                                                                           static_cast<U32>(fileSize),
                                                                           computedDigest,
                                                                           this->inactiveSlot_(),
                                                                           this->m_metadata.lastAcceptedVersion,
                                                                           this->m_bootTrustConfig);
    if (!decision.accepted) {
        this->rejectTrust_(decision.rejectReason, decision.manifest);
        (void)this->persistState_(ERROR_METADATA_WRITE);
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(decision.rejectReason);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->m_metadata.stageVerified = true;
    this->m_metadata.stagedPath = path;
    this->m_metadata.manifestPath = this->resolveManifestPath_(path);
    this->m_metadata.manifestImagePath = decision.manifest.imagePath;
    this->m_metadata.trustStatus = BOOT_TRUST_STATUS_TRUSTED;
    this->m_metadata.trustRejectReason = BOOT_TRUST_REJECT_NONE;
    this->m_metadata.imageId = decision.manifest.imageId;
    this->m_metadata.stagedSoftwareVersion = decision.manifest.softwareVersion;
    this->m_metadata.signerId = decision.manifest.signerId;
    this->m_metadata.keySlot = decision.manifest.keySlot;
    this->m_metadata.signatureAlgorithm = decision.manifest.signatureAlgorithm;
    this->m_updateProgress = PROGRESS_VERIFIED;
    this->publishError_(ERROR_NONE);

    if (!this->persistState_(ERROR_METADATA_WRITE)) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->log_ACTIVITY_HI_BOOT_STAGE_VERIFY_OK(this->inactiveSlot_());
    this->log_ACTIVITY_HI_BOOT_TRUST_ACCEPTED(this->inactiveSlot_(),
                                              decision.manifest.softwareVersion,
                                              decision.manifest.keySlot);
    this->publishState_();
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::BOOT_ACTIVATE_STAGED_IMAGE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (!this->m_metadata.stageVerified || this->m_metadata.stagedPath.empty() ||
        this->m_metadata.trustStatus != BOOT_TRUST_STATUS_TRUSTED || this->m_metadata.stagedSoftwareVersion == 0U) {
        this->publishError_(ERROR_STAGE_NOT_VERIFIED);
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (this->m_metadata.stagedSoftwareVersion <= this->m_metadata.lastAcceptedVersion) {
        this->rejectTrust_(BOOT_TRUST_REJECT_VERSION_DOWNGRADE, OBC::BootManifest());
        (void)this->persistState_(ERROR_METADATA_WRITE);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    OBC::BootManifestDecision activationDecision = {};
    if (!this->validateStagedTrust_(activationDecision)) {
        this->rejectTrust_(activationDecision.rejectReason, activationDecision.manifest);
        (void)this->persistState_(ERROR_METADATA_WRITE);
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(activationDecision.rejectReason);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (activationDecision.manifest.softwareVersion != this->m_metadata.stagedSoftwareVersion ||
        activationDecision.manifest.imagePath != this->m_metadata.manifestImagePath ||
        activationDecision.manifest.imageId != this->m_metadata.imageId ||
        activationDecision.manifest.signerId != this->m_metadata.signerId ||
        activationDecision.manifest.keySlot != this->m_metadata.keySlot ||
        activationDecision.manifest.signatureAlgorithm != this->m_metadata.signatureAlgorithm) {
        this->rejectTrust_(BOOT_TRUST_REJECT_MANIFEST_MALFORMED, activationDecision.manifest);
        (void)this->persistState_(ERROR_METADATA_WRITE);
        this->log_WARNING_HI_BOOT_STAGE_VERIFY_FAIL(BOOT_TRUST_REJECT_MANIFEST_MALFORMED);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    const OBC::BootSlot targetSlot = this->inactiveSlot_();
    this->m_metadata.activeSlot = targetSlot;
    this->m_metadata.pendingSlot = targetSlot;
    this->m_metadata.confirmed = false;
    this->m_metadata.lastBootAttemptTime = this->m_elapsedSeconds;
    this->m_metadata.activeSoftwareVersion = this->m_metadata.stagedSoftwareVersion;
    this->m_metadata.pendingSoftwareVersion = this->m_metadata.stagedSoftwareVersion;
    this->m_metadata.lastAcceptedVersion = this->m_metadata.stagedSoftwareVersion;
    this->m_metadata.trustStatus = BOOT_TRUST_STATUS_PENDING;
    this->m_metadata.trustRejectReason = BOOT_TRUST_REJECT_NONE;
    this->m_remainingConfirmSeconds = DEFAULT_CONFIRM_TIMEOUT_SEC;
    this->m_updateProgress = PROGRESS_ACTIVATED;
    this->publishError_(ERROR_NONE);

    if (!this->persistState_(ERROR_METADATA_WRITE)) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->log_ACTIVITY_HI_BOOT_SLOT_SWITCHED(this->m_metadata.activeSlot, this->m_metadata.pendingSlot);
    this->publishState_();
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::BOOT_CONFIRM_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if (this->m_metadata.pendingSlot == OBC::BootSlot::NONE || this->m_metadata.confirmed) {
        this->publishError_(ERROR_NO_PENDING_SLOT);
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    if (!this->isPendingTrustStateCoherent_()) {
        this->rollback_(ERROR_METADATA_INVALID, true);
        (void)this->persistState_(ERROR_METADATA_WRITE);
        this->publishState_();
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    this->confirmUpdate_();
    if (!this->persistState_(ERROR_METADATA_WRITE)) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->log_ACTIVITY_HI_BOOT_VERSION_CONFIRMED(this->m_metadata.activeSlot);
    this->publishState_();
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::BOOT_ROLLBACK_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->rollback_(ERROR_NO_PENDING_SLOT, true);
    if (!this->persistState_(ERROR_METADATA_WRITE)) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->publishState_();
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::GET_RESET_CAUSE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->loadOrInitializeMetadata_()) {
        this->respond_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->publishState_();
    this->log_ACTIVITY_HI_BOOT_RECOVERY_STATUS(this->m_metadata.resetCause,
                                               this->m_metadata.bootCount,
                                               this->m_metadata.consecutiveResetCount,
                                               this->m_metadata.bootSafeFallbackRequired,
                                               this->m_metadata.lastRecoverySource,
                                               this->m_metadata.lastRecoveryLevel);
    this->respond_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BootManager::GET_BOOT_COUNT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->GET_RESET_CAUSE_cmdHandler(opCode, cmdSeq);
}

void BootManager::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    this->m_elapsedSeconds++;

    if (this->m_metadata.pendingSlot == OBC::BootSlot::NONE || this->m_metadata.confirmed) {
        return;
    }

    if (this->m_remainingConfirmSeconds > 0U) {
        this->m_remainingConfirmSeconds--;
    }

    if (this->m_remainingConfirmSeconds == 0U) {
        this->rollback_(ERROR_CONFIRM_TIMEOUT, true);
        (void)this->persistState_(ERROR_METADATA_WRITE);
        this->publishState_();
    }
}

bool BootManager::loadOrInitializeMetadata_() {
    if (this->m_loaded) {
        return true;
    }

    if (!this->m_store.ensureStorage()) {
        return false;
    }

    OBC::BootMetadata loadedMetadata = {};
    const OBC::BootMetadataStore::LoadStatus status = this->m_store.load(loadedMetadata);
    if (status == OBC::BootMetadataStore::LoadStatus::OK) {
        this->m_metadata = loadedMetadata;
        if (!this->isPendingTrustStateCoherent_()) {
            this->rollback_(ERROR_METADATA_INVALID, true);
            this->m_loaded = this->m_store.save(this->m_metadata);
            return this->m_loaded;
        }
        this->m_updateProgress = this->m_metadata.stageVerified ? PROGRESS_VERIFIED :
                                 (this->m_metadata.expectedDigest.empty() ? PROGRESS_IDLE : PROGRESS_PREPARED);
        if (!this->m_metadata.confirmed && this->m_metadata.pendingSlot != OBC::BootSlot::NONE) {
            this->m_remainingConfirmSeconds = DEFAULT_CONFIRM_TIMEOUT_SEC;
            this->m_updateProgress = PROGRESS_ACTIVATED;
        } else {
            this->m_remainingConfirmSeconds = 0U;
        }
        this->m_loaded = true;
        return true;
    }

    if (status == OBC::BootMetadataStore::LoadStatus::NOT_FOUND) {
        this->m_metadata = {};
        this->m_metadata.trustStatus = BOOT_TRUST_STATUS_CONFIRMED;
        this->m_metadata.trustRejectReason = BOOT_TRUST_REJECT_NONE;
        this->m_updateProgress = PROGRESS_IDLE;
        this->m_remainingConfirmSeconds = 0U;
        this->m_loaded = this->m_store.save(this->m_metadata);
        return this->m_loaded;
    }

    this->m_metadata = {};
    this->rollback_(ERROR_METADATA_INVALID, true);
    this->m_loaded = this->m_store.save(this->m_metadata);
    return this->m_loaded;
}

bool BootManager::initializeRuntimeBoot_() {
    if (!this->loadOrInitializeMetadata_()) {
        return false;
    }
    if (this->m_runtimeBootInitialized) {
        return true;
    }

    const OBC::BootMetadata previous = this->m_metadata;
    if (this->m_metadata.recoveryResetPending && this->isRecoveryResetCause_(this->m_metadata.resetCause)) {
        saturatingIncrement(this->m_metadata.consecutiveResetCount);
        this->m_metadata.recoveryResetPending = false;
    } else {
        this->m_metadata.consecutiveResetCount = 0U;
        this->m_metadata.recoveryResetPending = false;
        if (this->isRecoveryResetCause_(this->m_metadata.resetCause)) {
            this->m_metadata.resetCause = OBC::ResetCause::UNKNOWN;
        }
    }
    saturatingIncrement(this->m_metadata.bootCount);
    this->m_metadata.bootSafeFallbackRequired =
        this->m_metadata.consecutiveResetCount >= RECOVERY_SAFE_FALLBACK_THRESHOLD;
    this->log_ACTIVITY_HI_BOOT_RECOVERY_STATUS(this->m_metadata.resetCause,
                                               this->m_metadata.bootCount,
                                               this->m_metadata.consecutiveResetCount,
                                               this->m_metadata.bootSafeFallbackRequired,
                                               this->m_metadata.lastRecoverySource,
                                               this->m_metadata.lastRecoveryLevel);
    if (this->persistState_(ERROR_METADATA_WRITE)) {
        OBC::PersistentFaultRecord record = {};
        record.kind = OBC::PersistentFaultRecordKind::BOOT_OBSERVED;
        record.source = this->m_metadata.lastRecoverySource;
        record.level = this->m_metadata.lastRecoveryLevel;
        record.resetCause = this->m_metadata.resetCause;
        record.timestampSec = OBC::persistentFaultTimestampSec(this->getTime());
        record.uptimeSec = this->m_elapsedSeconds;
        record.bootCount = this->m_metadata.bootCount;
        record.consecutiveResetCount = this->m_metadata.consecutiveResetCount;
        record.flags = this->m_metadata.bootSafeFallbackRequired ? OBC::PersistentFaultFlagBootSafeFallback : 0U;
        this->appendPersistentFaultRecord_(record);
        this->m_runtimeBootInitialized = true;
        return true;
    }
    this->m_metadata = previous;
    this->publishState_();
    return false;
}

bool BootManager::persistState_(U32 errorCodeOnFailure) {
    if (this->m_store.save(this->m_metadata)) {
        return true;
    }

    this->publishError_(errorCodeOnFailure);
    this->publishState_();
    return false;
}

void BootManager::appendPersistentFaultRecord_(const OBC::PersistentFaultRecord& record) const {
    if (this->m_persistentFaultRecorder == nullptr) {
        return;
    }
    (void)this->m_persistentFaultRecorder->appendPersistentFaultRecordForRuntime(record);
}

void BootManager::publishState_() {
    this->tlmWrite_BOOT_ACTIVE_SLOT(this->m_metadata.activeSlot);
    this->tlmWrite_BOOT_PENDING_SLOT(this->m_metadata.pendingSlot);
    this->tlmWrite_BOOT_CONFIRMED(this->m_metadata.confirmed);
    this->tlmWrite_BOOT_UPDATE_PROGRESS(this->m_updateProgress);
    this->tlmWrite_BOOT_LAST_ERROR(this->m_metadata.lastErrorCode);
    this->tlmWrite_BOOT_TRUST_STATUS(this->m_metadata.trustStatus);
    this->tlmWrite_BOOT_TRUST_REJECT_REASON(this->m_metadata.trustRejectReason);
    this->tlmWrite_BOOT_STAGED_VERSION(this->m_metadata.stagedSoftwareVersion);
    this->tlmWrite_BOOT_LAST_ACCEPTED_VERSION(this->m_metadata.lastAcceptedVersion);
    this->tlmWrite_BOOT_RESET_CAUSE(this->m_metadata.resetCause);
    this->tlmWrite_BOOT_BOOT_COUNT(this->m_metadata.bootCount);
    this->tlmWrite_BOOT_CONSECUTIVE_RESET_COUNT(this->m_metadata.consecutiveResetCount);
    this->tlmWrite_BOOT_SAFE_FALLBACK_REQUIRED(this->m_metadata.bootSafeFallbackRequired);
    this->tlmWrite_BOOT_LAST_RECOVERY_SOURCE(this->m_metadata.lastRecoverySource);
    this->tlmWrite_BOOT_LAST_RECOVERY_LEVEL(this->m_metadata.lastRecoveryLevel);
}

void BootManager::publishExplicitRefreshTelemetry_() {
    Fw::Time timeTag = this->getTime();
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_ACTIVE_SLOT, this->m_metadata.activeSlot, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_PENDING_SLOT, this->m_metadata.pendingSlot, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_CONFIRMED, this->m_metadata.confirmed, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_UPDATE_PROGRESS, this->m_updateProgress, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_LAST_ERROR, this->m_metadata.lastErrorCode, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_TRUST_STATUS, this->m_metadata.trustStatus, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_TRUST_REJECT_REASON, this->m_metadata.trustRejectReason, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_STAGED_VERSION, this->m_metadata.stagedSoftwareVersion, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_LAST_ACCEPTED_VERSION, this->m_metadata.lastAcceptedVersion, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_RESET_CAUSE, this->m_metadata.resetCause, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_BOOT_BOOT_COUNT, this->m_metadata.bootCount, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_CONSECUTIVE_RESET_COUNT, this->m_metadata.consecutiveResetCount, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_SAFE_FALLBACK_REQUIRED, this->m_metadata.bootSafeFallbackRequired, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_LAST_RECOVERY_SOURCE, this->m_metadata.lastRecoverySource, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_BOOT_LAST_RECOVERY_LEVEL, this->m_metadata.lastRecoveryLevel, timeTag);
}

void BootManager::emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag) {
    this->bootStatusRefreshTlmOut_out(0, this->getIdBase() + channelId, timeTag, buffer);
}

void BootManager::publishError_(U32 errorCode) {
    this->m_metadata.lastErrorCode = errorCode;
}

void BootManager::respond_(FwOpcodeType opCode, U32 cmdSeq, Fw::CmdResponse response) {
    this->m_lastCommandResponse = response;
    this->cmdResponse_out(opCode, cmdSeq, response);
}

bool BootManager::isDigestFormatValid_(const std::string& digest) const {
    if (digest.empty() || digest.size() > 64U || (digest.size() % 2U) != 0U) {
        return false;
    }

    return std::all_of(digest.begin(), digest.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

std::string BootManager::resolveStagingPath_(const char* stagingPath) const {
    const std::string path(stagingPath == nullptr ? "" : stagingPath);
    if (path.empty() || (!path.empty() && path.front() == '/')) {
        return path;
    }

    if (this->m_stagingRoot.empty()) {
        return path;
    }

    if (this->m_stagingRoot.back() == '/') {
        return this->m_stagingRoot + path;
    }

    return this->m_stagingRoot + "/" + path;
}

std::string BootManager::resolveManifestPath_(const std::string& resolvedStagingPath) const {
    return resolvedStagingPath + ".manifest-v1";
}

std::string BootManager::joinPath_(const std::string& base, const char* child) {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    if (base.back() == '/') {
        return base + child;
    }
    return base + "/" + child;
}

OBC::BootSlot BootManager::inactiveSlot_() const {
    return this->m_metadata.activeSlot == OBC::BootSlot::SLOT_A ? OBC::BootSlot::SLOT_B : OBC::BootSlot::SLOT_A;
}

bool BootManager::validateStagedTrust_(OBC::BootManifestDecision& decision) const {
    decision = {};

    FwSizeType fileSize = 0U;
    if (Os::FileSystem::getFileSize(this->m_metadata.stagedPath.c_str(), fileSize) != Os::FileSystem::Status::OP_OK) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH;
        return false;
    }

    if (fileSize != this->m_metadata.expectedSize) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_SIZE_MISMATCH;
        return false;
    }

    std::string computedDigest;
    if (!OBC::BootMetadataStore::computeDigestHex(this->m_metadata.stagedPath, computedDigest)) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH;
        return false;
    }

    if (computedDigest != this->m_metadata.expectedDigest) {
        decision.rejectReason = BOOT_TRUST_REJECT_MANIFEST_DIGEST_MISMATCH;
        return false;
    }

    const std::string manifestPath =
        this->m_metadata.manifestPath.empty() ? this->resolveManifestPath_(this->m_metadata.stagedPath)
                                             : this->m_metadata.manifestPath;
    const std::string manifestImagePath =
        this->m_metadata.manifestImagePath.empty() ? this->m_metadata.stagedPath : this->m_metadata.manifestImagePath;
    decision = OBC::verifyBootManifestFile(manifestPath,
                                           manifestImagePath,
                                           static_cast<U32>(fileSize),
                                           computedDigest,
                                           this->inactiveSlot_(),
                                           this->m_metadata.lastAcceptedVersion,
                                           this->m_bootTrustConfig);
    return decision.accepted;
}

bool BootManager::isPendingTrustStateCoherent_() const {
    if (this->m_metadata.pendingSlot == OBC::BootSlot::NONE || this->m_metadata.confirmed) {
        return true;
    }

    return this->m_metadata.pendingSlot == this->m_metadata.activeSlot &&
           this->m_metadata.trustStatus == BOOT_TRUST_STATUS_PENDING &&
           this->m_metadata.pendingSoftwareVersion != 0U &&
           this->m_metadata.activeSoftwareVersion == this->m_metadata.pendingSoftwareVersion &&
           this->m_metadata.lastAcceptedVersion == this->m_metadata.pendingSoftwareVersion &&
           !this->m_metadata.signerId.empty() && this->m_metadata.keySlot != 0U &&
           this->m_metadata.signatureAlgorithm == "hmac-sha256";
}

void BootManager::clearStagedTrust_() {
    this->m_metadata.stageVerified = false;
    this->m_metadata.stagedPath.clear();
    this->m_metadata.manifestPath.clear();
    this->m_metadata.manifestImagePath.clear();
    this->m_metadata.trustStatus = BOOT_TRUST_STATUS_UNVERIFIED;
    this->m_metadata.trustRejectReason = BOOT_TRUST_REJECT_NONE;
    this->m_metadata.imageId.clear();
    this->m_metadata.stagedSoftwareVersion = 0U;
    this->m_metadata.signerId.clear();
    this->m_metadata.keySlot = 0U;
    this->m_metadata.signatureAlgorithm.clear();
}

void BootManager::rejectTrust_(U32 reasonCode, const OBC::BootManifest& manifest) {
    this->m_metadata.stageVerified = false;
    this->m_metadata.stagedPath.clear();
    this->m_metadata.manifestPath.clear();
    this->m_metadata.manifestImagePath.clear();
    this->m_metadata.trustStatus = BOOT_TRUST_STATUS_REJECTED;
    this->m_metadata.trustRejectReason = reasonCode;
    this->m_metadata.imageId = manifest.imageId;
    this->m_metadata.stagedSoftwareVersion = manifest.softwareVersion;
    this->m_metadata.signerId = manifest.signerId;
    this->m_metadata.keySlot = manifest.keySlot;
    this->m_metadata.signatureAlgorithm = manifest.signatureAlgorithm;
    this->publishError_(reasonCode);
    this->log_WARNING_HI_BOOT_TRUST_REJECTED(reasonCode, manifest.softwareVersion, manifest.keySlot);
}

void BootManager::confirmUpdate_() {
    this->m_metadata.lastKnownGoodSlot = this->m_metadata.activeSlot;
    this->m_metadata.lastKnownGoodSoftwareVersion = this->m_metadata.activeSoftwareVersion;
    this->m_metadata.pendingSlot = OBC::BootSlot::NONE;
    this->m_metadata.pendingSoftwareVersion = 0U;
    this->m_metadata.confirmed = true;
    this->m_remainingConfirmSeconds = 0U;
    this->m_metadata.stageVerified = false;
    this->m_metadata.stagedPath.clear();
    this->m_metadata.manifestPath.clear();
    this->m_metadata.manifestImagePath.clear();
    this->m_metadata.stagedSoftwareVersion = 0U;
    this->m_metadata.trustStatus = BOOT_TRUST_STATUS_CONFIRMED;
    this->m_metadata.trustRejectReason = BOOT_TRUST_REJECT_NONE;
    this->publishError_(ERROR_NONE);
}

void BootManager::rollback_(U32 reasonCode, bool emitEvent) {
    OBC::BootSlot rollbackTarget = this->m_metadata.lastKnownGoodSlot;
    if (rollbackTarget == OBC::BootSlot::NONE) {
        rollbackTarget = OBC::BootSlot::SLOT_A;
    }

    this->m_metadata.activeSlot = rollbackTarget;
    this->m_metadata.pendingSlot = OBC::BootSlot::NONE;
    this->m_metadata.activeSoftwareVersion = this->m_metadata.lastKnownGoodSoftwareVersion;
    this->m_metadata.pendingSoftwareVersion = 0U;
    this->m_metadata.confirmed = true;
    this->m_metadata.stageVerified = false;
    this->m_metadata.stagedPath.clear();
    this->m_metadata.manifestPath.clear();
    this->m_metadata.manifestImagePath.clear();
    this->m_metadata.stagedSoftwareVersion = 0U;
    this->m_metadata.trustStatus = BOOT_TRUST_STATUS_CONFIRMED;
    this->m_metadata.trustRejectReason = reasonCode;
    this->m_remainingConfirmSeconds = 0U;
    this->m_updateProgress = PROGRESS_IDLE;
    this->publishError_(reasonCode);

    if (emitEvent) {
        this->log_WARNING_HI_BOOT_ROLLBACK_TRIGGERED(rollbackTarget, reasonCode);
    }
}

bool BootManager::isRecoveryResetCause_(OBC::ResetCause cause) {
    return cause == OBC::ResetCause::RECOVERY_WATCHDOG || cause == OBC::ResetCause::RECOVERY_EPS_TIMEOUT ||
           cause == OBC::ResetCause::RECOVERY_ADCS_FDIR || cause == OBC::ResetCause::RECOVERY_COMM_FDIR;
}

}  // namespace OBC

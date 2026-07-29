#include "OBC/Components/PayloadOpsController/PayloadOpsController.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#include "Fw/Dp/DpContainer.hpp"
#include "Fw/Types/String.hpp"
#include "Fw/Types/Serializable.hpp"
#include "Os/FileSystem.hpp"
#include "config/DpCfg.hpp"

namespace OBC {

namespace {

std::string captureLeafName_(U8 captureIndex, const char* extension) {
    char leaf[16] = {};
    std::snprintf(leaf, sizeof(leaf), "PIC%02X.%s", captureIndex, extension == nullptr ? "bin" : extension);
    return std::string(leaf);
}

constexpr FwSizeType LEGACY_CAPTURE_MANIFEST_ACTUAL_FIELDS_SIZE = sizeof(U32) * 5U + sizeof(bool);
constexpr FwSizeType LEGACY_CAPTURE_MANIFEST_SERIALIZED_SIZE =
    OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE - LEGACY_CAPTURE_MANIFEST_ACTUAL_FIELDS_SIZE;

void metadataFromArtifactHeader_(const OBC::PayloadCaptureArtifactHeaderV2& header, OBC::PayloadCaptureMetadata& metadata) {
    metadata = {};
    metadata.captureId = header.get_captureId();
    metadata.captureIndex = header.get_captureIndex();
    metadata.bootCount = header.get_bootCount();
    metadata.captureTimeSec = header.get_captureTimeSec();
    metadata.captureTimeUsec = header.get_captureTimeUsec();
    metadata.capturePolicy = header.get_capturePolicy();
    metadata.resultCode = header.get_resultCode();
    metadata.requestedMask = header.get_requestedMask();
    metadata.appliedMask = header.get_appliedMask();
    metadata.pixelFormat = header.get_pixelFormat();
    metadata.imageWidth = header.get_imageWidth();
    metadata.imageHeight = header.get_imageHeight();
    metadata.rawBytes = header.get_rawBytes();
    metadata.previewJpegBytes = header.get_previewJpegBytes();
    metadata.backendName = header.get_backendName().toChar();
    metadata.cameraModel = header.get_cameraModel().toChar();
    metadata.rawRelativePath = header.get_rawRelativePath().toChar();
    metadata.previewRelativePath = header.get_previewRelativePath().toChar();
    metadata.previewDataProductRelativePath = header.get_previewDataProductRelativePath().toChar();
    metadata.rawDataProductRelativePath = header.get_rawDataProductRelativePath().toChar();
    metadata.previewDataProductPublished = header.get_previewDataProductPublished();
    metadata.rawDataProductPublished = header.get_rawDataProductPublished();
    metadata.appliedSettings.readyKind =
        metadata.capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR
            ? OBC::PayloadReadyKind::READY_RAW_SENSOR
            : OBC::PayloadReadyKind::READY_NON_RAW;
    metadata.appliedSettings.capturePolicy = header.get_capturePolicy();
    metadata.appliedSettings.resolution = header.get_resolution();
    metadata.appliedSettings.jpegQuality = header.get_jpegQualityApplied();
    metadata.appliedSettings.exposureUsec = header.get_exposureUsec();
    metadata.appliedSettings.gainX100 = header.get_gainX100();
    metadata.actualExposureUsec = header.get_actualExposureUsec();
    metadata.actualGainX100 = header.get_actualGainX100();
    metadata.actualAwbValid = header.get_actualAwbValid();
    metadata.actualAwbColorTemperatureK = header.get_actualAwbColorTemperatureK();
    metadata.actualAwbRedGainX1000 = header.get_actualAwbRedGainX1000();
    metadata.actualAwbBlueGainX1000 = header.get_actualAwbBlueGainX1000();
    metadata.lastPublishedArtifactKind = header.get_artifactKind();
}

bool deserializeLegacyArtifactHeader_(Fw::ExternalSerializeBuffer& buffer, OBC::PayloadCaptureMetadata& metadata) {
    metadata = {};
    U16 version = 0U;
    U8 captureIndex = 0U;
    U8 reserved0 = 0U;
    OBC::PayloadArtifactKind artifactKind = OBC::PayloadArtifactKind::PREVIEW_JPEG;
    OBC::PayloadPixelFormat pixelFormat = OBC::PayloadPixelFormat::PIXEL_UNKNOWN;
    OBC::PayloadCapturePolicy capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    OBC::PayloadResultCode resultCode = OBC::PayloadResultCode::PRESULT_NONE;
    OBC::PayloadResolutionPreset resolution = OBC::PayloadResolutionPreset::PRESET_HD_1280X720;
    U32 captureId = 0U;
    U32 bootCount = 0U;
    U32 captureTimeSec = 0U;
    U32 captureTimeUsec = 0U;
    U32 requestedMask = 0U;
    U32 appliedMask = 0U;
    U32 jpegQualityApplied = 0U;
    U32 exposureUsec = 0U;
    U32 gainX100 = 0U;
    U32 imageWidth = 0U;
    U32 imageHeight = 0U;
    U32 rawBytes = 0U;
    U32 previewJpegBytes = 0U;
    char backendNameBuffer[64] = {};
    char cameraModelBuffer[64] = {};
    char rawRelativePathBuffer[255] = {};
    char previewRelativePathBuffer[255] = {};
    char previewDataProductRelativePathBuffer[255] = {};
    char rawDataProductRelativePathBuffer[255] = {};
    Fw::ExternalString backendName(backendNameBuffer, sizeof(backendNameBuffer));
    Fw::ExternalString cameraModel(cameraModelBuffer, sizeof(cameraModelBuffer));
    Fw::ExternalString rawRelativePath(rawRelativePathBuffer, sizeof(rawRelativePathBuffer));
    Fw::ExternalString previewRelativePath(previewRelativePathBuffer, sizeof(previewRelativePathBuffer));
    Fw::ExternalString previewDataProductRelativePath(previewDataProductRelativePathBuffer,
                                                      sizeof(previewDataProductRelativePathBuffer));
    Fw::ExternalString rawDataProductRelativePath(rawDataProductRelativePathBuffer,
                                                  sizeof(rawDataProductRelativePathBuffer));
    bool previewDataProductPublished = false;
    bool rawDataProductPublished = false;

    const auto readOk = [&](auto& value) {
        return buffer.deserializeTo(value) == Fw::SerializeStatus::FW_SERIALIZE_OK;
    };
    if (!readOk(version) || !readOk(captureId) || !readOk(captureIndex) || !readOk(artifactKind) || !readOk(pixelFormat) ||
        !readOk(reserved0) || !readOk(bootCount) || !readOk(captureTimeSec) || !readOk(captureTimeUsec) ||
        !readOk(capturePolicy) || !readOk(resultCode) || !readOk(requestedMask) || !readOk(appliedMask) ||
        !readOk(resolution) || !readOk(jpegQualityApplied) || !readOk(exposureUsec) || !readOk(gainX100) ||
        !readOk(imageWidth) || !readOk(imageHeight) || !readOk(rawBytes) || !readOk(previewJpegBytes) ||
        !readOk(backendName) || !readOk(cameraModel) || !readOk(rawRelativePath) || !readOk(previewRelativePath) ||
        !readOk(previewDataProductRelativePath) || !readOk(rawDataProductRelativePath) ||
        !readOk(previewDataProductPublished) || !readOk(rawDataProductPublished)) {
        return false;
    }

    metadata.captureId = captureId;
    metadata.captureIndex = captureIndex;
    metadata.bootCount = bootCount;
    metadata.captureTimeSec = captureTimeSec;
    metadata.captureTimeUsec = captureTimeUsec;
    metadata.capturePolicy = capturePolicy;
    metadata.resultCode = resultCode;
    metadata.requestedMask = requestedMask;
    metadata.appliedMask = appliedMask;
    metadata.pixelFormat = pixelFormat;
    metadata.imageWidth = imageWidth;
    metadata.imageHeight = imageHeight;
    metadata.rawBytes = rawBytes;
    metadata.previewJpegBytes = previewJpegBytes;
    metadata.backendName = backendName.toChar();
    metadata.cameraModel = cameraModel.toChar();
    metadata.rawRelativePath = rawRelativePath.toChar();
    metadata.previewRelativePath = previewRelativePath.toChar();
    metadata.previewDataProductRelativePath = previewDataProductRelativePath.toChar();
    metadata.rawDataProductRelativePath = rawDataProductRelativePath.toChar();
    metadata.previewDataProductPublished = previewDataProductPublished;
    metadata.rawDataProductPublished = rawDataProductPublished;
    metadata.appliedSettings.readyKind =
        capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR
            ? OBC::PayloadReadyKind::READY_RAW_SENSOR
            : OBC::PayloadReadyKind::READY_NON_RAW;
    metadata.appliedSettings.capturePolicy = capturePolicy;
    metadata.appliedSettings.resolution = resolution;
    metadata.appliedSettings.jpegQuality = jpegQualityApplied;
    metadata.appliedSettings.exposureUsec = exposureUsec;
    metadata.appliedSettings.gainX100 = gainX100;
    metadata.lastPublishedArtifactKind = artifactKind;
    return true;
}

}  // namespace

bool payloadMaskSubset(U32 mask, U32 allowedMask) {
    return (mask & ~allowedMask) == 0U;
}

bool payloadMaskIntersects(U32 mask, U32 disallowedMask) {
    return (mask & disallowedMask) != 0U;
}

std::string payloadReadyKindName(OBC::PayloadReadyKind kind) {
    switch (kind.e) {
        case OBC::PayloadReadyKind::READY_RAW_SENSOR:
            return "RAW_SENSOR";
        case OBC::PayloadReadyKind::READY_NON_RAW:
        default:
            return "NON_RAW";
    }
}

std::string payloadCapturePolicyName(OBC::PayloadCapturePolicy kind) {
    switch (kind.e) {
        case OBC::PayloadCapturePolicy::CAPTURE_AUTO:
            return "AUTO";
        case OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR:
            return "RAW_SENSOR";
        case OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC:
        default:
            return "DETERMINISTIC";
    }
}

PayloadOpsController::PayloadOpsController(const char* compName)
    : PayloadOpsControllerComponentBase(compName),
      m_runtimeRoot(),
      m_captureRoot(),
      m_captureManifestRoot(),
      m_modeProvider(nullptr),
      m_epsControl(nullptr),
      m_bootControl(nullptr),
      m_runtimeConfig(),
      m_capabilities(),
      m_cameraDefaults(),
      m_autoDefaults(),
      m_deterministicDefaults(),
      m_manager(),
      m_state(OBC::PayloadState::PSTATE_OFF),
      m_preparedReadyKind(OBC::PayloadReadyKind::READY_NON_RAW),
      m_lastResult(OBC::PayloadResultCode::PRESULT_NONE),
      m_lastDetail(DETAIL_NONE),
      m_lastCaptureId(0U),
      m_lastCaptureIndex(0U),
      m_lastRawRelativePath(),
      m_lastPreviewRelativePath(),
      m_lastRequestedMask(0U),
      m_lastAppliedMask(0U),
      m_lastCaptureMetadata(),
      m_captureCounter(0U),
      m_abortTotal(0U),
      m_runtimeConfigured(false),
      m_forcedCleanupActive(false),
      m_activeCommand(),
      m_abortCommand(),
      m_pendingDataProductPublish(),
      m_pendingDataProductWriteCompletion(),
      m_captureCatalog(),
      m_serviceSnapshotMutex(),
      m_serviceSnapshot() {}

PayloadOpsController::~PayloadOpsController() = default;

bool PayloadOpsController::configureRuntime(const std::string& runtimeRoot,
                                            const OBC::IModeSafetyModeControl* modeProvider,
                                            OBC::IPayloadEpsControl* epsControl,
                                            const OBC::IRecoveryBootControl* bootControl,
                                            std::unique_ptr<OBC::IPiCameraDriver> driver,
                                            const OBC::PayloadRuntimeConfig& runtimeConfig) {
    const std::string root = runtimeRoot.empty() ? std::string("runtime") : runtimeRoot;
    const std::string persistentRoot = this->joinPath_(root, "persistent-data");
    const std::string payloadRoot = this->joinPath_(persistentRoot, "payload");
    const std::string captureRoot = this->joinPath_(payloadRoot, "camera");
    const std::string captureManifestRoot = this->joinPath_(captureRoot, "catalog");
    if (!this->ensureDirectoryTree_(root) || !this->ensureDirectoryTree_(persistentRoot) ||
        !this->ensureDirectoryTree_(payloadRoot) || !this->ensureDirectoryTree_(captureRoot) ||
        !this->ensureDirectoryTree_(captureManifestRoot)) {
        return false;
    }

    this->m_runtimeRoot = root;
    this->m_captureRoot = captureRoot;
    this->m_captureManifestRoot = captureManifestRoot;
    this->m_modeProvider = modeProvider;
    this->m_epsControl = epsControl;
    this->m_bootControl = bootControl;
    this->m_runtimeConfig = runtimeConfig;

    this->m_cameraDefaults = {};
    this->m_cameraDefaults.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    this->m_cameraDefaults.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    this->m_cameraDefaults.resolution = runtimeConfig.defaultResolution;
    this->m_cameraDefaults.jpegQuality = runtimeConfig.defaultJpegQuality;
    this->m_cameraDefaults.awbMode = OBC::PayloadAwbMode::AWB_OFF;
    this->m_cameraDefaults.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;

    this->m_autoDefaults = this->m_cameraDefaults;
    this->m_autoDefaults.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;
    this->m_autoDefaults.awbMode = OBC::PayloadAwbMode::AWB_AUTO;
    this->m_autoDefaults.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;

    this->m_deterministicDefaults = this->m_cameraDefaults;
    this->m_deterministicDefaults.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    this->m_deterministicDefaults.exposureUsec = runtimeConfig.defaultExposureUsec;
    this->m_deterministicDefaults.gainX100 = runtimeConfig.defaultGainX100;
    this->m_deterministicDefaults.awbMode = OBC::PayloadAwbMode::AWB_OFF;
    this->m_deterministicDefaults.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;

    if (driver != nullptr) {
        this->m_capabilities = driver->getCapabilities();
    } else {
        this->m_capabilities = {};
        this->m_capabilities.backendName = "unconfigured";
    }

    this->m_runtimeConfigured = this->m_manager.configure(std::move(driver), epsControl, runtimeConfig);
    this->m_captureCatalog.clear();
    this->publishState_();
    return this->m_runtimeConfigured;
}

bool PayloadOpsController::getStatusSnapshotForRuntime(OBC::PayloadStatusSnapshot& snapshot) const {
    std::lock_guard<std::mutex> lock(this->m_serviceSnapshotMutex);
    snapshot = this->m_serviceSnapshot.status;
    return snapshot.runtimeConfigured;
}

bool PayloadOpsController::getCapabilitiesForRuntime(OBC::PayloadCapabilities& capabilities) const {
    std::lock_guard<std::mutex> lock(this->m_serviceSnapshotMutex);
    capabilities = this->m_serviceSnapshot.capabilities;
    return this->m_serviceSnapshot.status.runtimeConfigured;
}

bool PayloadOpsController::getLastCaptureMetadataForRuntime(OBC::PayloadCaptureMetadata& metadata) const {
    std::lock_guard<std::mutex> lock(this->m_serviceSnapshotMutex);
    metadata = this->m_serviceSnapshot.lastCaptureMetadata;
    return this->m_serviceSnapshot.status.runtimeConfigured;
}

#ifdef BUILD_UT
void PayloadOpsController::setAbortTotalForTest(U32 value) {
    this->m_abortTotal = value;
    this->tlmWrite_PAYLOAD_ABORT_TOTAL(this->m_abortTotal);
}
#endif

void PayloadOpsController::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    this->startForcedCleanupIfNeeded_();

    OBC::PayloadOperationResult result = {};
    if (this->m_manager.pollCompletion(result)) {
        this->handleCompletedOperation_(result);
    }

    if (this->m_pendingDataProductWriteCompletion.active) {
        this->handlePendingDataProductWriteCompletion_();
    }

    if (this->m_pendingDataProductPublish.active && this->m_pendingDataProductPublish.timeoutTicksRemaining > 0U) {
        this->m_pendingDataProductPublish.timeoutTicksRemaining--;
        if (this->m_pendingDataProductPublish.timeoutTicksRemaining == 0U) {
            this->finalizeDataProductPublishFailure_(DETAIL_DATA_PRODUCT_WRITE_TIMEOUT);
        }
    }
}

void PayloadOpsController::dpWrittenIn_handler(FwIndexType portNum,
                                               const Fw::StringBase& fileName,
                                               FwDpPriorityType priority,
                                               FwSizeType size) {
    static_cast<void>(portNum);
    static_cast<void>(priority);

    if (!this->m_pendingDataProductPublish.active) {
        return;
    }
    const std::string writtenPath = fileName.toChar();
    const auto& expectedPaths = this->m_pendingDataProductPublish.expectedPaths;
    if (std::find(expectedPaths.begin(), expectedPaths.end(), writtenPath) == expectedPaths.end()) {
        return;
    }
    this->m_pendingDataProductWriteCompletion.active = true;
    this->m_pendingDataProductWriteCompletion.fileName = writtenPath;
    this->m_pendingDataProductWriteCompletion.size = size;
}

void PayloadOpsController::PAYLOAD_SET_CAMERA_DEFAULTS_cmdHandler(FwOpcodeType opCode,
                                                                  U32 cmdSeq,
                                                                  OBC::PayloadResolutionPreset resolutionPreset,
                                                                  U32 jpegQuality,
                                                                  bool hflip,
                                                                  bool vflip) {
    OBC::PayloadCameraSettings cameraDefaults = this->m_cameraDefaults;
    cameraDefaults.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    cameraDefaults.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    cameraDefaults.resolution = resolutionPreset;
    cameraDefaults.jpegQuality = jpegQuality;
    cameraDefaults.hflip = hflip;
    cameraDefaults.vflip = vflip;
    cameraDefaults.awbMode = OBC::PayloadAwbMode::AWB_OFF;
    cameraDefaults.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;
    cameraDefaults.evCompX100 = 0;
    cameraDefaults.brightnessX100 = 0;
    cameraDefaults.contrastX100 = 0;
    cameraDefaults.saturationX100 = 0;
    cameraDefaults.sharpnessX100 = 0;
    U32 detailCode = DETAIL_NONE;
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (this->m_manager.isBusy()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->m_state != OBC::PayloadState::PSTATE_OFF) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (!this->validateProfileCommon_(cameraDefaults, detailCode)) {
        this->failOperation_(detailCode == DETAIL_UNSUPPORTED_FIELD ? OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED
                                                                    : OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG,
                             detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->m_cameraDefaults = cameraDefaults;
    this->m_autoDefaults.resolution = cameraDefaults.resolution;
    this->m_autoDefaults.jpegQuality = cameraDefaults.jpegQuality;
    this->m_autoDefaults.hflip = cameraDefaults.hflip;
    this->m_autoDefaults.vflip = cameraDefaults.vflip;
    this->m_deterministicDefaults.resolution = cameraDefaults.resolution;
    this->m_deterministicDefaults.jpegQuality = cameraDefaults.jpegQuality;
    this->m_deterministicDefaults.hflip = cameraDefaults.hflip;
    this->m_deterministicDefaults.vflip = cameraDefaults.vflip;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
    this->m_lastDetail = DETAIL_NONE;
    this->publishState_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_PREPARE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    static_cast<void>(this->beginPrepareReady_(opCode, cmdSeq, OBC::PayloadReadyKind::READY_NON_RAW));
}

void PayloadOpsController::PAYLOAD_ABORT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (this->m_abortCommand.active) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->m_pendingDataProductPublish.active) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    if (this->m_manager.isBusy()) {
        this->m_manager.requestAbort();
        ++this->m_abortTotal;
        this->tlmWrite_PAYLOAD_ABORT_TOTAL(this->m_abortTotal);
        this->markPending_(this->m_abortCommand, opCode, cmdSeq, OBC::PayloadOperationKind::ABORT);
        this->m_state = OBC::PayloadState::PSTATE_ABORTING;
        this->publishState_();
        return;
    }

    if (this->m_state == OBC::PayloadState::PSTATE_OFF) {
        this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
        this->m_lastDetail = DETAIL_NONE;
        this->publishState_();
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }

    if (!this->m_manager.beginShutdown(true)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_SHUTDOWN_FAILED, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    ++this->m_abortTotal;
    this->tlmWrite_PAYLOAD_ABORT_TOTAL(this->m_abortTotal);
    this->markPending_(this->m_abortCommand, opCode, cmdSeq, OBC::PayloadOperationKind::ABORT);
    this->m_state = OBC::PayloadState::PSTATE_ABORTING;
    this->publishState_();
}

void PayloadOpsController::PAYLOAD_SHUTDOWN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (this->payloadWorkInProgress_() || this->m_abortCommand.active) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->m_state == OBC::PayloadState::PSTATE_OFF) {
        this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
        this->m_lastDetail = DETAIL_NONE;
        this->publishState_();
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }
    if (!this->m_manager.beginShutdown(false)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_SHUTDOWN_FAILED, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->markPending_(this->m_activeCommand, opCode, cmdSeq, OBC::PayloadOperationKind::SHUTDOWN);
    this->m_state = OBC::PayloadState::PSTATE_ABORTING;
    this->publishState_();
}

void PayloadOpsController::PAYLOAD_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->publishStatusEvent_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_PREPARE_RAW_SENSOR_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    static_cast<void>(this->beginPrepareReady_(opCode, cmdSeq, OBC::PayloadReadyKind::READY_RAW_SENSOR));
}

void PayloadOpsController::PAYLOAD_SET_AUTO_DEFAULTS_cmdHandler(FwOpcodeType opCode,
                                                                U32 cmdSeq,
                                                                OBC::PayloadAwbMode awbMode,
                                                                OBC::PayloadMeteringMode meteringMode,
                                                                I32 evCompX100) {
    OBC::PayloadCameraSettings settings = this->m_autoDefaults;
    settings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;
    settings.awbMode = awbMode;
    settings.meteringMode = meteringMode;
    settings.evCompX100 = evCompX100;
    settings.brightnessX100 = 0;
    settings.contrastX100 = 0;
    settings.saturationX100 = 0;
    settings.sharpnessX100 = 0;

    U32 detailCode = DETAIL_NONE;
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (this->m_manager.isBusy() || this->m_state != OBC::PayloadState::PSTATE_OFF) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (!this->validateAutoDefaults_(settings, detailCode)) {
        this->failOperation_(detailCode == DETAIL_UNSUPPORTED_FIELD ? OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED
                                                                    : OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG,
                             detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->m_autoDefaults = settings;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
    this->m_lastDetail = DETAIL_NONE;
    this->publishState_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_SET_DETERMINISTIC_DEFAULTS_cmdHandler(FwOpcodeType opCode,
                                                                         U32 cmdSeq,
                                                                         U32 exposureUsec,
                                                                         U32 gainX100) {
    OBC::PayloadCameraSettings settings = this->m_deterministicDefaults;
    settings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    settings.exposureUsec = exposureUsec;
    settings.gainX100 = gainX100;
    settings.awbMode = OBC::PayloadAwbMode::AWB_OFF;
    settings.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;
    settings.evCompX100 = 0;
    settings.brightnessX100 = 0;
    settings.contrastX100 = 0;
    settings.saturationX100 = 0;
    settings.sharpnessX100 = 0;

    U32 detailCode = DETAIL_NONE;
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (this->m_manager.isBusy() || this->m_state != OBC::PayloadState::PSTATE_OFF) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (!this->validateDeterministicDefaults_(settings, detailCode)) {
        this->failOperation_(detailCode == DETAIL_UNSUPPORTED_FIELD ? OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED
                                                                    : OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG,
                             detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->m_deterministicDefaults = settings;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
    this->m_lastDetail = DETAIL_NONE;
    this->publishState_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_CAPTURE_AUTO_cmdHandler(FwOpcodeType opCode,
                                                           U32 cmdSeq,
                                                           U8 captureIndex,
                                         const Fw::CmdStringArg& tag,
                                         U32 applyMask,
                                         OBC::PayloadAwbMode awbMode,
                                         OBC::PayloadMeteringMode meteringMode,
                                         I32 evCompX100) {
    OBC::PayloadCameraSettings requested = this->m_autoDefaults;
    requested.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    requested.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;
    requested.awbMode = awbMode;
    requested.meteringMode = meteringMode;
    requested.evCompX100 = evCompX100;
    requested.brightnessX100 = 0;
    requested.contrastX100 = 0;
    requested.saturationX100 = 0;
    requested.sharpnessX100 = 0;
    static_cast<void>(
        this->beginCapture_(opCode, cmdSeq, captureIndex, tag.toChar(), OBC::PayloadCapturePolicy::CAPTURE_AUTO, requested, applyMask));
}

void PayloadOpsController::PAYLOAD_CAPTURE_DETERMINISTIC_cmdHandler(FwOpcodeType opCode,
                                                                    U32 cmdSeq,
                                                                    U8 captureIndex,
                                                  const Fw::CmdStringArg& tag,
                                                  U32 applyMask,
                                                  U32 exposureUsec,
                                                  U32 gainX100) {
    OBC::PayloadCameraSettings requested = this->m_deterministicDefaults;
    requested.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    requested.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    requested.exposureUsec = exposureUsec;
    requested.gainX100 = gainX100;
    requested.awbMode = OBC::PayloadAwbMode::AWB_OFF;
    requested.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;
    requested.evCompX100 = 0;
    requested.brightnessX100 = 0;
    requested.contrastX100 = 0;
    requested.saturationX100 = 0;
    requested.sharpnessX100 = 0;
    static_cast<void>(this->beginCapture_(opCode,
                                          cmdSeq,
                                          captureIndex,
                                          tag.toChar(),
                                          OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC,
                                          requested,
                                          applyMask));
}

void PayloadOpsController::PAYLOAD_GET_CAPABILITIES_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->publishCapabilitiesEvent_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_GET_LAST_CAPTURE_METADATA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->publishLastCaptureMetadataEvent_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_SENSOR_REG_READ_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 address) {
    U32 detailCode = DETAIL_NONE;
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->validateRawRegisterAddress_(address, detailCode)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG, detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (!this->readyForRawSensorSession_(detailCode)) {
        this->failOperation_(detailCode == DETAIL_RAW_SESSION_REQUIRED ? OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY
                                                                       : OBC::PayloadResultCode::PRESULT_REJECTED_BUSY,
                             detailCode);
        this->respondImmediate_(opCode,
                                cmdSeq,
                                detailCode == DETAIL_RAW_SESSION_REQUIRED ? Fw::CmdResponse::VALIDATION_ERROR
                                                                          : Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->m_capabilities.rawRegisterSupported) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    U32 value = 0U;
    const Fw::CmdResponse response =
        this->m_manager.driver().readSensorRegister(address, this->m_runtimeConfig.rawRegisterTimeoutMs, value, detailCode);
    if (response != Fw::CmdResponse::OK) {
        this->failOperation_(detailCode == DETAIL_UNSUPPORTED_FIELD ? OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED
                                                                    : OBC::PayloadResultCode::PRESULT_CAPTURE_FAILED,
                             detailCode);
        this->respondImmediate_(opCode, cmdSeq, response);
        return;
    }

    this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
    this->m_lastDetail = DETAIL_NONE;
    this->log_ACTIVITY_LO_PAYLOAD_SENSOR_REGISTER_VALUE(address, value);
    this->publishState_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_SENSOR_REG_WRITE_cmdHandler(FwOpcodeType opCode,
                                                               U32 cmdSeq,
                                                               U32 address,
                                                               U32 value,
                                                               bool verifyReadback) {
    U32 detailCode = DETAIL_NONE;
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->validateRawRegisterAddress_(address, detailCode) || !this->validateRawRegisterValue_(value, detailCode)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG, detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (!this->readyForRawSensorSession_(detailCode)) {
        this->failOperation_(detailCode == DETAIL_RAW_SESSION_REQUIRED ? OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY
                                                                       : OBC::PayloadResultCode::PRESULT_REJECTED_BUSY,
                             detailCode);
        this->respondImmediate_(opCode,
                                cmdSeq,
                                detailCode == DETAIL_RAW_SESSION_REQUIRED ? Fw::CmdResponse::VALIDATION_ERROR
                                                                          : Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->m_capabilities.rawRegisterSupported) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    U32 verifiedValue = 0U;
    const Fw::CmdResponse response = this->m_manager.driver().writeSensorRegister(
        address, value, verifyReadback, this->m_runtimeConfig.rawRegisterTimeoutMs, verifiedValue, detailCode);
    if (response != Fw::CmdResponse::OK) {
        this->failOperation_(detailCode == DETAIL_UNSUPPORTED_FIELD ? OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED
                                                                    : OBC::PayloadResultCode::PRESULT_CAPTURE_FAILED,
                             detailCode);
        this->respondImmediate_(opCode, cmdSeq, response);
        return;
    }

    this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
    this->m_lastDetail = DETAIL_NONE;
    this->log_ACTIVITY_LO_PAYLOAD_SENSOR_REGISTER_WRITTEN(address, value, verifiedValue, verifyReadback);
    this->publishState_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void PayloadOpsController::PAYLOAD_CAPTURE_RAW_cmdHandler(FwOpcodeType opCode,
                                                          U32 cmdSeq,
                                                          U8 captureIndex,
                                                          const Fw::CmdStringArg& tag) {
    OBC::PayloadCameraSettings requested = this->m_deterministicDefaults;
    requested.readyKind = OBC::PayloadReadyKind::READY_RAW_SENSOR;
    requested.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR;
    static_cast<void>(this->beginCapture_(
        opCode, cmdSeq, captureIndex, tag.toChar(), OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR, requested, 0U));
}

void PayloadOpsController::PAYLOAD_PUBLISH_CAPTURE_cmdHandler(FwOpcodeType opCode,
                                                              U32 cmdSeq,
                                                              U8 captureIndex,
                                                              OBC::PayloadArtifactKind artifactKind) {
    U32 detailCode = DETAIL_NONE;
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->currentModeAllowsPayloadOps_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_MODE, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    if (this->payloadWorkInProgress_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    OBC::PayloadCaptureMetadata metadata = {};
    if (!this->loadCaptureManifest_(captureIndex, metadata, detailCode)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG, detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    OBC::PayloadState completionState = this->m_state;
    if (completionState == OBC::PayloadState::PSTATE_PUBLISHING) {
        completionState = OBC::PayloadState::PSTATE_READY;
    }
    if (!this->startArtifactPublish_(metadata, artifactKind, completionState, detailCode)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_STORAGE_FAILED, detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->markPending_(this->m_activeCommand, opCode, cmdSeq, OBC::PayloadOperationKind::PUBLISH_CAPTURE);
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
    this->m_activeCommand.responseSent = true;
}

bool PayloadOpsController::validateProfileCommon_(const OBC::PayloadCameraSettings& settings, U32& detailCode) const {
    if (settings.jpegQuality == 0U || settings.jpegQuality > 100U) {
        detailCode = DETAIL_INVALID_JPEG_QUALITY;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::JPEG_QUALITY) == 0U &&
        settings.jpegQuality != this->m_runtimeConfig.defaultJpegQuality) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::HFLIP) == 0U && settings.hflip) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::VFLIP) == 0U && settings.vflip) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::AWB_MODE) == 0U) {
        const OBC::PayloadAwbMode supportedAwb =
            settings.capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_AUTO ? OBC::PayloadAwbMode::AWB_AUTO
                                                                              : OBC::PayloadAwbMode::AWB_OFF;
        if (settings.awbMode != supportedAwb) {
            detailCode = DETAIL_UNSUPPORTED_FIELD;
            return false;
        }
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::METERING_MODE) == 0U &&
        settings.meteringMode != OBC::PayloadMeteringMode::METER_CENTRE) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::EV_COMP_X100) == 0U && settings.evCompX100 != 0) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::BRIGHTNESS_X100) == 0U &&
        settings.brightnessX100 != 0) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::CONTRAST_X100) == 0U && settings.contrastX100 != 0) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::SATURATION_X100) == 0U &&
        settings.saturationX100 != 0) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    if ((this->m_capabilities.supportedMask & OBC::PayloadFieldMask::SHARPNESS_X100) == 0U &&
        settings.sharpnessX100 != 0) {
        detailCode = DETAIL_UNSUPPORTED_FIELD;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PayloadOpsController::validateDeterministicDefaults_(const OBC::PayloadCameraSettings& settings, U32& detailCode) const {
    if (!this->validateProfileCommon_(settings, detailCode)) {
        return false;
    }
    if (settings.exposureUsec == 0U) {
        detailCode = DETAIL_INVALID_EXPOSURE;
        return false;
    }
    if (settings.gainX100 < 100U) {
        detailCode = DETAIL_INVALID_GAIN;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PayloadOpsController::validateAutoDefaults_(const OBC::PayloadCameraSettings& settings, U32& detailCode) const {
    return this->validateProfileCommon_(settings, detailCode);
}

bool PayloadOpsController::validateRawRegisterAddress_(U32 address, U32& detailCode) const {
    if (address > 0xFFFFU) {
        detailCode = DETAIL_INVALID_REGISTER_ADDRESS;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PayloadOpsController::validateRawRegisterValue_(U32 value, U32& detailCode) const {
    if (value > 0xFFU) {
        detailCode = DETAIL_INVALID_REGISTER_VALUE;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PayloadOpsController::validateCaptureRequest_(OBC::PayloadCapturePolicy capturePolicy,
                                                   const OBC::PayloadCameraSettings& requested,
                                                   U32 applyMask,
                                                   U32& detailCode) const {
    const U32 allowedMask = capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_AUTO
                                ? this->m_capabilities.autoMutableMask
                                : (capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC
                                       ? this->m_capabilities.deterministicMutableMask
                                       : 0U);
    if (!payloadMaskSubset(applyMask, allowedMask)) {
        detailCode = payloadMaskIntersects(applyMask, ~this->m_capabilities.supportedMask) ? DETAIL_UNSUPPORTED_FIELD
                                                                                            : DETAIL_INVALID_MASK;
        return false;
    }
    if ((applyMask & OBC::PayloadFieldMask::EXPOSURE_USEC) != 0U && requested.exposureUsec == 0U) {
        detailCode = DETAIL_INVALID_EXPOSURE;
        return false;
    }
    if ((applyMask & OBC::PayloadFieldMask::GAIN_X100) != 0U && requested.gainX100 < 100U) {
        detailCode = DETAIL_INVALID_GAIN;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PayloadOpsController::currentModeAllowsPayloadOps_() const {
    return this->m_modeProvider != nullptr && this->m_modeProvider->getModeForRuntime() == OBC::SatMode::PAYLOAD;
}

bool PayloadOpsController::isRuntimeConfigured_() const {
    return this->m_runtimeConfigured;
}

bool PayloadOpsController::readyForRawSensorSession_(U32& detailCode) const {
    if (this->payloadWorkInProgress_()) {
        detailCode = DETAIL_NONE;
        return false;
    }
    if (!this->currentModeAllowsPayloadOps_() || this->m_state != OBC::PayloadState::PSTATE_READY ||
        this->m_preparedReadyKind != OBC::PayloadReadyKind::READY_RAW_SENSOR) {
        detailCode = DETAIL_RAW_SESSION_REQUIRED;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

std::string PayloadOpsController::captureRawRelativePathForIndex_(U8 captureIndex) const {
    return std::string("persistent-data/payload/camera/") + captureLeafName_(captureIndex, "bin");
}

std::string PayloadOpsController::capturePreviewRelativePathForIndex_(U8 captureIndex) const {
    return std::string("persistent-data/payload/camera/") + captureLeafName_(captureIndex, "jpg");
}

std::string PayloadOpsController::captureManifestRelativePathForIndex_(U8 captureIndex) const {
    return std::string("persistent-data/payload/camera/catalog/") + captureLeafName_(captureIndex, "meta.bin");
}

std::string PayloadOpsController::dataProductRelativePathForTime_(const Fw::Time& timeTag) const {
    std::ostringstream stream;
    stream << "data-products/";
    const FwDpIdType productId = this->getIdBase() + ContainerId::PayloadCaptureArtifactContainer;
    char leaf[128] = {};
    std::snprintf(leaf,
                  sizeof(leaf),
                  "Dp_%08" PRI_FwDpIdType "_%08" PRIu32 "_%08" PRIu32 DP_EXT,
                  productId,
                  timeTag.getSeconds(),
                  timeTag.getUSeconds());
    stream << leaf;
    return stream.str();
}

std::string PayloadOpsController::joinPath_(const std::string& base, const char* child) const {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    return base.back() == '/' ? base + child : base + "/" + child;
}

bool PayloadOpsController::ensureDirectoryTree_(const std::string& path) const {
    if (path.empty()) {
        return false;
    }
    std::string current;
    if (path.front() == '/') {
        current = "/";
    }
    std::stringstream stream(path);
    std::string segment;
    while (std::getline(stream, segment, '/')) {
        if (segment.empty()) {
            continue;
        }
        if (!current.empty() && current.back() != '/') {
            current.push_back('/');
        }
        current += segment;
        const Os::FileSystem::PathType type = Os::FileSystem::getPathType(current.c_str());
        if (type == Os::FileSystem::PathType::DIRECTORY) {
            continue;
        }
        if (type == Os::FileSystem::PathType::FILE) {
            return false;
        }
        const Os::FileSystem::Status status = Os::FileSystem::createDirectory(current.c_str(), false);
        if (status != Os::FileSystem::Status::OP_OK && status != Os::FileSystem::Status::ALREADY_EXISTS) {
            return false;
        }
    }
    return true;
}

bool PayloadOpsController::writeCaptureManifest_(const OBC::PayloadCaptureMetadata& metadata) const {
    const std::string manifestPath = this->joinPath_(this->m_runtimeRoot, this->captureManifestRelativePathForIndex_(metadata.captureIndex).c_str());
    std::ofstream stream(manifestPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        return false;
    }
    const std::string currentDataProductPath =
        metadata.lastPublishedArtifactKind == OBC::PayloadArtifactKind::RAW_FRAME ? metadata.rawDataProductRelativePath
                                                                                   : metadata.previewDataProductRelativePath;
    const bool currentPublished =
        metadata.lastPublishedArtifactKind == OBC::PayloadArtifactKind::RAW_FRAME ? metadata.rawDataProductPublished
                                                                                   : metadata.previewDataProductPublished;
    const OBC::PayloadCaptureArtifactHeaderV2 header =
        this->makeCaptureArtifactHeader_(metadata, metadata.lastPublishedArtifactKind, currentDataProductPath, currentPublished);
    std::array<U8, OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE> bytes = {};
    Fw::ExternalSerializeBuffer buffer(bytes.data(), static_cast<FwSizeType>(bytes.size()));
    if (header.serializeTo(buffer) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return false;
    }
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(buffer.getSize()));
    stream.flush();
    return stream.good();
}

bool PayloadOpsController::loadCaptureManifest_(U8 captureIndex,
                                                OBC::PayloadCaptureMetadata& metadata,
                                                U32& detailCode) const {
    const auto cached = this->m_captureCatalog.find(captureIndex);
    if (cached != this->m_captureCatalog.end()) {
        metadata = cached->second;
        detailCode = DETAIL_NONE;
        return true;
    }

    const std::string manifestPath =
        this->joinPath_(this->m_runtimeRoot, this->captureManifestRelativePathForIndex_(captureIndex).c_str());
    std::ifstream stream(manifestPath.c_str(), std::ios::binary | std::ios::ate);
    if (!stream.good()) {
        detailCode = DETAIL_CAPTURE_INDEX_NOT_FOUND;
        return false;
    }

    const std::streamsize size = stream.tellg();
    if (size != static_cast<std::streamsize>(OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE) &&
        size != static_cast<std::streamsize>(LEGACY_CAPTURE_MANIFEST_SERIALIZED_SIZE)) {
        detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
        return false;
    }
    stream.seekg(0, std::ios::beg);
    if (size == static_cast<std::streamsize>(OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE)) {
        std::array<U8, OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE> bytes = {};
        if (!stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
            detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
            return false;
        }

        Fw::ExternalSerializeBuffer buffer(bytes.data(), static_cast<FwSizeType>(bytes.size()));
        if (buffer.setBuffLen(static_cast<FwSizeType>(bytes.size())) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
            detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
            return false;
        }

        OBC::PayloadCaptureArtifactHeaderV2 header = {};
        if (header.deserializeFrom(buffer) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
            detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
            return false;
        }
        metadataFromArtifactHeader_(header, metadata);
        detailCode = DETAIL_NONE;
        return true;
    }

    std::array<U8, LEGACY_CAPTURE_MANIFEST_SERIALIZED_SIZE> legacyBytes = {};
    if (!stream.read(reinterpret_cast<char*>(legacyBytes.data()), size)) {
        detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
        return false;
    }

    Fw::ExternalSerializeBuffer legacyBuffer(legacyBytes.data(), static_cast<FwSizeType>(legacyBytes.size()));
    if (legacyBuffer.setBuffLen(static_cast<FwSizeType>(legacyBytes.size())) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
        return false;
    }

    if (!deserializeLegacyArtifactHeader_(legacyBuffer, metadata)) {
        detailCode = DETAIL_CAPTURE_MANIFEST_READ_FAILED;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PayloadOpsController::loadArtifactBytes_(const std::string& relativePath,
                                              std::vector<U8>& bytes,
                                              U32& detailCode) const {
    bytes.clear();
    detailCode = DETAIL_NONE;

    const std::string fullPath = this->joinPath_(this->m_runtimeRoot, relativePath.c_str());
    std::ifstream stream(fullPath.c_str(), std::ios::binary | std::ios::ate);
    if (!stream.good()) {
        detailCode = DETAIL_DATA_PRODUCT_READ_FAILED;
        return false;
    }

    const std::streamsize size = stream.tellg();
    if (size < 0) {
        detailCode = DETAIL_DATA_PRODUCT_READ_FAILED;
        return false;
    }
    if (static_cast<FwSizeType>(size) > this->m_runtimeConfig.dataProductMaxBytes) {
        detailCode = DETAIL_DATA_PRODUCT_TOO_LARGE;
        return false;
    }
    stream.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(size));
    if (size > 0 && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
        bytes.clear();
        detailCode = DETAIL_DATA_PRODUCT_READ_FAILED;
        return false;
    }
    if (!stream.good() && !stream.eof()) {
        bytes.clear();
        detailCode = DETAIL_DATA_PRODUCT_READ_FAILED;
        return false;
    }
    return true;
}

bool PayloadOpsController::payloadWorkInProgress_() const {
    return this->m_manager.isBusy() || this->m_activeCommand.active || this->m_abortCommand.active ||
           this->m_pendingDataProductPublish.active;
}

OBC::PayloadCameraSettings PayloadOpsController::prepareSettingsForReady_(OBC::PayloadReadyKind readyKind) const {
    if (readyKind == OBC::PayloadReadyKind::READY_RAW_SENSOR) {
        OBC::PayloadCameraSettings settings = this->m_deterministicDefaults;
        settings.readyKind = OBC::PayloadReadyKind::READY_RAW_SENSOR;
        settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR;
        return settings;
    }

    OBC::PayloadCameraSettings settings = this->m_cameraDefaults;
    settings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;
    settings.awbMode = this->m_autoDefaults.awbMode;
    settings.meteringMode = this->m_autoDefaults.meteringMode;
    settings.evCompX100 = this->m_autoDefaults.evCompX100;
    return settings;
}

bool PayloadOpsController::captureSessionMatchesPreparedSettings_(OBC::PayloadCapturePolicy capturePolicy,
                                                                  const OBC::PayloadCameraSettings& preparedSettings,
                                                                  const OBC::PayloadCameraSettings& requested,
                                                                  U32& detailCode) const {
    const bool rawCapture = capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR;
    const bool rawPrepared = preparedSettings.readyKind == OBC::PayloadReadyKind::READY_RAW_SENSOR;
    if (rawCapture != rawPrepared) {
        detailCode = DETAIL_SESSION_REPREPARE_REQUIRED;
        return false;
    }
    if (!rawCapture && preparedSettings.resolution != requested.resolution) {
        detailCode = DETAIL_SESSION_REPREPARE_REQUIRED;
        return false;
    }
    detailCode = DETAIL_NONE;
    return true;
}

OBC::PayloadCameraSettings PayloadOpsController::resolveCaptureSettings_(OBC::PayloadCapturePolicy capturePolicy,
                                                                         const OBC::PayloadCameraSettings& requested,
                                                                         U32 applyMask,
                                                                         U32& appliedMask) const {
    OBC::PayloadCameraSettings resolved = {};
    if (capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR) {
        resolved = requested;
        resolved.readyKind = OBC::PayloadReadyKind::READY_RAW_SENSOR;
        resolved.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR;
        appliedMask = 0U;
        return resolved;
    }

    resolved = this->m_cameraDefaults;
    resolved.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    resolved.capturePolicy = capturePolicy;
    if (capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_AUTO) {
        resolved.awbMode = this->m_autoDefaults.awbMode;
        resolved.meteringMode = this->m_autoDefaults.meteringMode;
        resolved.evCompX100 = this->m_autoDefaults.evCompX100;
        resolved.brightnessX100 = this->m_autoDefaults.brightnessX100;
        resolved.contrastX100 = this->m_autoDefaults.contrastX100;
        resolved.saturationX100 = this->m_autoDefaults.saturationX100;
        resolved.sharpnessX100 = this->m_autoDefaults.sharpnessX100;
    } else {
        resolved.exposureUsec = this->m_deterministicDefaults.exposureUsec;
        resolved.gainX100 = this->m_deterministicDefaults.gainX100;
        resolved.awbMode = this->m_deterministicDefaults.awbMode;
        resolved.meteringMode = this->m_deterministicDefaults.meteringMode;
        resolved.evCompX100 = this->m_deterministicDefaults.evCompX100;
        resolved.brightnessX100 = this->m_deterministicDefaults.brightnessX100;
        resolved.contrastX100 = this->m_deterministicDefaults.contrastX100;
        resolved.saturationX100 = this->m_deterministicDefaults.saturationX100;
        resolved.sharpnessX100 = this->m_deterministicDefaults.sharpnessX100;
    }
    appliedMask = 0U;
    const U32 allowedMask = capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_AUTO
                                ? this->m_capabilities.autoMutableMask
                                : this->m_capabilities.deterministicMutableMask;

    if ((applyMask & OBC::PayloadFieldMask::EXPOSURE_USEC) != 0U && (allowedMask & OBC::PayloadFieldMask::EXPOSURE_USEC) != 0U) {
        resolved.exposureUsec = requested.exposureUsec;
        appliedMask |= OBC::PayloadFieldMask::EXPOSURE_USEC;
    }
    if ((applyMask & OBC::PayloadFieldMask::GAIN_X100) != 0U && (allowedMask & OBC::PayloadFieldMask::GAIN_X100) != 0U) {
        resolved.gainX100 = requested.gainX100;
        appliedMask |= OBC::PayloadFieldMask::GAIN_X100;
    }
    if ((applyMask & OBC::PayloadFieldMask::AWB_MODE) != 0U && (allowedMask & OBC::PayloadFieldMask::AWB_MODE) != 0U) {
        resolved.awbMode = requested.awbMode;
        appliedMask |= OBC::PayloadFieldMask::AWB_MODE;
    }
    if ((applyMask & OBC::PayloadFieldMask::METERING_MODE) != 0U &&
        (allowedMask & OBC::PayloadFieldMask::METERING_MODE) != 0U) {
        resolved.meteringMode = requested.meteringMode;
        appliedMask |= OBC::PayloadFieldMask::METERING_MODE;
    }
    if ((applyMask & OBC::PayloadFieldMask::EV_COMP_X100) != 0U && (allowedMask & OBC::PayloadFieldMask::EV_COMP_X100) != 0U) {
        resolved.evCompX100 = requested.evCompX100;
        appliedMask |= OBC::PayloadFieldMask::EV_COMP_X100;
    }
    if ((applyMask & OBC::PayloadFieldMask::BRIGHTNESS_X100) != 0U &&
        (allowedMask & OBC::PayloadFieldMask::BRIGHTNESS_X100) != 0U) {
        resolved.brightnessX100 = requested.brightnessX100;
        appliedMask |= OBC::PayloadFieldMask::BRIGHTNESS_X100;
    }
    if ((applyMask & OBC::PayloadFieldMask::CONTRAST_X100) != 0U &&
        (allowedMask & OBC::PayloadFieldMask::CONTRAST_X100) != 0U) {
        resolved.contrastX100 = requested.contrastX100;
        appliedMask |= OBC::PayloadFieldMask::CONTRAST_X100;
    }
    if ((applyMask & OBC::PayloadFieldMask::SATURATION_X100) != 0U &&
        (allowedMask & OBC::PayloadFieldMask::SATURATION_X100) != 0U) {
        resolved.saturationX100 = requested.saturationX100;
        appliedMask |= OBC::PayloadFieldMask::SATURATION_X100;
    }
    if ((applyMask & OBC::PayloadFieldMask::SHARPNESS_X100) != 0U &&
        (allowedMask & OBC::PayloadFieldMask::SHARPNESS_X100) != 0U) {
        resolved.sharpnessX100 = requested.sharpnessX100;
        appliedMask |= OBC::PayloadFieldMask::SHARPNESS_X100;
    }
    return resolved;
}

bool PayloadOpsController::planArtifactDataProduct_(OBC::PayloadCaptureMetadata& metadata,
                                                    OBC::PayloadArtifactKind artifactKind,
                                                    const std::vector<U8>& artifactBytes,
                                                    const Fw::Time& timeTag,
                                                    std::vector<std::string>& expectedPaths,
                                                    U32& packetBytes,
                                                    U32& detailCode) {
    expectedPaths.clear();
    packetBytes = 0U;
    detailCode = DETAIL_NONE;

    if (!this->isConnected_productGetOut_OutputPort(0) || !this->isConnected_productSendOut_OutputPort(0) ||
        !this->isConnected_productBufferReturnOut_OutputPort(0)) {
        detailCode = DETAIL_DATA_PRODUCT_PORTS_UNAVAILABLE;
        return false;
    }
    if (artifactBytes.empty()) {
        detailCode = DETAIL_ARTIFACT_NOT_AVAILABLE;
        return false;
    }
    if (artifactKind == OBC::PayloadArtifactKind::RAW_FRAME &&
        metadata.appliedSettings.resolution == OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464) {
        detailCode = DETAIL_FULL_RAW_DEFERRED;
        return false;
    }

    const FwSizeType maxArtifactBytesPerDataProduct = this->maxPayloadArtifactBytesPerDataProduct_();
    if (maxArtifactBytesPerDataProduct == 0U) {
        detailCode = DETAIL_DATA_PRODUCT_SERIALIZE_FAILED;
        return false;
    }

    FwSizeType totalDataBytes = 0U;
    for (FwSizeType remainingBytes = static_cast<FwSizeType>(artifactBytes.size()); remainingBytes > 0U;) {
        const FwSizeType chunkBytes = FW_MIN(remainingBytes, maxArtifactBytesPerDataProduct);
        totalDataBytes += PayloadOpsControllerComponentBase::SIZE_OF_PayloadCaptureArtifactHeader_RECORD;
        totalDataBytes += this->computePayloadArtifactRecordDataSize_(chunkBytes);
        remainingBytes -= chunkBytes;
    }
    if (totalDataBytes > this->m_runtimeConfig.dataProductMaxBytes) {
        detailCode = DETAIL_DATA_PRODUCT_TOO_LARGE;
        return false;
    }

    for (FwSizeType offset = 0U, sliceIndex = 0U; offset < static_cast<FwSizeType>(artifactBytes.size()); ++sliceIndex) {
        const FwSizeType chunkBytes =
            FW_MIN(static_cast<FwSizeType>(artifactBytes.size()) - offset, maxArtifactBytesPerDataProduct);
        const Fw::Time sliceTimeTag = this->dataProductTimeTagForSlice_(timeTag, static_cast<U32>(sliceIndex));
        const std::string relativeDataProductPath = this->dataProductRelativePathForTime_(sliceTimeTag);
        if (sliceIndex == 0U) {
            if (artifactKind == OBC::PayloadArtifactKind::RAW_FRAME) {
                metadata.rawDataProductRelativePath = relativeDataProductPath;
            } else {
                metadata.previewDataProductRelativePath = relativeDataProductPath;
            }
        }

        const FwSizeType dataSize = PayloadOpsControllerComponentBase::SIZE_OF_PayloadCaptureArtifactHeader_RECORD +
                                    this->computePayloadArtifactRecordDataSize_(chunkBytes);
        if (dataSize > MAX_DATA_PRODUCT_CONTAINER_BYTES) {
            detailCode = DETAIL_DATA_PRODUCT_TOO_LARGE;
            return false;
        }
        packetBytes += static_cast<U32>(DpContainer::getPacketSizeForDataSize(dataSize));
        expectedPaths.push_back(this->joinPath_(this->m_runtimeRoot, relativeDataProductPath.c_str()));
        offset += chunkBytes;
    }
    return true;
}

bool PayloadOpsController::publishNextPendingDataProductSlice_(U32& detailCode) {
    detailCode = DETAIL_NONE;
    if (!this->m_pendingDataProductPublish.active) {
        return false;
    }
    if (this->m_pendingDataProductPublish.nextOffset >=
        static_cast<FwSizeType>(this->m_pendingDataProductPublish.artifactBytes.size())) {
        return true;
    }

    const FwSizeType maxArtifactBytesPerDataProduct = this->maxPayloadArtifactBytesPerDataProduct_();
    if (maxArtifactBytesPerDataProduct == 0U) {
        detailCode = DETAIL_DATA_PRODUCT_SERIALIZE_FAILED;
        return false;
    }

    const FwSizeType offset = this->m_pendingDataProductPublish.nextOffset;
    const FwSizeType remainingBytes =
        static_cast<FwSizeType>(this->m_pendingDataProductPublish.artifactBytes.size()) - offset;
    const FwSizeType chunkBytes = FW_MIN(remainingBytes, maxArtifactBytesPerDataProduct);
    const Fw::Time sliceTimeTag =
        this->dataProductTimeTagForSlice_(this->m_pendingDataProductPublish.baseTimeTag,
                                          this->m_pendingDataProductPublish.nextSliceIndex);
    const bool finalSlice =
        (offset + chunkBytes) >= static_cast<FwSizeType>(this->m_pendingDataProductPublish.artifactBytes.size());

    const std::string currentDataProductPath = this->dataProductRelativePathForTime_(sliceTimeTag);
    const OBC::PayloadCaptureArtifactHeaderV2 header =
        this->makeCaptureArtifactHeader_(this->m_pendingDataProductPublish.metadata,
                                         this->m_pendingDataProductPublish.artifactKind,
                                         currentDataProductPath,
                                         finalSlice);

    const FwSizeType dataSize = PayloadOpsControllerComponentBase::SIZE_OF_PayloadCaptureArtifactHeader_RECORD +
                                this->computePayloadArtifactRecordDataSize_(chunkBytes);
    if (dataSize > MAX_DATA_PRODUCT_CONTAINER_BYTES) {
        detailCode = DETAIL_DATA_PRODUCT_TOO_LARGE;
        return false;
    }

    DpContainer container;
    const Fw::Success status = this->dpGet_PayloadCaptureArtifactContainer(dataSize, container);
    if (status == Fw::Success::FAILURE) {
        detailCode = DETAIL_DATA_PRODUCT_BUFFER_UNAVAILABLE;
        return false;
    }

    Fw::SerializeStatus serializeStatus = container.serializeRecord_PayloadCaptureArtifactHeader(header);
    if (serializeStatus == Fw::SerializeStatus::FW_SERIALIZE_OK) {
        serializeStatus = container.serializeRecord_PayloadCaptureArtifactBytes(
            this->m_pendingDataProductPublish.artifactBytes.data() + offset, chunkBytes);
    }
    if (serializeStatus != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        Fw::Buffer buffer = container.getBuffer();
        container.invalidateBuffer();
        this->productBufferReturnOut_out(0, buffer);
        detailCode = DETAIL_DATA_PRODUCT_SERIALIZE_FAILED;
        return false;
    }

    this->m_pendingDataProductPublish.nextOffset += chunkBytes;
    this->m_pendingDataProductPublish.nextSliceIndex++;
    this->m_pendingDataProductPublish.timeoutTicksRemaining = this->m_runtimeConfig.dataProductPublishTimeoutTicks;
    this->dpSend(container, sliceTimeTag);
    return true;
}

void PayloadOpsController::handlePendingDataProductWriteCompletion_() {
    if (!this->m_pendingDataProductWriteCompletion.active || !this->m_pendingDataProductPublish.active) {
        this->m_pendingDataProductWriteCompletion = {};
        return;
    }

    const std::string writtenPath = this->m_pendingDataProductWriteCompletion.fileName;
    const FwSizeType writtenSize = this->m_pendingDataProductWriteCompletion.size;
    this->m_pendingDataProductWriteCompletion = {};

    auto& expectedPaths = this->m_pendingDataProductPublish.expectedPaths;
    const auto match = std::find(expectedPaths.begin(), expectedPaths.end(), writtenPath);
    if (match == expectedPaths.end()) {
        return;
    }
    expectedPaths.erase(match);
    this->m_pendingDataProductPublish.writtenPacketBytes += static_cast<U32>(writtenSize);
    this->m_pendingDataProductPublish.timeoutTicksRemaining = this->m_runtimeConfig.dataProductPublishTimeoutTicks;

    if (this->m_pendingDataProductPublish.nextOffset <
        static_cast<FwSizeType>(this->m_pendingDataProductPublish.artifactBytes.size())) {
        U32 detailCode = DETAIL_NONE;
        if (!this->publishNextPendingDataProductSlice_(detailCode)) {
            this->finalizeDataProductPublishFailure_(detailCode);
        }
        return;
    }

    if (expectedPaths.empty()) {
        this->finalizeDataProductPublishSuccess_();
    }
}

FwSizeType PayloadOpsController::computePayloadArtifactRecordDataSize_(FwSizeType artifactBytes) const {
    return PayloadOpsControllerComponentBase::SIZE_OF_PayloadCaptureArtifactBytes_RECORD(artifactBytes);
}

FwSizeType PayloadOpsController::maxPayloadArtifactBytesPerDataProduct_() const {
    const FwSizeType headerBytes = PayloadOpsControllerComponentBase::SIZE_OF_PayloadCaptureArtifactHeader_RECORD;
    if (headerBytes + MAX_DATA_PRODUCT_RECORD_OVERHEAD >= MAX_DATA_PRODUCT_CONTAINER_BYTES) {
        return 0U;
    }
    return FW_MIN(MAX_ARTIFACT_RECORD_BYTES, MAX_DATA_PRODUCT_CONTAINER_BYTES - headerBytes - MAX_DATA_PRODUCT_RECORD_OVERHEAD);
}

Fw::Time PayloadOpsController::dataProductTimeTagForSlice_(const Fw::Time& baseTimeTag, U32 sliceIndex) const {
    Fw::Time sliceTimeTag = baseTimeTag;
    sliceTimeTag.add(0U, sliceIndex);
    return sliceTimeTag;
}

bool PayloadOpsController::startCapturePreviewPublish_(const OBC::PayloadOperationResult& result) {
    OBC::PayloadCaptureMetadata metadata = result.metadata;
    metadata.resultCode = OBC::PayloadResultCode::PRESULT_NONE;
    metadata.previewDataProductPublished = false;
    metadata.rawDataProductPublished = false;
    metadata.previewDataProductBytes = 0U;
    metadata.rawDataProductBytes = 0U;
    metadata.lastPublishedArtifactKind = OBC::PayloadArtifactKind::PREVIEW_JPEG;

    U32 detailCode = DETAIL_NONE;
    if (!this->startArtifactPublish_(metadata, OBC::PayloadArtifactKind::PREVIEW_JPEG, OBC::PayloadState::PSTATE_READY, detailCode)) {
        this->m_lastResult = OBC::PayloadResultCode::PRESULT_STORAGE_FAILED;
        this->m_lastDetail = detailCode;
        metadata.resultCode = this->m_lastResult;
        this->m_captureCatalog[metadata.captureIndex] = metadata;
        this->m_lastCaptureMetadata = metadata;
        this->m_lastCaptureId = metadata.captureId;
        this->m_lastCaptureIndex = metadata.captureIndex;
        this->m_lastRawRelativePath = metadata.rawRelativePath;
        this->m_lastPreviewRelativePath = metadata.previewRelativePath;
        static_cast<void>(this->writeCaptureManifest_(this->m_lastCaptureMetadata));
        return false;
    }
    return true;
}

bool PayloadOpsController::startArtifactPublish_(const OBC::PayloadCaptureMetadata& sourceMetadata,
                                                 OBC::PayloadArtifactKind artifactKind,
                                                 OBC::PayloadState completionState,
                                                 U32& detailCode) {
    OBC::PayloadCaptureMetadata metadata = sourceMetadata;
    detailCode = DETAIL_NONE;
    if (artifactKind == OBC::PayloadArtifactKind::RAW_FRAME) {
        if (metadata.appliedSettings.resolution == OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464) {
            detailCode = DETAIL_FULL_RAW_DEFERRED;
            return false;
        }
    } else if (artifactKind != OBC::PayloadArtifactKind::PREVIEW_JPEG) {
        detailCode = DETAIL_ARTIFACT_KIND_INVALID;
        return false;
    }

    std::vector<U8> artifactBytes;
    const std::string relativePath =
        artifactKind == OBC::PayloadArtifactKind::RAW_FRAME ? metadata.rawRelativePath : metadata.previewRelativePath;
    if (!this->loadArtifactBytes_(relativePath, artifactBytes, detailCode)) {
        return false;
    }

    if (artifactKind == OBC::PayloadArtifactKind::RAW_FRAME) {
        metadata.rawBytes = static_cast<U32>(artifactBytes.size());
        metadata.rawDataProductPublished = false;
        metadata.rawDataProductBytes = 0U;
    } else {
        metadata.previewJpegBytes = static_cast<U32>(artifactBytes.size());
        metadata.previewDataProductPublished = false;
        metadata.previewDataProductBytes = 0U;
    }
    metadata.lastPublishedArtifactKind = artifactKind;

    const Fw::Time timeTag = this->getTime();
    std::vector<std::string> expectedPaths;
    U32 packetBytes = 0U;
    if (!this->planArtifactDataProduct_(metadata, artifactKind, artifactBytes, timeTag, expectedPaths, packetBytes, detailCode)) {
        return false;
    }

    if (artifactKind == OBC::PayloadArtifactKind::RAW_FRAME) {
        metadata.rawDataProductBytes = packetBytes;
    } else {
        metadata.previewDataProductBytes = packetBytes;
    }
    this->m_captureCatalog[metadata.captureIndex] = metadata;
    if (!this->writeCaptureManifest_(metadata)) {
        detailCode = DETAIL_CAPTURE_MANIFEST_WRITE_FAILED;
        return false;
    }

    this->m_pendingDataProductPublish.active = true;
    this->m_pendingDataProductPublish.captureId = metadata.captureId;
    this->m_pendingDataProductPublish.captureIndex = metadata.captureIndex;
    this->m_pendingDataProductPublish.artifactKind = artifactKind;
    this->m_pendingDataProductPublish.timeoutTicksRemaining = this->m_runtimeConfig.dataProductPublishTimeoutTicks;
    this->m_pendingDataProductPublish.expectedPaths = std::move(expectedPaths);
    this->m_pendingDataProductPublish.writtenPacketBytes = 0U;
    this->m_pendingDataProductPublish.metadata = metadata;
    this->m_pendingDataProductPublish.artifactBytes = std::move(artifactBytes);
    this->m_pendingDataProductPublish.baseTimeTag = timeTag;
    this->m_pendingDataProductPublish.nextOffset = 0U;
    this->m_pendingDataProductPublish.nextSliceIndex = 0U;
    this->m_pendingDataProductPublish.completionState = completionState;

    this->m_lastCaptureId = metadata.captureId;
    this->m_lastCaptureIndex = metadata.captureIndex;
    this->m_lastRequestedMask = metadata.requestedMask;
    this->m_lastAppliedMask = metadata.appliedMask;
    this->m_lastRawRelativePath = metadata.rawRelativePath;
    this->m_lastPreviewRelativePath = metadata.previewRelativePath;
    this->m_lastCaptureMetadata = metadata;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_NONE;
    this->m_lastDetail = DETAIL_NONE;
    this->m_state = OBC::PayloadState::PSTATE_PUBLISHING;
    this->log_ACTIVITY_HI_PAYLOAD_STATE_CHANGED(this->m_state, this->m_lastResult);
    this->publishState_();
    this->publishStatusEvent_();
    this->publishLastCaptureMetadataEvent_();

    if (!this->publishNextPendingDataProductSlice_(detailCode)) {
        this->finalizeDataProductPublishFailure_(detailCode);
        return false;
    }
    return true;
}

void PayloadOpsController::finalizeDataProductPublishSuccess_() {
    const bool completedCaptureCommand = this->m_activeCommand.active &&
                                         this->m_activeCommand.kind == OBC::PayloadOperationKind::CAPTURE;
    this->m_lastCaptureMetadata = this->m_pendingDataProductPublish.metadata;
    if (this->m_pendingDataProductPublish.artifactKind == OBC::PayloadArtifactKind::RAW_FRAME) {
        this->m_lastCaptureMetadata.rawDataProductPublished = true;
        this->m_lastCaptureMetadata.rawDataProductBytes = this->m_pendingDataProductPublish.writtenPacketBytes;
    } else {
        this->m_lastCaptureMetadata.previewDataProductPublished = true;
        this->m_lastCaptureMetadata.previewDataProductBytes = this->m_pendingDataProductPublish.writtenPacketBytes;
    }
    this->m_lastCaptureMetadata.lastPublishedArtifactKind = this->m_pendingDataProductPublish.artifactKind;
    this->m_lastCaptureMetadata.resultCode = OBC::PayloadResultCode::PRESULT_OK;
    this->m_captureCatalog[this->m_lastCaptureMetadata.captureIndex] = this->m_lastCaptureMetadata;
    this->m_lastCaptureId = this->m_lastCaptureMetadata.captureId;
    this->m_lastCaptureIndex = this->m_lastCaptureMetadata.captureIndex;
    this->m_lastRequestedMask = this->m_lastCaptureMetadata.requestedMask;
    this->m_lastAppliedMask = this->m_lastCaptureMetadata.appliedMask;
    this->m_lastRawRelativePath = this->m_lastCaptureMetadata.rawRelativePath;
    this->m_lastPreviewRelativePath = this->m_lastCaptureMetadata.previewRelativePath;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
    this->m_lastDetail = DETAIL_NONE;
    this->m_state = this->m_pendingDataProductPublish.completionState;
    static_cast<void>(this->writeCaptureManifest_(this->m_lastCaptureMetadata));
    this->resetPendingDataProductPublish_();
    if (this->m_activeCommand.active) {
        this->clearPending_(this->m_activeCommand, Fw::CmdResponse::OK);
    }

    this->log_ACTIVITY_HI_PAYLOAD_STATE_CHANGED(this->m_state, this->m_lastResult);
    if (completedCaptureCommand) {
        const Fw::String rawPathArg(this->m_lastRawRelativePath.c_str());
        const Fw::String previewPathArg(this->m_lastPreviewRelativePath.c_str());
        this->log_ACTIVITY_HI_PAYLOAD_CAPTURED(
            this->m_lastCaptureId, this->m_lastCaptureIndex, rawPathArg, previewPathArg);
    }
    this->publishState_();
    this->publishStatusEvent_();
    this->publishLastCaptureMetadataEvent_();
}

void PayloadOpsController::finalizeDataProductPublishFailure_(U32 detailCode) {
    this->m_lastCaptureMetadata = this->m_pendingDataProductPublish.metadata;
    if (this->m_pendingDataProductPublish.artifactKind == OBC::PayloadArtifactKind::RAW_FRAME) {
        this->m_lastCaptureMetadata.rawDataProductPublished = false;
        this->m_lastCaptureMetadata.rawDataProductBytes = 0U;
    } else {
        this->m_lastCaptureMetadata.previewDataProductPublished = false;
        this->m_lastCaptureMetadata.previewDataProductBytes = 0U;
    }
    this->m_lastCaptureMetadata.lastPublishedArtifactKind = this->m_pendingDataProductPublish.artifactKind;
    this->m_lastCaptureMetadata.resultCode = OBC::PayloadResultCode::PRESULT_STORAGE_FAILED;
    this->m_captureCatalog[this->m_lastCaptureMetadata.captureIndex] = this->m_lastCaptureMetadata;
    this->m_lastCaptureId = this->m_lastCaptureMetadata.captureId;
    this->m_lastCaptureIndex = this->m_lastCaptureMetadata.captureIndex;
    this->m_lastRequestedMask = this->m_lastCaptureMetadata.requestedMask;
    this->m_lastAppliedMask = this->m_lastCaptureMetadata.appliedMask;
    this->m_lastRawRelativePath = this->m_lastCaptureMetadata.rawRelativePath;
    this->m_lastPreviewRelativePath = this->m_lastCaptureMetadata.previewRelativePath;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_STORAGE_FAILED;
    this->m_lastDetail = detailCode;
    this->m_state = this->m_pendingDataProductPublish.completionState;
    static_cast<void>(this->writeCaptureManifest_(this->m_lastCaptureMetadata));
    this->resetPendingDataProductPublish_();
    if (this->m_activeCommand.active) {
        this->clearPending_(this->m_activeCommand, Fw::CmdResponse::EXECUTION_ERROR);
    }

    this->log_WARNING_HI_PAYLOAD_OPERATION_FAILED(this->m_lastResult, this->m_lastDetail);
    this->publishState_();
    this->publishStatusEvent_();
    this->publishLastCaptureMetadataEvent_();
}

void PayloadOpsController::resetPendingDataProductPublish_() {
    this->m_pendingDataProductPublish = {};
    this->m_pendingDataProductWriteCompletion = {};
}

OBC::PayloadCaptureArtifactHeaderV2 PayloadOpsController::makeCaptureArtifactHeader_(
    const OBC::PayloadCaptureMetadata& metadata,
    OBC::PayloadArtifactKind artifactKind,
    const std::string& currentDataProductPath,
    bool currentDataProductPublished) const {
    OBC::PayloadCaptureArtifactHeaderV2 header = {};
    header.set_version(2U);
    header.set_captureId(metadata.captureId);
    header.set_captureIndex(metadata.captureIndex);
    header.set_artifactKind(artifactKind);
    header.set_pixelFormat(metadata.pixelFormat);
    header.set_reserved0(0U);
    header.set_bootCount(metadata.bootCount);
    header.set_captureTimeSec(metadata.captureTimeSec);
    header.set_captureTimeUsec(metadata.captureTimeUsec);
    header.set_capturePolicy(metadata.capturePolicy);
    header.set_resultCode(metadata.resultCode);
    header.set_requestedMask(metadata.requestedMask);
    header.set_appliedMask(metadata.appliedMask);
    header.set_resolution(metadata.appliedSettings.resolution);
    header.set_jpegQualityApplied(metadata.appliedSettings.jpegQuality);
    header.set_exposureUsec(metadata.appliedSettings.exposureUsec);
    header.set_gainX100(metadata.appliedSettings.gainX100);
    header.set_actualExposureUsec(metadata.actualExposureUsec);
    header.set_actualGainX100(metadata.actualGainX100);
    header.set_actualAwbValid(metadata.actualAwbValid);
    header.set_actualAwbColorTemperatureK(metadata.actualAwbColorTemperatureK);
    header.set_actualAwbRedGainX1000(metadata.actualAwbRedGainX1000);
    header.set_actualAwbBlueGainX1000(metadata.actualAwbBlueGainX1000);
    header.set_imageWidth(metadata.imageWidth);
    header.set_imageHeight(metadata.imageHeight);
    header.set_rawBytes(metadata.rawBytes);
    header.set_previewJpegBytes(metadata.previewJpegBytes);
    header.set_backendName(Fw::String(metadata.backendName.c_str()));
    header.set_cameraModel(Fw::String(metadata.cameraModel.c_str()));
    header.set_rawRelativePath(Fw::String(metadata.rawRelativePath.c_str()));
    header.set_previewRelativePath(Fw::String(metadata.previewRelativePath.c_str()));
    header.set_previewDataProductRelativePath(
        Fw::String((artifactKind == OBC::PayloadArtifactKind::PREVIEW_JPEG ? currentDataProductPath
                                                                           : metadata.previewDataProductRelativePath)
                       .c_str()));
    header.set_rawDataProductRelativePath(
        Fw::String((artifactKind == OBC::PayloadArtifactKind::RAW_FRAME ? currentDataProductPath
                                                                         : metadata.rawDataProductRelativePath)
                       .c_str()));
    header.set_previewDataProductPublished(artifactKind == OBC::PayloadArtifactKind::PREVIEW_JPEG ? currentDataProductPublished
                                                                                                   : metadata.previewDataProductPublished);
    header.set_rawDataProductPublished(artifactKind == OBC::PayloadArtifactKind::RAW_FRAME ? currentDataProductPublished
                                                                                            : metadata.rawDataProductPublished);
    return header;
}

void PayloadOpsController::publishState_() {
    const bool busy = this->payloadWorkInProgress_();
    const bool logicalPower = this->m_manager.isLogicalPowerEnabled();
    const bool prepared = this->m_manager.isPrepared();
    const bool proxyAsserted = this->m_manager.isProxyAsserted();

    this->syncServiceSnapshot_();

    this->tlmWrite_PAYLOAD_STATE(this->m_state);
    this->tlmWrite_PAYLOAD_LOGICAL_POWERED(logicalPower);
    this->tlmWrite_PAYLOAD_PREPARED(prepared);
    this->tlmWrite_PAYLOAD_BUSY(busy);
    this->tlmWrite_PAYLOAD_LAST_RESULT(this->m_lastResult);
    this->tlmWrite_PAYLOAD_LAST_DETAIL(this->m_lastDetail);
    this->tlmWrite_PAYLOAD_LAST_CAPTURE_ID(this->m_lastCaptureId);
    this->tlmWrite_PAYLOAD_LAST_CAPTURE_INDEX(this->m_lastCaptureIndex);
    this->tlmWrite_PAYLOAD_PROXY_CHANNEL(this->m_runtimeConfig.proxyPduChannel);
    this->tlmWrite_PAYLOAD_PROXY_ASSERTED(proxyAsserted);
    this->tlmWrite_PAYLOAD_DEFAULT_RESOLUTION(this->m_cameraDefaults.resolution);
    this->tlmWrite_PAYLOAD_DEFAULT_EXPOSURE_USEC(this->m_deterministicDefaults.exposureUsec);
    this->tlmWrite_PAYLOAD_DEFAULT_GAIN_X100(this->m_deterministicDefaults.gainX100);
    this->tlmWrite_PAYLOAD_ABORT_TOTAL(this->m_abortTotal);
    this->tlmWrite_PAYLOAD_DEFAULT_JPEG_QUALITY(this->m_cameraDefaults.jpegQuality);
    this->tlmWrite_PAYLOAD_ACTIVE_READY(this->m_preparedReadyKind);
    this->tlmWrite_PAYLOAD_LAST_CAPTURE_POLICY(this->m_lastCaptureMetadata.capturePolicy);
    this->tlmWrite_PAYLOAD_LAST_REQUESTED_MASK(this->m_lastRequestedMask);
    this->tlmWrite_PAYLOAD_LAST_APPLIED_MASK(this->m_lastAppliedMask);
    this->tlmWrite_PAYLOAD_CAP_SUPPORTED_MASK(this->m_capabilities.supportedMask);
    this->tlmWrite_PAYLOAD_CAP_OFF_ONLY_MASK(this->m_capabilities.offOnlyMask);
    this->tlmWrite_PAYLOAD_CAP_AUTO_MUTABLE_MASK(this->m_capabilities.autoMutableMask);
    this->tlmWrite_PAYLOAD_CAP_DETERMINISTIC_MUTABLE_MASK(this->m_capabilities.deterministicMutableMask);
    this->tlmWrite_PAYLOAD_LAST_DATA_PRODUCT_PUBLISHED(this->m_lastCaptureMetadata.previewDataProductPublished);
    this->tlmWrite_PAYLOAD_LAST_DATA_PRODUCT_BYTES(this->m_lastCaptureMetadata.previewDataProductBytes);
    this->tlmWrite_PAYLOAD_LAST_RAW_DATA_PRODUCT_PUBLISHED(this->m_lastCaptureMetadata.rawDataProductPublished);
    this->tlmWrite_PAYLOAD_LAST_RAW_DATA_PRODUCT_BYTES(this->m_lastCaptureMetadata.rawDataProductBytes);
    this->tlmWrite_PAYLOAD_LAST_PUBLISHED_ARTIFACT_KIND(this->m_lastCaptureMetadata.lastPublishedArtifactKind);
}

void PayloadOpsController::syncServiceSnapshot_() {
    OBC::PayloadServiceSnapshot snapshot = {};
    snapshot.status.runtimeConfigured = this->m_runtimeConfigured;
    snapshot.status.busy = this->payloadWorkInProgress_();
    snapshot.status.logicalPowerEnabled = this->m_manager.isLogicalPowerEnabled();
    snapshot.status.prepared = this->m_manager.isPrepared();
    snapshot.status.proxyAsserted = this->m_manager.isProxyAsserted();
    snapshot.status.state = this->m_state;
    snapshot.status.preparedReadyKind = this->m_preparedReadyKind;
    snapshot.status.lastResult = this->m_lastResult;
    snapshot.status.lastDetail = this->m_lastDetail;
    snapshot.status.lastCaptureId = this->m_lastCaptureId;
    snapshot.status.lastCaptureIndex = this->m_lastCaptureIndex;
    snapshot.status.lastRequestedMask = this->m_lastRequestedMask;
    snapshot.status.lastAppliedMask = this->m_lastAppliedMask;
    snapshot.status.abortTotal = this->m_abortTotal;
    snapshot.capabilities = this->m_capabilities;
    snapshot.lastCaptureMetadata = this->m_lastCaptureMetadata;
    if (snapshot.lastCaptureMetadata.captureId == 0U) {
        snapshot.lastCaptureMetadata.rawRelativePath = this->m_lastRawRelativePath;
        snapshot.lastCaptureMetadata.previewRelativePath = this->m_lastPreviewRelativePath;
        snapshot.lastCaptureMetadata.resultCode = this->m_lastResult;
    }
    std::lock_guard<std::mutex> lock(this->m_serviceSnapshotMutex);
    this->m_serviceSnapshot = std::move(snapshot);
}

void PayloadOpsController::publishStatusEvent_() const {
    const Fw::String previewPathArg(this->m_lastPreviewRelativePath.c_str());
    const Fw::String previewDataProductPathArg(this->m_lastCaptureMetadata.previewDataProductRelativePath.c_str());
    this->log_ACTIVITY_LO_PAYLOAD_STATUS(this->m_state,
                                         this->m_manager.isLogicalPowerEnabled(),
                                         this->m_manager.isPrepared(),
                                         this->m_lastResult,
                                         this->m_lastCaptureId,
                                         this->m_lastCaptureIndex,
                                         previewPathArg,
                                         previewDataProductPathArg,
                                         this->m_lastCaptureMetadata.previewDataProductPublished,
                                         this->m_lastCaptureMetadata.rawDataProductPublished);
}

void PayloadOpsController::publishCapabilitiesEvent_() const {
    const Fw::String backendArg(this->m_capabilities.backendName.c_str());
    this->log_ACTIVITY_LO_PAYLOAD_CAPABILITIES(backendArg,
                                               this->m_capabilities.supportedMask,
                                               this->m_capabilities.offOnlyMask,
                                               this->m_capabilities.autoMutableMask,
                                               this->m_capabilities.deterministicMutableMask,
                                               this->m_capabilities.rawRegisterSupported,
                                               this->m_capabilities.realSensorPath);
}

void PayloadOpsController::publishLastCaptureMetadataEvent_() const {
    const Fw::String rawPathArg(this->m_lastCaptureMetadata.rawRelativePath.c_str());
    const Fw::String previewPathArg(this->m_lastCaptureMetadata.previewRelativePath.c_str());
    const Fw::String previewDataProductPathArg(this->m_lastCaptureMetadata.previewDataProductRelativePath.c_str());
    const Fw::String rawDataProductPathArg(this->m_lastCaptureMetadata.rawDataProductRelativePath.c_str());
    this->log_ACTIVITY_LO_PAYLOAD_CAPTURE_METADATA(this->m_lastCaptureMetadata.capturePolicy,
                                                   this->m_lastCaptureId,
                                                   this->m_lastCaptureIndex,
                                                   this->m_lastRequestedMask,
                                                   this->m_lastAppliedMask,
                                                   this->m_lastCaptureMetadata.actualExposureUsec,
                                                   this->m_lastCaptureMetadata.actualGainX100,
                                                   this->m_lastCaptureMetadata.actualAwbValid,
                                                   this->m_lastCaptureMetadata.actualAwbColorTemperatureK,
                                                   this->m_lastCaptureMetadata.actualAwbRedGainX1000,
                                                   this->m_lastCaptureMetadata.actualAwbBlueGainX1000,
                                                   rawPathArg,
                                                   previewPathArg,
                                                   previewDataProductPathArg,
                                                   rawDataProductPathArg,
                                                   this->m_lastCaptureMetadata.previewDataProductPublished,
                                                   this->m_lastCaptureMetadata.rawDataProductPublished);
}

void PayloadOpsController::logProxyPowerChange_() {
    this->log_ACTIVITY_LO_PAYLOAD_PROXY_POWER_CHANGED(this->m_manager.isProxyAsserted(), this->m_runtimeConfig.proxyPduChannel);
}

void PayloadOpsController::startForcedCleanupIfNeeded_() {
    if (!this->m_runtimeConfigured || this->m_modeProvider == nullptr) {
        return;
    }
    if (this->m_modeProvider->getModeForRuntime() == OBC::SatMode::PAYLOAD) {
        return;
    }
    if (this->m_forcedCleanupActive) {
        return;
    }
    if (this->m_state == OBC::PayloadState::PSTATE_OFF || this->m_state == OBC::PayloadState::PSTATE_FAULT) {
        return;
    }

    if (this->m_pendingDataProductPublish.active) {
        return;
    }

    this->m_forcedCleanupActive = true;
    if (this->m_manager.isBusy()) {
        this->m_manager.requestAbort();
        this->m_state = OBC::PayloadState::PSTATE_ABORTING;
        this->m_activeCommand.forcedModeExit = true;
        this->publishState_();
        return;
    }

    if (this->m_manager.beginShutdown(true)) {
        this->m_state = OBC::PayloadState::PSTATE_ABORTING;
        this->publishState_();
    } else {
        this->m_state = OBC::PayloadState::PSTATE_FAULT;
        this->m_lastResult = OBC::PayloadResultCode::PRESULT_MODE_EXIT_ABORTED;
        this->m_lastDetail = DETAIL_NONE;
        this->publishState_();
    }
}

void PayloadOpsController::handleCompletedOperation_(const OBC::PayloadOperationResult& result) {
    Fw::CmdResponse completionResponse = result.response;
    this->m_lastResult = result.resultCode;
    this->m_lastDetail = result.detailCode;
    this->m_lastCaptureId = result.captureId == 0U ? this->m_lastCaptureId : result.captureId;
    if (result.kind == OBC::PayloadOperationKind::CAPTURE) {
        this->m_lastCaptureIndex = result.captureIndex;
    }
    if (result.kind == OBC::PayloadOperationKind::CAPTURE) {
        this->m_lastRequestedMask = result.requestedMask;
        this->m_lastAppliedMask = result.appliedMask;
    }
    if (!result.rawRelativePath.empty()) {
        this->m_lastRawRelativePath = result.rawRelativePath;
    }
    if (!result.previewRelativePath.empty()) {
        this->m_lastPreviewRelativePath = result.previewRelativePath;
    }
    if (result.metadata.captureId != 0U) {
        this->m_lastCaptureMetadata = result.metadata;
        this->m_lastCaptureMetadata.resultCode = result.resultCode;
        this->m_captureCatalog[this->m_lastCaptureMetadata.captureIndex] = this->m_lastCaptureMetadata;
    }
    if (this->m_activeCommand.active && this->m_activeCommand.forcedModeExit) {
        this->m_lastResult = OBC::PayloadResultCode::PRESULT_MODE_EXIT_ABORTED;
    }

    if (result.response == Fw::CmdResponse::OK && result.kind == OBC::PayloadOperationKind::PREPARE) {
        this->m_state = OBC::PayloadState::PSTATE_READY;
        this->m_preparedReadyKind = result.readyKind;
    } else if (result.response == Fw::CmdResponse::OK && result.kind == OBC::PayloadOperationKind::CAPTURE) {
        this->m_preparedReadyKind = result.readyKind;
        if (this->startCapturePreviewPublish_(result)) {
            return;
        }
        completionResponse = Fw::CmdResponse::EXECUTION_ERROR;
        this->m_state = result.prepared ? OBC::PayloadState::PSTATE_READY : OBC::PayloadState::PSTATE_OFF;
    } else if (result.response == Fw::CmdResponse::OK &&
               (result.kind == OBC::PayloadOperationKind::SHUTDOWN || result.kind == OBC::PayloadOperationKind::ABORT ||
                result.kind == OBC::PayloadOperationKind::INTERNAL_MODE_EXIT)) {
        this->m_state = OBC::PayloadState::PSTATE_OFF;
    } else if (result.resultCode == OBC::PayloadResultCode::PRESULT_ABORTED ||
               result.resultCode == OBC::PayloadResultCode::PRESULT_MODE_EXIT_ABORTED) {
        this->m_state = OBC::PayloadState::PSTATE_OFF;
    } else if (result.response != Fw::CmdResponse::OK && result.prepared) {
        this->m_state = OBC::PayloadState::PSTATE_READY;
    } else if (result.response != Fw::CmdResponse::OK &&
               (result.kind == OBC::PayloadOperationKind::SHUTDOWN || result.kind == OBC::PayloadOperationKind::ABORT)) {
        this->m_state = OBC::PayloadState::PSTATE_FAULT;
    } else if (result.response != Fw::CmdResponse::OK && !result.prepared) {
        this->m_state = OBC::PayloadState::PSTATE_OFF;
    }

    if (completionResponse == Fw::CmdResponse::OK) {
        this->log_ACTIVITY_HI_PAYLOAD_STATE_CHANGED(this->m_state, this->m_lastResult);
        if (result.kind == OBC::PayloadOperationKind::CAPTURE && !this->m_lastPreviewRelativePath.empty()) {
            const Fw::String rawPathArg(this->m_lastRawRelativePath.c_str());
            const Fw::String previewPathArg(this->m_lastPreviewRelativePath.c_str());
            this->log_ACTIVITY_HI_PAYLOAD_CAPTURED(
                this->m_lastCaptureId, this->m_lastCaptureIndex, rawPathArg, previewPathArg);
        }
    } else {
        this->log_WARNING_HI_PAYLOAD_OPERATION_FAILED(this->m_lastResult, this->m_lastDetail);
    }

    this->logProxyPowerChange_();
    this->publishState_();
    this->publishStatusEvent_();
    if (result.kind == OBC::PayloadOperationKind::CAPTURE) {
        this->publishLastCaptureMetadataEvent_();
    }

    if (this->m_activeCommand.active) {
        Fw::CmdResponse response = completionResponse;
        if (this->m_activeCommand.forcedModeExit) {
            response = Fw::CmdResponse::EXECUTION_ERROR;
        } else if (this->m_activeCommand.kind == OBC::PayloadOperationKind::CAPTURE &&
                   completionResponse != Fw::CmdResponse::OK) {
            response = Fw::CmdResponse::EXECUTION_ERROR;
        }
        this->clearPending_(this->m_activeCommand, response);
    }
    if (this->m_abortCommand.active) {
        const Fw::CmdResponse abortResponse =
            (result.resultCode == OBC::PayloadResultCode::PRESULT_ABORTED || result.response == Fw::CmdResponse::OK)
                ? Fw::CmdResponse::OK
                : Fw::CmdResponse::EXECUTION_ERROR;
        this->clearPending_(this->m_abortCommand, abortResponse);
    }

    this->m_forcedCleanupActive = false;
}

void PayloadOpsController::respondImmediate_(FwOpcodeType opCode, U32 cmdSeq, Fw::CmdResponse response) {
    this->cmdResponse_out(opCode, cmdSeq, response);
}

void PayloadOpsController::failOperation_(OBC::PayloadResultCode resultCode, U32 detailCode) {
    this->m_lastResult = resultCode;
    this->m_lastDetail = detailCode;
    this->log_WARNING_HI_PAYLOAD_OPERATION_FAILED(this->m_lastResult, this->m_lastDetail);
    this->publishState_();
}

void PayloadOpsController::clearPending_(PendingCommand& pending, Fw::CmdResponse response) {
    if (!pending.responseSent) {
        this->cmdResponse_out(pending.opCode, pending.cmdSeq, response);
    }
    pending = {};
}

void PayloadOpsController::markPending_(PendingCommand& pending,
                                        FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        OBC::PayloadOperationKind kind) {
    pending.active = true;
    pending.opCode = opCode;
    pending.cmdSeq = cmdSeq;
    pending.kind = kind;
    pending.responseSent = false;
    pending.forcedModeExit = false;
}

bool PayloadOpsController::beginPrepareReady_(FwOpcodeType opCode, U32 cmdSeq, OBC::PayloadReadyKind readyKind) {
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return false;
    }
    if (!this->currentModeAllowsPayloadOps_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_MODE, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }
    if (this->payloadWorkInProgress_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }
    const bool rawPrepare = readyKind == OBC::PayloadReadyKind::READY_RAW_SENSOR;
    const bool alreadyInRequestedReadyState =
        this->m_state == OBC::PayloadState::PSTATE_READY &&
        (rawPrepare ? (this->m_preparedReadyKind == OBC::PayloadReadyKind::READY_RAW_SENSOR)
                    : (this->m_preparedReadyKind == OBC::PayloadReadyKind::READY_NON_RAW));
    if (alreadyInRequestedReadyState) {
        this->m_lastResult = OBC::PayloadResultCode::PRESULT_OK;
        this->m_lastDetail = DETAIL_NONE;
        this->publishState_();
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
        return true;
    }
    if (this->m_state == OBC::PayloadState::PSTATE_READY) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY, DETAIL_SESSION_REPREPARE_REQUIRED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }

    OBC::PayloadCameraSettings settings = this->prepareSettingsForReady_(readyKind);
    if (!this->m_manager.beginPrepare(readyKind, settings)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_PREPARE_FAILED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return false;
    }

    this->markPending_(this->m_activeCommand, opCode, cmdSeq, OBC::PayloadOperationKind::PREPARE);
    this->m_state = OBC::PayloadState::PSTATE_PREPARING;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_NONE;
    this->m_lastDetail = DETAIL_NONE;
    this->m_preparedReadyKind = readyKind;
    this->publishState_();
    return true;
}

bool PayloadOpsController::beginCapture_(FwOpcodeType opCode,
                                         U32 cmdSeq,
                                         U8 captureIndex,
                                         const std::string& tag,
                                         OBC::PayloadCapturePolicy capturePolicy,
                                         const OBC::PayloadCameraSettings& requested,
                                         U32 applyMask) {
    if (!this->isRuntimeConfigured_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_UNCONFIGURED, DETAIL_RUNTIME_UNCONFIGURED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return false;
    }
    if (!this->currentModeAllowsPayloadOps_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_MODE, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }
    if (this->payloadWorkInProgress_()) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_BUSY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }
    const bool rawCapture = capturePolicy == OBC::PayloadCapturePolicy::CAPTURE_RAW_SENSOR;
    const bool preparedForRawSensor = this->m_preparedReadyKind == OBC::PayloadReadyKind::READY_RAW_SENSOR;
    if (this->m_state != OBC::PayloadState::PSTATE_READY ||
        (rawCapture ? !preparedForRawSensor : preparedForRawSensor)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY, DETAIL_NONE);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }

    U32 detailCode = DETAIL_NONE;
    if (!this->validateCaptureRequest_(capturePolicy, requested, applyMask, detailCode)) {
        this->failOperation_(detailCode == DETAIL_UNSUPPORTED_FIELD ? OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED
                                                                    : OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG,
                             detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }
    OBC::PayloadCameraSettings preparedSettings = {};
    if (!this->m_manager.getPreparedSettings(preparedSettings) ||
        !this->captureSessionMatchesPreparedSettings_(capturePolicy, preparedSettings, requested, detailCode)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_REJECTED_INVALID_CONFIG, detailCode);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return false;
    }

    OBC::PayloadCaptureRequest request = {};
    request.tag = tag;
    request.captureId = ++this->m_captureCounter;
    request.captureIndex = captureIndex;
    request.bootCount = this->m_bootControl == nullptr ? 0U : this->m_bootControl->getBootCountForRuntime();
    request.requestedMask = applyMask;
    request.rawRelativePath = this->captureRawRelativePathForIndex_(captureIndex);
    request.rawOutputPath = this->joinPath_(this->m_runtimeRoot, request.rawRelativePath.c_str());
    request.previewRelativePath = this->capturePreviewRelativePathForIndex_(captureIndex);
    request.previewOutputPath = this->joinPath_(this->m_runtimeRoot, request.previewRelativePath.c_str());
    request.metadata.captureId = request.captureId;
    request.metadata.captureIndex = captureIndex;
    request.metadata.bootCount = request.bootCount;
    request.metadata.capturePolicy = capturePolicy;
    request.metadata.backendName = this->m_capabilities.backendName;
    request.metadata.cameraModel = this->m_capabilities.cameraModel;
    request.metadata.rawRelativePath = request.rawRelativePath;
    request.metadata.previewRelativePath = request.previewRelativePath;
    request.metadata.requestedMask = applyMask;
    request.metadata.requestedSettings = requested;
    request.metadata.resultCode = OBC::PayloadResultCode::PRESULT_NONE;

    OBC::PayloadCameraSettings resolved = {};
    request.appliedMask = 0U;
    resolved = this->resolveCaptureSettings_(capturePolicy, requested, applyMask, request.appliedMask);
    request.metadata.appliedMask = request.appliedMask;
    request.metadata.appliedSettings = resolved;

    if (!this->m_manager.beginCapture(request, resolved)) {
        this->failOperation_(OBC::PayloadResultCode::PRESULT_CAPTURE_FAILED, DETAIL_RUNTIME_ROOT_FAILED);
        this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return false;
    }

    this->markPending_(this->m_activeCommand, opCode, cmdSeq, OBC::PayloadOperationKind::CAPTURE);
    this->m_state = OBC::PayloadState::PSTATE_CAPTURING;
    this->m_lastResult = OBC::PayloadResultCode::PRESULT_NONE;
    this->m_lastDetail = DETAIL_NONE;
    this->publishState_();
    this->respondImmediate_(opCode, cmdSeq, Fw::CmdResponse::OK);
    this->m_activeCommand.responseSent = true;
    return true;
}

}  // namespace OBC

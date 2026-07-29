#ifndef OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADOPSCONTROLLER_HPP
#define OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADOPSCONTROLLER_HPP

#include <memory>
#include <map>
#include <mutex>
#include <limits>
#include <string>
#include <vector>

#include "OBC/Components/PayloadOpsController/PayloadOpsControllerComponentAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"
#include "OBC/Components/PayloadOpsController/PiCameraManager.hpp"

namespace OBC {

class PayloadOpsController final : public PayloadOpsControllerComponentBase {
  public:
    explicit PayloadOpsController(const char* compName);
    ~PayloadOpsController() override;

    bool configureRuntime(const std::string& runtimeRoot,
                          const OBC::IModeSafetyModeControl* modeProvider,
                          OBC::IPayloadEpsControl* epsControl,
                          const OBC::IRecoveryBootControl* bootControl,
                          std::unique_ptr<OBC::IPiCameraDriver> driver,
                          const OBC::PayloadRuntimeConfig& runtimeConfig = {});
    bool getStatusSnapshotForRuntime(OBC::PayloadStatusSnapshot& snapshot) const;
    bool getCapabilitiesForRuntime(OBC::PayloadCapabilities& capabilities) const;
    bool getLastCaptureMetadataForRuntime(OBC::PayloadCaptureMetadata& metadata) const;

#ifdef BUILD_UT
    void setAbortTotalForTest(U32 value);
#endif

  private:
    struct PendingCommand {
        bool active = false;
        bool responseSent = false;
        FwOpcodeType opCode = 0U;
        U32 cmdSeq = 0U;
        OBC::PayloadOperationKind kind = OBC::PayloadOperationKind::NONE;
        bool forcedModeExit = false;
    };

    struct PendingDataProductPublish {
        bool active = false;
        U32 captureId = 0U;
        U8 captureIndex = 0U;
        OBC::PayloadArtifactKind artifactKind = OBC::PayloadArtifactKind::PREVIEW_JPEG;
        U32 timeoutTicksRemaining = 0U;
        std::vector<std::string> expectedPaths;
        U32 writtenPacketBytes = 0U;
        OBC::PayloadCaptureMetadata metadata = {};
        std::vector<U8> artifactBytes;
        Fw::Time baseTimeTag = {};
        FwSizeType nextOffset = 0U;
        U32 nextSliceIndex = 0U;
        OBC::PayloadState completionState = OBC::PayloadState::PSTATE_READY;
    };

    struct PendingDataProductWriteCompletion {
        bool active = false;
        std::string fileName;
        FwSizeType size = 0U;
    };

    static constexpr U32 DETAIL_NONE = 0U;
    static constexpr U32 DETAIL_RUNTIME_UNCONFIGURED = 1U;
    static constexpr U32 DETAIL_INVALID_EXPOSURE = 2U;
    static constexpr U32 DETAIL_INVALID_GAIN = 3U;
    static constexpr U32 DETAIL_RUNTIME_ROOT_FAILED = 4U;
    static constexpr U32 DETAIL_INVALID_JPEG_QUALITY = 5U;
    static constexpr U32 DETAIL_INVALID_MASK = 6U;
    static constexpr U32 DETAIL_UNSUPPORTED_FIELD = 7U;
    static constexpr U32 DETAIL_INVALID_RESOLUTION = 8U;
    static constexpr U32 DETAIL_CAPTURE_METADATA_FAILED = 9U;
    static constexpr U32 DETAIL_RAW_SESSION_REQUIRED = 10U;
    static constexpr U32 DETAIL_INVALID_REGISTER_ADDRESS = 11U;
    static constexpr U32 DETAIL_INVALID_REGISTER_VALUE = 12U;
    static constexpr U32 DETAIL_DATA_PRODUCT_READ_FAILED = 13U;
    static constexpr U32 DETAIL_DATA_PRODUCT_TOO_LARGE = 14U;
    static constexpr U32 DETAIL_DATA_PRODUCT_PORTS_UNAVAILABLE = 15U;
    static constexpr U32 DETAIL_DATA_PRODUCT_BUFFER_UNAVAILABLE = 16U;
    static constexpr U32 DETAIL_DATA_PRODUCT_SERIALIZE_FAILED = 17U;
    static constexpr U32 DETAIL_DATA_PRODUCT_WRITE_TIMEOUT = 18U;
    static constexpr U32 DETAIL_CAPTURE_MANIFEST_WRITE_FAILED = 19U;
    static constexpr U32 DETAIL_CAPTURE_MANIFEST_READ_FAILED = 20U;
    static constexpr U32 DETAIL_CAPTURE_INDEX_NOT_FOUND = 21U;
    static constexpr U32 DETAIL_ARTIFACT_NOT_AVAILABLE = 22U;
    static constexpr U32 DETAIL_ARTIFACT_KIND_INVALID = 23U;
    static constexpr U32 DETAIL_FULL_RAW_DEFERRED = 24U;
    static constexpr U32 DETAIL_SESSION_REPREPARE_REQUIRED = 25U;
    static constexpr FwSizeType MAX_ARTIFACT_RECORD_BYTES =
        static_cast<FwSizeType>(std::numeric_limits<FwSizeStoreType>::max());
    static constexpr FwSizeType MAX_DATA_PRODUCT_RECORD_OVERHEAD = sizeof(FwDpIdType) + sizeof(FwSizeStoreType);
    static constexpr FwSizeType MAX_DATA_PRODUCT_CONTAINER_BYTES =
        static_cast<FwSizeType>(std::numeric_limits<FwSizeStoreType>::max());

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void PAYLOAD_SET_CAMERA_DEFAULTS_cmdHandler(FwOpcodeType opCode,
                                                U32 cmdSeq,
                                                OBC::PayloadResolutionPreset resolutionPreset,
                                                U32 jpegQuality,
                                                bool hflip,
                                                bool vflip) override;
    void PAYLOAD_PREPARE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_ABORT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_SHUTDOWN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_PREPARE_RAW_SENSOR_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_SET_AUTO_DEFAULTS_cmdHandler(FwOpcodeType opCode,
                                              U32 cmdSeq,
                                              OBC::PayloadAwbMode awbMode,
                                              OBC::PayloadMeteringMode meteringMode,
                                              I32 evCompX100) override;
    void PAYLOAD_SET_DETERMINISTIC_DEFAULTS_cmdHandler(FwOpcodeType opCode,
                                                       U32 cmdSeq,
                                                       U32 exposureUsec,
                                                       U32 gainX100) override;
    void PAYLOAD_CAPTURE_AUTO_cmdHandler(FwOpcodeType opCode,
                                         U32 cmdSeq,
                                         U8 captureIndex,
                                         const Fw::CmdStringArg& tag,
                                         U32 applyMask,
                                         OBC::PayloadAwbMode awbMode,
                                         OBC::PayloadMeteringMode meteringMode,
                                         I32 evCompX100) override;
    void PAYLOAD_CAPTURE_DETERMINISTIC_cmdHandler(FwOpcodeType opCode,
                                                  U32 cmdSeq,
                                                  U8 captureIndex,
                                                  const Fw::CmdStringArg& tag,
                                                  U32 applyMask,
                                                  U32 exposureUsec,
                                                  U32 gainX100) override;
    void PAYLOAD_GET_CAPABILITIES_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_GET_LAST_CAPTURE_METADATA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void PAYLOAD_SENSOR_REG_READ_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 address) override;
    void PAYLOAD_SENSOR_REG_WRITE_cmdHandler(FwOpcodeType opCode,
                                             U32 cmdSeq,
                                             U32 address,
                                             U32 value,
                                             bool verifyReadback) override;
    void PAYLOAD_CAPTURE_RAW_cmdHandler(FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        U8 captureIndex,
                                        const Fw::CmdStringArg& tag) override;
    void PAYLOAD_PUBLISH_CAPTURE_cmdHandler(FwOpcodeType opCode,
                                            U32 cmdSeq,
                                            U8 captureIndex,
                                            OBC::PayloadArtifactKind artifactKind) override;
    void dpWrittenIn_handler(FwIndexType portNum,
                             const Fw::StringBase& fileName,
                             FwDpPriorityType priority,
                             FwSizeType size) override;

  private:
    bool validateDeterministicDefaults_(const OBC::PayloadCameraSettings& settings, U32& detailCode) const;
    bool validateAutoDefaults_(const OBC::PayloadCameraSettings& settings, U32& detailCode) const;
    bool validateCaptureRequest_(OBC::PayloadCapturePolicy capturePolicy,
                                 const OBC::PayloadCameraSettings& requested,
                                 U32 applyMask,
                                 U32& detailCode) const;
    bool validateProfileCommon_(const OBC::PayloadCameraSettings& settings, U32& detailCode) const;
    bool validateRawRegisterAddress_(U32 address, U32& detailCode) const;
    bool validateRawRegisterValue_(U32 value, U32& detailCode) const;
    bool readyForRawSensorSession_(U32& detailCode) const;
    bool currentModeAllowsPayloadOps_() const;
    bool isRuntimeConfigured_() const;
    std::string captureRawRelativePathForIndex_(U8 captureIndex) const;
    std::string capturePreviewRelativePathForIndex_(U8 captureIndex) const;
    std::string captureManifestRelativePathForIndex_(U8 captureIndex) const;
    std::string dataProductRelativePathForTime_(const Fw::Time& timeTag) const;
    std::string joinPath_(const std::string& base, const char* child) const;
    bool ensureDirectoryTree_(const std::string& path) const;
    bool writeCaptureManifest_(const OBC::PayloadCaptureMetadata& metadata) const;
    bool loadCaptureManifest_(U8 captureIndex, OBC::PayloadCaptureMetadata& metadata, U32& detailCode) const;
    bool loadArtifactBytes_(const std::string& relativePath, std::vector<U8>& bytes, U32& detailCode) const;
    bool payloadWorkInProgress_() const;
    bool startCapturePreviewPublish_(const OBC::PayloadOperationResult& result);
    bool startArtifactPublish_(const OBC::PayloadCaptureMetadata& metadata,
                               OBC::PayloadArtifactKind artifactKind,
                               OBC::PayloadState completionState,
                               U32& detailCode);
    bool planArtifactDataProduct_(OBC::PayloadCaptureMetadata& metadata,
                                  OBC::PayloadArtifactKind artifactKind,
                                  const std::vector<U8>& artifactBytes,
                                  const Fw::Time& timeTag,
                                  std::vector<std::string>& expectedPaths,
                                  U32& packetBytes,
                                  U32& detailCode);
    bool publishNextPendingDataProductSlice_(U32& detailCode);
    void handlePendingDataProductWriteCompletion_();
    FwSizeType computePayloadArtifactRecordDataSize_(FwSizeType artifactBytes) const;
    FwSizeType maxPayloadArtifactBytesPerDataProduct_() const;
    Fw::Time dataProductTimeTagForSlice_(const Fw::Time& baseTimeTag, U32 sliceIndex) const;
    void finalizeDataProductPublishSuccess_();
    void finalizeDataProductPublishFailure_(U32 detailCode);
    void resetPendingDataProductPublish_();
    OBC::PayloadCaptureArtifactHeaderV2 makeCaptureArtifactHeader_(const OBC::PayloadCaptureMetadata& metadata,
                                                                   OBC::PayloadArtifactKind artifactKind,
                                                                   const std::string& currentDataProductPath,
                                                                   bool currentDataProductPublished) const;
    OBC::PayloadCameraSettings prepareSettingsForReady_(OBC::PayloadReadyKind readyKind) const;
    bool captureSessionMatchesPreparedSettings_(OBC::PayloadCapturePolicy capturePolicy,
                                                const OBC::PayloadCameraSettings& preparedSettings,
                                                const OBC::PayloadCameraSettings& requested,
                                                U32& detailCode) const;
    OBC::PayloadCameraSettings resolveCaptureSettings_(OBC::PayloadCapturePolicy capturePolicy,
                                                       const OBC::PayloadCameraSettings& requested,
                                                       U32 applyMask,
                                                       U32& appliedMask) const;
    void publishState_();
    void syncServiceSnapshot_();
    void publishStatusEvent_() const;
    void publishCapabilitiesEvent_() const;
    void publishLastCaptureMetadataEvent_() const;
    void logProxyPowerChange_();
    void startForcedCleanupIfNeeded_();
    void handleCompletedOperation_(const OBC::PayloadOperationResult& result);
    void respondImmediate_(FwOpcodeType opCode, U32 cmdSeq, Fw::CmdResponse response);
    void failOperation_(OBC::PayloadResultCode resultCode, U32 detailCode);
    void clearPending_(PendingCommand& pending, Fw::CmdResponse response);
    void markPending_(PendingCommand& pending, FwOpcodeType opCode, U32 cmdSeq, OBC::PayloadOperationKind kind);
    bool beginPrepareReady_(FwOpcodeType opCode, U32 cmdSeq, OBC::PayloadReadyKind readyKind);
    bool beginCapture_(FwOpcodeType opCode,
                       U32 cmdSeq,
                       U8 captureIndex,
                       const std::string& tag,
                       OBC::PayloadCapturePolicy capturePolicy,
                       const OBC::PayloadCameraSettings& requested,
                       U32 applyMask);

  private:
    std::string m_runtimeRoot;
    std::string m_captureRoot;
    std::string m_captureManifestRoot;
    const OBC::IModeSafetyModeControl* m_modeProvider;
    OBC::IPayloadEpsControl* m_epsControl;
    const OBC::IRecoveryBootControl* m_bootControl;
    OBC::PayloadRuntimeConfig m_runtimeConfig;
    OBC::PayloadCapabilities m_capabilities;
    OBC::PayloadCameraSettings m_cameraDefaults;
    OBC::PayloadCameraSettings m_autoDefaults;
    OBC::PayloadCameraSettings m_deterministicDefaults;
    OBC::PiCameraManager m_manager;
    OBC::PayloadState m_state;
    OBC::PayloadReadyKind m_preparedReadyKind;
    OBC::PayloadResultCode m_lastResult;
    U32 m_lastDetail;
    U32 m_lastCaptureId;
    U8 m_lastCaptureIndex;
    std::string m_lastRawRelativePath;
    std::string m_lastPreviewRelativePath;
    U32 m_lastRequestedMask;
    U32 m_lastAppliedMask;
    OBC::PayloadCaptureMetadata m_lastCaptureMetadata;
    U32 m_captureCounter;
    U32 m_abortTotal;
    bool m_runtimeConfigured;
    bool m_forcedCleanupActive;
    PendingCommand m_activeCommand;
    PendingCommand m_abortCommand;
    PendingDataProductPublish m_pendingDataProductPublish;
    PendingDataProductWriteCompletion m_pendingDataProductWriteCompletion;
    std::map<U8, OBC::PayloadCaptureMetadata> m_captureCatalog;
    mutable std::mutex m_serviceSnapshotMutex;
    OBC::PayloadServiceSnapshot m_serviceSnapshot;
};

}  // namespace OBC

#endif

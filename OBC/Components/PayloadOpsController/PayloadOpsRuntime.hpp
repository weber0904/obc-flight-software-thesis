#ifndef OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADOPSRUNTIME_HPP
#define OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADOPSRUNTIME_HPP

#include <atomic>
#include <memory>
#include <string>

#include "Fw/Cmd/CmdResponseEnumAc.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/PayloadOpsController/PayloadAwbModeEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadArtifactKindEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadMeteringModeEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadPixelFormatEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadResolutionPresetEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadResultCodeEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadCapturePolicyEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadReadyKindEnumAc.hpp"
#include "OBC/Components/PayloadOpsController/PayloadStateEnumAc.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"

namespace OBC {
namespace EPS {
struct StatusData;
}

static constexpr U32 PAYLOAD_DATA_PRODUCT_MAX_BYTES = 2U * 1024U * 1024U;

namespace PayloadFieldMask {
static constexpr U32 RESOLUTION = 1U << 0;
static constexpr U32 JPEG_QUALITY = 1U << 1;
static constexpr U32 HFLIP = 1U << 2;
static constexpr U32 VFLIP = 1U << 3;
static constexpr U32 EXPOSURE_USEC = 1U << 4;
static constexpr U32 GAIN_X100 = 1U << 5;
static constexpr U32 AWB_MODE = 1U << 6;
static constexpr U32 METERING_MODE = 1U << 7;
static constexpr U32 EV_COMP_X100 = 1U << 8;
static constexpr U32 BRIGHTNESS_X100 = 1U << 9;
static constexpr U32 CONTRAST_X100 = 1U << 10;
static constexpr U32 SATURATION_X100 = 1U << 11;
static constexpr U32 SHARPNESS_X100 = 1U << 12;
}  // namespace PayloadFieldMask

struct PayloadRuntimeConfig {
    U32 proxyPduChannel = 3U;
    U32 powerSettleMs = 2000U;
    U32 initTimeoutMs = 10000U;
    U32 captureTimeoutMs = 10000U;
    U32 rawRegisterTimeoutMs = 1000U;
    OBC::PayloadResolutionPreset defaultResolution = OBC::PayloadResolutionPreset::PRESET_HD_1280X720;
    U32 defaultExposureUsec = 10000U;
    U32 defaultGainX100 = 100U;
    U32 defaultJpegQuality = 90U;
    U32 dataProductMaxBytes = PAYLOAD_DATA_PRODUCT_MAX_BYTES;
    U32 dataProductPublishTimeoutTicks = 50U;
};

struct PayloadCameraSettings {
    OBC::PayloadReadyKind readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    OBC::PayloadCapturePolicy capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    OBC::PayloadResolutionPreset resolution = OBC::PayloadResolutionPreset::PRESET_HD_1280X720;
    U32 jpegQuality = 90U;
    bool hflip = false;
    bool vflip = false;
    U32 exposureUsec = 10000U;
    U32 gainX100 = 100U;
    OBC::PayloadAwbMode awbMode = OBC::PayloadAwbMode::AWB_AUTO;
    OBC::PayloadMeteringMode meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;
    I32 evCompX100 = 0;
    I32 brightnessX100 = 0;
    I32 contrastX100 = 0;
    I32 saturationX100 = 0;
    I32 sharpnessX100 = 0;
};

struct PayloadCapabilities {
    std::string backendName;
    std::string cameraModel;
    U32 supportedMask = 0U;
    U32 offOnlyMask = 0U;
    U32 autoMutableMask = 0U;
    U32 deterministicMutableMask = 0U;
    bool rawRegisterSupported = false;
    bool realSensorPath = false;
};

struct PayloadCaptureMetadata {
    OBC::PayloadCapturePolicy capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    std::string backendName;
    std::string cameraModel;
    U32 captureId = 0U;
    U8 captureIndex = 0U;
    U32 bootCount = 0U;
    U32 captureTimeSec = 0U;
    U32 captureTimeUsec = 0U;
    U32 requestedMask = 0U;
    U32 appliedMask = 0U;
    OBC::PayloadPixelFormat pixelFormat = OBC::PayloadPixelFormat::PIXEL_UNKNOWN;
    U32 imageWidth = 0U;
    U32 imageHeight = 0U;
    std::string rawRelativePath;
    std::string previewRelativePath;
    std::string previewDataProductRelativePath;
    std::string rawDataProductRelativePath;
    OBC::PayloadCameraSettings requestedSettings = {};
    OBC::PayloadCameraSettings appliedSettings = {};
    U32 actualExposureUsec = 0U;
    U32 actualGainX100 = 0U;
    bool actualAwbValid = false;
    U32 actualAwbColorTemperatureK = 0U;
    U32 actualAwbRedGainX1000 = 0U;
    U32 actualAwbBlueGainX1000 = 0U;
    U32 rawBytes = 0U;
    U32 previewJpegBytes = 0U;
    U32 previewDataProductBytes = 0U;
    U32 rawDataProductBytes = 0U;
    bool previewDataProductPublished = false;
    bool rawDataProductPublished = false;
    OBC::PayloadArtifactKind lastPublishedArtifactKind = OBC::PayloadArtifactKind::PREVIEW_JPEG;
    OBC::PayloadResultCode resultCode = OBC::PayloadResultCode::PRESULT_NONE;
};

struct PayloadStatusSnapshot {
    bool runtimeConfigured = false;
    bool busy = false;
    bool logicalPowerEnabled = false;
    bool prepared = false;
    bool proxyAsserted = false;
    OBC::PayloadState state = OBC::PayloadState::PSTATE_OFF;
    OBC::PayloadReadyKind preparedReadyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    OBC::PayloadResultCode lastResult = OBC::PayloadResultCode::PRESULT_NONE;
    U32 lastDetail = 0U;
    U32 lastCaptureId = 0U;
    U8 lastCaptureIndex = 0U;
    U32 lastRequestedMask = 0U;
    U32 lastAppliedMask = 0U;
    U32 abortTotal = 0U;
};

struct PayloadServiceSnapshot {
    OBC::PayloadStatusSnapshot status = {};
    OBC::PayloadCapabilities capabilities = {};
    OBC::PayloadCaptureMetadata lastCaptureMetadata = {};
};

struct PayloadCaptureRequest {
    std::string tag;
    U32 captureId = 0U;
    U8 captureIndex = 0U;
    U32 bootCount = 0U;
    U32 requestedMask = 0U;
    U32 appliedMask = 0U;
    std::string rawOutputPath;
    std::string previewOutputPath;
    std::string rawRelativePath;
    std::string previewRelativePath;
    OBC::PayloadCaptureMetadata metadata = {};
};

enum class PayloadOperationKind : U8 {
    NONE = 0U,
    PREPARE = 1U,
    CAPTURE = 2U,
    SHUTDOWN = 3U,
    ABORT = 4U,
    INTERNAL_MODE_EXIT = 5U,
    PUBLISH_CAPTURE = 6U,
};

struct PayloadOperationResult {
    OBC::PayloadOperationKind kind = OBC::PayloadOperationKind::NONE;
    Fw::CmdResponse response = Fw::CmdResponse::EXECUTION_ERROR;
    OBC::PayloadResultCode resultCode = OBC::PayloadResultCode::PRESULT_NONE;
    U32 detailCode = 0U;
    bool logicalPowerEnabled = false;
    bool prepared = false;
    bool proxyAsserted = false;
    OBC::PayloadReadyKind readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    U32 captureId = 0U;
    U8 captureIndex = 0U;
    U32 requestedMask = 0U;
    U32 appliedMask = 0U;
    std::string rawRelativePath;
    std::string previewRelativePath;
    OBC::PayloadCaptureMetadata metadata = {};
};

class IPayloadEpsControl {
  public:
    virtual ~IPayloadEpsControl() = default;

    virtual bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const = 0;

    virtual Fw::CmdResponse setPayloadProxyPower(bool enabled, OBC::EPS::StatusData& status) = 0;
};

class IPiCameraDriver {
  public:
    virtual ~IPiCameraDriver() = default;

    virtual const char* getName() const = 0;

    virtual bool isAvailable() const = 0;

    virtual OBC::PayloadCapabilities getCapabilities() const = 0;

    virtual Fw::CmdResponse prepare(const OBC::PayloadReadyKind readyKind,
                                    const OBC::PayloadCameraSettings& settings,
                                    U32 initTimeoutMs,
                                    const std::atomic<bool>& cancelRequested,
                                    U32& detailCode) = 0;

    virtual Fw::CmdResponse captureStill(const OBC::PayloadCaptureRequest& request,
                                         const OBC::PayloadCameraSettings& settings,
                                         U32 captureTimeoutMs,
                                         const std::atomic<bool>& cancelRequested,
                                         OBC::PayloadCaptureMetadata& metadata,
                                         U32& detailCode) = 0;

    virtual Fw::CmdResponse shutdown(U32& detailCode) = 0;

    virtual Fw::CmdResponse readSensorRegister(U32 address, U32 timeoutMs, U32& value, U32& detailCode) = 0;

    virtual Fw::CmdResponse writeSensorRegister(U32 address,
                                                U32 value,
                                                bool verifyReadback,
                                                U32 timeoutMs,
                                                U32& readbackValue,
                                                U32& detailCode) = 0;

    virtual void abort() = 0;
};

std::unique_ptr<OBC::IPiCameraDriver> makeDefaultPiCameraDriver();
std::unique_ptr<OBC::IPiCameraDriver> makePayloadHelperBackendDriver();

bool payloadMaskSubset(U32 mask, U32 allowedMask);
bool payloadMaskIntersects(U32 mask, U32 disallowedMask);
std::string payloadReadyKindName(OBC::PayloadReadyKind kind);
std::string payloadCapturePolicyName(OBC::PayloadCapturePolicy kind);

}  // namespace OBC

#endif

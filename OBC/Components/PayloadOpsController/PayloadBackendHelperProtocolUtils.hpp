#ifndef OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADBACKENDHELPERPROTOCOLUTILS_HPP
#define OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADBACKENDHELPERPROTOCOLUTILS_HPP

#include <algorithm>
#include <cstring>
#include <string>

#include "OBC/Components/PayloadOpsController/PayloadBackendHelperProtocol.hpp"
#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

namespace OBC {
namespace PayloadHelperProtocolUtils {

template <std::size_t N>
inline void copyBoundedString(char (&dst)[N], const std::string& value) {
    const std::size_t count = std::min<std::size_t>(value.size(), N - 1U);
    if (count > 0U) {
        std::memcpy(dst, value.c_str(), count);
    }
    dst[count] = '\0';
    if (count + 1U < N) {
        std::memset(dst + count + 1U, 0, N - count - 1U);
    }
}

template <std::size_t N>
inline std::string decodeBoundedString(const char (&src)[N]) {
    return std::string(src, strnlen(src, N));
}

inline OBC::PayloadHelperProtocol::CameraSettingsWire encodeSettings(const OBC::PayloadCameraSettings& settings) {
    OBC::PayloadHelperProtocol::CameraSettingsWire wire = {};
    wire.readyKind = static_cast<U8>(settings.readyKind.e);
    wire.capturePolicy = static_cast<U8>(settings.capturePolicy.e);
    wire.resolution = static_cast<U8>(settings.resolution.e);
    wire.awbMode = static_cast<U8>(settings.awbMode.e);
    wire.meteringMode = static_cast<U8>(settings.meteringMode.e);
    wire.jpegQuality = settings.jpegQuality;
    wire.hflip = settings.hflip ? 1U : 0U;
    wire.vflip = settings.vflip ? 1U : 0U;
    wire.exposureUsec = settings.exposureUsec;
    wire.gainX100 = settings.gainX100;
    wire.evCompX100 = settings.evCompX100;
    wire.brightnessX100 = settings.brightnessX100;
    wire.contrastX100 = settings.contrastX100;
    wire.saturationX100 = settings.saturationX100;
    wire.sharpnessX100 = settings.sharpnessX100;
    return wire;
}

inline OBC::PayloadCameraSettings decodeSettings(const OBC::PayloadHelperProtocol::CameraSettingsWire& wire) {
    OBC::PayloadCameraSettings settings = {};
    settings.readyKind = static_cast<OBC::PayloadReadyKind::T>(wire.readyKind);
    settings.capturePolicy = static_cast<OBC::PayloadCapturePolicy::T>(wire.capturePolicy);
    settings.resolution = static_cast<OBC::PayloadResolutionPreset::T>(wire.resolution);
    settings.awbMode = static_cast<OBC::PayloadAwbMode::T>(wire.awbMode);
    settings.meteringMode = static_cast<OBC::PayloadMeteringMode::T>(wire.meteringMode);
    settings.jpegQuality = wire.jpegQuality;
    settings.hflip = wire.hflip != 0U;
    settings.vflip = wire.vflip != 0U;
    settings.exposureUsec = wire.exposureUsec;
    settings.gainX100 = wire.gainX100;
    settings.evCompX100 = wire.evCompX100;
    settings.brightnessX100 = wire.brightnessX100;
    settings.contrastX100 = wire.contrastX100;
    settings.saturationX100 = wire.saturationX100;
    settings.sharpnessX100 = wire.sharpnessX100;
    return settings;
}

inline OBC::PayloadHelperProtocol::CapabilitiesWire encodeCapabilities(const OBC::PayloadCapabilities& capabilities) {
    OBC::PayloadHelperProtocol::CapabilitiesWire wire = {};
    wire.supportedMask = capabilities.supportedMask;
    wire.offOnlyMask = capabilities.offOnlyMask;
    wire.autoMutableMask = capabilities.autoMutableMask;
    wire.deterministicMutableMask = capabilities.deterministicMutableMask;
    wire.rawRegisterSupported = capabilities.rawRegisterSupported ? 1U : 0U;
    wire.realSensorPath = capabilities.realSensorPath ? 1U : 0U;
    copyBoundedString(wire.backendName, capabilities.backendName);
    copyBoundedString(wire.cameraModel, capabilities.cameraModel);
    return wire;
}

inline OBC::PayloadCapabilities decodeCapabilities(const OBC::PayloadHelperProtocol::CapabilitiesWire& wire) {
    OBC::PayloadCapabilities capabilities = {};
    capabilities.supportedMask = wire.supportedMask;
    capabilities.offOnlyMask = wire.offOnlyMask;
    capabilities.autoMutableMask = wire.autoMutableMask;
    capabilities.deterministicMutableMask = wire.deterministicMutableMask;
    capabilities.rawRegisterSupported = wire.rawRegisterSupported != 0U;
    capabilities.realSensorPath = wire.realSensorPath != 0U;
    capabilities.backendName = decodeBoundedString(wire.backendName);
    capabilities.cameraModel = decodeBoundedString(wire.cameraModel);
    return capabilities;
}

inline OBC::PayloadHelperProtocol::MetadataWire encodeMetadata(const OBC::PayloadCaptureMetadata& metadata) {
    OBC::PayloadHelperProtocol::MetadataWire wire = {};
    wire.capturePolicy = static_cast<U8>(metadata.capturePolicy.e);
    wire.resultCode = static_cast<U8>(metadata.resultCode.e);
    wire.captureIndex = metadata.captureIndex;
    wire.pixelFormat = static_cast<U8>(metadata.pixelFormat.e);
    wire.captureId = metadata.captureId;
    wire.bootCount = metadata.bootCount;
    wire.captureTimeSec = metadata.captureTimeSec;
    wire.captureTimeUsec = metadata.captureTimeUsec;
    wire.requestedMask = metadata.requestedMask;
    wire.appliedMask = metadata.appliedMask;
    wire.actualExposureUsec = metadata.actualExposureUsec;
    wire.actualGainX100 = metadata.actualGainX100;
    wire.actualAwbValid = metadata.actualAwbValid ? 1U : 0U;
    wire.actualAwbColorTemperatureK = metadata.actualAwbColorTemperatureK;
    wire.actualAwbRedGainX1000 = metadata.actualAwbRedGainX1000;
    wire.actualAwbBlueGainX1000 = metadata.actualAwbBlueGainX1000;
    wire.imageWidth = metadata.imageWidth;
    wire.imageHeight = metadata.imageHeight;
    wire.rawBytes = metadata.rawBytes;
    wire.previewJpegBytes = metadata.previewJpegBytes;
    wire.requestedSettings = encodeSettings(metadata.requestedSettings);
    wire.appliedSettings = encodeSettings(metadata.appliedSettings);
    copyBoundedString(wire.backendName, metadata.backendName);
    copyBoundedString(wire.cameraModel, metadata.cameraModel);
    copyBoundedString(wire.rawRelativePath, metadata.rawRelativePath);
    copyBoundedString(wire.previewRelativePath, metadata.previewRelativePath);
    return wire;
}

inline OBC::PayloadCaptureMetadata decodeMetadata(const OBC::PayloadHelperProtocol::MetadataWire& wire) {
    OBC::PayloadCaptureMetadata metadata = {};
    metadata.capturePolicy = static_cast<OBC::PayloadCapturePolicy::T>(wire.capturePolicy);
    metadata.resultCode = static_cast<OBC::PayloadResultCode::T>(wire.resultCode);
    metadata.captureIndex = wire.captureIndex;
    metadata.pixelFormat = static_cast<OBC::PayloadPixelFormat::T>(wire.pixelFormat);
    metadata.captureId = wire.captureId;
    metadata.bootCount = wire.bootCount;
    metadata.captureTimeSec = wire.captureTimeSec;
    metadata.captureTimeUsec = wire.captureTimeUsec;
    metadata.requestedMask = wire.requestedMask;
    metadata.appliedMask = wire.appliedMask;
    metadata.actualExposureUsec = wire.actualExposureUsec;
    metadata.actualGainX100 = wire.actualGainX100;
    metadata.actualAwbValid = wire.actualAwbValid != 0U;
    metadata.actualAwbColorTemperatureK = wire.actualAwbColorTemperatureK;
    metadata.actualAwbRedGainX1000 = wire.actualAwbRedGainX1000;
    metadata.actualAwbBlueGainX1000 = wire.actualAwbBlueGainX1000;
    metadata.imageWidth = wire.imageWidth;
    metadata.imageHeight = wire.imageHeight;
    metadata.rawBytes = wire.rawBytes;
    metadata.previewJpegBytes = wire.previewJpegBytes;
    metadata.requestedSettings = decodeSettings(wire.requestedSettings);
    metadata.appliedSettings = decodeSettings(wire.appliedSettings);
    metadata.backendName = decodeBoundedString(wire.backendName);
    metadata.cameraModel = decodeBoundedString(wire.cameraModel);
    metadata.rawRelativePath = decodeBoundedString(wire.rawRelativePath);
    metadata.previewRelativePath = decodeBoundedString(wire.previewRelativePath);
    return metadata;
}

}  // namespace PayloadHelperProtocolUtils
}  // namespace OBC

#endif

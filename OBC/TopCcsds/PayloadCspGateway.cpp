#include "OBC/TopCcsds/PayloadCspGateway.hpp"

#include <algorithm>
#include <cstring>

namespace OBCApp {

namespace {

template <std::size_t N>
void copyBoundedString(char (&dest)[N], const std::string& src) {
    static_assert(N > 0U, "destination buffer must be non-empty");
    const std::size_t count = std::min(src.size(), N - 1U);
    std::memset(dest, 0, N);
    if (count > 0U) {
        std::memcpy(dest, src.data(), count);
    }
}

}  // namespace

void PayloadCspGateway::populateStatusReply(std::uint16_t seq,
                                            const OBC::PayloadStatusSnapshot& status,
                                            PayloadCSP::StatusReply& reply) {
    reply = {};
    reply.header = PayloadCSP::makeReplyHeader(PayloadCSP::ServicePort::STATUS, seq, PayloadCSP::ResultCode::OK);
    reply.state = static_cast<std::uint8_t>(status.state.e);
    reply.preparedReadyKind = static_cast<std::uint8_t>(status.preparedReadyKind.e);
    reply.lastResult = static_cast<std::uint8_t>(status.lastResult.e);
    reply.lastDetail = status.lastDetail;
    reply.lastCaptureId = status.lastCaptureId;
    reply.lastRequestedMask = status.lastRequestedMask;
    reply.lastAppliedMask = status.lastAppliedMask;
    reply.abortTotal = status.abortTotal;
    if (status.busy) {
        reply.header.flags |= PayloadCSP::FLAG_BUSY;
    }
    if (status.logicalPowerEnabled) {
        reply.header.flags |= PayloadCSP::FLAG_LOGICAL_POWER_ENABLED;
    }
    if (status.prepared) {
        reply.header.flags |= PayloadCSP::FLAG_PREPARED;
    }
    if (status.proxyAsserted) {
        reply.header.flags |= PayloadCSP::FLAG_PROXY_ASSERTED;
    }
    reply.header.byteCount = static_cast<std::uint16_t>(sizeof(reply) - sizeof(reply.header));
}

void PayloadCspGateway::populateCapabilitiesReply(std::uint16_t seq,
                                                  const OBC::PayloadCapabilities& capabilities,
                                                  PayloadCSP::CapabilitiesReply& reply) {
    reply = {};
    reply.header = PayloadCSP::makeReplyHeader(PayloadCSP::ServicePort::CAPABILITIES, seq, PayloadCSP::ResultCode::OK);
    if (capabilities.rawRegisterSupported) {
        reply.header.flags |= PayloadCSP::FLAG_RAW_REGISTER_SUPPORTED;
    }
    if (capabilities.realSensorPath) {
        reply.header.flags |= PayloadCSP::FLAG_REAL_SENSOR_PATH;
    }
    reply.supportedMask = capabilities.supportedMask;
    reply.offOnlyMask = capabilities.offOnlyMask;
    reply.autoMutableMask = capabilities.autoMutableMask;
    reply.deterministicMutableMask = capabilities.deterministicMutableMask;
    copyBoundedString(reply.backendName, capabilities.backendName);
    copyBoundedString(reply.cameraModel, capabilities.cameraModel);
    reply.header.byteCount = static_cast<std::uint16_t>(sizeof(reply) - sizeof(reply.header));
}

void PayloadCspGateway::populateMetadataReply(std::uint16_t seq,
                                              const OBC::PayloadCaptureMetadata& metadata,
                                              PayloadCSP::MetadataReply& reply) {
    reply = {};
    reply.header =
        PayloadCSP::makeReplyHeader(PayloadCSP::ServicePort::LAST_CAPTURE_METADATA, seq, PayloadCSP::ResultCode::OK);
    reply.capturePolicy = static_cast<std::uint8_t>(metadata.capturePolicy.e);
    reply.resultCode = static_cast<std::uint8_t>(metadata.resultCode.e);
    reply.captureIndex = metadata.captureIndex;
    reply.pixelFormat = static_cast<std::uint8_t>(metadata.pixelFormat.e);
    reply.previewDataProductPublished = metadata.previewDataProductPublished ? 1U : 0U;
    reply.rawDataProductPublished = metadata.rawDataProductPublished ? 1U : 0U;
    reply.captureId = metadata.captureId;
    reply.bootCount = metadata.bootCount;
    reply.captureTimeSec = metadata.captureTimeSec;
    reply.captureTimeUsec = metadata.captureTimeUsec;
    reply.requestedMask = metadata.requestedMask;
    reply.appliedMask = metadata.appliedMask;
    reply.resolution = static_cast<std::uint32_t>(metadata.appliedSettings.resolution.e);
    reply.jpegQualityApplied = metadata.appliedSettings.jpegQuality;
    reply.exposureUsec = metadata.appliedSettings.exposureUsec;
    reply.gainX100 = metadata.appliedSettings.gainX100;
    reply.actualExposureUsec = metadata.actualExposureUsec;
    reply.actualGainX100 = metadata.actualGainX100;
    reply.actualAwbValid = metadata.actualAwbValid ? 1U : 0U;
    reply.actualAwbColorTemperatureK = metadata.actualAwbColorTemperatureK;
    reply.actualAwbRedGainX1000 = metadata.actualAwbRedGainX1000;
    reply.actualAwbBlueGainX1000 = metadata.actualAwbBlueGainX1000;
    reply.imageWidth = metadata.imageWidth;
    reply.imageHeight = metadata.imageHeight;
    reply.rawBytes = metadata.rawBytes;
    reply.previewJpegBytes = metadata.previewJpegBytes;
    reply.previewDataProductBytes = metadata.previewDataProductBytes;
    reply.rawDataProductBytes = metadata.rawDataProductBytes;
    copyBoundedString(reply.rawRelativePath, metadata.rawRelativePath);
    copyBoundedString(reply.previewRelativePath, metadata.previewRelativePath);
    copyBoundedString(reply.previewDataProductPath, metadata.previewDataProductRelativePath);
    copyBoundedString(reply.rawDataProductPath, metadata.rawDataProductRelativePath);
    reply.header.byteCount = static_cast<std::uint16_t>(sizeof(reply) - sizeof(reply.header));
}

}  // namespace OBCApp

#ifndef OBC_TOPCCSDS_PAYLOADCSPPROTOCOL_HPP
#define OBC_TOPCCSDS_PAYLOADCSPPROTOCOL_HPP

#include <cstddef>
#include <cstdint>

namespace OBCApp {
namespace PayloadCSP {

static constexpr std::uint8_t VERSION = 3U;
static constexpr std::uint16_t RESERVED_VIRTUAL_NODE_ID = 7U;
static constexpr std::uint16_t FIRST_LOCAL_SERVICE_NODE_ID = 1U;
static constexpr std::uint8_t RESERVED_VIRTUAL_SERVICE_PORT_BASE = 40U;
static constexpr std::uint8_t RESERVED_VIRTUAL_SERVICE_PORT_LIMIT = 49U;
static constexpr std::uint8_t BACKEND_NAME_LENGTH = 32U;
static constexpr std::uint8_t CAMERA_MODEL_LENGTH = 32U;
static constexpr std::uint8_t RELATIVE_PATH_LENGTH = 64U;

enum class ServicePort : std::uint8_t {
    STATUS = 34U,
    CAPABILITIES = 35U,
    LAST_CAPTURE_METADATA = 36U,
};

enum class ResultCode : std::uint8_t {
    OK = 0U,
    INVALID_REQUEST = 1U,
    NOT_CONFIGURED = 2U,
    INTERNAL_ERROR = 3U,
};

enum ReplyFlags : std::uint8_t {
    FLAG_BUSY = 0x01U,
    FLAG_LOGICAL_POWER_ENABLED = 0x02U,
    FLAG_PREPARED = 0x04U,
    FLAG_PROXY_ASSERTED = 0x08U,
    FLAG_RAW_REGISTER_SUPPORTED = 0x10U,
    FLAG_REAL_SENSOR_PATH = 0x20U,
};

struct RequestHeader {
    std::uint8_t version;
    std::uint8_t service;
    std::uint16_t seq;
};

struct ReplyHeader {
    std::uint8_t version;
    std::uint8_t service;
    std::uint16_t seq;
    std::uint8_t result;
    std::uint8_t flags;
    std::uint16_t byteCount;
};

struct BasicRequest {
    RequestHeader header;
    std::uint32_t reserved;
};

struct StatusReply {
    ReplyHeader header;
    std::uint8_t state;
    std::uint8_t preparedReadyKind;
    std::uint8_t lastResult;
    std::uint8_t reserved0;
    std::uint32_t lastDetail;
    std::uint32_t lastCaptureId;
    std::uint32_t lastRequestedMask;
    std::uint32_t lastAppliedMask;
    std::uint32_t abortTotal;
};

struct CapabilitiesReply {
    ReplyHeader header;
    std::uint32_t supportedMask;
    std::uint32_t offOnlyMask;
    std::uint32_t autoMutableMask;
    std::uint32_t deterministicMutableMask;
    char backendName[BACKEND_NAME_LENGTH];
    char cameraModel[CAMERA_MODEL_LENGTH];
};

struct MetadataReply {
    ReplyHeader header;
    std::uint8_t capturePolicy;
    std::uint8_t resultCode;
    std::uint8_t captureIndex;
    std::uint8_t pixelFormat;
    std::uint8_t previewDataProductPublished;
    std::uint8_t rawDataProductPublished;
    std::uint32_t captureId;
    std::uint32_t bootCount;
    std::uint32_t captureTimeSec;
    std::uint32_t captureTimeUsec;
    std::uint32_t requestedMask;
    std::uint32_t appliedMask;
    std::uint32_t resolution;
    std::uint32_t jpegQualityApplied;
    std::uint32_t exposureUsec;
    std::uint32_t gainX100;
    std::uint32_t actualExposureUsec;
    std::uint32_t actualGainX100;
    std::uint8_t actualAwbValid;
    std::uint8_t reserved0[3];
    std::uint32_t actualAwbColorTemperatureK;
    std::uint32_t actualAwbRedGainX1000;
    std::uint32_t actualAwbBlueGainX1000;
    std::uint32_t imageWidth;
    std::uint32_t imageHeight;
    std::uint32_t rawBytes;
    std::uint32_t previewJpegBytes;
    std::uint32_t previewDataProductBytes;
    std::uint32_t rawDataProductBytes;
    char rawRelativePath[RELATIVE_PATH_LENGTH];
    char previewRelativePath[RELATIVE_PATH_LENGTH];
    char previewDataProductPath[RELATIVE_PATH_LENGTH];
    char rawDataProductPath[RELATIVE_PATH_LENGTH];
};

inline RequestHeader makeRequestHeader(ServicePort service, std::uint16_t seq) {
    RequestHeader header = {};
    header.version = VERSION;
    header.service = static_cast<std::uint8_t>(service);
    header.seq = seq;
    return header;
}

inline BasicRequest makeBasicRequest(ServicePort service, std::uint16_t seq) {
    BasicRequest request = {};
    request.header = makeRequestHeader(service, seq);
    return request;
}

inline ReplyHeader makeReplyHeader(ServicePort service, std::uint16_t seq, ResultCode result) {
    ReplyHeader header = {};
    header.version = VERSION;
    header.service = static_cast<std::uint8_t>(service);
    header.seq = seq;
    header.result = static_cast<std::uint8_t>(result);
    return header;
}

}  // namespace PayloadCSP
}  // namespace OBCApp

#endif

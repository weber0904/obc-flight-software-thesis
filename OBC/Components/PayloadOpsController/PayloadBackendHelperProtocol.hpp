#ifndef OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADBACKENDHELPERPROTOCOL_HPP
#define OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PAYLOADBACKENDHELPERPROTOCOL_HPP

#include <cstdint>

namespace OBC {
namespace PayloadHelperProtocol {

static constexpr std::uint32_t MAGIC = 0x50484C50U;  // PHLP
static constexpr std::uint16_t VERSION = 3U;
static constexpr std::size_t TAG_LENGTH = 32U;
static constexpr std::size_t PATH_LENGTH = 256U;
static constexpr std::size_t RELATIVE_PATH_LENGTH = 128U;
static constexpr std::size_t BACKEND_NAME_LENGTH = 32U;
static constexpr std::size_t CAMERA_MODEL_LENGTH = 32U;

enum class OpCode : std::uint16_t {
    GET_CAPABILITIES = 1U,
    PREPARE = 2U,
    CAPTURE = 3U,
    SHUTDOWN = 4U,
    READ_REGISTER = 5U,
    WRITE_REGISTER = 6U,
};

struct Header {
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t opCode;
};

struct CameraSettingsWire {
    std::uint8_t readyKind;
    std::uint8_t capturePolicy;
    std::uint8_t resolution;
    std::uint8_t awbMode;
    std::uint8_t meteringMode;
    std::uint32_t jpegQuality;
    std::uint8_t hflip;
    std::uint8_t vflip;
    std::uint8_t reserved0[1];
    std::uint32_t exposureUsec;
    std::uint32_t gainX100;
    std::int32_t evCompX100;
    std::int32_t brightnessX100;
    std::int32_t contrastX100;
    std::int32_t saturationX100;
    std::int32_t sharpnessX100;
};

struct MetadataWire {
    std::uint8_t capturePolicy;
    std::uint8_t resultCode;
    std::uint8_t captureIndex;
    std::uint8_t pixelFormat;
    std::uint32_t captureId;
    std::uint32_t bootCount;
    std::uint32_t captureTimeSec;
    std::uint32_t captureTimeUsec;
    std::uint32_t requestedMask;
    std::uint32_t appliedMask;
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
    CameraSettingsWire requestedSettings;
    CameraSettingsWire appliedSettings;
    char backendName[BACKEND_NAME_LENGTH];
    char cameraModel[CAMERA_MODEL_LENGTH];
    char rawRelativePath[RELATIVE_PATH_LENGTH];
    char previewRelativePath[RELATIVE_PATH_LENGTH];
};

struct CapabilitiesWire {
    std::uint32_t supportedMask;
    std::uint32_t offOnlyMask;
    std::uint32_t autoMutableMask;
    std::uint32_t deterministicMutableMask;
    std::uint8_t rawRegisterSupported;
    std::uint8_t realSensorPath;
    std::uint8_t reserved0[2];
    char backendName[BACKEND_NAME_LENGTH];
    char cameraModel[CAMERA_MODEL_LENGTH];
};

struct EmptyRequest {
    Header header;
};

struct PrepareRequest {
    Header header;
    CameraSettingsWire settings;
    std::uint32_t initTimeoutMs;
};

struct CaptureRequestWire {
    Header header;
    CameraSettingsWire settings;
    std::uint32_t captureTimeoutMs;
    std::uint32_t captureId;
    std::uint8_t captureIndex;
    std::uint8_t reserved0[3];
    std::uint32_t bootCount;
    std::uint32_t requestedMask;
    std::uint32_t appliedMask;
    char tag[TAG_LENGTH];
    char rawOutputPath[PATH_LENGTH];
    char previewOutputPath[PATH_LENGTH];
    char rawRelativePath[RELATIVE_PATH_LENGTH];
    char previewRelativePath[RELATIVE_PATH_LENGTH];
};

struct ShutdownRequest {
    Header header;
};

struct RegisterReadRequest {
    Header header;
    std::uint32_t address;
    std::uint32_t timeoutMs;
};

struct RegisterWriteRequest {
    Header header;
    std::uint32_t address;
    std::uint32_t value;
    std::uint32_t timeoutMs;
    std::uint8_t verifyReadback;
    std::uint8_t reserved0[3];
};

struct Reply {
    Header header;
    std::uint32_t response;
    std::uint32_t detailCode;
    std::uint32_t value;
    std::uint32_t readbackValue;
    CapabilitiesWire capabilities;
    MetadataWire metadata;
};

}  // namespace PayloadHelperProtocol
}  // namespace OBC

#endif

#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

#ifndef OBC_HAS_LIBCAMERA

#include <cerrno>
#include <fstream>
#include <map>
#include <vector>

namespace OBC {

namespace {

constexpr U8 STUB_JPEG_BYTES[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xD9,
};

U32 widthForResolution_(OBC::PayloadResolutionPreset preset) {
    switch (preset.e) {
        case OBC::PayloadResolutionPreset::PRESET_VGA_640X480:
            return 640U;
        case OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464:
            return 3280U;
        case OBC::PayloadResolutionPreset::PRESET_HD_1280X720:
        default:
            return 1280U;
    }
}

U32 heightForResolution_(OBC::PayloadResolutionPreset preset) {
    switch (preset.e) {
        case OBC::PayloadResolutionPreset::PRESET_VGA_640X480:
            return 480U;
        case OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464:
            return 2464U;
        case OBC::PayloadResolutionPreset::PRESET_HD_1280X720:
        default:
            return 720U;
    }
}

class StubPiCameraDriver final : public OBC::IPiCameraDriver {
  public:
    StubPiCameraDriver()
        : m_registers{{0x3500U, 0x00U}, {0x3501U, 0x27U}, {0x3502U, 0x10U}, {0x350AU, 0x00U}, {0x350BU, 0x10U}} {}

    const char* getName() const override { return "stub"; }

    bool isAvailable() const override { return true; }

    OBC::PayloadCapabilities getCapabilities() const override {
        OBC::PayloadCapabilities capabilities = {};
        capabilities.backendName = "stub";
        capabilities.cameraModel = "OV5647-stub";
        capabilities.supportedMask = OBC::PayloadFieldMask::RESOLUTION | OBC::PayloadFieldMask::JPEG_QUALITY |
                                     OBC::PayloadFieldMask::HFLIP | OBC::PayloadFieldMask::VFLIP |
                                     OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100 |
                                     OBC::PayloadFieldMask::AWB_MODE | OBC::PayloadFieldMask::METERING_MODE |
                                     OBC::PayloadFieldMask::EV_COMP_X100;
        capabilities.offOnlyMask = OBC::PayloadFieldMask::RESOLUTION | OBC::PayloadFieldMask::JPEG_QUALITY |
                                   OBC::PayloadFieldMask::HFLIP | OBC::PayloadFieldMask::VFLIP;
        capabilities.autoMutableMask = OBC::PayloadFieldMask::AWB_MODE | OBC::PayloadFieldMask::METERING_MODE |
                                       OBC::PayloadFieldMask::EV_COMP_X100;
        capabilities.deterministicMutableMask =
            OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100;
        capabilities.rawRegisterSupported = true;
        return capabilities;
    }

    Fw::CmdResponse prepare(const OBC::PayloadReadyKind readyKind,
                            const OBC::PayloadCameraSettings& settings,
                            U32 initTimeoutMs,
                            const std::atomic<bool>& cancelRequested,
                            U32& detailCode) override {
        static_cast<void>(readyKind);
        static_cast<void>(settings);
        static_cast<void>(initTimeoutMs);
        if (cancelRequested.load()) {
            detailCode = 1U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse captureStill(const OBC::PayloadCaptureRequest& request,
                                 const OBC::PayloadCameraSettings& settings,
                                 U32 captureTimeoutMs,
                                 const std::atomic<bool>& cancelRequested,
                                 OBC::PayloadCaptureMetadata& metadata,
                                 U32& detailCode) override {
        static_cast<void>(captureTimeoutMs);
        if (cancelRequested.load()) {
            detailCode = 2U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        const U32 width = widthForResolution_(settings.resolution);
        const U32 height = heightForResolution_(settings.resolution);
        const std::size_t rawBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 2U;
        std::vector<U8> raw(rawBytes, 0U);
        for (std::size_t index = 0; index < raw.size(); ++index) {
            raw[index] = static_cast<U8>((index + request.captureIndex) & 0xFFU);
        }

        std::ofstream rawStream(request.rawOutputPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!rawStream.good()) {
            detailCode = static_cast<U32>(errno == 0 ? 3 : errno);
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        rawStream.write(reinterpret_cast<const char*>(raw.data()), static_cast<std::streamsize>(raw.size()));
        rawStream.flush();
        if (!rawStream.good()) {
            detailCode = 4U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        std::ofstream previewStream(request.previewOutputPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!previewStream.good()) {
            detailCode = static_cast<U32>(errno == 0 ? 5 : errno);
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        previewStream.write(reinterpret_cast<const char*>(STUB_JPEG_BYTES), sizeof(STUB_JPEG_BYTES));
        previewStream.flush();
        if (!previewStream.good()) {
            detailCode = 6U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        metadata = request.metadata;
        metadata.backendName = "stub";
        metadata.cameraModel = "OV5647-stub";
        metadata.captureIndex = request.captureIndex;
        metadata.captureTimeSec = request.captureId;
        metadata.captureTimeUsec = request.captureIndex;
        metadata.pixelFormat = OBC::PayloadPixelFormat::PIXEL_YUYV;
        metadata.imageWidth = width;
        metadata.imageHeight = height;
        metadata.rawRelativePath = request.rawRelativePath;
        metadata.previewRelativePath = request.previewRelativePath;
        metadata.rawBytes = static_cast<U32>(raw.size());
        metadata.previewJpegBytes = static_cast<U32>(sizeof(STUB_JPEG_BYTES));
        metadata.appliedSettings = settings;
        metadata.capturePolicy = settings.capturePolicy;
        metadata.actualExposureUsec =
            settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO ? 23123U : settings.exposureUsec;
        metadata.actualGainX100 =
            settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO ? 187U : settings.gainX100;
        metadata.actualAwbValid = settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO;
        metadata.actualAwbColorTemperatureK = metadata.actualAwbValid ? 5032U : 0U;
        metadata.actualAwbRedGainX1000 = metadata.actualAwbValid ? 1710U : 0U;
        metadata.actualAwbBlueGainX1000 = metadata.actualAwbValid ? 1385U : 0U;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse shutdown(U32& detailCode) override {
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse readSensorRegister(U32 address, U32 timeoutMs, U32& value, U32& detailCode) override {
        static_cast<void>(timeoutMs);
        auto it = this->m_registers.find(address);
        if (it == this->m_registers.end()) {
            detailCode = 7U;
            return Fw::CmdResponse::VALIDATION_ERROR;
        }
        value = it->second;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse writeSensorRegister(U32 address,
                                        U32 value,
                                        bool verifyReadback,
                                        U32 timeoutMs,
                                        U32& readbackValue,
                                        U32& detailCode) override {
        static_cast<void>(timeoutMs);
        this->m_registers[address] = value & 0xFFU;
        readbackValue = this->m_registers[address];
        detailCode = 0U;
        if (verifyReadback && readbackValue != (value & 0xFFU)) {
            detailCode = 8U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        return Fw::CmdResponse::OK;
    }

    void abort() override {}

  private:
    std::map<U32, U32> m_registers;
};

}  // namespace

std::unique_ptr<OBC::IPiCameraDriver> makePayloadHelperBackendDriver() {
    return std::unique_ptr<OBC::IPiCameraDriver>(new StubPiCameraDriver());
}

}  // namespace OBC

#endif

#include <atomic>
#include <thread>

#include <gtest/gtest.h>

#include "OBC/Components/PayloadOpsController/PiCameraManager.hpp"
#include "OBC/Components/test/TestSupport.hpp"
#include "simulators/eps/EpsTypes.hpp"

namespace {

class FakeDriver final : public OBC::IPiCameraDriver {
  public:
    const char* getName() const override { return "fake"; }
    bool isAvailable() const override { return true; }
    OBC::PayloadCapabilities getCapabilities() const override {
        OBC::PayloadCapabilities capabilities = {};
        capabilities.backendName = "fake";
        capabilities.cameraModel = "OV5647-fake";
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
        this->prepareCalls += 1U;
        while (this->blockPrepare && !cancelRequested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (cancelRequested.load()) {
            detailCode = 101U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (this->failPrepare) {
            detailCode = 102U;
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
        static_cast<void>(settings);
        static_cast<void>(captureTimeoutMs);
        this->captureCalls += 1U;
        while (this->blockCapture && !cancelRequested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (cancelRequested.load()) {
            detailCode = 201U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (this->failCapture) {
            detailCode = 202U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (!OBC::TestSupport::writeBinaryFile(request.rawOutputPath, {0x10, 0x20, 0x30, 0x40})) {
            detailCode = 203U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (!OBC::TestSupport::writeBinaryFile(request.previewOutputPath, {0xFF, 0xD8, 0xFF, 0xD9})) {
            detailCode = 204U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        metadata.backendName = "fake";
        metadata.cameraModel = "OV5647-fake";
        metadata.captureIndex = request.captureIndex;
        metadata.rawRelativePath = request.rawRelativePath;
        metadata.previewRelativePath = request.previewRelativePath;
        metadata.rawBytes = 4U;
        metadata.previewJpegBytes = 4U;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse shutdown(U32& detailCode) override {
        this->shutdownCalls += 1U;
        detailCode = this->shutdownDetail;
        return this->failShutdown ? Fw::CmdResponse::EXECUTION_ERROR : Fw::CmdResponse::OK;
    }

    Fw::CmdResponse readSensorRegister(U32 address, U32 timeoutMs, U32& value, U32& detailCode) override {
        static_cast<void>(timeoutMs);
        this->lastReadAddress = address;
        this->readRegisterCalls += 1U;
        value = this->lastRegisterValue;
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
        this->lastWriteAddress = address;
        this->lastRegisterValue = value;
        this->lastVerifyReadback = verifyReadback;
        this->writeRegisterCalls += 1U;
        readbackValue = value;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    void abort() override { this->abortCalls += 1U; }

    bool blockPrepare = false;
    bool blockCapture = false;
    bool failPrepare = false;
    bool failCapture = false;
    bool failShutdown = false;
    U32 prepareCalls = 0U;
    U32 captureCalls = 0U;
    U32 shutdownCalls = 0U;
    U32 abortCalls = 0U;
    U32 shutdownDetail = 0U;
    U32 readRegisterCalls = 0U;
    U32 writeRegisterCalls = 0U;
    U32 lastReadAddress = 0U;
    U32 lastWriteAddress = 0U;
    U32 lastRegisterValue = 0U;
    bool lastVerifyReadback = false;
};

class FakeEpsControl final : public OBC::IPayloadEpsControl {
  public:
    bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override {
        status = {};
        status.soc = 75.0F;
        return true;
    }

    Fw::CmdResponse setPayloadProxyPower(bool enabled, OBC::EPS::StatusData& status) override {
        status = {};
        status.soc = 75.0F;
        this->calls += 1U;
        this->lastEnabled = enabled;
        return Fw::CmdResponse::OK;
    }

    U32 calls = 0U;
    bool lastEnabled = false;
};

}  // namespace

TEST(PiCameraManager, AbortDuringPrepareCleansBackToOff) {
    FakeEpsControl eps;
    OBC::PiCameraManager manager;
    std::unique_ptr<FakeDriver> rawDriver(new FakeDriver());
    rawDriver->blockPrepare = true;
    FakeDriver* driver = rawDriver.get();
    OBC::PayloadRuntimeConfig config = {};
    config.powerSettleMs = 20U;

    ASSERT_TRUE(manager.configure(std::move(rawDriver), &eps, config));
    ASSERT_TRUE(manager.beginPrepare(OBC::PayloadReadyKind::READY_NON_RAW, {}));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_TRUE(manager.requestAbort());

    OBC::PayloadOperationResult result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_EQ(result.kind, OBC::PayloadOperationKind::PREPARE);
    ASSERT_EQ(result.resultCode, OBC::PayloadResultCode::PRESULT_ABORTED);
    ASSERT_FALSE(result.prepared);
    ASSERT_FALSE(result.logicalPowerEnabled);
    ASSERT_EQ(driver->abortCalls, 1U);
}

TEST(PiCameraManager, CaptureFailureTriggersCleanup) {
    FakeEpsControl eps;
    OBC::PiCameraManager manager;
    std::unique_ptr<FakeDriver> rawDriver(new FakeDriver());
    rawDriver->failCapture = true;
    FakeDriver* driver = rawDriver.get();
    OBC::PayloadRuntimeConfig config = {};
    config.powerSettleMs = 20U;

    ASSERT_TRUE(manager.configure(std::move(rawDriver), &eps, config));
    ASSERT_TRUE(manager.beginPrepare(OBC::PayloadReadyKind::READY_NON_RAW, {}));

    OBC::PayloadOperationResult result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(result.response, Fw::CmdResponse::OK);

    OBC::PayloadCaptureRequest request = {};
    request.captureId = 1U;
    request.captureIndex = 0x01U;
    request.rawRelativePath = "persistent-data/payload/camera/PIC01.bin";
    request.previewRelativePath = "persistent-data/payload/camera/PIC01.jpg";
    request.rawOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-fail.bin");
    request.previewOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-fail.jpg");
    ASSERT_TRUE(manager.beginCapture(request, {}));

    result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_EQ(result.kind, OBC::PayloadOperationKind::CAPTURE);
    ASSERT_EQ(result.resultCode, OBC::PayloadResultCode::PRESULT_CAPTURE_FAILED);
    ASSERT_FALSE(result.prepared);
    ASSERT_FALSE(result.logicalPowerEnabled);
    ASSERT_GE(driver->shutdownCalls, 1U);
}

TEST(PiCameraManager, CaptureFailurePreservesShutdownFailure) {
    FakeEpsControl eps;
    OBC::PiCameraManager manager;
    std::unique_ptr<FakeDriver> rawDriver(new FakeDriver());
    rawDriver->failCapture = true;
    rawDriver->failShutdown = true;
    rawDriver->shutdownDetail = 303U;
    FakeDriver* driver = rawDriver.get();
    OBC::PayloadRuntimeConfig config = {};
    config.powerSettleMs = 20U;

    ASSERT_TRUE(manager.configure(std::move(rawDriver), &eps, config));
    ASSERT_TRUE(manager.beginPrepare(OBC::PayloadReadyKind::READY_NON_RAW, {}));

    OBC::PayloadOperationResult result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(result.response, Fw::CmdResponse::OK);

    OBC::PayloadCaptureRequest request = {};
    request.captureId = 1U;
    request.captureIndex = 0x01U;
    request.rawRelativePath = "persistent-data/payload/camera/PIC01.bin";
    request.previewRelativePath = "persistent-data/payload/camera/PIC01.jpg";
    request.rawOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-fail-shutdown.bin");
    request.previewOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-fail-shutdown.jpg");
    ASSERT_TRUE(manager.beginCapture(request, {}));

    result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_EQ(result.kind, OBC::PayloadOperationKind::CAPTURE);
    ASSERT_EQ(result.response, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(result.resultCode, OBC::PayloadResultCode::PRESULT_SHUTDOWN_FAILED);
    ASSERT_EQ(result.detailCode, 303U);
    ASSERT_FALSE(result.prepared);
    ASSERT_FALSE(result.logicalPowerEnabled);
    ASSERT_GE(driver->shutdownCalls, 1U);
}

TEST(PiCameraManager, SameSessionRepeatedCaptureDoesNotReprepare) {
    FakeEpsControl eps;
    OBC::PiCameraManager manager;
    std::unique_ptr<FakeDriver> rawDriver(new FakeDriver());
    FakeDriver* driver = rawDriver.get();
    OBC::PayloadRuntimeConfig config = {};
    config.powerSettleMs = 20U;

    ASSERT_TRUE(manager.configure(std::move(rawDriver), &eps, config));
    OBC::PayloadCameraSettings preparedSettings = {};
    preparedSettings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    preparedSettings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    preparedSettings.resolution = OBC::PayloadResolutionPreset::PRESET_VGA_640X480;
    ASSERT_TRUE(manager.beginPrepare(OBC::PayloadReadyKind::READY_NON_RAW, preparedSettings));

    OBC::PayloadOperationResult result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(result.response, Fw::CmdResponse::OK);
    ASSERT_EQ(driver->prepareCalls, 1U);

    OBC::PayloadCaptureRequest firstRequest = {};
    firstRequest.captureId = 1U;
    firstRequest.captureIndex = 0x01U;
    firstRequest.rawRelativePath = "persistent-data/payload/camera/PIC01.bin";
    firstRequest.previewRelativePath = "persistent-data/payload/camera/PIC01.jpg";
    firstRequest.rawOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-repeat-1.bin");
    firstRequest.previewOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-repeat-1.jpg");
    OBC::PayloadCameraSettings autoSettings = preparedSettings;
    autoSettings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;
    ASSERT_TRUE(manager.beginCapture(firstRequest, autoSettings));

    result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(result.response, Fw::CmdResponse::OK);
    ASSERT_EQ(driver->prepareCalls, 1U);
    ASSERT_EQ(driver->captureCalls, 1U);

    OBC::PayloadCaptureRequest secondRequest = firstRequest;
    secondRequest.captureId = 2U;
    secondRequest.captureIndex = 0x02U;
    secondRequest.rawOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-repeat-2.bin");
    secondRequest.previewOutputPath = OBC::TestSupport::joinPath("/tmp", "payload-capture-repeat-2.jpg");
    OBC::PayloadCameraSettings deterministicSettings = preparedSettings;
    deterministicSettings.exposureUsec = 33000U;
    deterministicSettings.gainX100 = 450U;
    ASSERT_TRUE(manager.beginCapture(secondRequest, deterministicSettings));

    result = {};
    for (U32 index = 0; index < 100U && !manager.pollCompletion(result); ++index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(result.response, Fw::CmdResponse::OK);
    ASSERT_EQ(driver->prepareCalls, 1U);
    ASSERT_EQ(driver->captureCalls, 2U);
}

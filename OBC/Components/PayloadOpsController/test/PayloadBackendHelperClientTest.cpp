#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "gtest/gtest.h"

#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

namespace {

std::string makeTempDir() {
    char templ[] = "/tmp/payload-helper-client.XXXXXX";
    char* path = ::mkdtemp(templ);
    EXPECT_NE(nullptr, path);
    return path == nullptr ? std::string() : std::string(path);
}

bool fileExists(const std::string& path) {
    struct stat info = {};
    return ::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

std::string currentExecutablePath() {
#ifdef __APPLE__
    uint32_t size = 0U;
    EXPECT_EQ(-1, _NSGetExecutablePath(nullptr, &size));
    std::string path(size + 1U, '\0');
    EXPECT_EQ(0, _NSGetExecutablePath(&path[0], &size));
    path.resize(strnlen(path.c_str(), path.size()));
    return path;
#else
    char buffer[1024] = {};
    const ssize_t length = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    EXPECT_GT(length, 0);
    if (length <= 0) {
        return std::string();
    }
    buffer[length] = '\0';
    return std::string(buffer);
#endif
}

std::string parentDirectoryOf(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string(".") : path.substr(0U, slash);
}

long fileSize(const std::string& path) {
    struct stat info = {};
    if (::stat(path.c_str(), &info) != 0) {
        return -1;
    }
    return static_cast<long>(info.st_size);
}

void writeExecutableFile(const std::string& path, const std::string& contents) {
    std::ofstream stream(path.c_str(), std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(stream.good());
    stream << contents;
    stream.close();
    ASSERT_EQ(0, ::chmod(path.c_str(), 0755));
}

}  // namespace

TEST(PayloadBackendHelperClientTest, HelperRoundTripUsesStubBackendOnHosted) {
    const std::string helperPath = parentDirectoryOf(currentExecutablePath()) + "/payload_camera_backend_helper";
    ASSERT_EQ(0, ::setenv("OBC_PAYLOAD_HELPER_BIN", helperPath.c_str(), 1));
    std::unique_ptr<OBC::IPiCameraDriver> driver = OBC::makeDefaultPiCameraDriver();
    ASSERT_NE(nullptr, driver);
    ASSERT_TRUE(driver->isAvailable());

    const OBC::PayloadCapabilities capabilities = driver->getCapabilities();
    EXPECT_EQ("stub", capabilities.backendName);
    EXPECT_EQ("OV5647-stub", capabilities.cameraModel);
    EXPECT_TRUE(capabilities.rawRegisterSupported);
    EXPECT_FALSE(capabilities.realSensorPath);

    OBC::PayloadCameraSettings settings = {};
    settings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    settings.resolution = OBC::PayloadResolutionPreset::PRESET_HD_1280X720;
    settings.jpegQuality = 90U;
    settings.exposureUsec = 12000U;
    settings.gainX100 = 150U;

    std::atomic<bool> cancelRequested(false);
    U32 detailCode = 0U;
    ASSERT_EQ(Fw::CmdResponse::OK,
              driver->prepare(OBC::PayloadReadyKind::READY_NON_RAW, settings, 1000U, cancelRequested, detailCode));
    EXPECT_EQ(0U, detailCode);

    const std::string tempDir = makeTempDir();
    ASSERT_FALSE(tempDir.empty());
    const std::string rawOutputPath = tempDir + "/capture.bin";
    const std::string previewOutputPath = tempDir + "/capture.jpg";

    OBC::PayloadCaptureRequest request = {};
    request.tag = "helper-test";
    request.captureId = 1U;
    request.captureIndex = 0x2AU;
    request.bootCount = 1U;
    request.rawOutputPath = rawOutputPath;
    request.previewOutputPath = previewOutputPath;
    request.rawRelativePath = "persistent-data/payload/camera/PIC2A.bin";
    request.previewRelativePath = "persistent-data/payload/camera/PIC2A.jpg";
    request.requestedMask = OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100;
    request.appliedMask = request.requestedMask;

    OBC::PayloadCaptureMetadata metadata = {};
    ASSERT_EQ(Fw::CmdResponse::OK, driver->captureStill(request, settings, 1000U, cancelRequested, metadata, detailCode));
    EXPECT_EQ(0U, detailCode);
    EXPECT_EQ(OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC, metadata.capturePolicy);
    EXPECT_EQ("stub", metadata.backendName);
    EXPECT_EQ(request.captureIndex, metadata.captureIndex);
    EXPECT_EQ(request.rawRelativePath, metadata.rawRelativePath);
    EXPECT_EQ(request.previewRelativePath, metadata.previewRelativePath);
    EXPECT_TRUE(fileExists(rawOutputPath));
    EXPECT_GT(fileSize(rawOutputPath), 0);
    EXPECT_TRUE(fileExists(previewOutputPath));
    EXPECT_GT(fileSize(previewOutputPath), 0);
    EXPECT_GT(metadata.rawBytes, 0U);
    EXPECT_GT(metadata.previewJpegBytes, 0U);
    EXPECT_EQ(12000U, metadata.actualExposureUsec);
    EXPECT_EQ(150U, metadata.actualGainX100);
    EXPECT_FALSE(metadata.actualAwbValid);

    OBC::PayloadCameraSettings autoSettings = {};
    autoSettings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    autoSettings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;
    autoSettings.resolution = OBC::PayloadResolutionPreset::PRESET_VGA_640X480;
    autoSettings.jpegQuality = 75U;
    autoSettings.awbMode = OBC::PayloadAwbMode::AWB_AUTO;
    autoSettings.meteringMode = OBC::PayloadMeteringMode::METER_CENTRE;
    ASSERT_EQ(Fw::CmdResponse::OK,
              driver->prepare(OBC::PayloadReadyKind::READY_NON_RAW, autoSettings, 1000U, cancelRequested, detailCode));
    EXPECT_EQ(0U, detailCode);

    const std::string autoRawOutputPath = tempDir + "/capture-auto.bin";
    const std::string autoPreviewOutputPath = tempDir + "/capture-auto.jpg";
    OBC::PayloadCaptureRequest autoRequest = {};
    autoRequest.tag = "helper-auto-test";
    autoRequest.captureId = 2U;
    autoRequest.captureIndex = 0x2BU;
    autoRequest.bootCount = 1U;
    autoRequest.rawOutputPath = autoRawOutputPath;
    autoRequest.previewOutputPath = autoPreviewOutputPath;
    autoRequest.rawRelativePath = "persistent-data/payload/camera/PIC2B.bin";
    autoRequest.previewRelativePath = "persistent-data/payload/camera/PIC2B.jpg";
    OBC::PayloadCaptureMetadata autoMetadata = {};
    ASSERT_EQ(
        Fw::CmdResponse::OK, driver->captureStill(autoRequest, autoSettings, 1000U, cancelRequested, autoMetadata, detailCode));
    EXPECT_EQ(0U, detailCode);
    EXPECT_EQ(OBC::PayloadCapturePolicy::CAPTURE_AUTO, autoMetadata.capturePolicy);
    EXPECT_EQ(autoRequest.captureIndex, autoMetadata.captureIndex);
    EXPECT_EQ(autoRequest.rawRelativePath, autoMetadata.rawRelativePath);
    EXPECT_EQ(autoRequest.previewRelativePath, autoMetadata.previewRelativePath);
    EXPECT_TRUE(fileExists(autoRawOutputPath));
    EXPECT_GT(fileSize(autoRawOutputPath), 0);
    EXPECT_TRUE(fileExists(autoPreviewOutputPath));
    EXPECT_GT(fileSize(autoPreviewOutputPath), 0);
    EXPECT_GT(autoMetadata.actualExposureUsec, 0U);
    EXPECT_GT(autoMetadata.actualGainX100, 0U);
    EXPECT_TRUE(autoMetadata.actualAwbValid);
    EXPECT_GT(autoMetadata.actualAwbColorTemperatureK, 0U);
    EXPECT_GT(autoMetadata.actualAwbRedGainX1000, 0U);
    EXPECT_GT(autoMetadata.actualAwbBlueGainX1000, 0U);

    U32 value = 0U;
    ASSERT_EQ(Fw::CmdResponse::OK, driver->readSensorRegister(0x350BU, 100U, value, detailCode));
    EXPECT_EQ(0x10U, value);
    U32 readback = 0U;
    ASSERT_EQ(Fw::CmdResponse::OK, driver->writeSensorRegister(0x350BU, 0x22U, true, 100U, readback, detailCode));
    EXPECT_EQ(0x22U, readback);

    ASSERT_EQ(Fw::CmdResponse::OK, driver->shutdown(detailCode));
    EXPECT_EQ(0U, detailCode);
}

TEST(PayloadBackendHelperClientTest, AbortPreemptsBlockedHelperTransaction) {
    const std::string tempDir = makeTempDir();
    ASSERT_FALSE(tempDir.empty());
    const std::string helperPath = tempDir + "/payload_camera_backend_helper";
    writeExecutableFile(helperPath, "#!/bin/sh\nsleep 60\n");

    ASSERT_EQ(0, ::setenv("OBC_PAYLOAD_HELPER_BIN", helperPath.c_str(), 1));
    std::unique_ptr<OBC::IPiCameraDriver> driver = OBC::makeDefaultPiCameraDriver();
    ASSERT_NE(nullptr, driver);
    ASSERT_TRUE(driver->isAvailable());

    OBC::PayloadCameraSettings settings = {};
    settings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;

    std::atomic<bool> cancelRequested(false);
    std::atomic<int> responseValue(static_cast<int>(Fw::CmdResponse::OK));
    std::atomic<U32> detailCode(0U);

    std::thread worker([&]() {
        U32 localDetail = 0U;
        const Fw::CmdResponse response =
            driver->prepare(OBC::PayloadReadyKind::READY_NON_RAW, settings, 5000U, cancelRequested, localDetail);
        responseValue.store(static_cast<int>(response));
        detailCode.store(localDetail);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const auto abortStart = std::chrono::steady_clock::now();
    driver->abort();
    worker.join();
    const auto abortElapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - abortStart).count();

    EXPECT_LT(abortElapsedMs, 1500);
    EXPECT_EQ(static_cast<int>(Fw::CmdResponse::EXECUTION_ERROR), responseValue.load());
    EXPECT_NE(0U, detailCode.load());
}

TEST(PayloadBackendHelperClientTest, HelperExitDoesNotTerminateWriterOnBrokenPipe) {
    const std::string tempDir = makeTempDir();
    ASSERT_FALSE(tempDir.empty());
    const std::string helperPath = tempDir + "/payload_camera_backend_helper";
    writeExecutableFile(helperPath, "#!/bin/sh\nexit 0\n");

    ASSERT_EQ(0, ::setenv("OBC_PAYLOAD_HELPER_BIN", helperPath.c_str(), 1));
    std::unique_ptr<OBC::IPiCameraDriver> driver = OBC::makeDefaultPiCameraDriver();
    ASSERT_NE(nullptr, driver);
    ASSERT_TRUE(driver->isAvailable());

    OBC::PayloadCameraSettings settings = {};
    settings.readyKind = OBC::PayloadReadyKind::READY_NON_RAW;
    settings.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_AUTO;

    std::atomic<bool> cancelRequested(false);
    U32 detailCode = 0U;
    const Fw::CmdResponse response =
        driver->prepare(OBC::PayloadReadyKind::READY_NON_RAW, settings, 1000U, cancelRequested, detailCode);

    EXPECT_EQ(Fw::CmdResponse::EXECUTION_ERROR, response);
    EXPECT_NE(0U, detailCode);
}

#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

#ifdef OBC_HAS_LIBCAMERA

#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/libcamera.h>
#include "toojpeg.h"

namespace OBC {

namespace {

constexpr U32 DETAIL_PREPARE_CANCELLED = 1U;
constexpr U32 DETAIL_CAMERA_MANAGER_START_FAILED = 2U;
constexpr U32 DETAIL_NO_CAMERAS = 3U;
constexpr U32 DETAIL_CAMERA_LOOKUP_FAILED = 4U;
constexpr U32 DETAIL_CAMERA_ACQUIRE_FAILED = 5U;
constexpr U32 DETAIL_CONFIG_GENERATE_FAILED = 6U;
constexpr U32 DETAIL_CONFIG_INVALID = 7U;
constexpr U32 DETAIL_CAMERA_CONFIGURE_FAILED = 8U;
constexpr U32 DETAIL_ALLOCATOR_FAILED = 9U;
constexpr U32 DETAIL_PREPARE_NOT_READY = 10U;
constexpr U32 DETAIL_CAPTURE_CANCELLED = 11U;
constexpr U32 DETAIL_CAPTURE_NO_BUFFERS = 12U;
constexpr U32 DETAIL_CAPTURE_REQUEST_ALLOC_FAILED = 13U;
constexpr U32 DETAIL_CAPTURE_ADD_BUFFER_FAILED = 14U;
constexpr U32 DETAIL_CAPTURE_CAMERA_START_FAILED = 15U;
constexpr U32 DETAIL_CAPTURE_QUEUE_FAILED = 16U;
constexpr U32 DETAIL_CAPTURE_CANCEL_WAIT = 17U;
constexpr U32 DETAIL_CAPTURE_TIMEOUT = 18U;
constexpr U32 DETAIL_OUTPUT_OPEN_FAILED_BASE = 19U;
constexpr U32 DETAIL_OUTPUT_MMAP_FAILED_BASE = 20U;
constexpr U32 DETAIL_OUTPUT_WRITE_FAILED = 21U;
constexpr U32 DETAIL_CAPTURE_REQUEST_FAILED = 22U;
constexpr U32 DETAIL_CONFIG_STREAM_COUNT = 23U;
constexpr U32 DETAIL_CONFIG_UNSUPPORTED_RAW_PIXEL_FORMAT = 24U;
constexpr U32 DETAIL_PREVIEW_JPEG_ENCODE_FAILED = 25U;
constexpr U32 DETAIL_CAPTURE_WARMUP_TIMEOUT = 26U;
constexpr U32 DETAIL_PREPARE_TIMEOUT_BASE = 100U;
constexpr U32 CAPTURE_WARMUP_DISCARD_FRAMES = 6U;
// Even with a persistent started camera, the request queue can drain between
// operator captures. Re-queued still captures need the same conservative drain
// window as initial warm-up to keep deterministic actual exposure stable.
constexpr U32 CAPTURE_CONVERGENCE_DISCARD_FRAMES = 6U;

struct MappedPlaneView {
    void* address = nullptr;
    std::size_t length = 0U;
    std::size_t bytesUsed = 0U;
};

thread_local std::FILE* s_previewJpegOutput = nullptr;

void previewJpegWriteByte_(unsigned char oneByte) {
    if (s_previewJpegOutput != nullptr) {
        std::fputc(static_cast<int>(oneByte), s_previewJpegOutput);
    }
}

bool isSupportedRawPixelFormat_(const libcamera::PixelFormat& pixelFormat) {
    return pixelFormat == libcamera::formats::YUYV || pixelFormat == libcamera::formats::UYVY ||
           pixelFormat == libcamera::formats::NV12 || pixelFormat == libcamera::formats::RGB888 ||
           pixelFormat == libcamera::formats::BGR888;
}

bool chooseRawPixelFormat_(const libcamera::StreamFormats& formats, libcamera::PixelFormat& chosen) {
    const std::array<libcamera::PixelFormat, 5> preferred = {
        libcamera::formats::YUYV,
        libcamera::formats::UYVY,
        libcamera::formats::NV12,
        libcamera::formats::RGB888,
        libcamera::formats::BGR888,
    };
    const auto available = formats.pixelformats();
    for (const auto& candidate : preferred) {
        if (std::find(available.begin(), available.end(), candidate) != available.end()) {
            chosen = candidate;
            return true;
        }
    }
    return false;
}

void resolutionForPreset_(OBC::PayloadResolutionPreset preset, U32& width, U32& height) {
    switch (preset.e) {
        case OBC::PayloadResolutionPreset::PRESET_VGA_640X480:
            width = 640U;
            height = 480U;
            return;
        case OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464:
            width = 3280U;
            height = 2464U;
            return;
        case OBC::PayloadResolutionPreset::PRESET_HD_1280X720:
        default:
            width = 1280U;
            height = 720U;
            return;
    }
}

OBC::PayloadPixelFormat payloadPixelFormatFor_(const libcamera::PixelFormat& pixelFormat) {
    if (pixelFormat == libcamera::formats::YUYV) {
        return OBC::PayloadPixelFormat::PIXEL_YUYV;
    }
    if (pixelFormat == libcamera::formats::UYVY) {
        return OBC::PayloadPixelFormat::PIXEL_UYVY;
    }
    if (pixelFormat == libcamera::formats::NV12) {
        return OBC::PayloadPixelFormat::PIXEL_NV12;
    }
    if (pixelFormat == libcamera::formats::RGB888) {
        return OBC::PayloadPixelFormat::PIXEL_RGB888;
    }
    if (pixelFormat == libcamera::formats::BGR888) {
        return OBC::PayloadPixelFormat::PIXEL_BGR888;
    }
    return OBC::PayloadPixelFormat::PIXEL_UNKNOWN;
}

bool prepareDeadlineExceeded_(const std::chrono::steady_clock::time_point deadline,
                              const std::atomic<bool>& cancelRequested,
                              U32 timeoutDetailCode,
                              U32& detailCode) {
    if (cancelRequested.load()) {
        detailCode = DETAIL_PREPARE_CANCELLED;
        return true;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
        detailCode = timeoutDetailCode;
        return true;
    }
    return false;
}

inline U8 clampRgb_(int value) {
    return static_cast<U8>(std::clamp(value, 0, 255));
}

void yuvToRgb_(int y, int u, int v, U8& r, U8& g, U8& b) {
    const int c = y - 16;
    const int d = u - 128;
    const int e = v - 128;
    r = clampRgb_((298 * c + 409 * e + 128) >> 8);
    g = clampRgb_((298 * c - 100 * d - 208 * e + 128) >> 8);
    b = clampRgb_((298 * c + 516 * d + 128) >> 8);
}

class LibcameraPiCameraDriver final : public OBC::IPiCameraDriver {
  public:
    LibcameraPiCameraDriver()
        : m_cameraManager(new libcamera::CameraManager()),
          m_allocator(),
          m_camera(),
          m_rawStream(nullptr),
          m_rawPixelFormat(),
          m_rawWidth(0U),
          m_rawHeight(0U),
          m_streamActive(false) {}

    ~LibcameraPiCameraDriver() override {
        U32 ignored = 0U;
        static_cast<void>(this->shutdown(ignored));
    }

    const char* getName() const override { return "libcamera"; }

    bool isAvailable() const override {
        return true;
    }

    OBC::PayloadCapabilities getCapabilities() const override {
        OBC::PayloadCapabilities capabilities = {};
        capabilities.backendName = "libcamera";
        capabilities.cameraModel = "OV5647";
        capabilities.supportedMask = OBC::PayloadFieldMask::RESOLUTION | OBC::PayloadFieldMask::JPEG_QUALITY |
                                     OBC::PayloadFieldMask::HFLIP | OBC::PayloadFieldMask::VFLIP |
                                     OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100;
        capabilities.offOnlyMask = OBC::PayloadFieldMask::RESOLUTION | OBC::PayloadFieldMask::JPEG_QUALITY |
                                   OBC::PayloadFieldMask::HFLIP | OBC::PayloadFieldMask::VFLIP;
        capabilities.autoMutableMask = 0U;
        capabilities.deterministicMutableMask =
            OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100;
        capabilities.realSensorPath = true;
        return capabilities;
    }

    Fw::CmdResponse prepare(const OBC::PayloadReadyKind readyKind,
                            const OBC::PayloadCameraSettings& settings,
                            U32 initTimeoutMs,
                            const std::atomic<bool>& cancelRequested,
                            U32& detailCode) override {
        static_cast<void>(readyKind);
        detailCode = 0U;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(initTimeoutMs);

        if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 1U, detailCode)) {
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        if (!this->m_camera) {
            if (this->m_cameraManager->start() < 0) {
                detailCode = DETAIL_CAMERA_MANAGER_START_FAILED;
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 2U, detailCode)) {
                this->m_cameraManager->stop();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }

            auto cameras = this->m_cameraManager->cameras();
            if (cameras.empty()) {
                detailCode = DETAIL_NO_CAMERAS;
                this->m_cameraManager->stop();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 3U, detailCode)) {
                this->m_cameraManager->stop();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }

            this->m_camera = this->m_cameraManager->get(cameras[0]->id());
            if (!this->m_camera) {
                detailCode = DETAIL_CAMERA_LOOKUP_FAILED;
                this->m_cameraManager->stop();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 4U, detailCode)) {
                this->releaseCamera_();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (this->m_camera->acquire() < 0) {
                detailCode = DETAIL_CAMERA_ACQUIRE_FAILED;
                this->m_camera.reset();
                this->m_cameraManager->stop();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 5U, detailCode)) {
                this->releaseCamera_();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
        }

        std::unique_ptr<libcamera::CameraConfiguration> config =
            this->m_camera->generateConfiguration({libcamera::StreamRole::StillCapture});
        if (!config || config->size() != 1U) {
            detailCode = DETAIL_CONFIG_GENERATE_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 6U, detailCode)) {
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        libcamera::StreamConfiguration& rawConfig = config->at(0);
        U32 desiredWidth = 0U;
        U32 desiredHeight = 0U;
        resolutionForPreset_(settings.resolution, desiredWidth, desiredHeight);
        rawConfig.size.width = desiredWidth;
        rawConfig.size.height = desiredHeight;
        libcamera::PixelFormat chosenRawFormat;
        if (!chooseRawPixelFormat_(rawConfig.formats(), chosenRawFormat)) {
            detailCode = DETAIL_CONFIG_UNSUPPORTED_RAW_PIXEL_FORMAT;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        rawConfig.pixelFormat = chosenRawFormat;
        if (config->validate() == libcamera::CameraConfiguration::Invalid) {
            detailCode = DETAIL_CONFIG_INVALID;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (!isSupportedRawPixelFormat_(rawConfig.pixelFormat)) {
            detailCode = DETAIL_CONFIG_UNSUPPORTED_RAW_PIXEL_FORMAT;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 7U, detailCode)) {
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (this->m_allocator && this->m_rawStream != nullptr) {
            this->m_allocator->free(this->m_rawStream);
        }
        this->m_allocator.reset();
        this->m_rawStream = nullptr;
        if (this->m_camera->configure(config.get()) < 0) {
            detailCode = DETAIL_CAMERA_CONFIGURE_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 8U, detailCode)) {
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        this->m_rawStream = rawConfig.stream();
        this->m_rawPixelFormat = rawConfig.pixelFormat;
        this->m_rawWidth = static_cast<U32>(rawConfig.size.width);
        this->m_rawHeight = static_cast<U32>(rawConfig.size.height);
        this->m_allocator.reset(new libcamera::FrameBufferAllocator(this->m_camera));
        if (this->m_allocator->allocate(this->m_rawStream) < 0) {
            detailCode = DETAIL_ALLOCATOR_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (prepareDeadlineExceeded_(deadline, cancelRequested, DETAIL_PREPARE_TIMEOUT_BASE + 9U, detailCode)) {
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        struct CompletionState {
            std::mutex mutex;
            std::condition_variable cv;
            U32 completedCount = 0U;
            bool lastRequestComplete = false;
        };

        struct CompletionSlot {
            explicit CompletionSlot(const std::shared_ptr<CompletionState>& sharedState) : state(sharedState) {}

            void onRequestCompleted(libcamera::Request* completedRequest) {
                std::lock_guard<std::mutex> lock(this->state->mutex);
                this->state->completedCount += 1U;
                this->state->lastRequestComplete =
                    completedRequest->status() == libcamera::Request::RequestComplete;
                this->state->cv.notify_all();
            }

            std::shared_ptr<CompletionState> state;
        };

        const auto waitForNextCompletedFrame = [&](CompletionSlot& completionSlot,
                                                   std::shared_ptr<CompletionState>& completionState,
                                                   U32& observedCount) -> bool {
            std::unique_lock<std::mutex> lock(completionState->mutex);
            while (completionState->completedCount == observedCount && !cancelRequested.load()) {
                const auto now = std::chrono::steady_clock::now();
                if (now >= deadline) {
                    break;
                }
                const auto slice = std::min(std::chrono::milliseconds(50),
                                            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now));
                completionState->cv.wait_for(lock,
                                             slice,
                                             [&]() { return completionState->completedCount != observedCount; });
            }
            if (cancelRequested.load() && completionState->completedCount == observedCount) {
                detailCode = DETAIL_PREPARE_CANCELLED;
                return false;
            }
            if (completionState->completedCount == observedCount) {
                detailCode = DETAIL_CAPTURE_WARMUP_TIMEOUT;
                return false;
            }
            observedCount = completionState->completedCount;
            if (!completionState->lastRequestComplete) {
                detailCode = DETAIL_CAPTURE_REQUEST_FAILED;
                return false;
            }
            return true;
        };

        const auto& rawBuffers = this->m_allocator->buffers(this->m_rawStream);
        if (rawBuffers.empty()) {
            detailCode = DETAIL_CAPTURE_NO_BUFFERS;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        std::unique_ptr<libcamera::Request> requestHandle = this->m_camera->createRequest();
        if (!requestHandle) {
            detailCode = DETAIL_CAPTURE_REQUEST_ALLOC_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        libcamera::FrameBuffer* rawBuffer = rawBuffers[0].get();
        if (requestHandle->addBuffer(this->m_rawStream, rawBuffer) < 0) {
            detailCode = DETAIL_CAPTURE_ADD_BUFFER_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        this->applyCaptureControls_(*requestHandle, settings);
        auto completionState = std::make_shared<CompletionState>();
        CompletionSlot completionSlot(completionState);
        this->m_camera->requestCompleted.connect(&completionSlot, &CompletionSlot::onRequestCompleted);
        if (this->m_camera->start() < 0) {
            this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);
            detailCode = DETAIL_CAPTURE_CAMERA_START_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        this->m_streamActive = true;

        const auto stopAndDisconnect = [&]() {
            if (this->m_streamActive) {
                this->m_camera->stop();
                this->m_streamActive = false;
            }
            this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);
        };

        if (this->m_camera->queueRequest(requestHandle.get()) < 0) {
            stopAndDisconnect();
            detailCode = DETAIL_CAPTURE_QUEUE_FAILED;
            this->releaseCamera_();
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        U32 observedCompletedFrames = 0U;
        while (observedCompletedFrames < CAPTURE_WARMUP_DISCARD_FRAMES) {
            if (!waitForNextCompletedFrame(completionSlot, completionState, observedCompletedFrames)) {
                stopAndDisconnect();
                this->releaseCamera_();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (observedCompletedFrames >= CAPTURE_WARMUP_DISCARD_FRAMES) {
                break;
            }
            requestHandle->reuse(libcamera::Request::ReuseBuffers);
            this->applyCaptureControls_(*requestHandle, settings);
            if (this->m_camera->queueRequest(requestHandle.get()) < 0) {
                stopAndDisconnect();
                detailCode = DETAIL_CAPTURE_QUEUE_FAILED;
                this->releaseCamera_();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
        }
        this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);

        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse captureStill(const OBC::PayloadCaptureRequest& request,
                                 const OBC::PayloadCameraSettings& settings,
                                 U32 captureTimeoutMs,
                                 const std::atomic<bool>& cancelRequested,
                                 OBC::PayloadCaptureMetadata& metadata,
                                 U32& detailCode) override {
        detailCode = 0U;
        if (!this->m_camera || this->m_rawStream == nullptr || !this->m_allocator) {
            detailCode = DETAIL_PREPARE_NOT_READY;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (cancelRequested.load()) {
            detailCode = DETAIL_CAPTURE_CANCELLED;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        const auto& rawBuffers = this->m_allocator->buffers(this->m_rawStream);
        if (rawBuffers.empty()) {
            detailCode = DETAIL_CAPTURE_NO_BUFFERS;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        std::unique_ptr<libcamera::Request> requestHandle = this->m_camera->createRequest();
        if (!requestHandle) {
            detailCode = DETAIL_CAPTURE_REQUEST_ALLOC_FAILED;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        libcamera::FrameBuffer* rawBuffer = rawBuffers[0].get();
        if (requestHandle->addBuffer(this->m_rawStream, rawBuffer) < 0) {
            detailCode = DETAIL_CAPTURE_ADD_BUFFER_FAILED;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        this->applyCaptureControls_(*requestHandle, settings);

        struct CompletionState {
            std::mutex mutex;
            std::condition_variable cv;
            U32 completedCount = 0U;
            bool lastRequestComplete = false;
        };

        struct CompletionSlot {
            explicit CompletionSlot(const std::shared_ptr<CompletionState>& sharedState) : state(sharedState) {}

            void onRequestCompleted(libcamera::Request* completedRequest) {
                std::lock_guard<std::mutex> lock(this->state->mutex);
                this->state->completedCount += 1U;
                this->state->lastRequestComplete =
                    completedRequest->status() == libcamera::Request::RequestComplete;
                this->state->cv.notify_all();
            }

            std::shared_ptr<CompletionState> state;
        };

        auto completionState = std::make_shared<CompletionState>();
        CompletionSlot completionSlot(completionState);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(captureTimeoutMs);
        bool requestQueued = false;

        const auto waitForNextCompletedFrame = [&](U32& observedCount) -> bool {
            std::unique_lock<std::mutex> lock(completionState->mutex);
            while (completionState->completedCount == observedCount && !cancelRequested.load()) {
                const auto now = std::chrono::steady_clock::now();
                if (now >= deadline) {
                    break;
                }
                const auto slice = std::min(std::chrono::milliseconds(50),
                                            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now));
                completionState->cv.wait_for(lock,
                                             slice,
                                             [&]() { return completionState->completedCount != observedCount; });
            }
            if (cancelRequested.load() && completionState->completedCount == observedCount) {
                detailCode = DETAIL_CAPTURE_CANCEL_WAIT;
                return false;
            }
            if (completionState->completedCount == observedCount) {
                detailCode = DETAIL_CAPTURE_WARMUP_TIMEOUT;
                return false;
            }
            observedCount = completionState->completedCount;
            if (!completionState->lastRequestComplete) {
                detailCode = DETAIL_CAPTURE_REQUEST_FAILED;
                return false;
            }
            return true;
        };

        const auto stopActiveCaptureAndDisconnect = [&]() {
            if (requestQueued && this->m_streamActive) {
                this->m_camera->stop();
                this->m_streamActive = false;
            }
            this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);
        };

        this->m_camera->requestCompleted.connect(&completionSlot, &CompletionSlot::onRequestCompleted);
        if (this->m_camera->queueRequest(requestHandle.get()) < 0) {
            this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);
            detailCode = DETAIL_CAPTURE_QUEUE_FAILED;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        requestQueued = true;

        U32 observedCompletedFrames = 0U;
        while (true) {
            if (!waitForNextCompletedFrame(observedCompletedFrames)) {
                stopActiveCaptureAndDisconnect();
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            if (observedCompletedFrames > CAPTURE_CONVERGENCE_DISCARD_FRAMES) {
                break;
            }
            requestHandle->reuse(libcamera::Request::ReuseBuffers);
            this->applyCaptureControls_(*requestHandle, settings);
            if (this->m_camera->queueRequest(requestHandle.get()) < 0) {
                requestQueued = false;
                this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);
                detailCode = DETAIL_CAPTURE_QUEUE_FAILED;
                return Fw::CmdResponse::EXECUTION_ERROR;
            }
            requestQueued = true;
        }
        requestQueued = false;
        this->m_camera->requestCompleted.disconnect(&completionSlot, &CompletionSlot::onRequestCompleted);
        metadata.captureId = request.captureId;
        metadata.captureIndex = request.captureIndex;
        metadata.bootCount = request.bootCount;
        metadata.capturePolicy = settings.capturePolicy;
        metadata.backendName = "libcamera";
        metadata.cameraModel = "OV5647";
        metadata.requestedMask = request.requestedMask;
        metadata.appliedMask = request.appliedMask;
        metadata.rawRelativePath = request.rawRelativePath;
        metadata.previewRelativePath = request.previewRelativePath;
        metadata.previewDataProductRelativePath = request.metadata.previewDataProductRelativePath;
        metadata.rawDataProductRelativePath = request.metadata.rawDataProductRelativePath;
        metadata.requestedSettings = request.metadata.requestedSettings;
        metadata.appliedSettings = settings;
        metadata.pixelFormat = payloadPixelFormatFor_(this->m_rawPixelFormat);
        metadata.imageWidth = this->m_rawWidth;
        metadata.imageHeight = this->m_rawHeight;
        const auto nowUsec =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        metadata.captureTimeSec = static_cast<U32>(nowUsec / 1000000LL);
        metadata.captureTimeUsec = static_cast<U32>(nowUsec % 1000000LL);
        this->populateActualCaptureMetadata_(requestHandle->metadata(), settings, metadata);
        U32 rawBytes = 0U;
        if (!this->writeBuffer_(rawBuffer, request.rawOutputPath, rawBytes, detailCode)) {
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        U32 previewBytes = 0U;
        if (!this->encodePreviewJpeg_(
                rawBuffer, request.previewOutputPath, settings, settings.jpegQuality, previewBytes, detailCode)) {
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        metadata.rawBytes = rawBytes;
        metadata.previewJpegBytes = previewBytes;

        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse shutdown(U32& detailCode) override {
        detailCode = 0U;
        this->releaseCamera_();
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse readSensorRegister(U32 address, U32 timeoutMs, U32& value, U32& detailCode) override {
        static_cast<void>(address);
        static_cast<void>(timeoutMs);
        static_cast<void>(value);
        detailCode = DETAIL_CONFIG_INVALID;
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    Fw::CmdResponse writeSensorRegister(U32 address,
                                        U32 value,
                                        bool verifyReadback,
                                        U32 timeoutMs,
                                        U32& readbackValue,
                                        U32& detailCode) override {
        static_cast<void>(address);
        static_cast<void>(value);
        static_cast<void>(verifyReadback);
        static_cast<void>(timeoutMs);
        static_cast<void>(readbackValue);
        detailCode = DETAIL_CONFIG_INVALID;
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    void abort() override {}

  private:
    void applyCaptureControls_(libcamera::Request& requestHandle, const OBC::PayloadCameraSettings& settings) const {
        requestHandle.controls().clear();
        if (settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO) {
            requestHandle.controls().set(libcamera::controls::AeEnable, true);
            requestHandle.controls().set(libcamera::controls::AwbEnable, true);
            return;
        }
        requestHandle.controls().set(libcamera::controls::AeEnable, false);
        requestHandle.controls().set(libcamera::controls::AwbEnable, false);
        requestHandle.controls().set(libcamera::controls::ExposureTime, settings.exposureUsec);
        requestHandle.controls().set(libcamera::controls::AnalogueGain,
                                     static_cast<float>(settings.gainX100) / 100.0F);
    }

    void populateActualCaptureMetadata_(const libcamera::ControlList& controls,
                                        const OBC::PayloadCameraSettings& settings,
                                        OBC::PayloadCaptureMetadata& metadata) const {
        if (const auto exposureUsec = controls.get(libcamera::controls::ExposureTime)) {
            metadata.actualExposureUsec = static_cast<U32>(*exposureUsec);
        }
        if (const auto analogueGain = controls.get(libcamera::controls::AnalogueGain)) {
            metadata.actualGainX100 = static_cast<U32>(std::lround(static_cast<double>(*analogueGain) * 100.0));
        }
        if (settings.capturePolicy.e != OBC::PayloadCapturePolicy::CAPTURE_AUTO) {
            return;
        }

        const auto colorTemperature = controls.get(libcamera::controls::ColourTemperature);
        const auto colourGains = controls.get(libcamera::controls::ColourGains);
        if (!colorTemperature.has_value() || !colourGains.has_value() || colourGains->size() < 2U) {
            return;
        }

        metadata.actualAwbValid = true;
        metadata.actualAwbColorTemperatureK = static_cast<U32>(*colorTemperature);
        metadata.actualAwbRedGainX1000 =
            static_cast<U32>(std::lround(static_cast<double>((*colourGains)[0]) * 1000.0));
        metadata.actualAwbBlueGainX1000 =
            static_cast<U32>(std::lround(static_cast<double>((*colourGains)[1]) * 1000.0));
    }

    bool mapBufferPlanes_(libcamera::FrameBuffer* buffer, std::vector<MappedPlaneView>& planes, U32& detailCode) {
        planes.clear();
        planes.reserve(buffer->planes().size());
        for (const libcamera::FrameBuffer::Plane& plane : buffer->planes()) {
            void* mapped = ::mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
            if (mapped == MAP_FAILED) {
                detailCode = static_cast<U32>(errno == 0 ? DETAIL_OUTPUT_MMAP_FAILED_BASE : errno);
                this->unmapBufferPlanes_(planes);
                return false;
            }
            const auto planeIndex = static_cast<std::size_t>(&plane - &buffer->planes()[0]);
            const auto& metaPlane = buffer->metadata().planes()[planeIndex];
            planes.push_back({mapped, plane.length, static_cast<std::size_t>(metaPlane.bytesused)});
        }
        return true;
    }

    void unmapBufferPlanes_(std::vector<MappedPlaneView>& planes) {
        for (auto& plane : planes) {
            if (plane.address != nullptr) {
                ::munmap(plane.address, plane.length);
                plane.address = nullptr;
            }
        }
        planes.clear();
    }

    bool writeBuffer_(libcamera::FrameBuffer* buffer, const std::string& outputPath, U32& bytesWritten, U32& detailCode) {
        bytesWritten = 0U;
        std::ofstream output(outputPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!output.good()) {
            detailCode = static_cast<U32>(errno == 0 ? DETAIL_OUTPUT_OPEN_FAILED_BASE : errno);
            return false;
        }

        std::vector<MappedPlaneView> planes;
        if (!this->mapBufferPlanes_(buffer, planes, detailCode)) {
            return false;
        }
        for (const auto& plane : planes) {
            output.write(static_cast<const char*>(plane.address), static_cast<std::streamsize>(plane.bytesUsed));
            bytesWritten += static_cast<U32>(plane.bytesUsed);
            if (!output.good()) {
                this->unmapBufferPlanes_(planes);
                detailCode = DETAIL_OUTPUT_WRITE_FAILED;
                return false;
            }
        }
        this->unmapBufferPlanes_(planes);

        output.flush();
        return output.good();
    }

    bool encodePreviewJpeg_(libcamera::FrameBuffer* rawBuffer,
                            const std::string& outputPath,
                            const OBC::PayloadCameraSettings& settings,
                            U32 jpegQuality,
                            U32& bytesWritten,
                            U32& detailCode) {
        bytesWritten = 0U;
        if (this->m_rawWidth == 0U || this->m_rawHeight == 0U ||
            this->m_rawWidth > static_cast<U32>(std::numeric_limits<unsigned short>::max()) ||
            this->m_rawHeight > static_cast<U32>(std::numeric_limits<unsigned short>::max())) {
            detailCode = DETAIL_PREVIEW_JPEG_ENCODE_FAILED;
            return false;
        }
        std::vector<MappedPlaneView> planes;
        if (!this->mapBufferPlanes_(rawBuffer, planes, detailCode)) {
            return false;
        }

        std::FILE* output = std::fopen(outputPath.c_str(), "wb");
        if (output == nullptr) {
            this->unmapBufferPlanes_(planes);
            detailCode = static_cast<U32>(errno == 0 ? DETAIL_OUTPUT_OPEN_FAILED_BASE : errno);
            return false;
        }

        std::vector<U8> rgbImage(static_cast<std::size_t>(this->m_rawWidth) * static_cast<std::size_t>(this->m_rawHeight) * 3U,
                                 0U);
        std::vector<U8> rgbRow(static_cast<std::size_t>(this->m_rawWidth) * 3U, 0U);
        for (U32 row = 0U; row < this->m_rawHeight; ++row) {
            const U32 sourceRow = settings.vflip ? (this->m_rawHeight - 1U - row) : row;
            if (!this->fillRgbRow_(planes, sourceRow, rgbRow)) {
                std::fclose(output);
                this->unmapBufferPlanes_(planes);
                detailCode = DETAIL_PREVIEW_JPEG_ENCODE_FAILED;
                return false;
            }
            if (settings.hflip) {
                for (U32 left = 0U, right = this->m_rawWidth == 0U ? 0U : this->m_rawWidth - 1U; left < right; ++left, --right) {
                    const std::size_t leftOffset = static_cast<std::size_t>(left) * 3U;
                    const std::size_t rightOffset = static_cast<std::size_t>(right) * 3U;
                    for (std::size_t channel = 0U; channel < 3U; ++channel) {
                        std::swap(rgbRow[leftOffset + channel], rgbRow[rightOffset + channel]);
                    }
                }
            }
            const std::size_t rowOffset = static_cast<std::size_t>(row) * static_cast<std::size_t>(this->m_rawWidth) * 3U;
            std::copy(rgbRow.begin(), rgbRow.end(), rgbImage.begin() + static_cast<std::ptrdiff_t>(rowOffset));
        }

        const unsigned char quality = static_cast<unsigned char>(std::clamp(static_cast<int>(jpegQuality == 0U ? 90U : jpegQuality), 1, 100));
        s_previewJpegOutput = output;
        const bool encoded = TooJpeg::writeJpeg(previewJpegWriteByte_,
                                                rgbImage.data(),
                                                static_cast<unsigned short>(this->m_rawWidth),
                                                static_cast<unsigned short>(this->m_rawHeight),
                                                true,
                                                quality,
                                                true,
                                                nullptr);
        s_previewJpegOutput = nullptr;
        if (!encoded) {
            std::fclose(output);
            this->unmapBufferPlanes_(planes);
            detailCode = DETAIL_PREVIEW_JPEG_ENCODE_FAILED;
            return false;
        }
        if (std::ferror(output) != 0 || std::fflush(output) != 0) {
            std::fclose(output);
            this->unmapBufferPlanes_(planes);
            detailCode = DETAIL_OUTPUT_WRITE_FAILED;
            return false;
        }
        const long fileSize = std::ftell(output);
        std::fclose(output);
        this->unmapBufferPlanes_(planes);
        if (fileSize <= 0L) {
            detailCode = DETAIL_PREVIEW_JPEG_ENCODE_FAILED;
            return false;
        }
        bytesWritten = static_cast<U32>(fileSize);
        return true;
    }

    bool fillRgbRow_(const std::vector<MappedPlaneView>& planes, U32 row, std::vector<U8>& rgbRow) const {
        const std::size_t width = static_cast<std::size_t>(this->m_rawWidth);
        if (this->m_rawHeight == 0U || static_cast<std::size_t>(row) >= static_cast<std::size_t>(this->m_rawHeight) ||
            rgbRow.size() < width * 3U) {
            return false;
        }
        switch (this->m_rawPixelFormat) {
            case libcamera::formats::YUYV: {
                if (planes.empty() || planes[0].address == nullptr || (width & 1U) != 0U) {
                    return false;
                }
                const auto* input = static_cast<const U8*>(planes[0].address);
                const std::size_t stride = planes[0].bytesUsed / static_cast<std::size_t>(this->m_rawHeight);
                if (stride < width * 2U) {
                    return false;
                }
                const U8* src = input + static_cast<std::size_t>(row) * stride;
                for (std::size_t column = 0; column < width; column += 2U) {
                    const U8 y0 = src[column * 2U + 0U];
                    const U8 u = src[column * 2U + 1U];
                    const U8 y1 = src[column * 2U + 2U];
                    const U8 v = src[column * 2U + 3U];
                    yuvToRgb_(y0, u, v, rgbRow[column * 3U + 0U], rgbRow[column * 3U + 1U], rgbRow[column * 3U + 2U]);
                    if (column + 1U < width) {
                        yuvToRgb_(y1,
                                  u,
                                  v,
                                  rgbRow[(column + 1U) * 3U + 0U],
                                  rgbRow[(column + 1U) * 3U + 1U],
                                  rgbRow[(column + 1U) * 3U + 2U]);
                    }
                }
                return true;
            }
            case libcamera::formats::UYVY: {
                if (planes.empty() || planes[0].address == nullptr || (width & 1U) != 0U) {
                    return false;
                }
                const auto* input = static_cast<const U8*>(planes[0].address);
                const std::size_t stride = planes[0].bytesUsed / static_cast<std::size_t>(this->m_rawHeight);
                if (stride < width * 2U) {
                    return false;
                }
                const U8* src = input + static_cast<std::size_t>(row) * stride;
                for (std::size_t column = 0; column < width; column += 2U) {
                    const U8 u = src[column * 2U + 0U];
                    const U8 y0 = src[column * 2U + 1U];
                    const U8 v = src[column * 2U + 2U];
                    const U8 y1 = src[column * 2U + 3U];
                    yuvToRgb_(y0, u, v, rgbRow[column * 3U + 0U], rgbRow[column * 3U + 1U], rgbRow[column * 3U + 2U]);
                    if (column + 1U < width) {
                        yuvToRgb_(y1,
                                  u,
                                  v,
                                  rgbRow[(column + 1U) * 3U + 0U],
                                  rgbRow[(column + 1U) * 3U + 1U],
                                  rgbRow[(column + 1U) * 3U + 2U]);
                    }
                }
                return true;
            }
            case libcamera::formats::NV12: {
                if (planes.size() < 2U || planes[0].address == nullptr || planes[1].address == nullptr) {
                    return false;
                }
                const auto* yPlane = static_cast<const U8*>(planes[0].address);
                const auto* uvPlane = static_cast<const U8*>(planes[1].address);
                const std::size_t yStride = planes[0].bytesUsed / static_cast<std::size_t>(this->m_rawHeight);
                const std::size_t uvStride = planes[1].bytesUsed / std::max<std::size_t>(1U, static_cast<std::size_t>(this->m_rawHeight / 2U));
                const std::size_t requiredUvStride = (width + 1U) & ~static_cast<std::size_t>(1U);
                if (yStride < width || uvStride < requiredUvStride) {
                    return false;
                }
                const U8* ySrc = yPlane + static_cast<std::size_t>(row) * yStride;
                const U8* uvSrc = uvPlane + static_cast<std::size_t>(row / 2U) * uvStride;
                for (std::size_t column = 0; column < width; ++column) {
                    const U8 y = ySrc[column];
                    const U8 u = uvSrc[(column & ~1U) + 0U];
                    const U8 v = uvSrc[(column & ~1U) + 1U];
                    yuvToRgb_(y, u, v, rgbRow[column * 3U + 0U], rgbRow[column * 3U + 1U], rgbRow[column * 3U + 2U]);
                }
                return true;
            }
            case libcamera::formats::RGB888:
            case libcamera::formats::BGR888: {
                if (planes.empty() || planes[0].address == nullptr) {
                    return false;
                }
                const auto* input = static_cast<const U8*>(planes[0].address);
                const std::size_t stride = planes[0].bytesUsed / static_cast<std::size_t>(this->m_rawHeight);
                if (stride < width * 3U) {
                    return false;
                }
                const U8* src = input + static_cast<std::size_t>(row) * stride;
                const bool bgr = this->m_rawPixelFormat == libcamera::formats::BGR888;
                for (std::size_t column = 0; column < width; ++column) {
                    const std::size_t srcOffset = column * 3U;
                    if (bgr) {
                        rgbRow[column * 3U + 0U] = src[srcOffset + 2U];
                        rgbRow[column * 3U + 1U] = src[srcOffset + 1U];
                        rgbRow[column * 3U + 2U] = src[srcOffset + 0U];
                    } else {
                        rgbRow[column * 3U + 0U] = src[srcOffset + 0U];
                        rgbRow[column * 3U + 1U] = src[srcOffset + 1U];
                        rgbRow[column * 3U + 2U] = src[srcOffset + 2U];
                    }
                }
                return true;
            }
            default:
                return false;
        }
    }

    void releaseCamera_() {
        if (this->m_camera) {
            if (this->m_streamActive) {
                this->m_camera->stop();
                this->m_streamActive = false;
            }
            if (this->m_allocator) {
                if (this->m_rawStream != nullptr) {
                    this->m_allocator->free(this->m_rawStream);
                }
            }
            this->m_allocator.reset();
            this->m_rawStream = nullptr;
            this->m_camera->release();
            this->m_camera.reset();
        }
        if (this->m_cameraManager) {
            this->m_cameraManager->stop();
        }
    }

  private:
    std::unique_ptr<libcamera::CameraManager> m_cameraManager;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    std::shared_ptr<libcamera::Camera> m_camera;
    libcamera::Stream* m_rawStream;
    libcamera::PixelFormat m_rawPixelFormat;
    U32 m_rawWidth;
    U32 m_rawHeight;
    bool m_streamActive;
};

}  // namespace

std::unique_ptr<OBC::IPiCameraDriver> makePayloadHelperBackendDriver() {
    return std::unique_ptr<OBC::IPiCameraDriver>(new LibcameraPiCameraDriver());
}

}  // namespace OBC
#endif

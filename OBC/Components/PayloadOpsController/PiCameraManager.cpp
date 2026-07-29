#include "OBC/Components/PayloadOpsController/PiCameraManager.hpp"

#include <chrono>

#include "simulators/eps/EpsTypes.hpp"

namespace OBC {

namespace {

constexpr U32 DETAIL_NONE = 0U;
constexpr U32 DETAIL_DRIVER_UNAVAILABLE = 1U;
constexpr U32 DETAIL_PROXY_NOT_CONFIGURED = 2U;
constexpr U32 DETAIL_PROXY_SET_FAILED = 3U;
constexpr U32 DETAIL_SETTLE_ABORTED = 4U;
constexpr U32 DETAIL_DRIVER_PREPARE_FAILED = 5U;
constexpr U32 DETAIL_DRIVER_CAPTURE_FAILED = 6U;
constexpr U32 DETAIL_DRIVER_SHUTDOWN_FAILED = 7U;

}  // namespace

PiCameraManager::PiCameraManager()
    : m_epsControl(nullptr),
      m_runtimeConfig(),
      m_worker(),
      m_configured(false),
      m_busy(false),
      m_cancelRequested(false),
      m_prepared(false),
      m_preparedSettings(),
      m_logicalPowerEnabled(false),
      m_proxyAsserted(false),
      m_completionReady(false),
      m_completion() {}

PiCameraManager::~PiCameraManager() {
    this->m_cancelRequested.store(true);
    if (this->m_driver != nullptr) {
        this->m_driver->abort();
    }
    this->joinWorkerIfNeeded_();
}

bool PiCameraManager::configure(std::unique_ptr<OBC::IPiCameraDriver> driver,
                                OBC::IPayloadEpsControl* epsControl,
                                const OBC::PayloadRuntimeConfig& runtimeConfig) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->m_busy) {
        return false;
    }

    this->m_driver = std::move(driver);
    this->m_epsControl = epsControl;
    this->m_runtimeConfig = runtimeConfig;
    this->m_configured = this->m_driver != nullptr && this->m_epsControl != nullptr;
    this->m_cancelRequested.store(false);
    this->m_prepared = false;
    this->m_preparedSettings = {};
    this->m_logicalPowerEnabled = false;
    this->m_proxyAsserted = false;
    this->m_completionReady = false;
    this->m_completion = {};
    return this->m_configured;
}

bool PiCameraManager::isConfigured() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_configured;
}

bool PiCameraManager::isBusy() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_busy;
}

bool PiCameraManager::isPrepared() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_prepared;
}

bool PiCameraManager::getPreparedSettings(OBC::PayloadCameraSettings& settings) const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_prepared) {
        return false;
    }
    settings = this->m_preparedSettings;
    return true;
}

bool PiCameraManager::isLogicalPowerEnabled() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_logicalPowerEnabled;
}

bool PiCameraManager::isProxyAsserted() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_proxyAsserted;
}

bool PiCameraManager::beginPrepare(OBC::PayloadReadyKind readyKind, const OBC::PayloadCameraSettings& settings) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_configured || this->m_busy) {
        return false;
    }

    this->joinWorkerIfNeeded_();
    this->resetCompletion_();
    this->m_busy = true;
    this->m_cancelRequested.store(false);
    this->m_worker = std::thread([this, readyKind, settings]() { this->runPrepare_(readyKind, settings); });
    return true;
}

bool PiCameraManager::beginCapture(const OBC::PayloadCaptureRequest& request,
                                   const OBC::PayloadCameraSettings& settings) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_configured || this->m_busy || !this->m_prepared) {
        return false;
    }

    this->joinWorkerIfNeeded_();
    this->resetCompletion_();
    this->m_busy = true;
    this->m_cancelRequested.store(false);
    this->m_worker = std::thread([this, request, settings]() { this->runCapture_(request, settings); });
    return true;
}

bool PiCameraManager::beginShutdown(bool abortSemantics) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_configured || this->m_busy) {
        return false;
    }

    this->joinWorkerIfNeeded_();
    this->resetCompletion_();
    this->m_busy = true;
    this->m_cancelRequested.store(abortSemantics);
    this->m_worker = std::thread([this, abortSemantics]() { this->runShutdown_(abortSemantics); });
    return true;
}

bool PiCameraManager::requestAbort() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_configured) {
        return false;
    }
    this->m_cancelRequested.store(true);
    if (this->m_driver != nullptr) {
        this->m_driver->abort();
    }
    return true;
}

bool PiCameraManager::pollCompletion(OBC::PayloadOperationResult& result) {
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if (!this->m_completionReady) {
            return false;
        }
        result = this->m_completion;
        this->m_busy = false;
        this->m_completionReady = false;
    }
    this->joinWorkerIfNeeded_();
    return true;
}

void PiCameraManager::joinWorkerIfNeeded_() {
    if (this->m_worker.joinable()) {
        this->m_worker.join();
    }
}

void PiCameraManager::runPrepare_(OBC::PayloadReadyKind readyKind, OBC::PayloadCameraSettings settings) {
    OBC::PayloadOperationResult result = {};
    result.kind = OBC::PayloadOperationKind::PREPARE;
    result.response = Fw::CmdResponse::EXECUTION_ERROR;
    result.resultCode = OBC::PayloadResultCode::PRESULT_PREPARE_FAILED;
    result.detailCode = DETAIL_NONE;
    result.readyKind = readyKind;
    result.metadata.appliedSettings = settings;

    if (this->m_driver == nullptr || !this->m_driver->isAvailable()) {
        result.detailCode = DETAIL_DRIVER_UNAVAILABLE;
        this->finish_(result);
        return;
    }

    if (!this->setProxyPower_(true, result.detailCode)) {
        result.resultCode = OBC::PayloadResultCode::PRESULT_PROXY_POWER_FAILED;
        this->finish_(result);
        return;
    }

    result.logicalPowerEnabled = true;
    result.proxyAsserted = true;

    if (!this->settleDelay_(this->m_runtimeConfig.powerSettleMs)) {
        U32 ignoredDetail = DETAIL_NONE;
        static_cast<void>(this->m_driver->shutdown(ignoredDetail));
        static_cast<void>(this->setProxyPower_(false, ignoredDetail));
        result.logicalPowerEnabled = false;
        result.proxyAsserted = false;
        result.prepared = false;
        result.resultCode = OBC::PayloadResultCode::PRESULT_ABORTED;
        result.detailCode = DETAIL_SETTLE_ABORTED;
        this->finish_(result);
        return;
    }

    const Fw::CmdResponse response =
        this->m_driver->prepare(readyKind,
                                settings,
                                this->m_runtimeConfig.initTimeoutMs,
                                this->m_cancelRequested,
                                result.detailCode);
    if (response != Fw::CmdResponse::OK) {
        U32 ignoredDetail = DETAIL_NONE;
        static_cast<void>(this->m_driver->shutdown(ignoredDetail));
        static_cast<void>(this->setProxyPower_(false, ignoredDetail));
        result.logicalPowerEnabled = false;
        result.proxyAsserted = false;
        result.prepared = false;
        result.response = response;
        result.resultCode = this->m_cancelRequested.load() ? OBC::PayloadResultCode::PRESULT_ABORTED
                                                           : OBC::PayloadResultCode::PRESULT_PREPARE_FAILED;
        if (result.detailCode == DETAIL_NONE) {
            result.detailCode = DETAIL_DRIVER_PREPARE_FAILED;
        }
        this->finish_(result);
        return;
    }

    result.response = Fw::CmdResponse::OK;
    result.resultCode = OBC::PayloadResultCode::PRESULT_OK;
    result.prepared = true;
    this->finish_(result);
}

void PiCameraManager::runCapture_(OBC::PayloadCaptureRequest request, OBC::PayloadCameraSettings settings) {
    OBC::PayloadOperationResult result = {};
    result.kind = OBC::PayloadOperationKind::CAPTURE;
    result.response = Fw::CmdResponse::EXECUTION_ERROR;
    result.resultCode = OBC::PayloadResultCode::PRESULT_CAPTURE_FAILED;
    result.detailCode = DETAIL_NONE;
    result.logicalPowerEnabled = true;
    result.proxyAsserted = true;
    result.prepared = true;
    result.readyKind = settings.readyKind;
    result.captureId = request.captureId;
    result.captureIndex = request.captureIndex;
    result.requestedMask = request.requestedMask;
    result.appliedMask = request.appliedMask;
    result.rawRelativePath = request.rawRelativePath;
    result.previewRelativePath = request.previewRelativePath;
    result.metadata = request.metadata;
    result.metadata.appliedSettings = settings;

    const Fw::CmdResponse response =
        this->m_driver->captureStill(request,
                                     settings,
                                     this->m_runtimeConfig.captureTimeoutMs,
                                     this->m_cancelRequested,
                                     result.metadata,
                                     result.detailCode);
    if (response != Fw::CmdResponse::OK) {
        result.response = response;
        result.resultCode = this->m_cancelRequested.load() ? OBC::PayloadResultCode::PRESULT_ABORTED
                                                           : OBC::PayloadResultCode::PRESULT_CAPTURE_FAILED;
        if (result.detailCode == DETAIL_NONE) {
            result.detailCode = DETAIL_DRIVER_CAPTURE_FAILED;
        }

        U32 shutdownDetail = DETAIL_NONE;
        const Fw::CmdResponse shutdownResponse = this->m_driver->shutdown(shutdownDetail);
        if (shutdownResponse != Fw::CmdResponse::OK) {
            result.detailCode = shutdownDetail == DETAIL_NONE ? DETAIL_DRIVER_SHUTDOWN_FAILED : shutdownDetail;
            result.resultCode = OBC::PayloadResultCode::PRESULT_SHUTDOWN_FAILED;
            result.response = shutdownResponse;
        }

        U32 proxyDetail = DETAIL_NONE;
        static_cast<void>(this->setProxyPower_(false, proxyDetail));
        result.logicalPowerEnabled = false;
        result.proxyAsserted = false;
        result.prepared = false;
        this->finish_(result);
        return;
    }

    result.response = Fw::CmdResponse::OK;
    result.resultCode = OBC::PayloadResultCode::PRESULT_OK;
    this->finish_(result);
}

void PiCameraManager::runShutdown_(bool abortSemantics) {
    OBC::PayloadOperationResult result = {};
    result.kind = abortSemantics ? OBC::PayloadOperationKind::ABORT : OBC::PayloadOperationKind::SHUTDOWN;
    result.response = Fw::CmdResponse::OK;
    result.resultCode = abortSemantics ? OBC::PayloadResultCode::PRESULT_ABORTED : OBC::PayloadResultCode::PRESULT_OK;
    result.detailCode = DETAIL_NONE;

    U32 shutdownDetail = DETAIL_NONE;
    if (this->m_driver != nullptr) {
        const Fw::CmdResponse shutdownResponse = this->m_driver->shutdown(shutdownDetail);
        if (shutdownResponse != Fw::CmdResponse::OK) {
            result.response = shutdownResponse;
            result.resultCode = OBC::PayloadResultCode::PRESULT_SHUTDOWN_FAILED;
            result.detailCode = shutdownDetail == DETAIL_NONE ? DETAIL_DRIVER_SHUTDOWN_FAILED : shutdownDetail;
        }
    }

    U32 proxyDetail = DETAIL_NONE;
    if (!this->setProxyPower_(false, proxyDetail) && result.response == Fw::CmdResponse::OK) {
        result.response = Fw::CmdResponse::EXECUTION_ERROR;
        result.resultCode = OBC::PayloadResultCode::PRESULT_PROXY_POWER_FAILED;
        result.detailCode = proxyDetail;
    }

    result.logicalPowerEnabled = false;
    result.proxyAsserted = false;
    result.prepared = false;
    this->finish_(result);
}

bool PiCameraManager::setProxyPower_(bool enabled, U32& detailCode) {
    if (this->m_epsControl == nullptr) {
        detailCode = DETAIL_PROXY_NOT_CONFIGURED;
        return false;
    }

    OBC::EPS::StatusData status = {};
    const Fw::CmdResponse response = this->m_epsControl->setPayloadProxyPower(enabled, status);
    if (response != Fw::CmdResponse::OK) {
        detailCode = DETAIL_PROXY_SET_FAILED;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_proxyAsserted = enabled;
        this->m_logicalPowerEnabled = enabled;
    }
    detailCode = DETAIL_NONE;
    return true;
}

bool PiCameraManager::settleDelay_(U32 delayMs) {
    U32 elapsedMs = 0U;
    while (elapsedMs < delayMs) {
        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            if (this->m_cancelRequested.load()) {
                return false;
            }
        }
        const U32 sliceMs = (delayMs - elapsedMs) > 25U ? 25U : (delayMs - elapsedMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(sliceMs));
        elapsedMs += sliceMs;
    }
    return true;
}

void PiCameraManager::finish_(const OBC::PayloadOperationResult& result) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_prepared = result.prepared;
    if (result.prepared &&
        (result.kind == OBC::PayloadOperationKind::PREPARE || result.kind == OBC::PayloadOperationKind::CAPTURE)) {
        this->m_preparedSettings = result.metadata.appliedSettings;
    } else if (!result.prepared) {
        this->m_preparedSettings = {};
    }
    this->m_logicalPowerEnabled = result.logicalPowerEnabled;
    this->m_proxyAsserted = result.proxyAsserted;
    this->m_completion = result;
    this->m_completionReady = true;
}

void PiCameraManager::resetCompletion_() {
    this->m_completionReady = false;
    this->m_completion = {};
}

}  // namespace OBC

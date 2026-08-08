#ifndef OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PICAMERAMANAGER_HPP
#define OBC_COMPONENTS_PAYLOADOPSCONTROLLER_PICAMERAMANAGER_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

namespace OBC {

class PiCameraManager final {
  public:
    PiCameraManager();
    ~PiCameraManager();

    bool configure(std::unique_ptr<OBC::IPiCameraDriver> driver,
                   OBC::IPayloadEpsControl* epsControl,
                   const OBC::PayloadRuntimeConfig& runtimeConfig);

    bool isConfigured() const;
    bool isBusy() const;
    bool isPrepared() const;
    bool getPreparedSettings(OBC::PayloadCameraSettings& settings) const;
    bool isLogicalPowerEnabled() const;
    bool isProxyAsserted() const;
    OBC::IPiCameraDriver& driver() { return *this->m_driver; }
    const OBC::IPiCameraDriver& driver() const { return *this->m_driver; }

    bool beginPrepare(OBC::PayloadReadyKind readyKind, const OBC::PayloadCameraSettings& settings);
    bool beginCapture(const OBC::PayloadCaptureRequest& request, const OBC::PayloadCameraSettings& settings);
    bool beginShutdown(bool abortSemantics);
    bool requestAbort();
    bool pollCompletion(OBC::PayloadOperationResult& result);

  private:
    void joinWorkerIfNeeded_();
    void runPrepare_(OBC::PayloadReadyKind readyKind, OBC::PayloadCameraSettings settings);
    void runCapture_(OBC::PayloadCaptureRequest request, OBC::PayloadCameraSettings settings);
    void runShutdown_(bool abortSemantics);
    bool setProxyPower_(bool enabled, U32& detailCode);
    bool settleDelay_(U32 delayMs);
    void finish_(const OBC::PayloadOperationResult& result);
    void resetCompletion_();

  private:
    mutable std::mutex m_mutex;
    std::unique_ptr<OBC::IPiCameraDriver> m_driver;
    OBC::IPayloadEpsControl* m_epsControl;
    OBC::PayloadRuntimeConfig m_runtimeConfig;
    std::thread m_worker;
    bool m_configured;
    bool m_busy;
    std::atomic<bool> m_cancelRequested;
    bool m_prepared;
    OBC::PayloadCameraSettings m_preparedSettings;
    bool m_logicalPowerEnabled;
    bool m_proxyAsserted;
    bool m_completionReady;
    OBC::PayloadOperationResult m_completion;
};

}  // namespace OBC

#endif

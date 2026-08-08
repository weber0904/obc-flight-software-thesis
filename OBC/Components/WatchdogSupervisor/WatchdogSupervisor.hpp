#ifndef OBC_COMPONENTS_WATCHDOGSUPERVISOR_WATCHDOGSUPERVISOR_HPP
#define OBC_COMPONENTS_WATCHDOGSUPERVISOR_WATCHDOGSUPERVISOR_HPP

#include <array>
#include <cstddef>
#include <mutex>

#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"
#include "OBC/Components/WatchdogSupervisor/WatchdogPolicy.hpp"
#include "OBC/Components/WatchdogSupervisor/WatchdogSupervisorComponentAc.hpp"

namespace OBC {

class WatchdogSupervisor final : public WatchdogSupervisorComponentBase {
  public:
    explicit WatchdogSupervisor(const char* const compName);

    ~WatchdogSupervisor() override;

    void configureRuntime(OBC::IModeSafetyModeControl* modeControl, OBC::IRecoveryRequestSink* recoverySink);

    void setEnabledForRuntime(bool enable);

    Fw::CmdResponse setThresholdForRuntime(OBC::HealthItem item, F32 value);

    void updateResourceSample(F32 cpuUsage, F32 memRssMb);

    OBC::WatchdogRuntimeSnapshot getStatusForRuntime() const;

    Fw::CmdResponse setConfigForRuntime(OBC::WatchdogSource source,
                                        bool enabled,
                                        U32 warningTicks,
                                        U32 safeTicks,
                                        U32 suppressTicks);

    bool setProbeBeatSuppressionForRuntime(OBC::WatchdogSource source, bool suppressed);

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void beatIn_handler(FwIndexType portNum, U32 code) override;

    void HEALTH_ENABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enable) override;

    void HEALTH_SET_THRESHOLD_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::HealthItem item, F32 value) override;

    void GET_WATCHDOG_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void SET_WATCHDOG_CONFIG_cmdHandler(FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        OBC::WatchdogSource source,
                                        bool enabled,
                                        U32 warningTicks,
                                        U32 safeTicks,
                                        U32 suppressTicks) override;
    void SET_WATCHDOG_PROBE_SUPPRESSION_cmdHandler(FwOpcodeType opCode,
                                                   U32 cmdSeq,
                                                   OBC::WatchdogSource source,
                                                   bool suppressed) override;

    std::size_t thresholdIndex_(const OBC::HealthItem& item) const;

    void initializeSources_();

    void updateSourceStatus_(std::size_t sourceIndex, const OBC::WatchdogSourceEvaluation& evaluation);

    void publishTelemetry_(const OBC::WatchdogRuntimeSnapshot& snapshot);

    void emitStatusEvents_(const OBC::WatchdogRuntimeSnapshot& snapshot);

    void emitFeedEligibilityEvent_(const OBC::WatchdogAggregateRollup& rollup);

    void applyAggregateRollup_(const OBC::WatchdogAggregateRollup& rollup);

    void resetSourceAfterProbeSuppressionClear_(OBC::WatchdogSourceSnapshot& source);

  private:
    static constexpr U32 WATCHDOG_FEED_CODE = 0x57444731U;

    OBC::IModeSafetyModeControl* m_modeControl;
    OBC::IRecoveryRequestSink* m_recoverySink;
    bool m_resourceMonitoringEnabled;
    std::array<F32, OBC::HealthItem::NUM_CONSTANTS> m_resourceThresholds;
    std::array<bool, OBC::HealthItem::NUM_CONSTANTS> m_resourceThresholdExceeded;
    OBC::WatchdogRuntimeSnapshot m_runtimeStatus;
    bool m_safeRequestIssuedForCurrentFault;
    mutable std::mutex m_runtimeMutex;
};

}  // namespace OBC

#endif

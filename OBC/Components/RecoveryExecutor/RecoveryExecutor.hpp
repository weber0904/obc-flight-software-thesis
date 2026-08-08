#ifndef OBC_COMPONENTS_RECOVERYEXECUTOR_RECOVERYEXECUTOR_HPP
#define OBC_COMPONENTS_RECOVERYEXECUTOR_RECOVERYEXECUTOR_HPP

#include <mutex>
#include <vector>

#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryExecutorComponentAc.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"

namespace OBC {

class RecoveryExecutor final : public RecoveryExecutorComponentBase, public OBC::IRecoveryRequestSink {
  public:
    explicit RecoveryExecutor(const char* const compName);

    ~RecoveryExecutor() override;

    void configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                          OBC::IRecoveryEpsControl* epsControl,
                          OBC::IRecoveryAdcsControl* adcsControl,
                          OBC::IRecoveryBootControl* bootControl,
                          OBC::IRecoveryCommControl* commControl);
    void configureHardwareWatchdogModeForRuntime(bool enabled);

    void configurePersistentFaultRecorderForRuntime(OBC::IPersistentFaultRecorder* recorder);

    OBC::RecoveryRuntimeStatus getStatusForRuntime() const;

    OBC::RecoveryExitRequest consumeRecoveryExitRequestForRuntime();

    bool consumeRebootRequestForRuntime();

    void submitWatchdogFault(OBC::WatchdogSource source) override;
    void submitWatchdogSuppression(OBC::WatchdogSource source) override;
    void clearWatchdogFault(OBC::WatchdogSource source) override;
    void submitEpsTimeoutFault(U32 failureCount) override;
    void clearEpsTimeoutFault(U32 failureCount) override;
    void submitAdcsPollTransportFault(U32 failureCount) override;
    void clearAdcsPollTransportFault(U32 failureCount) override;
    void submitAdcsPollFreshnessFault(U32 failureCount) override;
    void clearAdcsPollFreshnessFault(U32 failureCount) override;
    void submitCommPrimaryUnavailableFault(U32 failureCount) override;
    void clearCommPrimaryUnavailableFault(U32 failureCount) override;
    void submitCommPrimaryTransportFault(U32 failureCount) override;
    void clearCommPrimaryTransportFault(U32 failureCount) override;

  private:
    static constexpr U32 STABLE_ACK_TICKS = 5U;
    static constexpr U32 UNCLEARED_REBOOT_TIMEOUT_TICKS = 3U;

    enum class ExternalActionKind {
        APPLY_SAFE,
        APPLY_BOOT_SAFE_FALLBACK,
        EPS_RESET,
        ADCS_RESET,
        COMM_FAILOVER,
        RECORD_PROCESS_RESTART_INTENT,
        RECORD_REBOOT_INTENT,
        ACK_STABLE,
    };

    struct ExternalActionRequest {
        ExternalActionKind kind;
        OBC::RecoveryIncidentSource source = OBC::RecoveryIncidentSource::NONE;
        OBC::RecoveryAction action = OBC::RecoveryAction::NONE;
        OBC::ModeApplySource modeSource = OBC::ModeApplySource::TestSetup;
    };

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;
    void GET_RECOVERY_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    OBC::SatMode currentModeForRuntime_() const;

    OBC::RecoveryIncidentState* incidentForSource_(OBC::RecoveryIncidentSource source);
    void openIncident_(OBC::RecoveryIncidentState& incident, OBC::RecoveryIncidentSource source);
    void closeIncident_(OBC::RecoveryIncidentState& incident);
    void issueActionRequested_(const OBC::RecoveryIncidentState& incident, OBC::RecoveryAction action);
    void issueActionExecuted_(const OBC::RecoveryIncidentState& incident, OBC::RecoveryAction action, U32 response);
    void prepareProcessRestartLocked_(OBC::RecoveryIncidentState& incident,
                                      OBC::ModeApplySource fallbackModeSource,
                                      std::vector<ExternalActionRequest>& actions);
    bool applyBootSafeFallbackClampLocked_(OBC::RecoveryIncidentState& incident,
                                           OBC::SatMode currentMode,
                                           OBC::ModeApplySource modeSource,
                                           std::vector<ExternalActionRequest>& actions);
    bool prepareSafeFallbackLocked_(OBC::RecoveryIncidentState& incident,
                                    OBC::SatMode currentMode,
                                    OBC::ModeApplySource modeSource,
                                    std::vector<ExternalActionRequest>& actions,
                                    bool allowAlreadySafe = false);
    void prepareEpsResetLocked_(OBC::RecoveryIncidentState& incident, std::vector<ExternalActionRequest>& actions);
    void prepareAdcsResetLocked_(OBC::RecoveryIncidentState& incident, std::vector<ExternalActionRequest>& actions);
    void prepareCommFailoverLocked_(OBC::RecoveryIncidentState& incident, std::vector<ExternalActionRequest>& actions);
    void prepareRebootLocked_(OBC::RecoveryIncidentState& incident, std::vector<ExternalActionRequest>& actions);
    void executeExternalActions_(std::vector<ExternalActionRequest>& actions);
    void executeExternalAction_(const ExternalActionRequest& action, std::vector<ExternalActionRequest>& followups);
    void handleSubsystemFault_(OBC::RecoveryIncidentSource source, OBC::SatMode currentMode);
    void clearIncidentBySource_(OBC::RecoveryIncidentSource source);
    bool isSubsystemRecoverySource_(OBC::RecoveryIncidentSource source) const;
    bool isWatchdogRecoverySource_(OBC::RecoveryIncidentSource source) const;
    bool isUnclearedTimeoutRecoverySource_(OBC::RecoveryIncidentSource source) const;
    void updateStatusSummaryLocked_(const OBC::RecoveryIncidentState* focusIncident);
    void appendPersistentFaultRecordLocked_(OBC::PersistentFaultRecordKind kind,
                                            const OBC::RecoveryIncidentState& incident,
                                            OBC::RecoveryAction action,
                                            U32 detail = 0U,
                                            U32 flags = 0U);

  private:
    OBC::IModeSafetyModeControl* m_modeControl;
    OBC::IRecoveryEpsControl* m_epsControl;
    OBC::IRecoveryAdcsControl* m_adcsControl;
    OBC::IRecoveryBootControl* m_bootControl;
    OBC::IRecoveryCommControl* m_commControl;
    OBC::IPersistentFaultRecorder* m_faultRecorder;
    OBC::RecoveryRuntimeStatus m_status;
    U32 m_stableTicks;
    bool m_bootClampApplied;
    bool m_stableAcked;
    bool m_hardwareWatchdogModeEnabled;
    bool m_processRestartRequested;
    bool m_rebootRequested;
    bool m_exitRequestConsumed;
    mutable std::mutex m_mutex;
};

}  // namespace OBC

#endif

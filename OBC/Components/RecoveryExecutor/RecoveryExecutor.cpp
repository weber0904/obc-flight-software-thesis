#include "OBC/Components/RecoveryExecutor/RecoveryExecutor.hpp"

#include <limits>

#include "simulators/adcs/AdcsTypes.hpp"
#include "simulators/eps/EpsTypes.hpp"

namespace OBC {

namespace {

void saturatingIncrement(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
}

bool modeIsSafeRequestable(const OBC::SatMode mode) {
    return mode == OBC::SatMode::IDLE || mode == OBC::SatMode::PAYLOAD || mode == OBC::SatMode::TTC;
}

U32 commActionResponseCode(const OBC::RecoveryCommActionResult& result) {
    if (result.noHealthyBackup) {
        return 1U;
    }
    if (result.alreadyOnHealthyPrimary) {
        return 2U;
    }
    return 0U;
}

}  // namespace

RecoveryExecutor::RecoveryExecutor(const char* const compName)
    : RecoveryExecutorComponentBase(compName),
      m_modeControl(nullptr),
      m_epsControl(nullptr),
      m_adcsControl(nullptr),
      m_bootControl(nullptr),
      m_commControl(nullptr),
      m_faultRecorder(nullptr),
      m_status(),
      m_stableTicks(0U),
      m_bootClampApplied(false),
      m_stableAcked(false),
      m_hardwareWatchdogModeEnabled(false),
      m_processRestartRequested(false),
      m_rebootRequested(false),
      m_exitRequestConsumed(false) {
    for (std::size_t index = 0; index < this->m_status.incidents.size(); index++) {
        this->m_status.incidents.at(index).source =
            OBC::RecoveryIncidentSource(static_cast<OBC::RecoveryIncidentSource::T>(static_cast<U8>(index)));
    }
}

RecoveryExecutor::~RecoveryExecutor() = default;

void RecoveryExecutor::configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                                        OBC::IRecoveryEpsControl* epsControl,
                                        OBC::IRecoveryAdcsControl* adcsControl,
                                        OBC::IRecoveryBootControl* bootControl,
                                        OBC::IRecoveryCommControl* commControl) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_modeControl = modeControl;
    this->m_epsControl = epsControl;
    this->m_adcsControl = adcsControl;
    this->m_bootControl = bootControl;
    this->m_commControl = commControl;
    this->m_stableTicks = 0U;
    this->m_bootClampApplied = false;
    this->m_stableAcked = false;
    this->m_processRestartRequested = false;
    this->m_rebootRequested = false;
    this->m_exitRequestConsumed = false;
}

void RecoveryExecutor::configureHardwareWatchdogModeForRuntime(bool enabled) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_hardwareWatchdogModeEnabled = enabled;
}

void RecoveryExecutor::configurePersistentFaultRecorderForRuntime(OBC::IPersistentFaultRecorder* recorder) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_faultRecorder = recorder;
}

OBC::RecoveryRuntimeStatus RecoveryExecutor::getStatusForRuntime() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_status;
}

OBC::RecoveryExitRequest RecoveryExecutor::consumeRecoveryExitRequestForRuntime() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->m_rebootRequested) {
        this->m_rebootRequested = false;
        this->m_exitRequestConsumed = true;
        for (auto& incident : this->m_status.incidents) {
            incident.rebootPending = false;
        }
        this->updateStatusSummaryLocked_(nullptr);
        return OBC::RecoveryExitRequest::OBC_REBOOT;
    }

    if (this->m_processRestartRequested) {
        this->m_processRestartRequested = false;
        this->m_exitRequestConsumed = true;
        for (auto& incident : this->m_status.incidents) {
            incident.processRestartPending = false;
        }
        this->updateStatusSummaryLocked_(nullptr);
        return OBC::RecoveryExitRequest::PROCESS_RESTART;
    }

    return OBC::RecoveryExitRequest::NONE;
}

bool RecoveryExecutor::consumeRebootRequestForRuntime() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    const bool requested = this->m_rebootRequested;
    this->m_rebootRequested = false;
    if (requested) {
        this->m_exitRequestConsumed = true;
        for (auto& incident : this->m_status.incidents) {
            incident.rebootPending = false;
        }
        this->updateStatusSummaryLocked_(nullptr);
    }
    return requested;
}

void RecoveryExecutor::submitWatchdogFault(OBC::WatchdogSource source) {
    if (!source.isValid()) {
        return;
    }

    const OBC::SatMode currentMode = this->currentModeForRuntime_();
    std::vector<ExternalActionRequest> actions = {};
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        OBC::RecoveryIncidentSource incidentSource = OBC::RecoveryIncidentSource::NONE;
        if (!recoveryIncidentSourceFromWatchdog(source, incidentSource)) {
            return;
        }
        OBC::RecoveryIncidentState* incident = this->incidentForSource_(incidentSource);
        if (incident == nullptr) {
            return;
        }
        if (!incident->active) {
            this->openIncident_(*incident, incidentSource);
            if (!this->applyBootSafeFallbackClampLocked_(
                    *incident, currentMode, OBC::ModeApplySource::WatchdogFault, actions)) {
                this->prepareProcessRestartLocked_(*incident, OBC::ModeApplySource::WatchdogFault, actions);
            }
        }
        this->updateStatusSummaryLocked_(incident);
    }
    this->executeExternalActions_(actions);
}

void RecoveryExecutor::submitWatchdogSuppression(OBC::WatchdogSource source) {
    if (!source.isValid()) {
        return;
    }

    std::vector<ExternalActionRequest> actions = {};
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        OBC::RecoveryIncidentSource incidentSource = OBC::RecoveryIncidentSource::NONE;
        if (!recoveryIncidentSourceFromWatchdog(source, incidentSource)) {
            return;
        }
        OBC::RecoveryIncidentState* incident = this->incidentForSource_(incidentSource);
        if (incident == nullptr) {
            return;
        }
        if (!incident->active) {
            this->openIncident_(*incident, incidentSource);
        }
        if (!incident->rebootPending) {
            this->prepareRebootLocked_(*incident, actions);
        }
        this->updateStatusSummaryLocked_(incident);
    }
    this->executeExternalActions_(actions);
}

void RecoveryExecutor::clearWatchdogFault(OBC::WatchdogSource source) {
    if (!source.isValid()) {
        return;
    }

    OBC::RecoveryIncidentSource incidentSource = OBC::RecoveryIncidentSource::NONE;
    if (!recoveryIncidentSourceFromWatchdog(source, incidentSource)) {
        return;
    }
    this->clearIncidentBySource_(incidentSource);
}

void RecoveryExecutor::submitEpsTimeoutFault(U32) {
    this->handleSubsystemFault_(OBC::RecoveryIncidentSource::EPS_TIMEOUT, this->currentModeForRuntime_());
}

void RecoveryExecutor::clearEpsTimeoutFault(U32) {
    this->clearIncidentBySource_(OBC::RecoveryIncidentSource::EPS_TIMEOUT);
}

void RecoveryExecutor::submitAdcsPollTransportFault(U32) {
    this->handleSubsystemFault_(OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT, this->currentModeForRuntime_());
}

void RecoveryExecutor::clearAdcsPollTransportFault(U32) {
    this->clearIncidentBySource_(OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
}

void RecoveryExecutor::submitAdcsPollFreshnessFault(U32) {
    this->handleSubsystemFault_(OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS, this->currentModeForRuntime_());
}

void RecoveryExecutor::clearAdcsPollFreshnessFault(U32) {
    this->clearIncidentBySource_(OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS);
}

void RecoveryExecutor::submitCommPrimaryUnavailableFault(U32) {
    this->handleSubsystemFault_(OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE, this->currentModeForRuntime_());
}

void RecoveryExecutor::clearCommPrimaryUnavailableFault(U32) {
    this->clearIncidentBySource_(OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE);
}

void RecoveryExecutor::submitCommPrimaryTransportFault(U32) {
    this->handleSubsystemFault_(OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT, this->currentModeForRuntime_());
}

void RecoveryExecutor::clearCommPrimaryTransportFault(U32) {
    this->clearIncidentBySource_(OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT);
}

void RecoveryExecutor::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    if (!this->m_bootClampApplied && this->m_bootControl != nullptr &&
        this->m_bootControl->isBootSafeFallbackRequiredForRuntime() && this->m_modeControl != nullptr) {
        this->m_modeControl->applyModeForInternalSource(OBC::SatMode::SAFE, OBC::ModeApplySource::RecoveryBootFallback);
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_bootClampApplied = true;
    }

    const U32 consecutiveResetCount =
        this->m_bootControl == nullptr ? 0U : this->m_bootControl->getConsecutiveResetCountForRuntime();
    std::vector<ExternalActionRequest> actions = {};
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if (!this->m_processRestartRequested && !this->m_rebootRequested && !this->m_exitRequestConsumed) {
            for (auto& incident : this->m_status.incidents) {
                if (this->isUnclearedTimeoutRecoverySource_(incident.source) && incident.active && incident.awaitingClear &&
                    !incident.processRestartPending && !incident.rebootPending && incident.firstActionIssued) {
                    saturatingIncrement(incident.openTicks);
                    if (incident.openTicks >= UNCLEARED_REBOOT_TIMEOUT_TICKS) {
                        this->prepareRebootLocked_(incident, actions);
                    }
                } else if (incident.active && !incident.processRestartPending && !incident.rebootPending &&
                           incident.currentLevel == OBC::RecoveryLevel::R6_OBC_REBOOT) {
                    this->prepareRebootLocked_(incident, actions);
                }
            }
        }

        this->updateStatusSummaryLocked_(nullptr);
        if (this->m_status.activeIncidentCount == 0U && !this->m_status.pendingProcessRestart &&
            !this->m_status.pendingReboot && consecutiveResetCount > 0U && !this->m_stableAcked) {
            saturatingIncrement(this->m_stableTicks);
            if (this->m_stableTicks >= STABLE_ACK_TICKS) {
                actions.push_back({ExternalActionKind::ACK_STABLE, OBC::RecoveryIncidentSource::NONE});
            }
        } else if (this->m_status.activeIncidentCount != 0U || this->m_status.pendingProcessRestart ||
                   this->m_status.pendingReboot) {
            this->m_stableTicks = 0U;
        }
    }

    this->executeExternalActions_(actions);

    const OBC::RecoveryRuntimeStatus snapshot = this->getStatusForRuntime();
    this->tlmWrite_RECOVERY_ACTIVE_INCIDENTS(snapshot.activeIncidentCount);
    this->tlmWrite_RECOVERY_ACTIVE_SOURCE(snapshot.activeSource);
    this->tlmWrite_RECOVERY_CURRENT_LEVEL(snapshot.currentLevel);
    this->tlmWrite_RECOVERY_HIGHEST_LEVEL(snapshot.highestLevel);
    this->tlmWrite_RECOVERY_LAST_ACTION(snapshot.lastAction);
    this->tlmWrite_RECOVERY_PENDING_REBOOT(snapshot.pendingReboot);
    this->tlmWrite_RECOVERY_RELATCH_COUNT(snapshot.relatchCount);
    this->tlmWrite_RECOVERY_TOTAL_SAFE_FALLBACKS(snapshot.totalSafeFallbacks);
    this->tlmWrite_RECOVERY_TOTAL_RESET_ACTIONS(snapshot.totalResetActions);
    this->tlmWrite_RECOVERY_TOTAL_REBOOTS(snapshot.totalReboots);
    this->tlmWrite_RECOVERY_PENDING_PROCESS_RESTART(snapshot.pendingProcessRestart);
    this->tlmWrite_RECOVERY_TOTAL_PROCESS_RESTARTS(snapshot.totalProcessRestarts);
}

void RecoveryExecutor::GET_RECOVERY_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    const OBC::RecoveryRuntimeStatus status = this->getStatusForRuntime();
    this->log_ACTIVITY_HI_RECOVERY_STATUS(status.activeIncidentCount,
                                          status.activeSource,
                                          status.highestLevel,
                                          status.lastAction,
                                          status.pendingProcessRestart,
                                          status.pendingReboot,
                                          status.relatchCount);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

OBC::SatMode RecoveryExecutor::currentModeForRuntime_() const {
    if (this->m_modeControl == nullptr) {
        return OBC::SatMode(OBC::SatMode::SAFE);
    }
    return this->m_modeControl->getModeForRuntime();
}

OBC::RecoveryIncidentState* RecoveryExecutor::incidentForSource_(OBC::RecoveryIncidentSource source) {
    if (!recoveryIncidentSourceIsIndexed(source)) {
        return nullptr;
    }
    return &this->m_status.incidents.at(recoveryIncidentIndex(source));
}

void RecoveryExecutor::openIncident_(OBC::RecoveryIncidentState& incident, OBC::RecoveryIncidentSource source) {
    if (incident.hasOpenedBefore) {
        saturatingIncrement(incident.relatchCount);
    }
    incident.active = true;
    incident.awaitingClear = true;
    incident.hasOpenedBefore = true;
    incident.firstActionIssued = false;
    incident.restartIntentIssued = false;
    incident.processRestartPending = false;
    incident.safeFallbackIssued = false;
    incident.resetIssued = false;
    incident.commFailoverIssued = false;
    incident.rebootPending = false;
    incident.source = source;
    incident.currentLevel = recoveryInitialLevelForSource(source);
    incident.highestLevel = incident.currentLevel;
    incident.lastAction = OBC::RecoveryAction::NONE;
    incident.openTicks = 0U;
    saturatingIncrement(incident.epoch);
    this->log_WARNING_HI_RECOVERY_INCIDENT_OPENED(source, incident.currentLevel, incident.relatchCount);
    this->appendPersistentFaultRecordLocked_(
        OBC::PersistentFaultRecordKind::INCIDENT_OPENED, incident, OBC::RecoveryAction::NONE, incident.relatchCount);
}

void RecoveryExecutor::closeIncident_(OBC::RecoveryIncidentState& incident) {
    OBC::RecoveryIncidentState clearedRecord = incident;
    incident.active = false;
    incident.awaitingClear = false;
    if (!incident.processRestartPending && !incident.rebootPending) {
        incident.currentLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
        incident.lastAction = OBC::RecoveryAction::INCIDENT_CLEARED;
    }
    this->log_ACTIVITY_HI_RECOVERY_INCIDENT_CLEARED(incident.source, incident.highestLevel, incident.relatchCount);
    clearedRecord.currentLevel = clearedRecord.highestLevel;
    clearedRecord.lastAction = OBC::RecoveryAction::INCIDENT_CLEARED;
    this->appendPersistentFaultRecordLocked_(
        OBC::PersistentFaultRecordKind::INCIDENT_CLEARED, clearedRecord, OBC::RecoveryAction::INCIDENT_CLEARED);
}

void RecoveryExecutor::issueActionRequested_(const OBC::RecoveryIncidentState& incident, OBC::RecoveryAction action) {
    this->log_WARNING_HI_RECOVERY_ACTION_REQUESTED(incident.source, incident.currentLevel, action);
    this->appendPersistentFaultRecordLocked_(OBC::PersistentFaultRecordKind::ACTION_REQUESTED, incident, action);
}

void RecoveryExecutor::issueActionExecuted_(const OBC::RecoveryIncidentState& incident,
                                            OBC::RecoveryAction action,
                                            U32 response) {
    this->log_ACTIVITY_HI_RECOVERY_ACTION_EXECUTED(incident.source, action, response);
    this->appendPersistentFaultRecordLocked_(
        OBC::PersistentFaultRecordKind::ACTION_EXECUTED, incident, action, response);
}

void RecoveryExecutor::prepareProcessRestartLocked_(OBC::RecoveryIncidentState& incident,
                                                    OBC::ModeApplySource fallbackModeSource,
                                                    std::vector<ExternalActionRequest>& actions) {
    if (incident.restartIntentIssued) {
        return;
    }
    incident.restartIntentIssued = true;
    incident.firstActionIssued = true;
    incident.currentLevel = OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT;
    if (incident.highestLevel.e < incident.currentLevel.e) {
        incident.highestLevel = incident.currentLevel;
    }
    incident.lastAction = OBC::RecoveryAction::PROCESS_RESTART;
    this->issueActionRequested_(incident, OBC::RecoveryAction::PROCESS_RESTART);
    actions.push_back({ExternalActionKind::RECORD_PROCESS_RESTART_INTENT,
                       incident.source,
                       OBC::RecoveryAction::PROCESS_RESTART,
                       fallbackModeSource});
}

bool RecoveryExecutor::applyBootSafeFallbackClampLocked_(OBC::RecoveryIncidentState& incident,
                                                         OBC::SatMode currentMode,
                                                         OBC::ModeApplySource modeSource,
                                                         std::vector<ExternalActionRequest>& actions) {
    if (this->m_bootControl == nullptr || !this->m_bootControl->isBootSafeFallbackRequiredForRuntime()) {
        return false;
    }

    return this->prepareSafeFallbackLocked_(incident, currentMode, modeSource, actions, true);
}

bool RecoveryExecutor::prepareSafeFallbackLocked_(OBC::RecoveryIncidentState& incident,
                                                  OBC::SatMode currentMode,
                                                  OBC::ModeApplySource modeSource,
                                                  std::vector<ExternalActionRequest>& actions,
                                                  bool allowAlreadySafe) {
    const bool canRequestSafe =
        modeIsSafeRequestable(currentMode) || (allowAlreadySafe && currentMode == OBC::SatMode::SAFE);
    if (incident.safeFallbackIssued || this->m_modeControl == nullptr || !canRequestSafe) {
        return false;
    }
    incident.safeFallbackIssued = true;
    incident.currentLevel = OBC::RecoveryLevel::R5_MODE_FALLBACK;
    if (incident.highestLevel.e < incident.currentLevel.e) {
        incident.highestLevel = incident.currentLevel;
    }
    incident.lastAction = OBC::RecoveryAction::SAFE_FALLBACK;
    this->issueActionRequested_(incident, OBC::RecoveryAction::SAFE_FALLBACK);
    actions.push_back({ExternalActionKind::APPLY_SAFE, incident.source, OBC::RecoveryAction::SAFE_FALLBACK, modeSource});
    return true;
}

void RecoveryExecutor::prepareEpsResetLocked_(OBC::RecoveryIncidentState& incident,
                                              std::vector<ExternalActionRequest>& actions) {
    if (incident.resetIssued) {
        return;
    }
    incident.resetIssued = true;
    incident.firstActionIssued = true;
    incident.currentLevel = OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE;
    if (incident.highestLevel.e < incident.currentLevel.e) {
        incident.highestLevel = incident.currentLevel;
    }
    incident.lastAction = OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET;
    this->issueActionRequested_(incident, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET);
    actions.push_back({ExternalActionKind::EPS_RESET, incident.source, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET});
}

void RecoveryExecutor::prepareAdcsResetLocked_(OBC::RecoveryIncidentState& incident,
                                               std::vector<ExternalActionRequest>& actions) {
    if (incident.resetIssued) {
        return;
    }
    incident.resetIssued = true;
    incident.firstActionIssued = true;
    incident.currentLevel = OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE;
    if (incident.highestLevel.e < incident.currentLevel.e) {
        incident.highestLevel = incident.currentLevel;
    }
    incident.lastAction = OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET;
    this->issueActionRequested_(incident, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET);
    actions.push_back({ExternalActionKind::ADCS_RESET, incident.source, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET});
}

void RecoveryExecutor::prepareCommFailoverLocked_(OBC::RecoveryIncidentState& incident,
                                                  std::vector<ExternalActionRequest>& actions) {
    if (incident.commFailoverIssued) {
        return;
    }
    incident.commFailoverIssued = true;
    incident.firstActionIssued = true;
    incident.currentLevel = OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE;
    if (incident.highestLevel.e < incident.currentLevel.e) {
        incident.highestLevel = incident.currentLevel;
    }
    incident.lastAction = OBC::RecoveryAction::COMM_LINK_FAILOVER;
    this->issueActionRequested_(incident, OBC::RecoveryAction::COMM_LINK_FAILOVER);
    actions.push_back({ExternalActionKind::COMM_FAILOVER, incident.source, OBC::RecoveryAction::COMM_LINK_FAILOVER});
}

void RecoveryExecutor::prepareRebootLocked_(OBC::RecoveryIncidentState& incident,
                                            std::vector<ExternalActionRequest>& actions) {
    if (incident.rebootPending) {
        return;
    }
    incident.currentLevel = OBC::RecoveryLevel::R6_OBC_REBOOT;
    if (incident.highestLevel.e < incident.currentLevel.e) {
        incident.highestLevel = incident.currentLevel;
    }
    incident.lastAction = OBC::RecoveryAction::OBC_REBOOT;
    this->issueActionRequested_(incident, OBC::RecoveryAction::OBC_REBOOT);
    this->log_WARNING_HI_RECOVERY_REBOOT_PENDING(incident.source, incident.currentLevel);
    this->appendPersistentFaultRecordLocked_(
        OBC::PersistentFaultRecordKind::REBOOT_PENDING, incident, OBC::RecoveryAction::OBC_REBOOT);
    actions.push_back({ExternalActionKind::RECORD_REBOOT_INTENT, incident.source, OBC::RecoveryAction::OBC_REBOOT});
}

void RecoveryExecutor::executeExternalActions_(std::vector<ExternalActionRequest>& actions) {
    while (!actions.empty()) {
        const ExternalActionRequest action = actions.front();
        actions.erase(actions.begin());
        std::vector<ExternalActionRequest> followups = {};
        this->executeExternalAction_(action, followups);
        actions.insert(actions.end(), followups.begin(), followups.end());
    }
}

void RecoveryExecutor::executeExternalAction_(const ExternalActionRequest& action,
                                              std::vector<ExternalActionRequest>& followups) {
    switch (action.kind) {
        case ExternalActionKind::APPLY_SAFE: {
            if (this->m_modeControl != nullptr) {
                this->m_modeControl->applyModeForInternalSource(OBC::SatMode::SAFE, action.modeSource);
            }
            std::lock_guard<std::mutex> lock(this->m_mutex);
            OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
            if (incident != nullptr) {
                saturatingIncrement(this->m_status.totalSafeFallbacks);
                this->issueActionExecuted_(*incident, OBC::RecoveryAction::SAFE_FALLBACK, 0U);
                this->updateStatusSummaryLocked_(incident);
            }
            return;
        }
        case ExternalActionKind::EPS_RESET: {
            OBC::EPS::StatusData status = {};
            const Fw::CmdResponse response = this->m_epsControl == nullptr
                                                 ? Fw::CmdResponse(Fw::CmdResponse::EXECUTION_ERROR)
                                                 : this->m_epsControl->resetEpsForRecovery(status);
            std::lock_guard<std::mutex> lock(this->m_mutex);
            OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
            if (incident != nullptr) {
                saturatingIncrement(this->m_status.totalResetActions);
                this->issueActionExecuted_(
                    *incident, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET, static_cast<U32>(response.e));
                this->updateStatusSummaryLocked_(incident);
            }
            return;
        }
        case ExternalActionKind::ADCS_RESET: {
            OBC::ADCS::StateData state = {};
            const Fw::CmdResponse response = this->m_adcsControl == nullptr
                                                 ? Fw::CmdResponse(Fw::CmdResponse::EXECUTION_ERROR)
                                                 : this->m_adcsControl->resetAdcsForRecovery(state);
            std::lock_guard<std::mutex> lock(this->m_mutex);
            OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
            if (incident != nullptr) {
                saturatingIncrement(this->m_status.totalResetActions);
                this->issueActionExecuted_(
                    *incident, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET, static_cast<U32>(response.e));
                this->updateStatusSummaryLocked_(incident);
            }
            return;
        }
        case ExternalActionKind::COMM_FAILOVER: {
            const OBC::RecoveryCommActionResult result =
                this->m_commControl == nullptr ? OBC::RecoveryCommActionResult{} : this->m_commControl->performRecoveryLinkFailoverForRuntime();
            {
                std::lock_guard<std::mutex> lock(this->m_mutex);
                OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
                if (incident != nullptr) {
                    this->issueActionExecuted_(*incident, OBC::RecoveryAction::COMM_LINK_FAILOVER, commActionResponseCode(result));
                    this->updateStatusSummaryLocked_(incident);
                }
            }

            if (result.noHealthyBackup) {
                const OBC::SatMode currentMode = this->currentModeForRuntime_();
                std::lock_guard<std::mutex> lock(this->m_mutex);
                OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
                if (incident != nullptr && incident->active && !incident->rebootPending) {
                    static_cast<void>(this->prepareSafeFallbackLocked_(
                        *incident, currentMode, OBC::ModeApplySource::FdirSubsystemFault, followups));
                    this->updateStatusSummaryLocked_(incident);
                }
            }
            return;
        }
        case ExternalActionKind::RECORD_PROCESS_RESTART_INTENT: {
            const bool recorded = this->m_bootControl != nullptr &&
                                  this->m_bootControl->recordRecoveryRestartIntentForRuntime(
                                      recoveryResetCauseForSource(action.source),
                                      action.source,
                                      OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
            const OBC::SatMode currentMode = this->currentModeForRuntime_();
            std::lock_guard<std::mutex> lock(this->m_mutex);
            OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
            if (incident != nullptr) {
                if (recorded) {
                    incident->processRestartPending = true;
                    this->m_processRestartRequested = true;
                    this->m_status.pendingProcessRestart = true;
                    saturatingIncrement(this->m_status.totalProcessRestarts);
                    this->issueActionExecuted_(*incident, OBC::RecoveryAction::PROCESS_RESTART, 0U);
                } else {
                    this->issueActionExecuted_(
                        *incident, OBC::RecoveryAction::PROCESS_RESTART, static_cast<U32>(Fw::CmdResponse::EXECUTION_ERROR));
                    if (incident->active && !incident->processRestartPending && !incident->rebootPending) {
                        static_cast<void>(this->prepareSafeFallbackLocked_(
                            *incident, currentMode, action.modeSource, followups, true));
                    }
                }
                this->updateStatusSummaryLocked_(incident);
            }
            return;
        }
        case ExternalActionKind::RECORD_REBOOT_INTENT: {
            const bool recorded = this->m_bootControl != nullptr &&
                                  this->m_bootControl->recordRecoveryRestartIntentForRuntime(
                                      recoveryResetCauseForSource(action.source),
                                      action.source,
                                      OBC::RecoveryLevel::R6_OBC_REBOOT);
            std::lock_guard<std::mutex> lock(this->m_mutex);
            OBC::RecoveryIncidentState* incident = this->incidentForSource_(action.source);
            if (incident != nullptr) {
                if (recorded) {
                    const bool requestRuntimeExit =
                        !(this->m_hardwareWatchdogModeEnabled && recoveryIncidentSourceIsWatchdog(action.source));
                    incident->rebootPending = true;
                    this->m_rebootRequested = this->m_rebootRequested || requestRuntimeExit;
                    this->m_status.pendingReboot = true;
                    saturatingIncrement(this->m_status.totalReboots);
                    this->log_WARNING_HI_RECOVERY_REBOOT_ISSUED(incident->source, incident->currentLevel);
                    this->issueActionExecuted_(*incident, OBC::RecoveryAction::OBC_REBOOT, 0U);
                    this->appendPersistentFaultRecordLocked_(
                        OBC::PersistentFaultRecordKind::REBOOT_ISSUED, *incident, OBC::RecoveryAction::OBC_REBOOT);
                } else {
                    this->issueActionExecuted_(
                        *incident, OBC::RecoveryAction::OBC_REBOOT, static_cast<U32>(Fw::CmdResponse::EXECUTION_ERROR));
                }
                this->updateStatusSummaryLocked_(incident);
            }
            return;
        }
        case ExternalActionKind::ACK_STABLE: {
            const bool acked = this->m_bootControl != nullptr && this->m_bootControl->acknowledgeRuntimeStableForRuntime();
            std::lock_guard<std::mutex> lock(this->m_mutex);
            if (acked) {
                this->m_stableAcked = true;
                this->m_stableTicks = 0U;
            }
            return;
        }
        case ExternalActionKind::APPLY_BOOT_SAFE_FALLBACK:
        default:
            return;
    }
}

void RecoveryExecutor::appendPersistentFaultRecordLocked_(OBC::PersistentFaultRecordKind kind,
                                                          const OBC::RecoveryIncidentState& incident,
                                                          OBC::RecoveryAction action,
                                                          U32 detail,
                                                          U32 flags) {
    if (this->m_faultRecorder == nullptr) {
        return;
    }

    OBC::PersistentFaultRecord record = {};
    record.kind = kind;
    record.source = incident.source;
    record.level = incident.currentLevel;
    record.action = action;
    record.resetCause = recoveryResetCauseForSource(incident.source);
    record.timestampSec = OBC::persistentFaultTimestampSec(this->getTime());
    record.uptimeSec = this->m_bootControl == nullptr ? 0U : this->m_bootControl->getUptimeForRuntime();
    record.bootCount = this->m_bootControl == nullptr ? 0U : this->m_bootControl->getBootCountForRuntime();
    record.consecutiveResetCount =
        this->m_bootControl == nullptr ? 0U : this->m_bootControl->getConsecutiveResetCountForRuntime();
    record.detail = detail;
    record.flags = flags;
    (void)this->m_faultRecorder->appendPersistentFaultRecordForRuntime(record);
}

void RecoveryExecutor::handleSubsystemFault_(OBC::RecoveryIncidentSource source, OBC::SatMode currentMode) {
    std::vector<ExternalActionRequest> actions = {};
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        OBC::RecoveryIncidentState* incident = this->incidentForSource_(source);
        if (incident == nullptr || incident->active || this->m_processRestartRequested || this->m_rebootRequested ||
            this->m_exitRequestConsumed) {
            return;
        }

        this->openIncident_(*incident, source);
        if (incident->relatchCount > 0U) {
            this->prepareRebootLocked_(*incident, actions);
            this->updateStatusSummaryLocked_(incident);
        } else {
            switch (source.e) {
                case OBC::RecoveryIncidentSource::EPS_TIMEOUT:
                    this->prepareEpsResetLocked_(*incident, actions);
                    static_cast<void>(this->prepareSafeFallbackLocked_(
                        *incident, currentMode, OBC::ModeApplySource::FdirSubsystemFault, actions));
                    break;
                case OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT:
                case OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS:
                    this->prepareAdcsResetLocked_(*incident, actions);
                    if (this->m_bootControl != nullptr && this->m_bootControl->isBootSafeFallbackRequiredForRuntime()) {
                        static_cast<void>(this->prepareSafeFallbackLocked_(
                            *incident, currentMode, OBC::ModeApplySource::FdirSubsystemFault, actions, true));
                    }
                    break;
                case OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE:
                case OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT:
                    this->prepareCommFailoverLocked_(*incident, actions);
                    break;
                default:
                    break;
            }
            this->updateStatusSummaryLocked_(incident);
        }
    }
    this->executeExternalActions_(actions);
}

void RecoveryExecutor::clearIncidentBySource_(OBC::RecoveryIncidentSource source) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    OBC::RecoveryIncidentState* incident = this->incidentForSource_(source);
    if (incident == nullptr || !incident->active) {
        return;
    }
    this->closeIncident_(*incident);
    this->updateStatusSummaryLocked_(incident);
}

bool RecoveryExecutor::isSubsystemRecoverySource_(OBC::RecoveryIncidentSource source) const {
    return source == OBC::RecoveryIncidentSource::EPS_TIMEOUT ||
           source == OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT ||
           source == OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS ||
           source == OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE ||
           source == OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT;
}

bool RecoveryExecutor::isWatchdogRecoverySource_(OBC::RecoveryIncidentSource source) const {
    return source == OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE ||
           source == OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR ||
           source == OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY ||
           source == OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER ||
           source == OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR;
}

bool RecoveryExecutor::isUnclearedTimeoutRecoverySource_(OBC::RecoveryIncidentSource source) const {
    return this->isSubsystemRecoverySource_(source) || this->isWatchdogRecoverySource_(source);
}

void RecoveryExecutor::updateStatusSummaryLocked_(const OBC::RecoveryIncidentState* focusIncident) {
    this->m_status.activeIncidentCount = 0U;
    this->m_status.pendingProcessRestart = false;
    this->m_status.pendingReboot = false;
    const OBC::RecoveryIncidentState* summaryIncident =
        (focusIncident != nullptr &&
         (focusIncident->active || focusIncident->processRestartPending || focusIncident->rebootPending))
            ? focusIncident
            : nullptr;
    for (const auto& incident : this->m_status.incidents) {
        if (incident.active) {
            saturatingIncrement(this->m_status.activeIncidentCount);
        }
        this->m_status.pendingProcessRestart = this->m_status.pendingProcessRestart || incident.processRestartPending;
        this->m_status.pendingReboot = this->m_status.pendingReboot || incident.rebootPending;
        if ((incident.active || incident.processRestartPending || incident.rebootPending) &&
            (summaryIncident == nullptr || incident.currentLevel.e > summaryIncident->currentLevel.e ||
             (incident.currentLevel == summaryIncident->currentLevel && incident.epoch > summaryIncident->epoch))) {
            summaryIncident = &incident;
        }
    }

    if (summaryIncident != nullptr) {
        this->m_status.activeSource = summaryIncident->source;
        this->m_status.currentLevel = summaryIncident->currentLevel;
        this->m_status.highestLevel = summaryIncident->highestLevel;
        this->m_status.lastAction = summaryIncident->lastAction;
        this->m_status.relatchCount = summaryIncident->relatchCount;
    } else if (this->m_status.activeIncidentCount == 0U) {
        this->m_status.activeSource = OBC::RecoveryIncidentSource::NONE;
        this->m_status.currentLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
        this->m_status.highestLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
        this->m_status.lastAction = OBC::RecoveryAction::NONE;
        this->m_status.relatchCount = 0U;
    }
}

}  // namespace OBC

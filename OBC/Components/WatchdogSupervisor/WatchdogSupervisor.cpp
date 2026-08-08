#include "OBC/Components/WatchdogSupervisor/WatchdogSupervisor.hpp"

#include <limits>

namespace OBC {

namespace {

U32 saturatingIncrement(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
    return value;
}

bool hasActiveAggregateFault(const OBC::WatchdogState state) {
    return state == OBC::WatchdogState::LATCHED_FAULT || state == OBC::WatchdogState::FEED_SUPPRESSED;
}

}  // namespace

WatchdogSupervisor::WatchdogSupervisor(const char* const compName)
    : WatchdogSupervisorComponentBase(compName),
      m_modeControl(nullptr),
      m_recoverySink(nullptr),
      m_resourceMonitoringEnabled(false),
      m_resourceThresholds{80.0F, 256.0F, 4.0F},
      m_resourceThresholdExceeded{false, false, false},
      m_runtimeStatus(),
      m_safeRequestIssuedForCurrentFault(false) {
    this->initializeSources_();
}

WatchdogSupervisor::~WatchdogSupervisor() = default;

void WatchdogSupervisor::configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                                          OBC::IRecoveryRequestSink* recoverySink) {
    this->m_modeControl = modeControl;
    this->m_recoverySink = recoverySink;
}

void WatchdogSupervisor::setEnabledForRuntime(bool enable) {
    {
        std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
        this->m_resourceMonitoringEnabled = enable;
        if (!enable) {
            this->m_resourceThresholdExceeded.fill(false);
        }
    }
    this->log_ACTIVITY_HI_HEALTH_CHECKING_SET(enable);
}

Fw::CmdResponse WatchdogSupervisor::setThresholdForRuntime(OBC::HealthItem item, F32 value) {
    if (!item.isValid()) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    {
        std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
        const std::size_t index = this->thresholdIndex_(item);
        this->m_resourceThresholds.at(index) = value;
        this->m_resourceThresholdExceeded.at(index) = false;
    }
    this->log_ACTIVITY_HI_HEALTH_THRESHOLD_UPDATED(item, value);
    return Fw::CmdResponse::OK;
}

void WatchdogSupervisor::updateResourceSample(F32 cpuUsage, F32 memRssMb) {
    bool resourceMonitoringEnabled = false;
    F32 cpuThreshold = 0.0F;
    F32 memThreshold = 0.0F;
    bool emitCpuWarning = false;
    bool emitMemWarning = false;
    {
        std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
        resourceMonitoringEnabled = this->m_resourceMonitoringEnabled;
        const std::size_t cpuIndex = this->thresholdIndex_(OBC::HealthItem::CPU_USAGE);
        const std::size_t memIndex = this->thresholdIndex_(OBC::HealthItem::MEM_RSS_MB);
        cpuThreshold = this->m_resourceThresholds.at(cpuIndex);
        memThreshold = this->m_resourceThresholds.at(memIndex);

        const bool cpuExceeded = cpuUsage > cpuThreshold;
        const bool memExceeded = memRssMb > memThreshold;
        emitCpuWarning = resourceMonitoringEnabled && cpuExceeded && !this->m_resourceThresholdExceeded.at(cpuIndex);
        emitMemWarning = resourceMonitoringEnabled && memExceeded && !this->m_resourceThresholdExceeded.at(memIndex);
        if (resourceMonitoringEnabled) {
            this->m_resourceThresholdExceeded.at(cpuIndex) = cpuExceeded;
            this->m_resourceThresholdExceeded.at(memIndex) = memExceeded;
        }
    }

    this->tlmWrite_SYS_CPU_USAGE(cpuUsage);
    this->tlmWrite_SYS_MEM_RSS_MB(memRssMb);

    if (emitCpuWarning) {
        this->log_WARNING_HI_SYS_RESOURCE_DEGRADED(OBC::HealthItem::CPU_USAGE, cpuUsage, cpuThreshold);
    }
    if (emitMemWarning) {
        this->log_WARNING_HI_SYS_LOW_MEMORY(memRssMb, memThreshold);
    }
}

OBC::WatchdogRuntimeSnapshot WatchdogSupervisor::getStatusForRuntime() const {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    return this->m_runtimeStatus;
}

Fw::CmdResponse WatchdogSupervisor::setConfigForRuntime(OBC::WatchdogSource source,
                                                        bool enabled,
                                                        U32 warningTicks,
                                                        U32 safeTicks,
                                                        U32 suppressTicks) {
    if (!source.isValid()) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    OBC::WatchdogSourceConfig config = {};
    config.enabled = enabled;
    config.warningTicks = warningTicks;
    config.safeTicks = safeTicks;
    config.suppressTicks = suppressTicks;
    if (!OBC::WatchdogPolicy::isValidConfig(config)) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    OBC::WatchdogRuntimeSnapshot snapshot = {};
    {
        std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
        auto& sourceSnapshot = this->m_runtimeStatus.sources.at(watchdogSourceIndex(source));
        sourceSnapshot.config = config;
        sourceSnapshot.state = OBC::WatchdogPolicy::stateForAge(config, sourceSnapshot.ageTicks);
        if (!config.enabled) {
            sourceSnapshot.state = OBC::WatchdogState::DISABLED;
            sourceSnapshot.ageTicks = 0U;
            sourceSnapshot.beatPending = false;
        }
        const OBC::WatchdogAggregateRollup rollup =
            OBC::WatchdogPolicy::rollup(this->m_runtimeStatus.sources, this->m_safeRequestIssuedForCurrentFault);
        if (!hasActiveAggregateFault(rollup.aggregateState)) {
            this->m_safeRequestIssuedForCurrentFault = false;
        }
        this->applyAggregateRollup_(rollup);
        snapshot = this->m_runtimeStatus;
    }
    this->publishTelemetry_(snapshot);

    this->log_ACTIVITY_HI_WATCHDOG_CONFIG_UPDATED(source, enabled, warningTicks, safeTicks, suppressTicks);
    return Fw::CmdResponse::OK;
}

bool WatchdogSupervisor::setProbeBeatSuppressionForRuntime(OBC::WatchdogSource source, bool suppressed) {
    if (!source.isValid()) {
        return false;
    }

    bool emitFeedSuppressedEvent = false;
    bool emitFeedEligibilityEvent = false;
    bool notifySuppression = false;
    bool notifyClear = false;
    U32 suppressedAgeTicks = 0U;
    U32 suppressTicks = 0U;
    OBC::WatchdogAggregateRollup rollup = {};
    OBC::WatchdogRuntimeSnapshot snapshot = {};
    {
        std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
        auto& status = this->m_runtimeStatus.sources.at(watchdogSourceIndex(source));
        const OBC::WatchdogState previousAggregateState = this->m_runtimeStatus.aggregateState;
        const bool wasSuppressed = status.state == OBC::WatchdogState::FEED_SUPPRESSED;
        status.probeSuppressed = suppressed;
        status.beatPending = false;
        if (suppressed) {
            if (!wasSuppressed && status.config.enabled) {
                status.ageTicks = status.config.suppressTicks;
                status.state = OBC::WatchdogState::FEED_SUPPRESSED;
                status.suppressionCount = saturatingIncrement(status.suppressionCount);
                emitFeedSuppressedEvent = true;
                notifySuppression = true;
                suppressedAgeTicks = status.ageTicks;
                suppressTicks = status.config.suppressTicks;
            }
        } else if (wasSuppressed) {
            this->resetSourceAfterProbeSuppressionClear_(status);
            notifyClear = true;
        }
        rollup = OBC::WatchdogPolicy::rollup(this->m_runtimeStatus.sources, this->m_safeRequestIssuedForCurrentFault);
        if (!hasActiveAggregateFault(rollup.aggregateState)) {
            this->m_safeRequestIssuedForCurrentFault = false;
        }
        emitFeedEligibilityEvent =
            rollup.feedEligible != this->m_runtimeStatus.feedEligible ||
            rollup.recoveryLevel != this->m_runtimeStatus.recoveryLevel ||
            rollup.faultMask != this->m_runtimeStatus.faultMask ||
            rollup.suppressMask != this->m_runtimeStatus.suppressMask;
        if (rollup.aggregateState == OBC::WatchdogState::FEED_SUPPRESSED &&
            previousAggregateState != OBC::WatchdogState::FEED_SUPPRESSED) {
            this->m_runtimeStatus.feedSuppressCount = saturatingIncrement(this->m_runtimeStatus.feedSuppressCount);
        }
        this->applyAggregateRollup_(rollup);
        snapshot = this->m_runtimeStatus;
    }
    this->publishTelemetry_(snapshot);
    this->log_ACTIVITY_HI_WATCHDOG_PROBE_SUPPRESSION_UPDATED(source, suppressed);
    if (emitFeedSuppressedEvent) {
        this->log_WARNING_HI_WATCHDOG_SOURCE_FEED_SUPPRESSED(source, suppressedAgeTicks, suppressTicks);
    }
    if (emitFeedEligibilityEvent) {
        this->emitFeedEligibilityEvent_(rollup);
    }
    if (this->m_recoverySink != nullptr) {
        if (notifySuppression) {
            this->m_recoverySink->submitWatchdogSuppression(source);
        } else if (notifyClear) {
            this->m_recoverySink->clearWatchdogFault(source);
        }
    }
    return true;
}

void WatchdogSupervisor::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    bool requestSafe = false;
    bool strokeWatchdog = false;

    OBC::SatMode currentMode = OBC::SatMode::SAFE;
    if (this->m_modeControl != nullptr) {
        currentMode = this->m_modeControl->getModeForRuntime();
    }
    OBC::WatchdogRuntimeSnapshot snapshot = {};
    {
        std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
        const OBC::WatchdogState previousAggregateState = this->m_runtimeStatus.aggregateState;

        for (std::size_t sourceIndex = 0; sourceIndex < this->m_runtimeStatus.sources.size(); sourceIndex++) {
            auto& source = this->m_runtimeStatus.sources.at(sourceIndex);
            const bool beatObserved = source.beatPending;
            source.beatPending = false;
            const OBC::WatchdogSourceEvaluation evaluation =
                OBC::WatchdogPolicy::evaluateSource(source.config, source, beatObserved, currentMode);

            if (evaluation.crossedWarning) {
                saturatingIncrement(source.warningCount);
                this->log_WARNING_LO_WATCHDOG_SOURCE_WARNING(source.source,
                                                             evaluation.nextAgeTicks,
                                                             source.config.warningTicks);
            }
            if (evaluation.crossedFault) {
                saturatingIncrement(source.faultCount);
                const bool shouldRequestSafe =
                    !requestSafe && !this->m_safeRequestIssuedForCurrentFault && evaluation.shouldRequestSafe;
                this->log_WARNING_HI_WATCHDOG_SOURCE_FAULT_ENTERED(
                    source.source, evaluation.nextAgeTicks, currentMode, shouldRequestSafe);
                if (this->m_recoverySink != nullptr) {
                    this->m_recoverySink->submitWatchdogFault(source.source);
                }
                if (shouldRequestSafe) {
                    requestSafe = true;
                    this->m_safeRequestIssuedForCurrentFault = true;
                    saturatingIncrement(this->m_runtimeStatus.safeRequestCount);
                }
            }
            if (evaluation.crossedSuppression) {
                saturatingIncrement(source.suppressionCount);
                this->log_WARNING_HI_WATCHDOG_SOURCE_FEED_SUPPRESSED(
                    source.source, evaluation.nextAgeTicks, source.config.suppressTicks);
                if (this->m_recoverySink != nullptr) {
                    this->m_recoverySink->submitWatchdogSuppression(source.source);
                }
            }
            if (evaluation.recovered) {
                saturatingIncrement(source.recoveryCount);
                this->log_ACTIVITY_HI_WATCHDOG_SOURCE_RECOVERED(source.source, source.state);
                if (this->m_recoverySink != nullptr) {
                    this->m_recoverySink->clearWatchdogFault(source.source);
                }
            }

            this->updateSourceStatus_(sourceIndex, evaluation);
        }

        OBC::WatchdogAggregateRollup rollup =
            OBC::WatchdogPolicy::rollup(this->m_runtimeStatus.sources, this->m_safeRequestIssuedForCurrentFault);
        if (hasActiveAggregateFault(rollup.aggregateState) && !this->m_safeRequestIssuedForCurrentFault &&
            OBC::WatchdogPolicy::shouldRequestSafeForMode(currentMode)) {
            requestSafe = true;
            this->m_safeRequestIssuedForCurrentFault = true;
            saturatingIncrement(this->m_runtimeStatus.safeRequestCount);
            if (this->m_recoverySink != nullptr) {
                for (const auto& source : this->m_runtimeStatus.sources) {
                    if (source.state == OBC::WatchdogState::LATCHED_FAULT ||
                        source.state == OBC::WatchdogState::FEED_SUPPRESSED) {
                        this->m_recoverySink->submitWatchdogFault(source.source);
                    }
                }
            }
            rollup = OBC::WatchdogPolicy::rollup(this->m_runtimeStatus.sources, this->m_safeRequestIssuedForCurrentFault);
        }
        if (!hasActiveAggregateFault(rollup.aggregateState)) {
            this->m_safeRequestIssuedForCurrentFault = false;
        }
        if (rollup.feedEligible != this->m_runtimeStatus.feedEligible ||
            rollup.recoveryLevel != this->m_runtimeStatus.recoveryLevel ||
            rollup.faultMask != this->m_runtimeStatus.faultMask ||
            rollup.suppressMask != this->m_runtimeStatus.suppressMask) {
            this->emitFeedEligibilityEvent_(rollup);
        }
        if (hasActiveAggregateFault(previousAggregateState) &&
            (rollup.aggregateState == OBC::WatchdogState::HEALTHY ||
             rollup.aggregateState == OBC::WatchdogState::WARNING)) {
            saturatingIncrement(this->m_runtimeStatus.aggregateRecoveryCount);
        }
        if (rollup.aggregateState == OBC::WatchdogState::FEED_SUPPRESSED &&
            previousAggregateState != OBC::WatchdogState::FEED_SUPPRESSED) {
            saturatingIncrement(this->m_runtimeStatus.feedSuppressCount);
        }

        this->applyAggregateRollup_(rollup);
        saturatingIncrement(this->m_runtimeStatus.evaluationCount);
        strokeWatchdog = this->m_runtimeStatus.feedEligible && this->isConnected_watchdogFeedOut_OutputPort(0);
        if (strokeWatchdog) {
            saturatingIncrement(this->m_runtimeStatus.feedStrokeAttemptCount);
        }
        snapshot = this->m_runtimeStatus;
    }
    this->publishTelemetry_(snapshot);

    if (strokeWatchdog) {
        this->watchdogFeedOut_out(0, WATCHDOG_FEED_CODE);
    }
}

void WatchdogSupervisor::beatIn_handler(FwIndexType portNum, U32 code) {
    std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    auto& source = this->m_runtimeStatus.sources.at(static_cast<std::size_t>(portNum));
    source.lastBeatCode = code;
    if (!source.probeSuppressed) {
        source.beatPending = true;
    }
}

void WatchdogSupervisor::HEALTH_ENABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enable) {
    this->setEnabledForRuntime(enable);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void WatchdogSupervisor::HEALTH_SET_THRESHOLD_cmdHandler(FwOpcodeType opCode,
                                                         U32 cmdSeq,
                                                         OBC::HealthItem item,
                                                         F32 value) {
    this->cmdResponse_out(opCode, cmdSeq, this->setThresholdForRuntime(item, value));
}

void WatchdogSupervisor::GET_WATCHDOG_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    const OBC::WatchdogRuntimeSnapshot snapshot = this->getStatusForRuntime();
    this->emitStatusEvents_(snapshot);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void WatchdogSupervisor::SET_WATCHDOG_CONFIG_cmdHandler(FwOpcodeType opCode,
                                                        U32 cmdSeq,
                                                        OBC::WatchdogSource source,
                                                        bool enabled,
                                                        U32 warningTicks,
                                                        U32 safeTicks,
                                                        U32 suppressTicks) {
    this->cmdResponse_out(
        opCode, cmdSeq, this->setConfigForRuntime(source, enabled, warningTicks, safeTicks, suppressTicks));
}

void WatchdogSupervisor::SET_WATCHDOG_PROBE_SUPPRESSION_cmdHandler(FwOpcodeType opCode,
                                                                   U32 cmdSeq,
                                                                   OBC::WatchdogSource source,
                                                                   bool suppressed) {
    this->cmdResponse_out(opCode,
                          cmdSeq,
                          this->setProbeBeatSuppressionForRuntime(source, suppressed)
                              ? Fw::CmdResponse::OK
                              : Fw::CmdResponse::VALIDATION_ERROR);
}

std::size_t WatchdogSupervisor::thresholdIndex_(const OBC::HealthItem& item) const {
    return static_cast<std::size_t>(item.e);
}

void WatchdogSupervisor::initializeSources_() {
    for (std::size_t index = 0; index < this->m_runtimeStatus.sources.size(); index++) {
        auto& source = this->m_runtimeStatus.sources.at(index);
        source.source = watchdogSourceFromIndex(static_cast<FwIndexType>(index));
        source.config = OBC::WatchdogPolicy::defaultConfig(source.source);
        source.state = OBC::WatchdogPolicy::stateForAge(source.config, 0U);
    }
    this->applyAggregateRollup_(
        OBC::WatchdogPolicy::rollup(this->m_runtimeStatus.sources, this->m_safeRequestIssuedForCurrentFault));
}

void WatchdogSupervisor::updateSourceStatus_(std::size_t sourceIndex, const OBC::WatchdogSourceEvaluation& evaluation) {
    auto& source = this->m_runtimeStatus.sources.at(sourceIndex);
    source.ageTicks = evaluation.nextAgeTicks;
    source.state = evaluation.nextState;
}

void WatchdogSupervisor::publishTelemetry_(const OBC::WatchdogRuntimeSnapshot& snapshot) {
    this->tlmWrite_WATCHDOG_AGGREGATE_STATE(snapshot.aggregateState);
    this->tlmWrite_WATCHDOG_RECOVERY_LEVEL(snapshot.recoveryLevel);
    this->tlmWrite_WATCHDOG_FEED_ELIGIBLE(snapshot.feedEligible);
    this->tlmWrite_WATCHDOG_WARNING_MASK(snapshot.warningMask);
    this->tlmWrite_WATCHDOG_FAULT_MASK(snapshot.faultMask);
    this->tlmWrite_WATCHDOG_SUPPRESS_MASK(snapshot.suppressMask);
    this->tlmWrite_WATCHDOG_SAFE_REQUEST_TOTAL(snapshot.safeRequestCount);
    this->tlmWrite_WATCHDOG_FEED_SUPPRESS_TOTAL(snapshot.feedSuppressCount);
    this->tlmWrite_WATCHDOG_FEED_STROKE_ATTEMPT_TOTAL(snapshot.feedStrokeAttemptCount);
    this->tlmWrite_WATCHDOG_AGGREGATE_RECOVERY_TOTAL(snapshot.aggregateRecoveryCount);

    const auto& epsBridge = snapshot.sources.at(watchdogSourceIndex(OBC::WatchdogSource::EPS_BRIDGE));
    const auto& epsFdir = snapshot.sources.at(watchdogSourceIndex(OBC::WatchdogSource::EPS_FDIR));
    const auto& modeSafety = snapshot.sources.at(watchdogSourceIndex(OBC::WatchdogSource::MODE_SAFETY));
    const auto& commController = snapshot.sources.at(watchdogSourceIndex(OBC::WatchdogSource::COMM_CONTROLLER));
    const auto& adcsFdir = snapshot.sources.at(watchdogSourceIndex(OBC::WatchdogSource::ADCS_FDIR));

    this->tlmWrite_WATCHDOG_EPS_BRIDGE_STATE(epsBridge.state);
    this->tlmWrite_WATCHDOG_EPS_BRIDGE_AGE_TICKS(epsBridge.ageTicks);
    this->tlmWrite_WATCHDOG_EPS_FDIR_STATE(epsFdir.state);
    this->tlmWrite_WATCHDOG_EPS_FDIR_AGE_TICKS(epsFdir.ageTicks);
    this->tlmWrite_WATCHDOG_MODE_SAFETY_STATE(modeSafety.state);
    this->tlmWrite_WATCHDOG_MODE_SAFETY_AGE_TICKS(modeSafety.ageTicks);
    this->tlmWrite_WATCHDOG_COMM_CONTROLLER_STATE(commController.state);
    this->tlmWrite_WATCHDOG_COMM_CONTROLLER_AGE_TICKS(commController.ageTicks);
    this->tlmWrite_WATCHDOG_ADCS_FDIR_STATE(adcsFdir.state);
    this->tlmWrite_WATCHDOG_ADCS_FDIR_AGE_TICKS(adcsFdir.ageTicks);
}

void WatchdogSupervisor::emitStatusEvents_(const OBC::WatchdogRuntimeSnapshot& snapshot) {
    this->log_ACTIVITY_HI_WATCHDOG_STATUS(snapshot.aggregateState,
                                          snapshot.recoveryLevel,
                                          snapshot.feedEligible,
                                          snapshot.warningMask,
                                          snapshot.faultMask,
                                          snapshot.suppressMask);
    for (const auto& source : snapshot.sources) {
        this->log_ACTIVITY_HI_WATCHDOG_SOURCE_STATUS(source.source,
                                                     source.config.enabled,
                                                     source.state,
                                                     source.ageTicks,
                                                     source.config.warningTicks,
                                                     source.config.safeTicks,
                                                     source.config.suppressTicks);
    }
}

void WatchdogSupervisor::emitFeedEligibilityEvent_(const OBC::WatchdogAggregateRollup& rollup) {
    this->log_ACTIVITY_HI_WATCHDOG_FEED_ELIGIBILITY_CHANGED(
        rollup.feedEligible, rollup.faultMask, rollup.suppressMask, rollup.recoveryLevel);
}

void WatchdogSupervisor::applyAggregateRollup_(const OBC::WatchdogAggregateRollup& rollup) {
    this->m_runtimeStatus.aggregateState = rollup.aggregateState;
    this->m_runtimeStatus.recoveryLevel = rollup.recoveryLevel;
    this->m_runtimeStatus.feedEligible = rollup.feedEligible;
    this->m_runtimeStatus.warningMask = rollup.warningMask;
    this->m_runtimeStatus.faultMask = rollup.faultMask;
    this->m_runtimeStatus.suppressMask = rollup.suppressMask;
}

void WatchdogSupervisor::resetSourceAfterProbeSuppressionClear_(OBC::WatchdogSourceSnapshot& source) {
    source.ageTicks = 0U;
    source.state = source.config.enabled ? OBC::WatchdogState::HEALTHY : OBC::WatchdogState::DISABLED;
}

}  // namespace OBC

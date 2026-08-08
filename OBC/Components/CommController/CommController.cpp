#include "OBC/Components/CommController/CommController.hpp"
#include "OBC/Components/CspRuntimeOwner/AsyncCspRuntimeOwner.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/SecureAuthRevocationReasonEnumAc.hpp"
#include "OBC/Types/WatchdogSourceEnumAc.hpp"

#include <cstdlib>

namespace OBC {

constexpr U32 CommController::UHF_BEACON_SUPPRESS_TIMEOUT_TICKS;

namespace {

constexpr FwIndexType SBAND_INGRESS_PORT = 0;
constexpr FwIndexType UHF_INGRESS_PORT = 1;
constexpr U32 UHF_FILE_LAUNCH_QUIESCE_TICKS = 12U;

U32 sanitizeProbePeriodTicks(U32 ticks) {
    return ticks == 0U ? 1U : ticks;
}

U32 sanitizeProbeFailureThreshold(U32 threshold) {
    return threshold == 0U ? 1U : threshold;
}

enum class CommPolicyTransitionReason : U32 {
    OPERATOR_SWITCH = 1,
    LINK_FAILOVER = 2,
    PASS_END = 3,
    RECOVERY_NO_BACKUP = 4,
    INITIAL_BOOTSTRAP = 5,
};

AuthorityConfig withRole(const AuthorityConfig& config, AuthorityLinkRole role) {
    AuthorityConfig updated = config;
    updated.role = role;
    return updated;
}

bool fileIngressAllowedForConfig(const AuthorityConfig& config) {
    return config.valid &&
           ((config.identity == AuthorityLinkIdentity::SBAND && config.role == AuthorityLinkRole::PRIMARY) ||
            (config.identity == AuthorityLinkIdentity::UHF && config.role == AuthorityLinkRole::PRIMARY_AFTER_FAILOVER));
}

}  // namespace

CommController::CommController(const char* const compName, OBC::CSP::ICspRuntime& runtime)
    : CommControllerComponentBase(compName),
      m_activeBand(OBC::CommBand::SBAND),
      m_passActive(false),
      m_passRemainingSec(0U),
      m_totalPasses(0U),
      m_primaryCommandLink(OBC::CommBand::SBAND),
      m_primaryTelemetryLink(OBC::CommBand::SBAND),
      m_primaryFileLink(OBC::CommBand::SBAND),
      m_primaryLinkSelectionReason(0U),
      m_sbandAvailable(false),
      m_uhfAvailable(false),
      m_sessionRevokeTotal(0U),
      m_downlinkRejectTotal(0U),
      m_groundLinkHealthProvider(nullptr),
      m_recoverySink(nullptr),
      m_commandIngressAuthority(nullptr),
      m_beaconSuppressControl(nullptr),
      m_commEgressMux(nullptr),
      m_commSubsystemHealthProbe(nullptr),
      m_cspRuntimeOwner(nullptr),
      m_sbandSubsystemHealthNodeId(0U),
      m_uhfSubsystemHealthNodeId(0U),
      m_sbandIngressConfig(),
      m_sbandBackupIngressConfig(),
      m_uhfBackupIngressConfig(),
      m_uhfPrimaryIngressConfig(),
      m_downlinkScheduler(),
      m_deferredLaunchPending(false),
      m_deferredLaunchTicksRemaining(0U),
      m_deferredLaunchRequest(),
      m_fdirFaultLatched(false),
      m_fdirFaultKind(OBC::CommFdirFaultKind::NONE),
      m_consecutivePrimaryUnavailable(0U),
      m_consecutivePrimaryTransportGrowth(0U),
      m_lastSbandHealth(),
      m_lastUhfHealth(),
      m_sbandSubsystemProbeCooldownTicks(0U),
      m_uhfSubsystemProbeCooldownTicks(0U),
      m_sbandSubsystemProbeFailureStreak(0U),
      m_uhfSubsystemProbeFailureStreak(0U),
      m_sbandAsyncSubsystemProbe(),
      m_uhfAsyncSubsystemProbe(),
      m_recoveryFailoverTotal(0U),
      m_recoveryOwnerClearTotal(0U),
      m_sbandLiveObservabilitySessionActive(false),
      m_sbandLiveObservabilityIngressPort(SBAND_INGRESS_PORT),
      m_sbandLiveObservabilityRole(AuthorityLinkRole::UNKNOWN),
      m_sbandLiveObservabilitySessionId(0U),
      m_sbandLiveObservabilityLastAcceptedSequence(0U),
      m_lastPublishedSbandLiveObservabilityActive(false),
      m_lastPublishedSbandLiveObservabilityReason(OBC::CommLiveObservabilityStateReason::NONE),
      m_lastPublishedSbandLiveObservabilityIngressPort(SBAND_INGRESS_PORT),
      m_lastPublishedSbandLiveObservabilityRole(AuthorityLinkRole::UNKNOWN),
      m_lastPublishedSbandLiveObservabilitySessionId(0U),
      m_uhfBeaconSuppressActive(false),
      m_uhfBeaconSuppressIngressPort(UHF_INGRESS_PORT),
      m_uhfBeaconSuppressRole(AuthorityLinkRole::UNKNOWN),
      m_uhfBeaconSuppressSessionId(0U),
      m_uhfBeaconSuppressLastAcceptedSequence(0U),
      m_uhfBeaconSuppressRemainingTicks(0U),
      m_reliableTransfer(runtime) {
    this->updateCachedRuntimeStateLocked_();
}

CommController::~CommController() = default;

void CommController::tickForTest() {
    this->schedIn_handler(0, 0U);
}

void CommController::configureRuntime(GroundLinkHealthProvider* groundLinkHealthProvider,
                                      OBC::IRecoveryRequestSink* recoverySink,
                                      CommandIngressAuthority* commandIngressAuthority,
                                      OBC::ICommBeaconSuppressControl* beaconSuppressControl,
                                      CommEgressMux* commEgressMux,
                                      OBC::ICommSubsystemHealthProbe* commSubsystemHealthProbe,
                                      U16 sbandSubsystemHealthNodeId,
                                      U16 uhfSubsystemHealthNodeId,
                                      const OBC::CommSubsystemFdirConfig& subsystemFdirConfig,
                                      OBC::CommBand initialPrimaryBand,
                                      const AuthorityConfig& sbandIngressConfig,
                                      const AuthorityConfig& uhfBackupIngressConfig) {
    this->lockComponent_();
    this->m_groundLinkHealthProvider = groundLinkHealthProvider;
    this->m_recoverySink = recoverySink;
    this->m_commandIngressAuthority = commandIngressAuthority;
    this->m_beaconSuppressControl = beaconSuppressControl;
    this->m_commEgressMux = commEgressMux;
    this->m_commSubsystemHealthProbe = commSubsystemHealthProbe;
    this->m_sbandSubsystemHealthNodeId = sbandSubsystemHealthNodeId;
    this->m_uhfSubsystemHealthNodeId = uhfSubsystemHealthNodeId;
    this->m_subsystemFdirConfig = subsystemFdirConfig;
    this->m_sbandIngressConfig = sbandIngressConfig;
    this->m_sbandBackupIngressConfig = withRole(sbandIngressConfig, AuthorityLinkRole::BACKUP);
    this->m_uhfBackupIngressConfig = uhfBackupIngressConfig;
    this->m_uhfPrimaryIngressConfig = withRole(uhfBackupIngressConfig, AuthorityLinkRole::PRIMARY_AFTER_FAILOVER);
    this->m_sbandSubsystemProbeCooldownTicks = 0U;
    this->m_uhfSubsystemProbeCooldownTicks = 0U;
    this->m_sbandSubsystemProbeFailureStreak = 0U;
    this->m_uhfSubsystemProbeFailureStreak = 0U;
    this->clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason::NONE);

    if (this->m_commandIngressAuthority != nullptr) {
        this->m_commandIngressAuthority->setSessionRuntimeObserverForRuntime(this);
    }

    this->m_downlinkScheduler.setPrimaryFileLink(this->m_primaryFileLink);
    if (initialPrimaryBand != this->m_primaryCommandLink) {
        static_cast<void>(this->setPrimaryLinks_(initialPrimaryBand,
                                                static_cast<U32>(CommPolicyTransitionReason::INITIAL_BOOTSTRAP)));
    }
    this->syncEgressPolicy_();
    this->applyAuthorityProfiles_(0U);
    this->publishState_();
    this->unlockComponent_();
}

void CommController::configureCspRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime) {
    this->lockComponent_();
    this->m_reliableTransfer.setRuntimeForRuntime(runtime);
    this->m_cspRuntimeOwner = dynamic_cast<OBC::IAsyncCspRuntimeOwner*>(&runtime);
    this->unlockComponent_();
}

Fw::CmdResponse CommController::setActiveBandForRuntime(OBC::CommBand band) {
    this->lockComponent_();
    const bool available = this->refreshBandAvailability_(band, true);
    if (!available) {
        this->unlockComponent_();
        return Fw::CmdResponse::EXECUTION_ERROR;
    }
    const Fw::CmdResponse response = this->setPrimaryLinks_(band, static_cast<U32>(CommPolicyTransitionReason::OPERATOR_SWITCH));
    this->unlockComponent_();
    return response;
}

Fw::CmdResponse CommController::startPassForRuntime(U32 durationSec) {
    this->lockComponent_();
    if (durationSec == 0U) {
        this->unlockComponent_();
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    this->m_passActive = true;
    this->m_passRemainingSec = durationSec;
    this->m_totalPasses += 1U;
    this->log_ACTIVITY_HI_COMM_PASS_START(durationSec);
    this->publishState_();
    this->unlockComponent_();
    return Fw::CmdResponse::OK;
}

Fw::CmdResponse CommController::stopPassForRuntime() {
    this->lockComponent_();
    this->endPass_();
    this->unlockComponent_();
    return Fw::CmdResponse::OK;
}

Fw::CmdResponse CommController::getStatusForRuntime() {
    this->lockComponent_();
    this->refreshStatusForQuery_();
    this->publishExplicitRefreshTelemetry_();
    this->unlockComponent_();
    return Fw::CmdResponse::OK;
}

void CommController::onCommandSessionOpenedForRuntime(FwIndexType ingressPort,
                                                      const AuthorityConfig& config,
                                                      U32 sessionId,
                                                      U32 sequenceNumber,
                                                      bool secureAuthenticated,
                                                      bool replaced) {
    this->lockComponent_();
    const bool qualifiesSbandLive =
        secureAuthenticated && this->isQualifyingSbandLiveSession_(ingressPort, config) && sessionId != 0U;
    if (qualifiesSbandLive) {
        this->m_sbandLiveObservabilitySessionActive = true;
        this->m_sbandLiveObservabilityIngressPort = ingressPort;
        this->m_sbandLiveObservabilityRole = config.role;
        this->m_sbandLiveObservabilitySessionId = sessionId;
        this->m_sbandLiveObservabilityLastAcceptedSequence = sequenceNumber;
    } else if (replaced && this->m_sbandLiveObservabilitySessionActive &&
               this->m_sbandLiveObservabilityIngressPort == ingressPort) {
        this->clearSbandLiveObservability_(sequenceNumber);
    }
    this->startOrRefreshUhfBeaconSuppress_(ingressPort, config, sessionId, sequenceNumber, replaced);
    this->publishState_();
    this->unlockComponent_();
}

void CommController::onCommandSessionActivityForRuntime(FwIndexType ingressPort,
                                                        const AuthorityConfig& config,
                                                        U32 sessionId,
                                                        U32 sequenceNumber) {
    this->lockComponent_();
    bool publishSbandLiveState = false;
    bool publishUhfSuppressState = false;
    if (this->m_sbandLiveObservabilitySessionActive && this->m_sbandLiveObservabilityIngressPort == ingressPort &&
        this->m_sbandLiveObservabilitySessionId == sessionId && this->isQualifyingSbandLiveSession_(ingressPort, config)) {
        this->m_sbandLiveObservabilityLastAcceptedSequence = sequenceNumber;
        publishSbandLiveState = true;
    }
    if (this->suppressOwnerMatches_(ingressPort, sessionId) && this->isQualifyingUhfSuppressSession_(ingressPort, config)) {
        this->startOrRefreshUhfBeaconSuppress_(ingressPort, config, sessionId, sequenceNumber, false);
        publishUhfSuppressState = true;
    }
    if (publishSbandLiveState) {
        this->publishSbandLiveObservabilityState_(this->computeSbandLiveObservabilityState_());
    }
    if (publishUhfSuppressState) {
        this->publishUhfBeaconSuppressState_();
    }
    if (publishSbandLiveState || publishUhfSuppressState) {
        this->updateCachedRuntimeStateLocked_();
    }
    this->unlockComponent_();
}

void CommController::onCommandSessionRevokedForRuntime(FwIndexType ingressPort,
                                                       const AuthorityConfig& config,
                                                       U32 sessionId,
                                                       U32 lastAcceptedSequence,
                                                       U32 reason) {
    static_cast<void>(config);
    static_cast<void>(lastAcceptedSequence);
    static_cast<void>(reason);
    this->lockComponent_();
    if (this->m_sbandLiveObservabilitySessionActive && this->m_sbandLiveObservabilityIngressPort == ingressPort &&
        this->m_sbandLiveObservabilitySessionId == sessionId) {
        this->clearSbandLiveObservability_(lastAcceptedSequence);
    }
    if (this->suppressOwnerMatches_(ingressPort, sessionId)) {
        this->clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason::SESSION_REVOKED);
    }
    this->publishState_();
    this->unlockComponent_();
}

OBC::CommRuntimeState CommController::getStateForRuntime() const {
    std::lock_guard<std::mutex> guard(this->m_runtimeStateCacheMutex);
    return this->m_cachedRuntimeState;
}

Svc::SendFileResponse CommController::dpFileRequestIn_handler(FwIndexType portNum,
                                                              const Fw::StringBase& sourceFileName,
                                                              const Fw::StringBase& destFileName,
                                                              U32 offset,
                                                              U32 length) {
    static_cast<void>(portNum);
    return this->submitDownlink_(CommDownlinkOwner::DP_CATALOG, sourceFileName, destFileName, offset, length);
}

void CommController::fileCompleteIn_handler(FwIndexType portNum, const Svc::SendFileResponse& response) {
    static_cast<void>(portNum);
    this->lockComponent_();
    const CommDownlinkCompleteResult result = this->m_downlinkScheduler.complete(response);
    this->handleSchedulerResult_(result, &response);
    this->unlockComponent_();
}

void CommController::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    RecoveryNotification notification = {};
    this->lockComponent_();

    const OBC::CommLinkHealthView sbandHealth = this->m_groundLinkHealthProvider != nullptr
                                                    ? this->m_groundLinkHealthProvider->getHealthForRuntime(OBC::CommBand::SBAND)
                                                    : OBC::CommLinkHealthView{};
    const OBC::CommLinkHealthView uhfHealth = this->m_groundLinkHealthProvider != nullptr
                                                  ? this->m_groundLinkHealthProvider->getHealthForRuntime(OBC::CommBand::UHF)
                                                  : OBC::CommLinkHealthView{};
    this->m_lastSbandHealth = sbandHealth;
    this->m_lastUhfHealth = uhfHealth;
    this->pollAsyncSubsystemProbeCompletions_();
    this->refreshLinkAvailability_();
    notification = this->updateFdirState_(sbandHealth, uhfHealth);
    this->processDeferredLaunch_();
    this->pollReliableTransfer_();
    if (this->m_uhfBeaconSuppressActive && this->m_uhfBeaconSuppressRemainingTicks > 0U) {
        this->m_uhfBeaconSuppressRemainingTicks -= 1U;
        if (this->m_uhfBeaconSuppressRemainingTicks == 0U) {
            this->clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason::INACTIVITY_TIMEOUT);
        }
    }

    if (this->m_passActive && this->m_passRemainingSec > 0U) {
        this->m_passRemainingSec -= 1U;
        if (this->m_passRemainingSec == 0U) {
            this->endPass_();
            if (this->isConnected_watchdogBeatOut_OutputPort(0)) {
                this->watchdogBeatOut_out(0, static_cast<U32>(OBC::WatchdogSource::COMM_CONTROLLER));
            }
            this->unlockComponent_();
            this->emitRecoveryNotification_(notification);
            return;
        }
    }

    this->updateCachedRuntimeStateLocked_();
    if (this->isConnected_watchdogBeatOut_OutputPort(0)) {
        this->watchdogBeatOut_out(0, static_cast<U32>(OBC::WatchdogSource::COMM_CONTROLLER));
    }
    this->unlockComponent_();
    this->emitRecoveryNotification_(notification);
}

void CommController::COMM_SET_ACTIVE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::CommBand band) {
    this->cmdResponse_out(opCode, cmdSeq, this->setActiveBandForRuntime(band));
}

void CommController::COMM_START_PASS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 durationSec) {
    this->cmdResponse_out(opCode, cmdSeq, this->startPassForRuntime(durationSec));
}

void CommController::COMM_STOP_PASS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->cmdResponse_out(opCode, cmdSeq, this->stopPassForRuntime());
}

void CommController::COMM_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->cmdResponse_out(opCode, cmdSeq, this->getStatusForRuntime());
}

void CommController::publishState_() {
    this->syncEgressPolicy_();
    const ComputedSbandLiveObservabilityState sbandLiveState = this->computeSbandLiveObservabilityState_();
    this->tlmWrite_COMM_ACTIVE_BAND(this->m_activeBand);
    this->tlmWrite_COMM_PASS_ACTIVE(this->m_passActive);
    this->tlmWrite_COMM_PASS_REMAINING(this->m_passRemainingSec);
    this->tlmWrite_COMM_TOTAL_PASSES(this->m_totalPasses);
    this->tlmWrite_COMM_PRIMARY_COMMAND_LINK(this->m_primaryCommandLink);
    this->tlmWrite_COMM_PRIMARY_TELEMETRY_LINK(this->m_primaryTelemetryLink);
    this->tlmWrite_COMM_PRIMARY_FILE_LINK(this->m_primaryFileLink);
    this->tlmWrite_COMM_S_BAND_AVAILABLE(this->m_sbandAvailable);
    this->tlmWrite_COMM_UHF_AVAILABLE(this->m_uhfAvailable);
    this->tlmWrite_COMM_DOWNLINK_ACTIVE_OWNER(static_cast<U32>(this->m_downlinkScheduler.getActiveOwner()));
    this->tlmWrite_COMM_DOWNLINK_PENDING_OWNER(static_cast<U32>(this->m_downlinkScheduler.getPendingOwner()));
    this->tlmWrite_COMM_SESSION_REVOKE_TOTAL(this->m_sessionRevokeTotal);
    this->tlmWrite_COMM_DOWNLINK_REJECT_TOTAL(this->m_downlinkRejectTotal);
    this->tlmWrite_COMM_FDIR_FAULT_LATCHED(this->m_fdirFaultLatched);
    this->tlmWrite_COMM_FDIR_FAULT_KIND(static_cast<U32>(this->m_fdirFaultKind));
    this->tlmWrite_COMM_S_BAND_ACTIVITY_AGE_TICKS(this->m_lastSbandHealth.activityAgeTicks);
    this->tlmWrite_COMM_UHF_ACTIVITY_AGE_TICKS(this->m_lastUhfHealth.activityAgeTicks);
    this->tlmWrite_COMM_S_BAND_AVAILABILITY_REASON(static_cast<U32>(this->m_lastSbandHealth.availabilityReason));
    this->tlmWrite_COMM_UHF_AVAILABILITY_REASON(static_cast<U32>(this->m_lastUhfHealth.availabilityReason));
    this->tlmWrite_COMM_RT_ACTIVE_TRANSFER_ID(this->m_reliableTransfer.state().active ? this->m_reliableTransfer.state().transferId : 0U);
    this->tlmWrite_COMM_RT_LAST_ACK_SEGMENT(this->m_reliableTransfer.state().contiguousSegments);
    this->tlmWrite_COMM_RT_ACKED_BYTES(this->m_reliableTransfer.state().committedBytes);
    this->tlmWrite_COMM_RT_RESEND_TOTAL(this->m_reliableTransfer.state().resendCount);
    this->tlmWrite_COMM_RT_LAST_RESULT(static_cast<U32>(this->m_reliableTransfer.state().lastResult));
    this->tlmWrite_COMM_FDIR_CONSEC_PRIMARY_UNAVAILABLE(this->m_consecutivePrimaryUnavailable);
    this->tlmWrite_COMM_FDIR_CONSEC_PRIMARY_TRANSPORT(this->m_consecutivePrimaryTransportGrowth);
    this->tlmWrite_COMM_RECOVERY_FAILOVER_TOTAL(this->m_recoveryFailoverTotal);
    this->tlmWrite_COMM_RECOVERY_OWNER_CLEAR_TOTAL(this->m_recoveryOwnerClearTotal);
    this->publishUhfBeaconSuppressState_();
    this->publishSbandLiveObservabilityState_(sbandLiveState);
    this->updateCachedRuntimeStateLocked_();
}

void CommController::publishExplicitRefreshTelemetry_() {
    Fw::Time timeTag = this->getTime();
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_ACTIVE_BAND, this->m_activeBand, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_PRIMARY_COMMAND_LINK, this->m_primaryCommandLink, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_PRIMARY_TELEMETRY_LINK, this->m_primaryTelemetryLink, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_PRIMARY_FILE_LINK, this->m_primaryFileLink, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_S_BAND_AVAILABLE, this->m_sbandAvailable, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_UHF_AVAILABLE, this->m_uhfAvailable, timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_COMM_S_BAND_AVAILABILITY_REASON,
        static_cast<U32>(this->m_lastSbandHealth.availabilityReason),
        timeTag);
    this->emitStatusRefreshTelemetryValue_(
        CHANNELID_COMM_UHF_AVAILABILITY_REASON,
        static_cast<U32>(this->m_lastUhfHealth.availabilityReason),
        timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_FDIR_FAULT_LATCHED, this->m_fdirFaultLatched, timeTag);
    this->emitStatusRefreshTelemetryValue_(CHANNELID_COMM_FDIR_FAULT_KIND, static_cast<U32>(this->m_fdirFaultKind), timeTag);
}

OBC::CommRuntimeState CommController::buildRuntimeStateLocked_() const {
    const ComputedSbandLiveObservabilityState sbandLiveState = this->computeSbandLiveObservabilityState_();
    return OBC::CommRuntimeState{
        this->m_activeBand,
        this->m_passActive,
        this->m_passRemainingSec,
        this->m_totalPasses,
        this->m_primaryCommandLink,
        this->m_primaryTelemetryLink,
        this->m_primaryFileLink,
        this->m_sbandAvailable,
        this->m_uhfAvailable,
        static_cast<U32>(this->m_downlinkScheduler.getActiveOwner()),
        static_cast<U32>(this->m_downlinkScheduler.getPendingOwner()),
        this->m_sessionRevokeTotal,
        this->m_downlinkRejectTotal,
        this->m_fdirFaultLatched,
        this->m_fdirFaultKind,
        this->m_lastSbandHealth.activityAgeTicks,
        this->m_lastUhfHealth.activityAgeTicks,
        this->m_lastSbandHealth.availabilityReason,
        this->m_lastUhfHealth.availabilityReason,
        this->m_consecutivePrimaryUnavailable,
        this->m_consecutivePrimaryTransportGrowth,
        this->m_recoveryFailoverTotal,
        this->m_recoveryOwnerClearTotal,
        sbandLiveState.active,
        sbandLiveState.reason,
        static_cast<U32>(sbandLiveState.ingressPort),
        sbandLiveState.role,
        sbandLiveState.sessionId,
        sbandLiveState.lastAcceptedSequence,
        this->m_commEgressMux != nullptr ? this->m_commEgressMux->getUhfPrimaryPacketQuietForRuntime() : false,
        this->m_uhfBeaconSuppressActive,
        static_cast<U32>(this->m_uhfBeaconSuppressIngressPort),
        this->m_uhfBeaconSuppressRole,
        this->m_uhfBeaconSuppressSessionId,
        this->m_uhfBeaconSuppressLastAcceptedSequence,
        this->m_uhfBeaconSuppressRemainingTicks,
        UHF_BEACON_SUPPRESS_TIMEOUT_TICKS,
    };
}

void CommController::updateCachedRuntimeStateLocked_() {
    std::lock_guard<std::mutex> guard(this->m_runtimeStateCacheMutex);
    this->m_cachedRuntimeState = this->buildRuntimeStateLocked_();
}

void CommController::refreshStatusViews_() {
    this->m_lastSbandHealth = this->m_groundLinkHealthProvider != nullptr
                                  ? this->m_groundLinkHealthProvider->getHealthForRuntime(OBC::CommBand::SBAND)
                                  : OBC::CommLinkHealthView{};
    this->m_lastUhfHealth = this->m_groundLinkHealthProvider != nullptr
                                ? this->m_groundLinkHealthProvider->getHealthForRuntime(OBC::CommBand::UHF)
                                : OBC::CommLinkHealthView{};
}

void CommController::refreshStatusForQuery_() {
    this->refreshStatusViews_();
    this->pollAsyncSubsystemProbeCompletions_();
    static_cast<void>(this->recomputeBandAvailabilityForQuery_(OBC::CommBand::SBAND));
    static_cast<void>(this->recomputeBandAvailabilityForQuery_(OBC::CommBand::UHF));
    this->updateCachedRuntimeStateLocked_();
}

bool CommController::recomputeBandAvailabilityForQuery_(OBC::CommBand band) {
    const bool previous = this->cachedAvailabilityForBand_(band);
    bool available = previous;
    if (!this->usesSubsystemHealthForBand_(band)) {
        const OBC::CommLinkHealthView& health = band == OBC::CommBand::SBAND ? this->m_lastSbandHealth : this->m_lastUhfHealth;
        available = health.available;
    } else if (this->m_cspRuntimeOwner != nullptr) {
        available = this->consumeAsyncProbeCompletionForQuery_(band, previous);
    }

    this->updateCachedAvailability_(band, available);
    return available;
}

bool CommController::consumeAsyncProbeCompletionForQuery_(OBC::CommBand band, bool previous) {
    AsyncSubsystemProbeState& asyncState =
        band == OBC::CommBand::SBAND ? this->m_sbandAsyncSubsystemProbe : this->m_uhfAsyncSubsystemProbe;
    if (!asyncState.completionReady) {
        return previous;
    }

    asyncState.completionReady = false;
    U32& cooldown = this->subsystemProbeCooldownForBand_(band);
    U32& failureStreak = this->subsystemProbeFailureStreakForBand_(band);
    const U32 probePeriod = sanitizeProbePeriodTicks(this->m_subsystemFdirConfig.primaryProbePeriodTicks);
    const U32 failureThreshold =
        sanitizeProbeFailureThreshold(this->m_subsystemFdirConfig.primaryProbeFailureThreshold);
    cooldown = probePeriod - 1U;

    if (asyncState.completionAvailable) {
        failureStreak = 0U;
        return true;
    }

    failureStreak += 1U;
    if (!previous) {
        return false;
    }
    return failureStreak < failureThreshold;
}

void CommController::updateCachedAvailability_(OBC::CommBand band, bool available) {
    bool* const cached = band == OBC::CommBand::SBAND ? &this->m_sbandAvailable : &this->m_uhfAvailable;
    if (available == *cached) {
        return;
    }
    *cached = available;
    this->m_downlinkScheduler.setLinkAvailability(band, available);
    this->handleLinkAvailabilityChange_(band, available);
}

void CommController::emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag) {
    this->commStatusRefreshTlmOut_out(0, this->getIdBase() + channelId, timeTag, buffer);
}

void CommController::syncEgressPolicy_() {
    if (this->m_commEgressMux == nullptr) {
        return;
    }

    this->m_commEgressMux->setPrimaryTelemetryLinkForRuntime(this->m_primaryTelemetryLink);
    this->m_commEgressMux->setPrimaryFileLinkForRuntime(this->m_primaryFileLink);
    this->m_commEgressMux->setBandLiveObservabilityEnabledForRuntime(
        OBC::CommBand::SBAND, this->computeSbandLiveObservabilityState_().active);
    this->m_commEgressMux->setBandLiveObservabilityEnabledForRuntime(OBC::CommBand::UHF, true);
    this->m_commEgressMux->setUhfPrimaryPacketQuietForRuntime(false);
    const bool suspendUhfPackets = (this->m_primaryTelemetryLink == OBC::CommBand::UHF) &&
                                   (this->m_downlinkScheduler.getActiveOwner() != CommDownlinkOwner::NONE);
    this->m_commEgressMux->setUhfFileTransferActiveForRuntime(suspendUhfPackets);
}

Fw::CmdResponse CommController::setPrimaryLinks_(OBC::CommBand band, U32 reason) {
    const bool switchingFileLink = this->m_primaryFileLink != band;
    if (switchingFileLink) {
        const CommDownlinkCompleteResult dropped = this->m_downlinkScheduler.dropForPrimarySwitch();
        this->handleSchedulerResult_(dropped, nullptr);
    }

    this->m_activeBand = band;
    this->m_primaryCommandLink = band;
    this->m_primaryTelemetryLink = band;
    this->m_primaryFileLink = band;
    this->m_primaryLinkSelectionReason = reason;
    this->m_downlinkScheduler.setPrimaryFileLink(band);
    this->syncEgressPolicy_();
    this->applyAuthorityProfiles_(reason);
    this->log_ACTIVITY_HI_COMM_BAND_SWITCH(band);
    this->log_ACTIVITY_HI_COMM_PRIMARY_LINK_CHANGED(
        this->m_primaryCommandLink, this->m_primaryTelemetryLink, this->m_primaryFileLink, reason);
    this->publishState_();
    return Fw::CmdResponse::OK;
}

void CommController::refreshLinkAvailability_() {
    const OBC::CommBand primaryBand = this->currentPrimaryBand_();
    const OBC::CommBand backupBand = primaryBand == OBC::CommBand::SBAND ? OBC::CommBand::UHF : OBC::CommBand::SBAND;
    static_cast<void>(this->refreshBandAvailability_(primaryBand, false));
    if (!this->usesSubsystemHealthForBand_(backupBand)) {
        static_cast<void>(this->refreshBandAvailability_(backupBand, false));
    }
}

bool CommController::refreshBandAvailability_(OBC::CommBand band, bool forceProbe) {
    bool available = false;
    if (this->usesSubsystemHealthForBand_(band) &&
        (forceProbe || band == this->currentPrimaryBand_())) {
        available = this->probePrimaryBandAvailability_(band, forceProbe);
    } else if (this->usesSubsystemHealthForBand_(band) && !forceProbe) {
        available = this->cachedAvailabilityForBand_(band);
    } else {
        available = this->probeSubsystemAvailability_(band);
    }
    bool* const cached = band == OBC::CommBand::SBAND ? &this->m_sbandAvailable : &this->m_uhfAvailable;
    const bool changed = available != *cached;
    if (changed) {
        *cached = available;
        this->m_downlinkScheduler.setLinkAvailability(band, available);
        this->handleLinkAvailabilityChange_(band, available);
    }
    return available;
}

void CommController::applyAuthorityProfiles_(U32 reason) {
    if (this->m_commandIngressAuthority == nullptr) {
        return;
    }

    const AuthorityConfig& sbandConfig =
        this->m_primaryCommandLink == OBC::CommBand::SBAND ? this->m_sbandIngressConfig : this->m_sbandBackupIngressConfig;
    const AuthorityConfig& uhfConfig =
        this->m_primaryCommandLink == OBC::CommBand::UHF ? this->m_uhfPrimaryIngressConfig : this->m_uhfBackupIngressConfig;

    this->reconfigureIngressProfile_(SBAND_INGRESS_PORT, sbandConfig, reason);
    this->reconfigureIngressProfile_(UHF_INGRESS_PORT, uhfConfig, reason);
}

void CommController::reconfigureIngressProfile_(FwIndexType portNum, const AuthorityConfig& config, U32 reason) {
    if (this->suppressOwnerMatches_(portNum, this->m_uhfBeaconSuppressSessionId)) {
        this->clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason::ROLE_INVALIDATED);
    }

    if (this->m_commandIngressAuthority != nullptr &&
        this->m_commandIngressAuthority->hasOpenSessionForRuntime(portNum) &&
        this->m_commandIngressAuthority->revokeIngressSource(portNum, reason, false)) {
        this->m_sessionRevokeTotal++;
    }

    const U8 serviceId = secureServiceIdForAuthorityIdentity(config.identity);
    if (serviceId != 0U && this->isConnected_secureAuthInvalidateOut_OutputPort(0)) {
        SecureAuthRevocation revocation;
        revocation.set_ingressPort(static_cast<U32>(portNum));
        revocation.set_serviceId(serviceId);
        revocation.set_reason(static_cast<U32>(SecureAuthRevocationReason::INVALIDATED));
        this->secureAuthInvalidateOut_out(0, revocation);
    }

    if (this->m_commandIngressAuthority != nullptr &&
        this->m_commandIngressAuthority->reconfigureIngressSource(portNum, config)) {
        this->m_sessionRevokeTotal++;
    }

    if (portNum == SBAND_INGRESS_PORT) {
        this->m_sbandLiveObservabilitySessionActive = false;
        this->m_sbandLiveObservabilityIngressPort = SBAND_INGRESS_PORT;
        this->m_sbandLiveObservabilityRole = AuthorityLinkRole::UNKNOWN;
        this->m_sbandLiveObservabilitySessionId = 0U;
    }

    this->publishFileIngressPolicy_(portNum, config);
}

void CommController::publishFileIngressPolicy_(FwIndexType portNum, const AuthorityConfig& config) {
    if (!this->isConnected_filePolicyOut_OutputPort(0)) {
        return;
    }

    FileIngressPolicyState policy = {};
    policy.set_ingressPort(static_cast<U32>(portNum));
    policy.set_linkIdentity(static_cast<U32>(config.identity));
    policy.set_linkRole(static_cast<U32>(config.role));
    policy.set_fileAllowed(fileIngressAllowedForConfig(config));
    this->filePolicyOut_out(0, policy);
}

bool CommController::isQualifyingUhfSuppressSession_(FwIndexType ingressPort, const AuthorityConfig& config) const {
    return ingressPort == UHF_INGRESS_PORT &&
           config.valid &&
           config.identity == AuthorityLinkIdentity::UHF &&
           (config.role == AuthorityLinkRole::BACKUP || config.role == AuthorityLinkRole::PRIMARY_AFTER_FAILOVER);
}

bool CommController::isQualifyingSbandLiveSession_(FwIndexType ingressPort, const AuthorityConfig& config) const {
    return ingressPort == SBAND_INGRESS_PORT && config.valid && config.identity == AuthorityLinkIdentity::SBAND &&
           isCommManagedAuthority(config);
}

void CommController::clearSbandLiveObservability_(U32 lastAcceptedSequence) {
    this->m_sbandLiveObservabilitySessionActive = false;
    this->m_sbandLiveObservabilityIngressPort = SBAND_INGRESS_PORT;
    this->m_sbandLiveObservabilityRole = AuthorityLinkRole::UNKNOWN;
    this->m_sbandLiveObservabilitySessionId = 0U;
    this->m_sbandLiveObservabilityLastAcceptedSequence = lastAcceptedSequence;
}

CommController::ComputedSbandLiveObservabilityState CommController::computeSbandLiveObservabilityState_() const {
    ComputedSbandLiveObservabilityState state = {};
    state.ingressPort = this->m_sbandLiveObservabilityIngressPort;
    state.role = this->m_sbandLiveObservabilityRole;
    state.sessionId = this->m_sbandLiveObservabilitySessionId;
    state.lastAcceptedSequence = this->m_sbandLiveObservabilityLastAcceptedSequence;

    if (this->m_commEgressMux != nullptr && this->m_commEgressMux->getDiagnosticQuietPacketEgressForRuntime()) {
        state.reason = OBC::CommLiveObservabilityStateReason::INACTIVE_DIAGNOSTIC_QUIET;
        return state;
    }
    if (this->m_primaryTelemetryLink != OBC::CommBand::SBAND) {
        state.reason = OBC::CommLiveObservabilityStateReason::INACTIVE_NON_PRIMARY_BAND;
        return state;
    }
    if (!this->m_sbandLiveObservabilitySessionActive || this->m_sbandLiveObservabilitySessionId == 0U) {
        state.reason = OBC::CommLiveObservabilityStateReason::INACTIVE_NO_SESSION;
        return state;
    }

    state.active = true;
    state.reason = OBC::CommLiveObservabilityStateReason::ACTIVE_AUTHENTICATED_SESSION;
    return state;
}

void CommController::publishSbandLiveObservabilityState_(const ComputedSbandLiveObservabilityState& state) {
    const bool wasActive = this->m_lastPublishedSbandLiveObservabilityActive;
    this->tlmWrite_COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE(state.active);
    this->tlmWrite_COMM_S_BAND_LIVE_OBSERVABILITY_REASON(static_cast<U32>(state.reason));
    this->tlmWrite_COMM_S_BAND_LIVE_OBSERVABILITY_INGRESS_PORT(static_cast<U32>(state.ingressPort));
    this->tlmWrite_COMM_S_BAND_LIVE_OBSERVABILITY_ROLE(static_cast<U32>(state.role));
    this->tlmWrite_COMM_S_BAND_LIVE_OBSERVABILITY_SESSION_ID(state.sessionId);
    this->tlmWrite_COMM_S_BAND_LIVE_OBSERVABILITY_LAST_SEQUENCE(state.lastAcceptedSequence);

    const bool changed = state.active != this->m_lastPublishedSbandLiveObservabilityActive ||
                         state.reason != this->m_lastPublishedSbandLiveObservabilityReason ||
                         state.ingressPort != this->m_lastPublishedSbandLiveObservabilityIngressPort ||
                         state.role != this->m_lastPublishedSbandLiveObservabilityRole ||
                         state.sessionId != this->m_lastPublishedSbandLiveObservabilitySessionId;
    if (changed) {
        this->log_ACTIVITY_HI_COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED(
            state.active ? 1U : 0U, static_cast<U32>(state.reason), static_cast<U32>(state.ingressPort),
            static_cast<U32>(state.role), state.sessionId);
        this->m_lastPublishedSbandLiveObservabilityActive = state.active;
        this->m_lastPublishedSbandLiveObservabilityReason = state.reason;
        this->m_lastPublishedSbandLiveObservabilityIngressPort = state.ingressPort;
        this->m_lastPublishedSbandLiveObservabilityRole = state.role;
        this->m_lastPublishedSbandLiveObservabilitySessionId = state.sessionId;
        if (!wasActive && state.active && this->m_groundLinkHealthProvider != nullptr) {
            this->m_groundLinkHealthProvider->markTelemetryDirtyForRuntime(OBC::CommBand::SBAND);
        }
    }
}

void CommController::publishUhfBeaconSuppressState_() {
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_ACTIVE(this->m_uhfBeaconSuppressActive);
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_INGRESS_PORT(static_cast<U32>(this->m_uhfBeaconSuppressIngressPort));
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_ROLE(static_cast<U32>(this->m_uhfBeaconSuppressRole));
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_SESSION_ID(this->m_uhfBeaconSuppressSessionId);
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_LAST_SEQUENCE(this->m_uhfBeaconSuppressLastAcceptedSequence);
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_REMAINING_TICKS(this->m_uhfBeaconSuppressRemainingTicks);
    this->tlmWrite_COMM_UHF_BEACON_SUPPRESS_TIMEOUT_TICKS(UHF_BEACON_SUPPRESS_TIMEOUT_TICKS);
}

void CommController::startOrRefreshUhfBeaconSuppress_(FwIndexType ingressPort,
                                                      const AuthorityConfig& config,
                                                      U32 sessionId,
                                                      U32 sequenceNumber,
                                                      bool replaced) {
    if (!this->isQualifyingUhfSuppressSession_(ingressPort, config) || sessionId == 0U) {
        return;
    }

    const bool wasActive = this->m_uhfBeaconSuppressActive;
    const bool sameOwner = this->suppressOwnerMatches_(ingressPort, sessionId);
    if (wasActive && !sameOwner && replaced) {
        this->clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason::SESSION_REPLACED);
    }

    this->m_uhfBeaconSuppressActive = true;
    this->m_uhfBeaconSuppressIngressPort = ingressPort;
    this->m_uhfBeaconSuppressRole = config.role;
    this->m_uhfBeaconSuppressSessionId = sessionId;
    this->m_uhfBeaconSuppressLastAcceptedSequence = sequenceNumber;
    this->m_uhfBeaconSuppressRemainingTicks = UHF_BEACON_SUPPRESS_TIMEOUT_TICKS;
    if (this->m_beaconSuppressControl != nullptr) {
        this->m_beaconSuppressControl->setUhfBeaconSuppressedForRuntime(true);
    }
    this->syncEgressPolicy_();

    if (!wasActive || !sameOwner) {
        this->log_ACTIVITY_HI_COMM_UHF_BEACON_SUPPRESS_STARTED(static_cast<U32>(ingressPort),
                                                               static_cast<U32>(config.role),
                                                               sessionId,
                                                               UHF_BEACON_SUPPRESS_TIMEOUT_TICKS,
                                                               replaced);
    } else if (sequenceNumber > 0U) {
        this->log_ACTIVITY_LO_COMM_UHF_BEACON_SUPPRESS_REFRESHED(static_cast<U32>(ingressPort),
                                                                 static_cast<U32>(config.role),
                                                                 sessionId,
                                                                 sequenceNumber,
                                                                 this->m_uhfBeaconSuppressRemainingTicks);
    }
}

void CommController::clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason reason) {
    const bool hadActiveSuppress = this->m_uhfBeaconSuppressActive;
    const FwIndexType ingressPort = this->m_uhfBeaconSuppressIngressPort;
    const AuthorityLinkRole role = this->m_uhfBeaconSuppressRole;
    const U32 sessionId = this->m_uhfBeaconSuppressSessionId;
    const U32 lastSequence = this->m_uhfBeaconSuppressLastAcceptedSequence;

    this->m_uhfBeaconSuppressActive = false;
    this->m_uhfBeaconSuppressIngressPort = UHF_INGRESS_PORT;
    this->m_uhfBeaconSuppressRole = AuthorityLinkRole::UNKNOWN;
    this->m_uhfBeaconSuppressSessionId = 0U;
    this->m_uhfBeaconSuppressLastAcceptedSequence = 0U;
    this->m_uhfBeaconSuppressRemainingTicks = 0U;

    if (this->m_beaconSuppressControl != nullptr) {
        this->m_beaconSuppressControl->setUhfBeaconSuppressedForRuntime(false);
    }
    this->syncEgressPolicy_();

    if (hadActiveSuppress && reason != OBC::CommUhfBeaconSuppressClearReason::NONE) {
        this->log_ACTIVITY_HI_COMM_UHF_BEACON_SUPPRESS_CLEARED(static_cast<U32>(reason),
                                                               static_cast<U32>(ingressPort),
                                                               static_cast<U32>(role),
                                                               sessionId,
                                                               lastSequence);
    }
}

bool CommController::suppressOwnerMatches_(FwIndexType ingressPort, U32 sessionId) const {
    return this->m_uhfBeaconSuppressActive && this->m_uhfBeaconSuppressIngressPort == ingressPort &&
           this->m_uhfBeaconSuppressSessionId == sessionId;
}

void CommController::handleLinkAvailabilityChange_(OBC::CommBand band, bool available) {
    this->log_ACTIVITY_LO_COMM_LINK_AVAILABILITY_CHANGED(band, available);
}

CommController::RecoveryNotification CommController::updateFdirState_(const OBC::CommLinkHealthView& sbandHealth,
                                                                      const OBC::CommLinkHealthView& uhfHealth) {
    RecoveryNotification notification = {};
    if (this->m_subsystemFdirConfig.unavailableFailureThreshold == 0U) {
        this->m_consecutivePrimaryUnavailable = 0U;
        this->m_consecutivePrimaryTransportGrowth = 0U;
        if (this->m_fdirFaultLatched) {
            notification.failureCount = 0U;
            notification.kind = this->m_fdirFaultKind == OBC::CommFdirFaultKind::PRIMARY_UNAVAILABLE
                                    ? RecoveryNotification::Kind::CLEAR_UNAVAILABLE
                                    : RecoveryNotification::Kind::CLEAR_TRANSPORT;
            this->clearCommFault_();
        }
        return notification;
    }

    const OBC::CommBand primaryBand = this->currentPrimaryBand_();
    const bool primaryAvailable = primaryBand == OBC::CommBand::SBAND ? this->m_sbandAvailable : this->m_uhfAvailable;
    const bool primaryErrorGrowth = this->usesSubsystemHealthForBand_(primaryBand)
                                        ? false
                                        : (primaryBand == OBC::CommBand::SBAND ? sbandHealth.errorGrowthThisCycle
                                                                               : uhfHealth.errorGrowthThisCycle);

    if (!primaryAvailable) {
        this->m_consecutivePrimaryUnavailable++;
        this->m_consecutivePrimaryTransportGrowth = 0U;
    } else if (primaryErrorGrowth) {
        this->m_consecutivePrimaryUnavailable = 0U;
        this->m_consecutivePrimaryTransportGrowth++;
    } else {
        this->m_consecutivePrimaryUnavailable = 0U;
        this->m_consecutivePrimaryTransportGrowth = 0U;
    }

    const OBC::CommFdirDecision decision =
        OBC::CommFdirPolicy::evaluate(primaryAvailable,
                                      primaryErrorGrowth,
                                      this->m_consecutivePrimaryUnavailable,
                                      this->m_consecutivePrimaryTransportGrowth,
                                      this->m_subsystemFdirConfig.unavailableFailureThreshold,
                                      this->m_fdirFaultLatched,
                                      this->m_fdirFaultKind);

    if (decision.shouldLatchFault) {
        this->latchCommFault_(decision.kind, decision.failureCount);
        notification.failureCount = decision.failureCount;
        notification.kind = decision.kind == OBC::CommFdirFaultKind::PRIMARY_UNAVAILABLE
                                ? RecoveryNotification::Kind::SUBMIT_UNAVAILABLE
                                : RecoveryNotification::Kind::SUBMIT_TRANSPORT;
    } else if (decision.shouldClearFault) {
        notification.failureCount = decision.failureCount;
        notification.kind = this->m_fdirFaultKind == OBC::CommFdirFaultKind::PRIMARY_UNAVAILABLE
                                ? RecoveryNotification::Kind::CLEAR_UNAVAILABLE
                                : RecoveryNotification::Kind::CLEAR_TRANSPORT;
        this->clearCommFault_();
    }

    return notification;
}

void CommController::latchCommFault_(OBC::CommFdirFaultKind kind, U32 failureCount) {
    this->m_fdirFaultLatched = true;
    this->m_fdirFaultKind = kind;
    static_cast<void>(failureCount);
}

void CommController::clearCommFault_() {
    this->m_fdirFaultLatched = false;
    this->m_fdirFaultKind = OBC::CommFdirFaultKind::NONE;
}

void CommController::emitRecoveryNotification_(const RecoveryNotification& notification) {
    if (this->m_recoverySink == nullptr) {
        return;
    }

    switch (notification.kind) {
        case RecoveryNotification::Kind::SUBMIT_UNAVAILABLE:
            this->m_recoverySink->submitCommPrimaryUnavailableFault(notification.failureCount);
            break;
        case RecoveryNotification::Kind::CLEAR_UNAVAILABLE:
            this->m_recoverySink->clearCommPrimaryUnavailableFault(notification.failureCount);
            break;
        case RecoveryNotification::Kind::SUBMIT_TRANSPORT:
            this->m_recoverySink->submitCommPrimaryTransportFault(notification.failureCount);
            break;
        case RecoveryNotification::Kind::CLEAR_TRANSPORT:
            this->m_recoverySink->clearCommPrimaryTransportFault(notification.failureCount);
            break;
        case RecoveryNotification::Kind::NONE:
        default:
            break;
    }
}

OBC::CommBand CommController::currentPrimaryBand_() const {
    return this->m_primaryCommandLink;
}

bool CommController::probeSubsystemAvailability_(OBC::CommBand band) {
    const OBC::CommLinkHealthView& fallbackHealth =
        band == OBC::CommBand::SBAND ? this->m_lastSbandHealth : this->m_lastUhfHealth;

    const U16 nodeId = band == OBC::CommBand::SBAND ? this->m_sbandSubsystemHealthNodeId : this->m_uhfSubsystemHealthNodeId;
    if (this->usesSubsystemHealthForBand_(band)) {
        return this->m_commSubsystemHealthProbe->probeNodeResponsiveForRuntime(nodeId, this->m_subsystemFdirConfig.pingTimeoutMs);
    }

    return fallbackHealth.available;
}

bool CommController::probePrimaryBandAvailability_(OBC::CommBand band, bool forceProbe) {
    AsyncSubsystemProbeState* asyncState =
        band == OBC::CommBand::SBAND ? &this->m_sbandAsyncSubsystemProbe : &this->m_uhfAsyncSubsystemProbe;
    if (forceProbe && this->m_cspRuntimeOwner != nullptr && this->usesSubsystemHealthForBand_(band)) {
        asyncState->completionReady = false;
        asyncState->completionAvailable = false;
        asyncState->discardNextCompletion = asyncState->inFlight;
    }

    if (!forceProbe && this->m_cspRuntimeOwner != nullptr && this->usesSubsystemHealthForBand_(band)) {
        return this->probePrimaryBandAvailabilityAsync_(band);
    }

    U32& cooldown = this->subsystemProbeCooldownForBand_(band);
    U32& failureStreak = this->subsystemProbeFailureStreakForBand_(band);
    const bool previous = this->cachedAvailabilityForBand_(band);

    if (!forceProbe && cooldown > 0U) {
        cooldown -= 1U;
        return previous;
    }

    const U32 probePeriod = sanitizeProbePeriodTicks(this->m_subsystemFdirConfig.primaryProbePeriodTicks);
    const U32 failureThreshold =
        sanitizeProbeFailureThreshold(this->m_subsystemFdirConfig.primaryProbeFailureThreshold);
    const bool available = this->probeSubsystemAvailability_(band);
    cooldown = probePeriod - 1U;

    if (forceProbe) {
        failureStreak = available ? 0U : failureStreak + 1U;
        return available;
    }

    if (available) {
        failureStreak = 0U;
        return true;
    }

    failureStreak += 1U;
    if (!previous) {
        return false;
    }

    return failureStreak < failureThreshold;
}

void CommController::pollAsyncSubsystemProbeCompletions_() {
    if (this->m_cspRuntimeOwner == nullptr) {
        return;
    }

    auto pollOne = [this](AsyncSubsystemProbeState& state) {
        if (!state.inFlight) {
            return;
        }
        OBC::AsyncCspPingCompletion completion = {};
        if (!this->m_cspRuntimeOwner->takeAsyncPingCompletion(state.handle, completion)) {
            return;
        }
        state.inFlight = false;
        if (state.discardNextCompletion) {
            state.discardNextCompletion = false;
            state.completionReady = false;
            state.completionAvailable = false;
            return;
        }
        state.completionReady = true;
        state.completionAvailable =
            completion.status == OBC::CSP::RuntimeStatus::OK && completion.success;
    };

    pollOne(this->m_sbandAsyncSubsystemProbe);
    pollOne(this->m_uhfAsyncSubsystemProbe);
}

bool CommController::probePrimaryBandAvailabilityAsync_(OBC::CommBand band) {
    AsyncSubsystemProbeState& asyncState =
        band == OBC::CommBand::SBAND ? this->m_sbandAsyncSubsystemProbe : this->m_uhfAsyncSubsystemProbe;
    U32& cooldown = this->subsystemProbeCooldownForBand_(band);
    U32& failureStreak = this->subsystemProbeFailureStreakForBand_(band);
    const bool previous = this->cachedAvailabilityForBand_(band);

    const U32 probePeriod = sanitizeProbePeriodTicks(this->m_subsystemFdirConfig.primaryProbePeriodTicks);
    const U32 failureThreshold =
        sanitizeProbeFailureThreshold(this->m_subsystemFdirConfig.primaryProbeFailureThreshold);

    if (asyncState.completionReady) {
        const bool available = asyncState.completionAvailable;
        asyncState.completionReady = false;
        cooldown = probePeriod - 1U;

        if (available) {
            failureStreak = 0U;
            return true;
        }

        failureStreak += 1U;
        if (!previous) {
            return false;
        }
        return failureStreak < failureThreshold;
    }

    if (cooldown > 0U) {
        cooldown -= 1U;
        return previous;
    }

    if (!asyncState.inFlight) {
        const U16 nodeId =
            band == OBC::CommBand::SBAND ? this->m_sbandSubsystemHealthNodeId : this->m_uhfSubsystemHealthNodeId;
        std::uint64_t handle = 0U;
        if (this->m_cspRuntimeOwner->submitAsyncPing(nodeId, this->m_subsystemFdirConfig.pingTimeoutMs, handle)) {
            asyncState.inFlight = true;
            asyncState.handle = handle;
        }
    }

    return previous;
}

bool CommController::cachedAvailabilityForBand_(OBC::CommBand band) const {
    return band == OBC::CommBand::SBAND ? this->m_sbandAvailable : this->m_uhfAvailable;
}

bool CommController::usesSubsystemHealthForBand_(OBC::CommBand band) const {
    const OBC::CommLinkHealthView& fallbackHealth =
        band == OBC::CommBand::SBAND ? this->m_lastSbandHealth : this->m_lastUhfHealth;
    const U16 nodeId = band == OBC::CommBand::SBAND ? this->m_sbandSubsystemHealthNodeId : this->m_uhfSubsystemHealthNodeId;
    return this->m_commSubsystemHealthProbe != nullptr &&
           nodeId != 0U &&
           (fallbackHealth.backendMode == OBC::COMM::GroundLinkBackendMode::COMM_CSP ||
            (this->m_subsystemFdirConfig.useSubsystemResponsiveness &&
             fallbackHealth.backendMode == OBC::COMM::GroundLinkBackendMode::DISABLED)) &&
           this->m_subsystemFdirConfig.pingTimeoutMs != 0U;
}

U32& CommController::subsystemProbeCooldownForBand_(OBC::CommBand band) {
    return band == OBC::CommBand::SBAND
               ? this->m_sbandSubsystemProbeCooldownTicks
               : this->m_uhfSubsystemProbeCooldownTicks;
}

U32& CommController::subsystemProbeFailureStreakForBand_(OBC::CommBand band) {
    return band == OBC::CommBand::SBAND ? this->m_sbandSubsystemProbeFailureStreak
                                        : this->m_uhfSubsystemProbeFailureStreak;
}

OBC::RecoveryCommActionResult CommController::performRecoveryLinkFailoverForRuntime() {
    this->lockComponent_();

    OBC::RecoveryCommActionResult result = {};
    const OBC::CommBand currentPrimary = this->currentPrimaryBand_();
    const OBC::CommBand backup = currentPrimary == OBC::CommBand::SBAND ? OBC::CommBand::UHF : OBC::CommBand::SBAND;
    const bool currentAvailable = this->refreshBandAvailability_(currentPrimary, true);
    const bool backupAvailable = this->refreshBandAvailability_(backup, true);
    const U32 sessionBefore = this->m_sessionRevokeTotal;
    const CommDownlinkOwner activeBefore = this->m_downlinkScheduler.getActiveOwner();
    const CommDownlinkOwner pendingBefore = this->m_downlinkScheduler.getPendingOwner();

    if (currentAvailable && !this->m_fdirFaultLatched) {
        result.alreadyOnHealthyPrimary = true;
    } else if (backupAvailable) {
        static_cast<void>(this->setPrimaryLinks_(backup, static_cast<U32>(CommPolicyTransitionReason::LINK_FAILOVER)));
        result.switched = backup != currentPrimary;
    } else {
        const CommDownlinkCompleteResult dropped = this->m_downlinkScheduler.dropForLinkLoss(currentPrimary);
        this->handleSchedulerResult_(dropped, nullptr);
        const FwIndexType revokePort = currentPrimary == OBC::CommBand::SBAND ? SBAND_INGRESS_PORT : UHF_INGRESS_PORT;
        if (this->suppressOwnerMatches_(revokePort, this->m_uhfBeaconSuppressSessionId)) {
            this->clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason::ROLE_INVALIDATED);
        }
        if (this->m_commandIngressAuthority != nullptr) {
            const AuthorityConfig& revokeConfig =
                revokePort == SBAND_INGRESS_PORT ? this->m_commandIngressAuthority->getIngressConfigForRuntime(SBAND_INGRESS_PORT)
                                                 : this->m_commandIngressAuthority->getIngressConfigForRuntime(UHF_INGRESS_PORT);
            const U8 serviceId = secureServiceIdForAuthorityIdentity(revokeConfig.identity);
        }
        if (revokePort == SBAND_INGRESS_PORT && this->m_sbandLiveObservabilitySessionActive &&
            this->m_sbandLiveObservabilityIngressPort == revokePort) {
            this->clearSbandLiveObservability_(this->m_sbandLiveObservabilityLastAcceptedSequence);
        }
        if (this->m_commandIngressAuthority != nullptr &&
            this->m_commandIngressAuthority->revokeIngressSource(
                revokePort,
                static_cast<U32>(CommPolicyTransitionReason::RECOVERY_NO_BACKUP),
                false)) {
            this->m_sessionRevokeTotal++;
        }
        if (this->m_commandIngressAuthority != nullptr) {
            const AuthorityConfig& revokeConfig =
                revokePort == SBAND_INGRESS_PORT ? this->m_commandIngressAuthority->getIngressConfigForRuntime(SBAND_INGRESS_PORT)
                                                 : this->m_commandIngressAuthority->getIngressConfigForRuntime(UHF_INGRESS_PORT);
            const U8 serviceId = secureServiceIdForAuthorityIdentity(revokeConfig.identity);
            if (serviceId != 0U && this->isConnected_secureAuthInvalidateOut_OutputPort(0)) {
                SecureAuthRevocation revocation;
                revocation.set_ingressPort(static_cast<U32>(revokePort));
                revocation.set_serviceId(serviceId);
                revocation.set_reason(static_cast<U32>(SecureAuthRevocationReason::INVALIDATED));
                this->secureAuthInvalidateOut_out(0, revocation);
            }
        }
        this->publishState_();
        result.noHealthyBackup = true;
    }

    const CommDownlinkOwner activeAfter = this->m_downlinkScheduler.getActiveOwner();
    const CommDownlinkOwner pendingAfter = this->m_downlinkScheduler.getPendingOwner();
    if (activeBefore != CommDownlinkOwner::NONE && activeAfter == CommDownlinkOwner::NONE) {
        result.ownersCleared++;
    }
    if (pendingBefore != CommDownlinkOwner::NONE && pendingAfter == CommDownlinkOwner::NONE) {
        result.ownersCleared++;
    }
    result.sessionsRevoked = this->m_sessionRevokeTotal - sessionBefore;
    result.finalPrimaryCommandLink = this->m_primaryCommandLink;
    result.finalPrimaryTelemetryLink = this->m_primaryTelemetryLink;
    result.finalPrimaryFileLink = this->m_primaryFileLink;

    if (result.switched || result.noHealthyBackup || result.sessionsRevoked > 0U || result.ownersCleared > 0U) {
        this->m_recoveryFailoverTotal++;
        this->m_recoveryOwnerClearTotal += result.ownersCleared;
    }
    this->log_ACTIVITY_HI_COMM_RECOVERY_FAILOVER_RESULT(result.switched,
                                                        result.noHealthyBackup,
                                                        result.sessionsRevoked,
                                                        result.ownersCleared,
                                                        result.finalPrimaryCommandLink,
                                                        result.finalPrimaryTelemetryLink,
                                                        result.finalPrimaryFileLink);

    this->unlockComponent_();
    return result;
}

Svc::SendFileResponse CommController::submitDownlink_(CommDownlinkOwner owner,
                                                      const Fw::StringBase& sourceFileName,
                                                      const Fw::StringBase& destFileName,
                                                      U32 offset,
                                                      U32 length) {
    CommDownlinkRequest request = {};
    request.owner = owner;
    request.sourceFileName = sourceFileName.toChar();
    request.destFileName = destFileName.toChar();
    request.offset = offset;
    request.length = length;

    const CommDownlinkSubmitResult result = this->m_downlinkScheduler.submit(request);
    if (!result.launchNow) {
        if (result.response.get_status() != Svc::SendFileStatus::STATUS_OK) {
            this->m_downlinkRejectTotal++;
        }
        this->publishDownlinkState_(static_cast<U32>(result.reason));
        this->publishState_();
        return result.response;
    }

    if (this->shouldDelayUhfLaunch_()) {
        this->scheduleDeferredLaunch_(result.launchRequest);
        this->publishDownlinkState_(static_cast<U32>(result.reason));
        this->publishState_();
        return result.response;
    }

    const Svc::SendFileResponse response = this->launchSelectedDownlink_(result.launchRequest);
    if (response.get_status() == Svc::SendFileStatus::STATUS_OK) {
        this->m_downlinkScheduler.confirmActiveLaunch(response.get_context());
        this->publishDownlinkState_(static_cast<U32>(result.reason));
        this->publishState_();
        return result.response;
    }

    const Svc::SendFileResponse mappedResponse(response.get_status(), result.response.get_context());
    const CommDownlinkCompleteResult launchFailed = this->m_downlinkScheduler.failActiveLaunch(false);
    this->handleSchedulerResult_(launchFailed, nullptr);
    this->m_downlinkRejectTotal++;
    this->publishState_();
    return mappedResponse;
}

void CommController::handleSchedulerResult_(const CommDownlinkCompleteResult& result,
                                            const Svc::SendFileResponse* responseOverride) {
    if (result.ownerCleared) {
        if (this->m_reliableTransfer.isActive()) {
            const CommReliableTransferStepResult cancelResult = this->m_reliableTransfer.cancel(true);
            const CommReliableTransferState& state = this->m_reliableTransfer.state();
            this->log_ACTIVITY_HI_COMM_RT_FINAL_RESULT(
                state.transferId, static_cast<U32>(cancelResult.finalResult), state.committedBytes, state.duplicateSegments);
        }
        this->clearDeferredLaunch_();
        this->publishDownlinkState_(static_cast<U32>(result.reason));
    }

    if (result.notifyDpCatalog && this->isConnected_dpFileCompleteOut_OutputPort(0)) {
        const Svc::SendFileResponse& response =
            result.hasDpCatalogResponse ? result.dpCatalogResponse : *responseOverride;
        this->dpFileCompleteOut_out(0, response);
    }

    if (result.launchPending) {
        if (this->shouldDelayUhfLaunch_()) {
            this->scheduleDeferredLaunch_(result.launchRequest);
            this->publishDownlinkState_(static_cast<U32>(CommDownlinkTransitionReason::ACCEPTED));
            this->publishState_();
            return;
        }

        const Svc::SendFileResponse response = this->launchSelectedDownlink_(result.launchRequest);
        if (response.get_status() == Svc::SendFileStatus::STATUS_OK) {
            this->m_downlinkScheduler.confirmActiveLaunch(response.get_context());
            this->publishDownlinkState_(static_cast<U32>(CommDownlinkTransitionReason::ACCEPTED));
        } else {
            const Svc::SendFileResponse mappedResponse(response.get_status(), result.launchRequest.requestContext);
            if (result.launchRequest.owner == CommDownlinkOwner::DP_CATALOG && this->isConnected_dpFileCompleteOut_OutputPort(0)) {
                this->dpFileCompleteOut_out(0, mappedResponse);
            }
            this->m_downlinkScheduler.failActiveLaunch();
            this->m_downlinkRejectTotal++;
            this->publishDownlinkState_(static_cast<U32>(CommDownlinkTransitionReason::PRIMARY_LINK_UNAVAILABLE));
        }
    }

    this->publishState_();
}

Svc::SendFileResponse CommController::launchFileRequest_(const CommDownlinkRequest& request) {
    if (!this->isConnected_sendFileOut_OutputPort(0)) {
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, 0U);
    }
    return this->sendFileOut_out(0, request.sourceFileName, request.destFileName, request.offset, request.length);
}

void CommController::publishDownlinkState_(U32 reason) {
    this->log_ACTIVITY_HI_COMM_DOWNLINK_STATE_CHANGED(static_cast<U32>(this->m_downlinkScheduler.getActiveOwner()),
                                                      static_cast<U32>(this->m_downlinkScheduler.getPendingOwner()),
                                                      reason);
}

void CommController::endPass_() {
    const bool wasActive = this->m_passActive;
    this->m_passActive = false;
    this->m_passRemainingSec = 0U;
    if (wasActive) {
        this->log_ACTIVITY_HI_COMM_PASS_END(this->m_totalPasses);
    }
    this->publishState_();
}

bool CommController::shouldDelayUhfLaunch_() const {
    return this->m_primaryFileLink == OBC::CommBand::UHF;
}

void CommController::scheduleDeferredLaunch_(const CommDownlinkRequest& request) {
    this->m_deferredLaunchPending = true;
    this->m_deferredLaunchTicksRemaining = UHF_FILE_LAUNCH_QUIESCE_TICKS;
    this->m_deferredLaunchRequest = request;
}

void CommController::processDeferredLaunch_() {
    if (!this->m_deferredLaunchPending) {
        return;
    }

    if (this->m_deferredLaunchTicksRemaining > 0U) {
        this->m_deferredLaunchTicksRemaining -= 1U;
        return;
    }

    const CommDownlinkRequest request = this->m_deferredLaunchRequest;
    this->clearDeferredLaunch_();

    const Svc::SendFileResponse response = this->launchSelectedDownlink_(request);
    if (response.get_status() == Svc::SendFileStatus::STATUS_OK) {
        this->m_downlinkScheduler.confirmActiveLaunch(response.get_context());
        this->publishState_();
        return;
    }

    const CommDownlinkCompleteResult launchFailed = this->m_downlinkScheduler.failActiveLaunch();
    this->handleSchedulerResult_(launchFailed, &response);
    this->m_downlinkRejectTotal++;
    this->publishState_();
}

void CommController::clearDeferredLaunch_() {
    this->m_deferredLaunchPending = false;
    this->m_deferredLaunchTicksRemaining = 0U;
    this->m_deferredLaunchRequest = {};
}

bool CommController::currentFileLinkSupportsReliableTransfer_() const {
    if (this->m_primaryFileLink == OBC::CommBand::SBAND) {
        return true;
    }

    if (this->m_primaryFileLink != OBC::CommBand::UHF) {
        return false;
    }

    return this->m_primaryLinkSelectionReason == static_cast<U32>(CommPolicyTransitionReason::OPERATOR_SWITCH);
}

bool CommController::shouldUseReliableTransfer_(const CommDownlinkRequest& request) const {
    if (!this->currentFileLinkSupportsReliableTransfer_() || request.offset != 0U || request.length != 0U) {
        return false;
    }
    const char* reliableTransferOutputDir = std::getenv("COMM_RT_OUTPUT_DIR");
    if (reliableTransferOutputDir == nullptr || reliableTransferOutputDir[0] == '\0') {
        return false;
    }
    const std::string sourceName = request.sourceFileName.toChar();
    return sourceName.size() >= 4U && sourceName.substr(sourceName.size() - 4U) == ".fdp";
}

U16 CommController::reliableTransferTargetNodeForBand_(OBC::CommBand band) const {
    return band == OBC::CommBand::UHF
               ? OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID
               : OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID;
}

Svc::SendFileResponse CommController::launchSelectedDownlink_(const CommDownlinkRequest& request) {
    if (!this->shouldUseReliableTransfer_(request)) {
        return this->launchFileRequest_(request);
    }

    this->log_ACTIVITY_HI_COMM_RT_ROUTE_SELECTED(request.requestContext, static_cast<U32>(request.owner));
    const Svc::SendFileResponse response = this->launchReliableTransfer_(request);
    if (response.get_status() != Svc::SendFileStatus::STATUS_OK) {
        const CommReliableTransferState& state = this->m_reliableTransfer.state();
        this->log_WARNING_HI_COMM_RT_START_FAILED(request.requestContext,
                                                  static_cast<U32>(state.lastStartFailureStage),
                                                  state.lastStartFailureDetail);
    }
    return response;
}

Svc::SendFileResponse CommController::launchReliableTransfer_(const CommDownlinkRequest& request) {
    CommReliableTransferConfig config = {};
    config.targetNode = this->reliableTransferTargetNodeForBand_(this->m_primaryFileLink);
    this->m_reliableTransfer.configure(config);
    const Svc::SendFileResponse response =
        this->m_reliableTransfer.start(request.requestContext, request.sourceFileName, request.destFileName, request.offset, request.length);
    if (response.get_status() == Svc::SendFileStatus::STATUS_OK) {
        const CommReliableTransferState& state = this->m_reliableTransfer.state();
        this->log_ACTIVITY_HI_COMM_RT_TRANSFER_STARTED(state.transferId, state.fileSize, state.totalSegments);
    }
    return response;
}

void CommController::pollReliableTransfer_() {
    if (!this->m_reliableTransfer.isActive()) {
        return;
    }

    const CommReliableTransferStepResult stepResult = this->m_reliableTransfer.step();
    const CommReliableTransferState& state = this->m_reliableTransfer.state();

    if (stepResult.progressAdvanced || stepResult.duplicateObserved) {
        this->log_ACTIVITY_LO_COMM_RT_PROGRESS(
            state.transferId, state.contiguousSegments, state.totalSegments, state.committedBytes, state.duplicateSegments);
    }
    if (stepResult.resendAttempted) {
        this->log_ACTIVITY_HI_COMM_RT_RESEND(state.transferId, state.resendCount, state.contiguousSegments);
    }
    if (stepResult.finished) {
        if (stepResult.finalResult == CommReliableTransferResult::RETRY_EXHAUSTED) {
            this->log_WARNING_HI_COMM_RT_RETRY_EXHAUSTED(
                state.transferId, state.resendCount, state.contiguousSegments, state.committedBytes);
        }
        this->log_ACTIVITY_HI_COMM_RT_FINAL_RESULT(
            state.transferId, static_cast<U32>(stepResult.finalResult), state.committedBytes, state.duplicateSegments);
        const CommDownlinkCompleteResult completion = this->m_downlinkScheduler.complete(stepResult.response);
        this->handleSchedulerResult_(completion, &stepResult.response);
    }
}

void CommController::lockComponent_() const {
    const_cast<CommController*>(this)->lock();
}

void CommController::unlockComponent_() const {
    const_cast<CommController*>(this)->unLock();
}

}  // namespace OBC

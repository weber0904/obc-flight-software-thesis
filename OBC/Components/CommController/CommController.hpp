#ifndef OBC_Components_CommController_HPP
#define OBC_Components_CommController_HPP

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandSessionRuntimeObserver.hpp"
#include "OBC/Components/CommController/CommBeaconSuppressControl.hpp"
#include "OBC/Components/CommController/CommControllerRuntime.hpp"
#include "OBC/Components/CommController/CommFdirPolicy.hpp"
#include "OBC/Components/CommController/CommDownlinkScheduler.hpp"
#include "OBC/Components/CommController/CommReliableTransfer.hpp"
#include "OBC/Components/CommController/CommControllerComponentAc.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandIngressAuthority.hpp"
#include "OBC/Components/CommEgressMux/CommEgressMux.hpp"
#include "OBC/Components/FileIngressControlProtocol/FileIngressPolicyStateSerializableAc.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthProvider.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"
#include "OBC/Components/TtcPassManager/TtcPassRuntime.hpp"

#include <mutex>

namespace OBC {

class IAsyncCspRuntimeOwner;

class CommController final : public CommControllerComponentBase,
                             public OBC::IRecoveryCommControl,
                             public OBC::ITtcPassCommStateProvider,
                             public OBC::ICommandSessionRuntimeObserver {
  public:
    static constexpr U32 UHF_BEACON_SUPPRESS_TIMEOUT_TICKS = 60U;

    explicit CommController(const char* const compName,
                            OBC::CSP::ICspRuntime& runtime = OBC::CSP::defaultRuntime());

    ~CommController() override;

    void tickForTest();

    void configureRuntime(GroundLinkHealthProvider* groundLinkHealthProvider,
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
                          const AuthorityConfig& uhfBackupIngressConfig);

    void configureCspRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime);

    Fw::CmdResponse setActiveBandForRuntime(OBC::CommBand band);

    Fw::CmdResponse startPassForRuntime(U32 durationSec);

    Fw::CmdResponse stopPassForRuntime();

    Fw::CmdResponse getStatusForRuntime();

    OBC::CommRuntimeState getStateForRuntime() const override;

    OBC::RecoveryCommActionResult performRecoveryLinkFailoverForRuntime() override;

    void onCommandSessionOpenedForRuntime(FwIndexType ingressPort,
                                          const AuthorityConfig& config,
                                          U32 sessionId,
                                          U32 sequenceNumber,
                                          bool secureAuthenticated,
                                          bool replaced) override;

    void onCommandSessionActivityForRuntime(FwIndexType ingressPort,
                                            const AuthorityConfig& config,
                                            U32 sessionId,
                                            U32 sequenceNumber) override;

    void onCommandSessionRevokedForRuntime(FwIndexType ingressPort,
                                           const AuthorityConfig& config,
                                           U32 sessionId,
                                           U32 lastAcceptedSequence,
                                           U32 reason) override;

  private:
    struct ComputedSbandLiveObservabilityState {
        bool active = false;
        OBC::CommLiveObservabilityStateReason reason = OBC::CommLiveObservabilityStateReason::NONE;
        FwIndexType ingressPort = 0U;
        AuthorityLinkRole role = AuthorityLinkRole::UNKNOWN;
        U32 sessionId = 0U;
        U32 lastAcceptedSequence = 0U;
    };

  private:
    struct AsyncSubsystemProbeState {
        bool inFlight = false;
        bool completionReady = false;
        bool completionAvailable = false;
        bool discardNextCompletion = false;
        std::uint64_t handle = 0U;
    };

  private:
    struct RecoveryNotification {
        enum class Kind {
            NONE,
            SUBMIT_UNAVAILABLE,
            CLEAR_UNAVAILABLE,
            SUBMIT_TRANSPORT,
            CLEAR_TRANSPORT,
        };

        Kind kind = Kind::NONE;
        U32 failureCount = 0U;
    };

  private:
    Svc::SendFileResponse dpFileRequestIn_handler(FwIndexType portNum,
                                                  const Fw::StringBase& sourceFileName,
                                                  const Fw::StringBase& destFileName,
                                                  U32 offset,
                                                  U32 length) override;

    void fileCompleteIn_handler(FwIndexType portNum, const Svc::SendFileResponse& response) override;

    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void COMM_SET_ACTIVE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::CommBand band) override;

    void COMM_START_PASS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 durationSec) override;

    void COMM_STOP_PASS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void COMM_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void publishState_();
    void publishExplicitRefreshTelemetry_();
    OBC::CommRuntimeState buildRuntimeStateLocked_() const;
    void updateCachedRuntimeStateLocked_();
    void refreshStatusViews_();
    void refreshStatusForQuery_();
    bool recomputeBandAvailabilityForQuery_(OBC::CommBand band);
    bool consumeAsyncProbeCompletionForQuery_(OBC::CommBand band, bool previous);
    void updateCachedAvailability_(OBC::CommBand band, bool available);
    void emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag);

    template <typename T>
    void emitStatusRefreshTelemetryValue_(FwChanIdType channelId, const T& value, Fw::Time& timeTag) {
        Fw::TlmBuffer buffer;
        const Fw::SerializeStatus status = buffer.serializeFrom(value);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));
        this->emitStatusRefreshTelemetry_(channelId, buffer, timeTag);
    }

    void syncEgressPolicy_();

    Fw::CmdResponse setPrimaryLinks_(OBC::CommBand band, U32 reason);

    void refreshLinkAvailability_();
    bool refreshBandAvailability_(OBC::CommBand band, bool forceProbe);
    void pollAsyncSubsystemProbeCompletions_();
    bool probePrimaryBandAvailabilityAsync_(OBC::CommBand band);

    void applyAuthorityProfiles_(U32 reason);

    void reconfigureIngressProfile_(FwIndexType portNum, const AuthorityConfig& config, U32 reason);
    void publishFileIngressPolicy_(FwIndexType portNum, const AuthorityConfig& config);

    bool isQualifyingUhfSuppressSession_(FwIndexType ingressPort, const AuthorityConfig& config) const;
    bool isQualifyingSbandLiveSession_(FwIndexType ingressPort, const AuthorityConfig& config) const;
    void clearSbandLiveObservability_(U32 lastAcceptedSequence);
    ComputedSbandLiveObservabilityState computeSbandLiveObservabilityState_() const;
    void publishSbandLiveObservabilityState_(const ComputedSbandLiveObservabilityState& state);
    void publishUhfBeaconSuppressState_();

    void startOrRefreshUhfBeaconSuppress_(FwIndexType ingressPort,
                                          const AuthorityConfig& config,
                                          U32 sessionId,
                                          U32 sequenceNumber,
                                          bool replaced);

    void clearUhfBeaconSuppress_(OBC::CommUhfBeaconSuppressClearReason reason);

    bool suppressOwnerMatches_(FwIndexType ingressPort, U32 sessionId) const;

    void handleLinkAvailabilityChange_(OBC::CommBand band, bool available);

    RecoveryNotification updateFdirState_(const OBC::CommLinkHealthView& sbandHealth,
                                          const OBC::CommLinkHealthView& uhfHealth);

    void latchCommFault_(OBC::CommFdirFaultKind kind, U32 failureCount);

    void clearCommFault_();

    void emitRecoveryNotification_(const RecoveryNotification& notification);

    OBC::CommBand currentPrimaryBand_() const;

    bool probeSubsystemAvailability_(OBC::CommBand band);
    bool probePrimaryBandAvailability_(OBC::CommBand band, bool forceProbe);
    bool cachedAvailabilityForBand_(OBC::CommBand band) const;
    bool usesSubsystemHealthForBand_(OBC::CommBand band) const;
    U32& subsystemProbeCooldownForBand_(OBC::CommBand band);
    U32& subsystemProbeFailureStreakForBand_(OBC::CommBand band);

    Svc::SendFileResponse submitDownlink_(CommDownlinkOwner owner,
                                          const Fw::StringBase& sourceFileName,
                                          const Fw::StringBase& destFileName,
                                          U32 offset,
                                          U32 length);

    void handleSchedulerResult_(const CommDownlinkCompleteResult& result, const Svc::SendFileResponse* responseOverride);

    Svc::SendFileResponse launchFileRequest_(const CommDownlinkRequest& request);

    void publishDownlinkState_(U32 reason);

    void endPass_();

    bool shouldDelayUhfLaunch_() const;

    void scheduleDeferredLaunch_(const CommDownlinkRequest& request);

    void processDeferredLaunch_();

    void clearDeferredLaunch_();

    bool currentFileLinkSupportsReliableTransfer_() const;

    bool shouldUseReliableTransfer_(const CommDownlinkRequest& request) const;

    U16 reliableTransferTargetNodeForBand_(OBC::CommBand band) const;

    Svc::SendFileResponse launchSelectedDownlink_(const CommDownlinkRequest& request);

    Svc::SendFileResponse launchReliableTransfer_(const CommDownlinkRequest& request);

    void pollReliableTransfer_();

    void lockComponent_() const;

    void unlockComponent_() const;

  private:
    OBC::CommBand m_activeBand;
    bool m_passActive;
    U32 m_passRemainingSec;
    U32 m_totalPasses;
    OBC::CommBand m_primaryCommandLink;
    OBC::CommBand m_primaryTelemetryLink;
    OBC::CommBand m_primaryFileLink;
    U32 m_primaryLinkSelectionReason;
    bool m_sbandAvailable;
    bool m_uhfAvailable;
    U32 m_sessionRevokeTotal;
    U32 m_downlinkRejectTotal;
    GroundLinkHealthProvider* m_groundLinkHealthProvider;
    OBC::IRecoveryRequestSink* m_recoverySink;
    CommandIngressAuthority* m_commandIngressAuthority;
    OBC::ICommBeaconSuppressControl* m_beaconSuppressControl;
    CommEgressMux* m_commEgressMux;
    OBC::ICommSubsystemHealthProbe* m_commSubsystemHealthProbe;
    OBC::IAsyncCspRuntimeOwner* m_cspRuntimeOwner;
    U16 m_sbandSubsystemHealthNodeId;
    U16 m_uhfSubsystemHealthNodeId;
    OBC::CommSubsystemFdirConfig m_subsystemFdirConfig;
    AuthorityConfig m_sbandIngressConfig;
    AuthorityConfig m_sbandBackupIngressConfig;
    AuthorityConfig m_uhfBackupIngressConfig;
    AuthorityConfig m_uhfPrimaryIngressConfig;
    CommDownlinkScheduler m_downlinkScheduler;
    bool m_deferredLaunchPending;
    U32 m_deferredLaunchTicksRemaining;
    CommDownlinkRequest m_deferredLaunchRequest;
    bool m_fdirFaultLatched;
    OBC::CommFdirFaultKind m_fdirFaultKind;
    U32 m_consecutivePrimaryUnavailable;
    U32 m_consecutivePrimaryTransportGrowth;
    OBC::CommLinkHealthView m_lastSbandHealth;
    OBC::CommLinkHealthView m_lastUhfHealth;
    U32 m_sbandSubsystemProbeCooldownTicks;
    U32 m_uhfSubsystemProbeCooldownTicks;
    U32 m_sbandSubsystemProbeFailureStreak;
    U32 m_uhfSubsystemProbeFailureStreak;
    AsyncSubsystemProbeState m_sbandAsyncSubsystemProbe;
    AsyncSubsystemProbeState m_uhfAsyncSubsystemProbe;
    U32 m_recoveryFailoverTotal;
    U32 m_recoveryOwnerClearTotal;
    bool m_sbandLiveObservabilitySessionActive;
    FwIndexType m_sbandLiveObservabilityIngressPort;
    AuthorityLinkRole m_sbandLiveObservabilityRole;
    U32 m_sbandLiveObservabilitySessionId;
    U32 m_sbandLiveObservabilityLastAcceptedSequence;
    bool m_lastPublishedSbandLiveObservabilityActive;
    OBC::CommLiveObservabilityStateReason m_lastPublishedSbandLiveObservabilityReason;
    FwIndexType m_lastPublishedSbandLiveObservabilityIngressPort;
    AuthorityLinkRole m_lastPublishedSbandLiveObservabilityRole;
    U32 m_lastPublishedSbandLiveObservabilitySessionId;
    bool m_uhfBeaconSuppressActive;
    FwIndexType m_uhfBeaconSuppressIngressPort;
    AuthorityLinkRole m_uhfBeaconSuppressRole;
    U32 m_uhfBeaconSuppressSessionId;
    U32 m_uhfBeaconSuppressLastAcceptedSequence;
    U32 m_uhfBeaconSuppressRemainingTicks;
    mutable std::mutex m_runtimeStateCacheMutex;
    OBC::CommRuntimeState m_cachedRuntimeState{};
    CommReliableTransfer m_reliableTransfer;
};

}  // namespace OBC

#endif

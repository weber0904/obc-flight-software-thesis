#ifndef OBC_CommControllerTester_HPP
#define OBC_CommControllerTester_HPP

#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "OBC/Components/CommController/CommController.hpp"
#include "OBC/Components/CommController/CommControllerGTestBase.hpp"
#include "OBC/Components/CspRuntimeOwner/AsyncCspRuntimeOwner.hpp"
#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthProvider.hpp"
#include "simulators/comm/GroundLinkBackend.hpp"

namespace OBC {

class FakeCommControllerGroundLinkBackend final : public OBC::COMM::IGroundLinkBackend {
  public:
    explicit FakeCommControllerGroundLinkBackend(
        OBC::COMM::GroundLinkBackendMode mode = OBC::COMM::GroundLinkBackendMode::COMM_CSP);

    bool start() override;

    void stop() override;

    OBC::COMM::GroundLinkReceiveStatus receive(std::string& outChunk, std::uint32_t timeoutMs) override;

    OBC::COMM::GroundLinkSendStatus send(const std::uint8_t* data, std::size_t size) override;

    OBC::COMM::GroundLinkStats getStats() const override;

    OBC::COMM::GroundLinkObservationState getObservationState() const override;

    bool observeHealth() override;

    void setConnected(bool connected);

    void setMode(OBC::COMM::GroundLinkBackendMode mode);

    void setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics healthSemantics);

    void addErrors(U32 txErrors, U32 rxErrors);

    void queueHealthObservation(bool success, bool connected);

  private:
    struct HealthReply {
        bool success;
        bool connected;
    };

    OBC::COMM::GroundLinkStats m_stats;
    OBC::COMM::GroundLinkHealthSemantics m_healthSemantics;
    U32 m_successfulStatusObservations;
    std::deque<HealthReply> m_healthReplies;
};

class FakeCommReliableTransferRuntime final : public OBC::CSP::ICspRuntime, public OBC::IAsyncCspRuntimeOwner {
  public:
    FakeCommReliableTransferRuntime();

    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig& config) override;

    OBC::CSP::RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) override;

    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t targetNode,
                                    std::uint8_t targetPort,
                                    const std::string& data) override;

    OBC::CSP::RuntimeStatus requestReply(std::uint16_t targetNode,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         void* replyData,
                                         std::size_t replyCapacity,
                                         std::size_t& replySize,
                                         std::uint32_t timeoutMs) override;

    OBC::CSP::RuntimeMetrics metrics() const override;

    void shutdown() override;

    std::uint32_t getDataFrameCount() const;

    std::uint32_t getAbortCount() const;

    std::uint32_t getBeginCount() const;

    std::uint16_t getLastBeginTargetNode() const;

    void setSyncPingResponse(std::uint16_t nodeId, bool success);
    void queueAsyncPingCompletion(std::uint16_t nodeId, OBC::CSP::RuntimeStatus status, bool success);
    bool completeNextAsyncPing();

    bool submitAsyncPing(std::uint16_t targetNode, std::uint32_t timeoutMs, std::uint64_t& handle) override;
    bool takeAsyncPingCompletion(std::uint64_t handle, OBC::AsyncCspPingCompletion& completion) override;
    bool submitAsyncRequestReply(std::uint16_t targetNode,
                                 std::uint8_t targetPort,
                                 const void* requestData,
                                 std::size_t requestSize,
                                 std::size_t replyCapacity,
                                 std::uint32_t timeoutMs,
                                 std::uint64_t& handle) override;
    bool takeAsyncRequestReplyCompletion(std::uint64_t handle, OBC::AsyncCspRequestReplyCompletion& completion) override;
    void recordCoalescedForRuntime() override {}

  private:
    struct QueuedAsyncPingCompletion {
        OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
        bool success = false;
    };

    OBC::CSP::RuntimeMetrics m_metrics;
    std::uint32_t m_dataFrameCount;
    std::uint32_t m_beginCount;
    std::uint32_t m_abortCount;
    std::uint32_t m_cancelCount;
    std::uint16_t m_lastBeginTargetNode;
    std::uint64_t m_nextAsyncHandle;
    std::map<std::uint16_t, bool> m_syncPingResponses;
    std::deque<std::pair<std::uint64_t, QueuedAsyncPingCompletion>> m_pendingAsyncPingCompletions;
    std::unordered_map<std::uint64_t, OBC::AsyncCspPingCompletion> m_asyncPingCompletions;
};

class CommControllerTester final : public CommControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 256;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    CommControllerTester();

    ~CommControllerTester() override;

    void testPrimarySwitchRequiresAvailableLink();

    void testPassLifecycleIsObserveOnly();

    void testZeroLengthPassRejected();

    void testGatewayDetachDoesNotFaultHealthyPrimarySubsystem();

    void testSbandLossFailsOverAndRestoreReturnsPrimary();

    void testSimultaneousSbandLossAndUhfRestoreFailsOver();

    void testOperatorSelectedUhfPrimaryDoesNotAutoRestoreOnSbandRecovery();

    void testUnavailableFaultClearsOnHealthyReconnectDespiteTransportNoise();

    void testUhfPrimaryDemotesSbandIngressAuthority();

    void testDpActiveRejectsAdditionalDp();

    void testInlineDpLaunchFailureDoesNotEmitCompletion();

    void testDpCompletionClearsOwner();

    void testPrimarySwitchDropsActiveDownlinkAndNotifiesDp();

    void testUhfPrimaryActiveDownlinkSuspendsPacketEgress();

    void testUhfPrimaryDeferredLaunchDropsOnLinkLoss();

    void testPrimarySubsystemPingFailureTriggersUnavailableFault();

    void testDirectTcpPrimaryDoesNotGoStale();

    void testSubsystemPrimaryIgnoresGroundTransportGrowth();

    void testConnectedOnlyCompatibilityDoesNotTriggerTransportFault();

    void testSecondarySubsystemProbeIsOnDemandOnly();

    void testPrimarySubsystemProbeUsesCadence();

    void testPrimarySubsystemSingleProbeFailureIsDebounced();

    void testPrimaryFailbackForceProbeClearsDebounceState();

    void testForcedProbeDiscardsStaleAsyncCompletion();

    void testDisabledGroundLinkConfigDoesNotLatchUnavailableFault();

    void testReliableTransferAdmissionUsesHelper();

    void testReliableTransferSwitchedUhfPrimaryUsesHelper();

    void testReliableTransferFallsBackWhenReceiverDisabled();

    void testReliableTransferBootstrapUhfPrimaryFallsBackToStockDownlink();

    void testReliableTransferFailoverToUhfFallsBackToStockDownlink();

    void testReliableTransferBusyRejectsWhileActive();

    void testReliableTransferPrimarySwitchAbortsActiveTransfer();

    void testReliableTransferFailoverAbortsActiveTransfer();

    void testReliableTransferUhfFailoverAbortsActiveTransfer();

    void testSbandLiveObservabilityStartsQuietUntilAuth();

    void testSbandLiveObservabilityOpensAfterAcceptedSession();

    void testSbandLiveObservabilityActivityDoesNotRepublishFullCommState();

    void testSbandLiveObservabilityIgnoresLegacySessionOpen();

    void testSbandLiveObservabilityClosesWhenSecureSessionIsReplacedByLegacySession();

    void testSbandLiveObservabilityClosesOnSessionRevoke();

    void testSbandLiveObservabilityClosesOnSilentRecoveryRevoke();

    void testSbandLiveObservabilityClosesOnBandSwitch();

    void testUhfBeaconSuppressStartsAfterAcceptedSessionOpen();

    void testUhfBeaconSuppressRefreshesAndTimesOut();

    void testUhfBeaconSuppressRoleSwitchClearsImmediately();

    void testUhfBeaconSuppressSessionReplaceRebindsOwner();

    void testUhfBeaconSuppressIgnoresNonQualifyingActivity();

    void testUhfPrimaryBandLeavesPacketEgressNonQuiet();

    void testUhfBackupSuppressDoesNotEnablePacketQuietWhileSbandPrimary();

    void testGetStatusRepublishesCorePostureWhenValuesUnchanged();

    void testGetStatusDoesNotRepublishDeferredOrDiagnosticTelemetry();

    void testGetStatusDoesNotAdvancePassTimerOrForceSyncProbe();

    void testGetStatusDoesNotAdvanceCommFdirDebounce();

  private:
    class FakeRecoverySink final : public OBC::IRecoveryRequestSink {
      public:
        void submitWatchdogFault(OBC::WatchdogSource) override {}
        void submitWatchdogSuppression(OBC::WatchdogSource) override {}
        void clearWatchdogFault(OBC::WatchdogSource) override {}
        void submitEpsTimeoutFault(U32) override {}
        void clearEpsTimeoutFault(U32) override {}
        void submitAdcsPollTransportFault(U32) override {}
        void clearAdcsPollTransportFault(U32) override {}
        void submitAdcsPollFreshnessFault(U32) override {}
        void clearAdcsPollFreshnessFault(U32) override {}
        void submitCommPrimaryUnavailableFault(U32 failureCount) override;
        void clearCommPrimaryUnavailableFault(U32 failureCount) override;
        void submitCommPrimaryTransportFault(U32 failureCount) override;
        void clearCommPrimaryTransportFault(U32 failureCount) override;

        U32 commUnavailableFaultCount = 0U;
        U32 commUnavailableClearCount = 0U;
        U32 commTransportFaultCount = 0U;
        U32 commTransportClearCount = 0U;
        U32 lastFailureCount = 0U;
    };

    class FakeCommSubsystemHealthProbe final : public OBC::ICommSubsystemHealthProbe {
      public:
        bool probeNodeResponsiveForRuntime(U16 nodeId, U32 timeoutMs) override;

        void setNodeResponsive(U16 nodeId, bool responsive);
        U32 getProbeCount(U16 nodeId) const;
        void resetProbeCounts();

      private:
        std::map<U16, bool> m_nodeResponsive;
        std::map<U16, U32> m_probeCounts;
    };

    class FakeBeaconSuppressControl final : public OBC::ICommBeaconSuppressControl {
      public:
        void setUhfBeaconSuppressedForRuntime(bool suppressed) override;

        bool suppressed = false;
        U32 setCalls = 0U;
    };

    void connectPorts();

    void initComponents();

    Svc::SendFileResponse from_sendFileOut_handler(FwIndexType portNum,
                                                   const Fw::StringBase& sourceFileName,
                                                   const Fw::StringBase& destFileName,
                                                   U32 offset,
                                                   U32 length) override;

    void setLinkAvailability(bool sbandConnected, bool uhfConnected);

    void setSubsystemAvailability(bool sbandResponsive, bool uhfResponsive);

    void refreshAvailability();

    void tickTimes(U32 count);

    void queueSendFileResponse(const Svc::SendFileResponse& response);

    void from_secureAuthInvalidateOut_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) override;
    void from_filePolicyOut_handler(FwIndexType portNum, const FileIngressPolicyState& policy) override;
    void from_commStatusRefreshTlmOut_handler(FwIndexType portNum,
                                              FwChanIdType id,
                                              Fw::Time& timeTag,
                                              Fw::TlmBuffer& val) override;

  private:
    FakeCommReliableTransferRuntime m_reliableTransferRuntime;
    OBC::CommController component;
    OBC::GroundLinkDriver m_sbandGroundLinkDriver;
    OBC::GroundLinkDriver m_uhfGroundLinkDriver;
    OBC::GroundLinkHealthProvider m_groundLinkHealthProvider;
    OBC::CommandIngressAuthority m_commandIngressAuthority;
    OBC::CommEgressMux m_commEgressMux;
    FakeRecoverySink m_recoverySink;
    FakeCommSubsystemHealthProbe m_commSubsystemHealthProbe;
    std::unique_ptr<FakeCommControllerGroundLinkBackend> m_sbandBackend;
    std::unique_ptr<FakeCommControllerGroundLinkBackend> m_uhfBackend;
    FakeBeaconSuppressControl m_beaconSuppressControl;
    std::vector<SecureAuthRevocation> m_secureAuthInvalidations;
    std::vector<FileIngressPolicyState> m_filePolicies;
    std::deque<Svc::SendFileResponse> m_sendFileResponses;
    U32 m_nextSendFileContext;
};

}  // namespace OBC

#endif

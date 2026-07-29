#ifndef OBC_CommandIngressAuthorityTester_HPP
#define OBC_CommandIngressAuthorityTester_HPP

#include <vector>

#include "Fw/Types/WaitEnumAc.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandIngressAuthority.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandIngressAuthorityGTestBase.hpp"

namespace OBC {

class CommandIngressAuthorityTester final : public CommandIngressAuthorityGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct ForwardedCommand {
        FwIndexType portNum;
        Fw::ComBuffer data;
        U32 context;
    };

    struct ForwardedStatus {
        FwIndexType portNum;
        FwOpcodeType opcode;
        U32 context;
        Fw::CmdResponse response;
    };

    struct SequenceControlCall {
        FwIndexType portNum;
        SequenceControlRequest request;
    };

    struct RuntimeObservation {
        enum class Kind {
            OPEN,
            ACTIVITY,
            REVOKE,
        };

        Kind kind;
        FwIndexType portNum;
        AuthorityConfig config;
        U32 sessionId;
        U32 sequenceNumber;
        bool secureAuthenticated;
        bool replaced;
        U32 reason;
    };

    struct SecureAuthActivityObservation {
        FwIndexType portNum;
        SecureAuthActivity activity;
    };

    CommandIngressAuthorityTester();
    ~CommandIngressAuthorityTester() override;

    void testAllowedCommandForwardsExactlyOnce();
    void testUnconfiguredPortFailsClosed();
    void testRestrictedMalformedFailsClosed();
    void testPrimaryIngressRejectsLegacyCommandWithoutEnvelope();
    void testLegacyEnvelopeFailsClosedBeforeSessionOrSequenceMutation();
    void testDirectOfficialSeqDispatcherRunDenied();
    void testWrapperSequenceRunRoutesToSequenceController();
    void testAuthGrantSynthesizesSecureSessionAndAcceptsSecureCommand();
    void testSecureCommandRejectsWithoutAuth();
    void testSecureCommandRejectsBadMacWithoutMutatingSequence();
    void testAuthRevokedClearsSecureSessionAndBlocksFurtherCommands();
    void testRuntimeObserverTracksSecureSessionLifecycle();

  private:
    void connectPorts();
    void initComponents();

    void from_seqCmdBuffOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;
    void from_seqCmdStatusOut_handler(FwIndexType portNum,
                                      FwOpcodeType opCode,
                                      U32 cmdSeq,
                                      const Fw::CmdResponse& response) override;
    void from_authActivityOut_handler(FwIndexType portNum, const SecureAuthActivity& activity) override;
    SequenceControlResult from_sequenceControlOut_handler(FwIndexType portNum,
                                                          const SequenceControlRequest& controlReq) override;

    Fw::ComBuffer makeCommand(FwOpcodeType opcode) const;
    Fw::ComBuffer makeSequenceRunCommand(const char* fileName, Fw::Wait waitMode) const;
    Fw::ComBuffer makeEnvelopeCommand(const char* profile,
                                      FwOpcodeType innerOpcode,
                                      U32 sessionId,
                                      U32 sequenceNumber) const;
    Fw::ComBuffer makeEnvelopeWithMutatedAuthTag(const char* profile,
                                                 FwOpcodeType innerOpcode,
                                                 U32 sessionId,
                                                 U32 sequenceNumber) const;
    Fw::ComBuffer makeSecureCommand(FwOpcodeType innerOpcode, U32 sequenceNumber) const;
    Fw::ComBuffer makeSecureCommandWithMutatedAuthTag(FwOpcodeType innerOpcode, U32 sequenceNumber) const;
    SecureAuthGrant makeSecureAuthGrant(FwIndexType ingressPort, U8 serviceId) const;
    void establishSecureAuth(FwIndexType ingressPort, U8 serviceId);
    FwOpcodeType deserializeOpcode(Fw::ComBuffer buffer) const;
    void clearObservations();

    class FakeRuntimeObserver final : public ICommandSessionRuntimeObserver {
      public:
        explicit FakeRuntimeObserver(std::vector<RuntimeObservation>& observations);

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
        std::vector<RuntimeObservation>& m_observations;
    };

  private:
    CommandIngressAuthority component;
    std::vector<ForwardedCommand> m_forwardedCommands;
    std::vector<ForwardedStatus> m_forwardedStatuses;
    std::vector<SequenceControlCall> m_sequenceControlCalls;
    std::vector<RuntimeObservation> m_runtimeObservations;
    std::vector<SecureAuthActivityObservation> m_secureAuthActivities;
    FakeRuntimeObserver m_runtimeObserver;
    SequenceControlResult m_sequenceControlResult;
};

}  // namespace OBC

#endif

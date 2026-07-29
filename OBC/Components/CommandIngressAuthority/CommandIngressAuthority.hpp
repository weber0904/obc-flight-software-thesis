#ifndef OBC_CommandIngressAuthority_HPP
#define OBC_CommandIngressAuthority_HPP

#include <array>
#include <string>

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandIngressAuthorityComponentAc.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandSessionRuntimeObserver.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandSessionSequence.hpp"
#include "OBC/Components/CommandIngressAuthority/SecureCommandV2Metadata.hpp"
#include "OBC/Components/CommandIngressAuthority/FppConstantsAc.hpp"

namespace OBC {

class CommandIngressAuthority final : public CommandIngressAuthorityComponentBase {
  public:
    explicit CommandIngressAuthority(const char* compName);

    ~CommandIngressAuthority() override;

    void configure(const AuthorityConfig& config);

    void clearIngressSources();

    void configureIngressSource(FwIndexType portNum, const AuthorityConfig& config);

    bool configurePersistentRootForRuntime(const std::string& persistentRoot);

    bool reconfigureIngressSource(FwIndexType portNum, const AuthorityConfig& config);

    bool revokeIngressSource(FwIndexType portNum, U32 reason, bool notifyObserver = true);

    bool hasOpenSessionForRuntime(FwIndexType portNum) const;

    AuthorityConfig getIngressConfigForRuntime(FwIndexType portNum) const;

    void setSessionRuntimeObserverForRuntime(ICommandSessionRuntimeObserver* observer);

  private:
    struct PendingDispatch {
        bool used = false;
        U32 context = 0U;
        FwIndexType ingressPort = 0U;
    };

    struct ActiveSessionState {
        bool open = false;
        bool secureAuthenticated = false;
        AuthorityLinkIdentity linkIdentity = AuthorityLinkIdentity::UNKNOWN;
        AuthorityLinkRole linkRole = AuthorityLinkRole::UNKNOWN;
        U8 serviceId = 0U;
        U32 sessionId = 0;
        U32 lastAcceptedSequence = 0;
        std::array<U8, SECURE_COMMAND_V2_SESSION_KEY_SIZE> secureSessionKey = {};
    };

  private:
    void seqCmdBuffIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void seqCmdStatusIn_handler(FwIndexType portNum,
                                FwOpcodeType opCode,
                                U32 cmdSeq,
                                const Fw::CmdResponse& response) override;

    void authGrantedIn_handler(FwIndexType portNum, const SecureAuthGrant& grant) override;

    void authRevokedIn_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) override;

    void sequenceControlStatusIn_handler(FwIndexType portNum, const SequenceControlStatus& status) override;

  private:
    void rejectCommand(FwIndexType portNum,
                       FwOpcodeType opcode,
                       U32 context,
                       const AuthorityDecision& decision,
                       const AuthorityConfig& config);

    void rejectEnvelope(FwIndexType portNum,
                        FwOpcodeType opcode,
                        U32 context,
                        CommandEnvelopeRejectReason reason);

    void rejectEnvelopeAuth(FwIndexType portNum,
                            FwOpcodeType opcode,
                            U32 context,
                            const AuthorityConfig& config,
                            const CommandEnvelopeMetadata& metadata,
                            CommandEnvelopeAuthRejectReason reason);

    void observeEnvelope(FwIndexType portNum,
                         const AuthorityConfig& config,
                         const CommandEnvelopeMetadata& metadata);

    void rejectSequence(FwIndexType portNum,
                        FwOpcodeType opcode,
                        U32 context,
                        const AuthorityConfig& config,
                        const CommandEnvelopeMetadata& metadata,
                        CommandSequenceRejectReason reason,
                        const Fw::CmdResponse& response);

    void rejectSession(FwIndexType portNum,
                       FwOpcodeType opcode,
                       U32 context,
                       const AuthorityConfig& config,
                       U32 sessionId,
                       U32 sequenceNumber,
                       FwOpcodeType innerOpcode,
                       U32 reason,
                       const Fw::CmdResponse& response);

    void handleSecureCommandV2(FwIndexType portNum,
                               U32 context,
                               const AuthorityConfig& config,
                               ActiveSessionState& activeSession,
                               Fw::CmdPacket& outerPacket);

    void publishActiveSession(FwIndexType portNum, const ActiveSessionState& state);

    void publishInactiveSession(FwIndexType portNum);

    void publishRemainingActiveSessionOrInactive(FwIndexType clearedPortNum);

    void clearActiveSession(FwIndexType portNum);

    void updateAcceptedSessionSequence(FwIndexType portNum, U32 sequenceNumber);

    void rejectSecureCommand(FwIndexType portNum,
                             FwOpcodeType opcode,
                             U32 context,
                             const AuthorityConfig& config,
                             U32 sessionId,
                             U32 sequenceNumber,
                             FwOpcodeType innerOpcode,
                             SecureCommandV2RejectReason reason,
                             const Fw::CmdResponse& response);

    void synthesizeSecureSessionFromAuthGrant(FwIndexType portNum, const AuthorityConfig& config, const SecureAuthGrant& grant);

    void revokeSecureSession(FwIndexType portNum, U8 serviceId, U32 reason, bool notifyObserver);

    U32 nextRuntimeSessionToken_();

    void rememberPendingDispatch(FwIndexType ingressPort, U32 context);

    bool consumePendingDispatch(FwIndexType statusPort, U32 context, FwIndexType& ingressPort);

    static CommandSequenceRejectReason sequenceRejectReason(CommandSequenceResult result);

    static Fw::CmdResponse sequenceRejectResponse(CommandSequenceResult result);

    static Fw::CmdResponse authRejectResponse(CommandEnvelopeAuthRejectReason reason);

    void writeRejectTelemetry(FwIndexType portNum,
                              FwOpcodeType opcode,
                              const AuthorityDecision& decision,
                              const AuthorityConfig& config);

    void writeEnvelopeAuthRejectTelemetry(FwIndexType portNum,
                                          const AuthorityConfig& config,
                                          const CommandEnvelopeMetadata& metadata,
                                          FwOpcodeType opcode,
                                          CommandEnvelopeAuthRejectReason reason);

    void notifySessionOpenedForRuntime(FwIndexType portNum,
                                       const AuthorityConfig& config,
                                       const ActiveSessionState& state,
                                       bool replaced);

    void notifySessionActivityForRuntime(FwIndexType portNum, const AuthorityConfig& config, const ActiveSessionState& state);

    void notifySessionRevokedForRuntime(FwIndexType portNum,
                                        const ActiveSessionState& state,
                                        U32 reason,
                                        bool notifyObserver);

  private:
    AuthorityConfig m_ingressConfigs[CommandIngressAuthorityPorts];
    U32 m_rejectTotal = 0;
    U32 m_rejectPolicy = 0;
    U32 m_rejectMalformed = 0;
    U32 m_rejectUnknownOpcode = 0;
    U32 m_rejectConfig = 0;
    U32 m_rejectUhfBackup = 0;
    U32 m_envelopeObservedTotal = 0;
    U32 m_envelopeRejectedTotal = 0;
    U32 m_envelopeAuthRejectTotal = 0;
    U32 m_envelopeAuthRejectConfig = 0;
    U32 m_envelopeAuthRejectSourceMismatch = 0;
    U32 m_envelopeAuthRejectUnknownKeySlot = 0;
    U32 m_envelopeAuthRejectBadMac = 0;
    U32 m_sequenceRejectTotal = 0;
    U32 m_sequenceRejectNotIncreasing = 0;
    U32 m_sequenceRejectWindowFull = 0;
    U32 m_sessionOpenTotal = 0;
    U32 m_sessionRejectTotal = 0;
    U32 m_sessionRevokeTotal = 0;
    U32 m_secureCommandRejectTotal = 0;
    U32 m_secureCommandRejectBadMac = 0;
    U32 m_secureCommandRejectNoAuth = 0;
    bool m_lastPublishedSessionActive = false;
    FwIndexType m_lastPublishedSessionPort = 0;
    ActiveSessionState m_activeSessions[CommandIngressAuthorityPorts];
    CommandSequenceWindow m_sequenceWindow;
    PendingDispatch m_pendingDispatches[CommandIngressAuthorityPorts * 4] = {};
    FwIndexType m_nextPendingDispatchSlot = 0U;
    U32 m_nextRuntimeSessionToken = 1U;
    ICommandSessionRuntimeObserver* m_sessionRuntimeObserver = nullptr;
};

}  // namespace OBC

#endif

#include "OBC/Components/CommandIngressAuthority/CommandIngressAuthority.hpp"

#include <string>

#include <Fw/Cmd/CmdPacket.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/StringTemplate.hpp>
#include <Fw/Types/WaitEnumAc.hpp>

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityCatalog.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp"
#include "OBC/Components/CommandIngressAuthority/FppConstantsAc.hpp"
#include "OBC/Components/SequenceControlProtocol/SequenceFileNameAliasAc.hpp"
#include "OBC/Components/SequenceAdmissionController/OfficialSequenceOpcodes.hpp"

namespace OBC {

namespace {

constexpr FwOpcodeType UNKNOWN_COMMAND_OPCODE = static_cast<FwOpcodeType>(0xFFFFFFFFU);
constexpr U32 UNKNOWN_INGRESS_PORT = 0xFFFFFFFFU;

CommandSessionKey sessionKeyFor(FwIndexType ingressPort,
                                const AuthorityConfig& config,
                                const CommandEnvelopeMetadata& metadata) {
    CommandSessionKey key;
    key.ingressPort = ingressPort;
    key.linkIdentity = config.identity;
    key.linkRole = config.role;
    key.sessionId = metadata.sessionId;
    return key;
}

bool requiresAuthenticatedEnvelope(const AuthorityConfig& config) {
    return isCommManagedAuthority(config);
}

U8 secureServiceIdForConfig(const AuthorityConfig& config) {
    return secureServiceIdForAuthorityIdentity(config.identity);
}

Fw::CmdResponse secureCommandRejectResponse(SecureCommandV2RejectReason reason) {
    switch (reason) {
        case SecureCommandV2RejectReason::BAD_MAGIC:
        case SecureCommandV2RejectReason::UNSUPPORTED_VERSION:
        case SecureCommandV2RejectReason::NONZERO_FLAGS:
        case SecureCommandV2RejectReason::BAD_HEADER_LENGTH:
        case SecureCommandV2RejectReason::NONZERO_RESERVED:
        case SecureCommandV2RejectReason::TRUNCATED_HEADER:
        case SecureCommandV2RejectReason::TRUNCATED_INNER_COMMAND:
        case SecureCommandV2RejectReason::OVERSIZED_INNER_COMMAND:
        case SecureCommandV2RejectReason::BAD_MAC_LENGTH:
        case SecureCommandV2RejectReason::TRUNCATED_AUTH_TAG:
        case SecureCommandV2RejectReason::UNEXPECTED_TRAILING_BYTES:
        case SecureCommandV2RejectReason::INVALID_INNER_COMMAND:
            return Fw::CmdResponse::FORMAT_ERROR;
        case SecureCommandV2RejectReason::AUTH_REQUIRED:
        case SecureCommandV2RejectReason::SERVICE_MISMATCH:
        case SecureCommandV2RejectReason::BAD_MAC:
        case SecureCommandV2RejectReason::SEQUENCE_NOT_INCREASING:
            return Fw::CmdResponse::VALIDATION_ERROR;
        case SecureCommandV2RejectReason::SEQUENCE_WINDOW_FULL:
        case SecureCommandV2RejectReason::INVALID_CONFIG:
            return Fw::CmdResponse::EXECUTION_ERROR;
        case SecureCommandV2RejectReason::NONE:
        default:
            return Fw::CmdResponse::EXECUTION_ERROR;
    }
}

AuthorityDecision envelopeRequiredDecision(FwOpcodeType opcode) {
    AuthorityDecision decision;
    decision.reason = AuthorityRejectReason::ENVELOPE_REQUIRED;
    decision.response = Fw::CmdResponse::VALIDATION_ERROR;

    const CommandAuthorityCatalogEntry* entry = findCommandAuthorityCatalogEntry(opcode);
    if (entry != nullptr) {
        decision.commandClass = entry->commandClass;
        decision.resource = entry->resource;
    }

    return decision;
}

bool isGovernedSequenceWrapperOpcode(FwOpcodeType opcode) {
    return opcode == OBC_SEQ_VALIDATE_OPCODE || opcode == OBC_SEQ_RUN_OPCODE || opcode == OBC_SEQ_PREPARE_MANUAL_OPCODE ||
           opcode == OBC_SEQ_START_OPCODE || opcode == OBC_SEQ_STEP_OPCODE || opcode == OBC_SEQ_CANCEL_OPCODE ||
           opcode == OBC_SEQ_LOG_STATUS_OPCODE;
}

bool isDeniedDirectOfficialSequencingOpcode(FwOpcodeType opcode) {
    return opcode == OBC_SEQ_DISPATCHER_RUN_OPCODE || opcode == OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_VALIDATE_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_RUN_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_VALIDATE_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_START_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_STEP_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_STEP_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_AUTO_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_MANUAL_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_CANCEL_OPCODE(OBC_CMD_SEQ_B_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_JOIN_WAIT_OPCODE(OBC_CMD_SEQ_A_BASE_ID) ||
           opcode == OBC_CMD_SEQ_CS_JOIN_WAIT_OPCODE(OBC_CMD_SEQ_B_BASE_ID);
}

bool parseSequenceControlRequest(FwOpcodeType opcode,
                                 Fw::CmdPacket packet,
                                 FwIndexType ingressPort,
                                 const AuthorityConfig& config,
                                 U32 context,
                                 SequenceControlRequest& request) {
    request = SequenceControlRequest();
    request.set_ingressPort(static_cast<U32>(ingressPort));
    request.set_linkIdentity(static_cast<U32>(config.identity));
    request.set_linkRole(static_cast<U32>(config.role));
    request.set_originalOpcode(static_cast<U32>(opcode));
    request.set_originalCmdSeq(context);
    request.set_waitMode(static_cast<U32>(Fw::Wait::NO_WAIT));
    request.set_contextId(0U);

    Fw::CmdArgBuffer args = packet.getArgBuffer();
    args.resetDeser();

    if (opcode == OBC_SEQ_VALIDATE_OPCODE) {
        SequenceFileName fileName;
        if (args.deserializeTo(fileName) != Fw::FW_SERIALIZE_OK || args.getDeserializeSizeLeft() != 0U) {
            return false;
        }
        request.set_operation(SequenceControlAction::SEQ_VALIDATE);
        request.set_fileName(fileName);
        return true;
    }
    if (opcode == OBC_SEQ_RUN_OPCODE) {
        SequenceFileName fileName;
        Fw::Wait waitMode(Fw::Wait::NO_WAIT);
        if (args.deserializeTo(fileName) != Fw::FW_SERIALIZE_OK ||
            args.deserializeTo(waitMode) != Fw::FW_SERIALIZE_OK ||
            args.getDeserializeSizeLeft() != 0U) {
            return false;
        }
        request.set_operation(SequenceControlAction::SEQ_RUN);
        request.set_fileName(fileName);
        request.set_waitMode(static_cast<U32>(waitMode.e));
        return true;
    }
    if (opcode == OBC_SEQ_PREPARE_MANUAL_OPCODE) {
        SequenceFileName fileName;
        if (args.deserializeTo(fileName) != Fw::FW_SERIALIZE_OK || args.getDeserializeSizeLeft() != 0U) {
            return false;
        }
        request.set_operation(SequenceControlAction::SEQ_PREPARE_MANUAL);
        request.set_fileName(fileName);
        return true;
    }
    if (opcode == OBC_SEQ_START_OPCODE || opcode == OBC_SEQ_STEP_OPCODE || opcode == OBC_SEQ_CANCEL_OPCODE) {
        U32 contextId = 0U;
        if (args.deserializeTo(contextId) != Fw::FW_SERIALIZE_OK || args.getDeserializeSizeLeft() != 0U) {
            return false;
        }
        request.set_operation(opcode == OBC_SEQ_START_OPCODE   ? SequenceControlAction::SEQ_START
                              : opcode == OBC_SEQ_STEP_OPCODE ? SequenceControlAction::SEQ_STEP
                                                              : SequenceControlAction::SEQ_CANCEL);
        request.set_contextId(contextId);
        return true;
    }
    if (opcode == OBC_SEQ_LOG_STATUS_OPCODE) {
        if (args.getDeserializeSizeLeft() != 0U) {
            return false;
        }
        request.set_operation(SequenceControlAction::SEQ_LOG_STATUS);
        return true;
    }

    return false;
}

}  // namespace

CommandIngressAuthority::CommandIngressAuthority(const char* compName) : CommandIngressAuthorityComponentBase(compName) {}

CommandIngressAuthority::~CommandIngressAuthority() = default;

void CommandIngressAuthority::configure(const AuthorityConfig& config) {
    this->clearIngressSources();
    this->configureIngressSource(0, config);
}

void CommandIngressAuthority::clearIngressSources() {
    this->m_sequenceWindow.resetAll();
    for (FwIndexType i = 0; i < CommandIngressAuthorityPorts; ++i) {
        this->m_ingressConfigs[i] = AuthorityConfig();
        this->m_activeSessions[i] = ActiveSessionState();
    }
    for (PendingDispatch& pending : this->m_pendingDispatches) {
        pending = PendingDispatch();
    }
    this->m_nextPendingDispatchSlot = 0U;
    this->m_lastPublishedSessionActive = false;
    this->m_lastPublishedSessionPort = 0;
    this->tlmWrite_SESSION_ACTIVE(0U);
    this->tlmWrite_SESSION_ACTIVE_PORT(0U);
    this->tlmWrite_SESSION_ACTIVE_IDENTITY(static_cast<U32>(AuthorityLinkIdentity::UNKNOWN));
    this->tlmWrite_SESSION_ACTIVE_ROLE(static_cast<U32>(AuthorityLinkRole::UNKNOWN));
    this->tlmWrite_SESSION_ACTIVE_ID(0U);
    this->tlmWrite_SESSION_LAST_ACCEPTED_SEQUENCE(0U);
    this->tlmWrite_SESSION_REVOKE_TOTAL(this->m_sessionRevokeTotal);
    this->tlmWrite_SESSION_LAST_REVOKE_PORT(UNKNOWN_INGRESS_PORT);
    this->tlmWrite_SESSION_LAST_REVOKE_IDENTITY(static_cast<U32>(AuthorityLinkIdentity::UNKNOWN));
    this->tlmWrite_SESSION_LAST_REVOKE_ROLE(static_cast<U32>(AuthorityLinkRole::UNKNOWN));
    this->tlmWrite_SESSION_LAST_REVOKE_ID(0U);
    this->tlmWrite_SESSION_LAST_REVOKE_REASON(0U);
    this->tlmWrite_SESSION_PERSISTENCE_AVAILABLE(0U);
    this->tlmWrite_SESSION_PERSISTENCE_ACTIVE_COPY(0U);
    this->tlmWrite_SESSION_PERSISTENCE_GENERATION(0U);
    this->tlmWrite_SESSION_PERSISTENCE_LOAD_FAULT_TOTAL(0U);
    this->tlmWrite_SESSION_PERSISTENCE_SAVE_FAULT_TOTAL(0U);
    this->tlmWrite_SESSION_PERSISTED_FLOOR_PORT(UNKNOWN_INGRESS_PORT);
    this->tlmWrite_SESSION_PERSISTED_FLOOR_IDENTITY(static_cast<U32>(AuthorityLinkIdentity::UNKNOWN));
    this->tlmWrite_SESSION_PERSISTED_FLOOR_ROLE(static_cast<U32>(AuthorityLinkRole::UNKNOWN));
    this->tlmWrite_SESSION_PERSISTED_FLOOR(0U);
    this->tlmWrite_SECURE_COMMAND_REJECT_TOTAL(this->m_secureCommandRejectTotal);
    this->tlmWrite_SECURE_COMMAND_REJECT_BAD_MAC(this->m_secureCommandRejectBadMac);
    this->tlmWrite_SECURE_COMMAND_REJECT_NO_AUTH(this->m_secureCommandRejectNoAuth);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_PORT(UNKNOWN_INGRESS_PORT);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_IDENTITY(static_cast<U32>(AuthorityLinkIdentity::UNKNOWN));
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_ROLE(static_cast<U32>(AuthorityLinkRole::UNKNOWN));
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_SESSION_ID(0U);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER(0U);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_INNER_OPCODE(0U);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_REASON(0U);
    this->tlmWrite_SECURE_SESSION_ACTIVE_SERVICE(0U);
    this->m_nextRuntimeSessionToken = 1U;
}

void CommandIngressAuthority::configureIngressSource(FwIndexType portNum, const AuthorityConfig& config) {
    static_cast<void>(this->reconfigureIngressSource(portNum, config));
}

bool CommandIngressAuthority::configurePersistentRootForRuntime(const std::string& persistentRoot) {
    static_cast<void>(persistentRoot);
    return true;
}

bool CommandIngressAuthority::reconfigureIngressSource(FwIndexType portNum, const AuthorityConfig& config) {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts);
    ActiveSessionState& session = this->m_activeSessions[portNum];
    const bool hadOpenSession = session.open;
    if (session.open) {
        this->m_sequenceWindow.resetSource(portNum, session.linkIdentity, session.linkRole);
    }
    this->clearActiveSession(portNum);
    this->m_ingressConfigs[portNum] = config;
    return hadOpenSession;
}

bool CommandIngressAuthority::revokeIngressSource(FwIndexType portNum, U32 reason, bool notifyObserver) {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts);

    ActiveSessionState& session = this->m_activeSessions[portNum];
    if (!session.open) {
        return false;
    }

    this->m_sequenceWindow.resetSource(portNum, session.linkIdentity, session.linkRole);
    this->m_sessionRevokeTotal++;
    this->tlmWrite_SESSION_REVOKE_TOTAL(this->m_sessionRevokeTotal);
    this->tlmWrite_SESSION_LAST_REVOKE_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SESSION_LAST_REVOKE_IDENTITY(static_cast<U32>(session.linkIdentity));
    this->tlmWrite_SESSION_LAST_REVOKE_ROLE(static_cast<U32>(session.linkRole));
    this->tlmWrite_SESSION_LAST_REVOKE_ID(session.sessionId);
    this->tlmWrite_SESSION_LAST_REVOKE_REASON(reason);
    this->log_ACTIVITY_HI_COMMAND_SESSION_REVOKED(static_cast<U32>(portNum),
                                                  static_cast<U32>(session.linkIdentity),
                                                  static_cast<U32>(session.linkRole),
                                                  session.sessionId,
                                                  reason);
    this->notifySessionRevokedForRuntime(portNum, session, reason, notifyObserver);
    this->clearActiveSession(portNum);
    return true;
}

bool CommandIngressAuthority::hasOpenSessionForRuntime(FwIndexType portNum) const {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts);
    return this->m_activeSessions[portNum].open;
}

AuthorityConfig CommandIngressAuthority::getIngressConfigForRuntime(FwIndexType portNum) const {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts);
    return this->m_ingressConfigs[portNum];
}

void CommandIngressAuthority::setSessionRuntimeObserverForRuntime(ICommandSessionRuntimeObserver* observer) {
    this->m_sessionRuntimeObserver = observer;
}

void CommandIngressAuthority::seqCmdBuffIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts);
    const AuthorityConfig& config = this->m_ingressConfigs[portNum];
    ActiveSessionState& activeSession = this->m_activeSessions[portNum];
    const FwOpcodeType secureCommandRoutedOpcode = OBC_SECURE_COMMAND_V2_OPCODE;

    Fw::CmdPacket cmdPkt;
    const Fw::SerializeStatus stat = cmdPkt.deserializeFrom(data);

    if (!config.valid) {
        AuthorityDecision decision;
        decision.reason = AuthorityRejectReason::INVALID_CONFIG;
        decision.response = Fw::CmdResponse::EXECUTION_ERROR;
        this->rejectCommand(
            portNum, stat == Fw::FW_SERIALIZE_OK ? cmdPkt.getOpCode() : UNKNOWN_COMMAND_OPCODE, context, decision, config);
        return;
    }

    if (stat != Fw::FW_SERIALIZE_OK) {
        if (requiresAuthenticatedEnvelope(config)) {
            AuthorityDecision decision;
            decision.reason = AuthorityRejectReason::MALFORMED_RESTRICTED;
            decision.response = Fw::CmdResponse::FORMAT_ERROR;
            this->rejectCommand(portNum, UNKNOWN_COMMAND_OPCODE, context, decision, config);
        } else {
            this->seqCmdBuffOut_out(portNum, data, context);
        }
        return;
    }

    const FwOpcodeType opcode = cmdPkt.getOpCode();
    if (opcode == secureCommandRoutedOpcode) {
        this->handleSecureCommandV2(portNum, context, config, activeSession, cmdPkt);
        return;
    }

    if (opcode == OBC_COMMAND_ENVELOPE_V1_OPCODE) {
        this->rejectEnvelope(portNum, opcode, context, CommandEnvelopeRejectReason::LEGACY_UNSUPPORTED);
        return;
    }

    if (requiresAuthenticatedEnvelope(config)) {
        this->rejectCommand(portNum, opcode, context, envelopeRequiredDecision(opcode), config);
        return;
    }

    if (isDeniedDirectOfficialSequencingOpcode(opcode)) {
        AuthorityDecision decision;
        decision.reason = AuthorityRejectReason::POLICY_DENIED;
        decision.response = Fw::CmdResponse::VALIDATION_ERROR;
        this->rejectCommand(portNum, opcode, context, decision, config);
        return;
    }

    if (isGovernedSequenceWrapperOpcode(opcode)) {
        SequenceControlRequest request;
        if (!parseSequenceControlRequest(opcode, cmdPkt, portNum, config, context, request)) {
            AuthorityDecision decision;
            decision.reason = AuthorityRejectReason::MALFORMED_RESTRICTED;
            decision.response = Fw::CmdResponse::FORMAT_ERROR;
            this->rejectCommand(portNum, opcode, context, decision, config);
            return;
        }
        const SequenceControlResult result = this->sequenceControlOut_out(0, request);
        if (result.get_accepted() && !result.get_deferred()) {
            this->seqCmdStatusOut_out(
                portNum, opcode, context, Fw::CmdResponse(static_cast<Fw::CmdResponse::T>(result.get_cmdResponse())));
        } else if (!result.get_accepted()) {
            this->seqCmdStatusOut_out(
                portNum, opcode, context, Fw::CmdResponse(static_cast<Fw::CmdResponse::T>(result.get_cmdResponse())));
        }
        return;
    }

    const AuthorityDecision decision = evaluateCommandAuthority(config, opcode);
    if (decision.allow) {
        this->rememberPendingDispatch(portNum, context);
        this->seqCmdBuffOut_out(portNum, data, context);
    } else {
        this->rejectCommand(portNum, opcode, context, decision, config);
    }
}

void CommandIngressAuthority::seqCmdStatusIn_handler(FwIndexType portNum,
                                                     FwOpcodeType opCode,
                                                     U32 cmdSeq,
                                                     const Fw::CmdResponse& response) {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts);
    FwIndexType consumedIngressPort = portNum;
    static_cast<void>(this->consumePendingDispatch(portNum, cmdSeq, consumedIngressPort));
    this->seqCmdStatusOut_out(portNum, opCode, cmdSeq, response);
}

void CommandIngressAuthority::authGrantedIn_handler(FwIndexType portNum, const SecureAuthGrant& grant) {
    static_cast<void>(portNum);
    FW_ASSERT(grant.get_ingressPort() < CommandIngressAuthorityPorts, grant.get_ingressPort());
    const FwIndexType ingressPort = grant.get_ingressPort();
    const AuthorityConfig& config = this->m_ingressConfigs[ingressPort];
    if (!config.valid || !isCommManagedAuthority(config) || secureServiceIdForConfig(config) != grant.get_serviceId()) {
        return;
    }

    this->synthesizeSecureSessionFromAuthGrant(ingressPort, config, grant);
}

void CommandIngressAuthority::authRevokedIn_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) {
    static_cast<void>(portNum);
    FW_ASSERT(revocation.get_ingressPort() < CommandIngressAuthorityPorts, revocation.get_ingressPort());
    this->revokeSecureSession(revocation.get_ingressPort(),
                              revocation.get_serviceId(),
                              revocation.get_reason(),
                              true);
}

void CommandIngressAuthority::sequenceControlStatusIn_handler(FwIndexType portNum, const SequenceControlStatus& status) {
    static_cast<void>(portNum);
    FW_ASSERT(status.get_ingressPort() < CommandIngressAuthorityPorts, status.get_ingressPort());
    this->seqCmdStatusOut_out(status.get_ingressPort(),
                              static_cast<FwOpcodeType>(status.get_originalOpcode()),
                              status.get_originalCmdSeq(),
                              Fw::CmdResponse(static_cast<Fw::CmdResponse::T>(status.get_cmdResponse())));
}

void CommandIngressAuthority::rejectCommand(FwIndexType portNum,
                                            FwOpcodeType opcode,
                                            U32 context,
                                            const AuthorityDecision& decision,
                                            const AuthorityConfig& config) {
    this->writeRejectTelemetry(portNum, opcode, decision, config);
    this->log_WARNING_HI_COMMAND_AUTHORITY_REJECTED(
        static_cast<U32>(opcode),
        static_cast<U32>(portNum),
        static_cast<U32>(config.identity),
        static_cast<U32>(config.role),
        static_cast<U32>(decision.commandClass),
        static_cast<U32>(decision.reason),
        decision.response);
    this->seqCmdStatusOut_out(portNum, opcode, context, decision.response);
}

void CommandIngressAuthority::rejectEnvelope(FwIndexType portNum,
                                             FwOpcodeType opcode,
                                             U32 context,
                                             CommandEnvelopeRejectReason reason) {
    this->m_envelopeRejectedTotal++;
    this->tlmWrite_ENVELOPE_REJECTED_TOTAL(this->m_envelopeRejectedTotal);
    this->tlmWrite_ENVELOPE_LAST_REJECT_REASON(static_cast<U32>(reason));
    this->log_WARNING_HI_COMMAND_ENVELOPE_REJECTED(
        static_cast<U32>(portNum), static_cast<U32>(reason), Fw::CmdResponse::FORMAT_ERROR);
    this->seqCmdStatusOut_out(portNum, opcode, context, Fw::CmdResponse::FORMAT_ERROR);
}

void CommandIngressAuthority::rejectEnvelopeAuth(FwIndexType portNum,
                                                 FwOpcodeType opcode,
                                                 U32 context,
                                                 const AuthorityConfig& config,
                                                 const CommandEnvelopeMetadata& metadata,
                                                 CommandEnvelopeAuthRejectReason reason) {
    const Fw::CmdResponse response = authRejectResponse(reason);
    this->writeEnvelopeAuthRejectTelemetry(portNum, config, metadata, opcode, reason);
    this->log_WARNING_HI_COMMAND_ENVELOPE_AUTH_REJECTED(static_cast<U32>(portNum),
                                                        static_cast<U32>(config.identity),
                                                        static_cast<U32>(config.role),
                                                        metadata.sourceId,
                                                        metadata.keySlot,
                                                        metadata.sessionId,
                                                        metadata.sequenceNumber,
                                                        static_cast<U32>(opcode),
                                                        static_cast<U32>(reason),
                                                        response);
    this->seqCmdStatusOut_out(portNum, opcode, context, response);
}

void CommandIngressAuthority::rejectSequence(FwIndexType portNum,
                                             FwOpcodeType opcode,
                                             U32 context,
                                             const AuthorityConfig& config,
                                             const CommandEnvelopeMetadata& metadata,
                                             CommandSequenceRejectReason reason,
                                             const Fw::CmdResponse& response) {
    this->m_sequenceRejectTotal++;
    bool handledReason = false;
    switch (reason) {
        case CommandSequenceRejectReason::NOT_INCREASING:
            this->m_sequenceRejectNotIncreasing++;
            handledReason = true;
            break;
        case CommandSequenceRejectReason::WINDOW_FULL:
            this->m_sequenceRejectWindowFull++;
            handledReason = true;
            break;
        case CommandSequenceRejectReason::NONE:
            FW_ASSERT(false, static_cast<FwAssertArgType>(reason));
            handledReason = true;
            break;
    }
    FW_ASSERT(handledReason, static_cast<FwAssertArgType>(reason));

    this->tlmWrite_SEQUENCE_REJECT_TOTAL(this->m_sequenceRejectTotal);
    this->tlmWrite_SEQUENCE_REJECT_NOT_INCREASING(this->m_sequenceRejectNotIncreasing);
    this->tlmWrite_SEQUENCE_REJECT_WINDOW_FULL(this->m_sequenceRejectWindowFull);
    this->tlmWrite_SEQUENCE_LAST_REJECT_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SEQUENCE_LAST_REJECT_IDENTITY(static_cast<U32>(config.identity));
    this->tlmWrite_SEQUENCE_LAST_REJECT_ROLE(static_cast<U32>(config.role));
    this->tlmWrite_SEQUENCE_LAST_REJECT_SESSION_ID(metadata.sessionId);
    this->tlmWrite_SEQUENCE_LAST_REJECT_SEQUENCE_NUMBER(metadata.sequenceNumber);
    this->tlmWrite_SEQUENCE_LAST_REJECT_INNER_OPCODE(static_cast<U32>(opcode));
    this->tlmWrite_SEQUENCE_LAST_REJECT_REASON(static_cast<U32>(reason));

    this->log_WARNING_HI_COMMAND_SEQUENCE_REJECTED(static_cast<U32>(portNum),
                                                  static_cast<U32>(config.identity),
                                                  static_cast<U32>(config.role),
                                                  metadata.sessionId,
                                                  metadata.sequenceNumber,
                                                  static_cast<U32>(opcode),
                                                  static_cast<U32>(reason),
                                                  response);
    this->seqCmdStatusOut_out(portNum, opcode, context, response);
}

void CommandIngressAuthority::rejectSession(FwIndexType portNum,
                                            FwOpcodeType opcode,
                                            U32 context,
                                            const AuthorityConfig& config,
                                            U32 sessionId,
                                            U32 sequenceNumber,
                                            FwOpcodeType innerOpcode,
                                            U32 reason,
                                            const Fw::CmdResponse& response) {
    this->m_sessionRejectTotal++;
    this->tlmWrite_SESSION_REJECT_TOTAL(this->m_sessionRejectTotal);
    this->tlmWrite_SESSION_LAST_REJECT_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SESSION_LAST_REJECT_IDENTITY(static_cast<U32>(config.identity));
    this->tlmWrite_SESSION_LAST_REJECT_ROLE(static_cast<U32>(config.role));
    this->tlmWrite_SESSION_LAST_REJECT_SESSION_ID(sessionId);
    this->tlmWrite_SESSION_LAST_REJECT_SEQUENCE_NUMBER(sequenceNumber);
    this->tlmWrite_SESSION_LAST_REJECT_INNER_OPCODE(static_cast<U32>(innerOpcode));
    this->tlmWrite_SESSION_LAST_REJECT_REASON(reason);
    this->log_WARNING_HI_COMMAND_SESSION_REJECTED(static_cast<U32>(portNum),
                                                  static_cast<U32>(config.identity),
                                                  static_cast<U32>(config.role),
                                                  sessionId,
                                                  sequenceNumber,
                                                  static_cast<U32>(innerOpcode),
                                                  reason,
                                                  response);
    this->seqCmdStatusOut_out(portNum, opcode, context, response);
}

void CommandIngressAuthority::rejectSecureCommand(FwIndexType portNum,
                                                  FwOpcodeType opcode,
                                                  U32 context,
                                                  const AuthorityConfig& config,
                                                  U32 sessionId,
                                                  U32 sequenceNumber,
                                                  FwOpcodeType innerOpcode,
                                                  SecureCommandV2RejectReason reason,
                                                  const Fw::CmdResponse& response) {
    this->m_secureCommandRejectTotal++;
    if (reason == SecureCommandV2RejectReason::BAD_MAC) {
        this->m_secureCommandRejectBadMac++;
    }
    if (reason == SecureCommandV2RejectReason::AUTH_REQUIRED || reason == SecureCommandV2RejectReason::SERVICE_MISMATCH) {
        this->m_secureCommandRejectNoAuth++;
    }

    this->tlmWrite_SECURE_COMMAND_REJECT_TOTAL(this->m_secureCommandRejectTotal);
    this->tlmWrite_SECURE_COMMAND_REJECT_BAD_MAC(this->m_secureCommandRejectBadMac);
    this->tlmWrite_SECURE_COMMAND_REJECT_NO_AUTH(this->m_secureCommandRejectNoAuth);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_IDENTITY(static_cast<U32>(config.identity));
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_ROLE(static_cast<U32>(config.role));
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_SESSION_ID(sessionId);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER(sequenceNumber);
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_INNER_OPCODE(static_cast<U32>(innerOpcode));
    this->tlmWrite_SECURE_COMMAND_LAST_REJECT_REASON(static_cast<U32>(reason));

    this->log_WARNING_HI_SECURE_COMMAND_REJECTED(static_cast<U32>(portNum),
                                                 static_cast<U32>(config.identity),
                                                 static_cast<U32>(config.role),
                                                 sessionId,
                                                 sequenceNumber,
                                                 static_cast<U32>(innerOpcode),
                                                 static_cast<U32>(reason),
                                                 response);
    this->seqCmdStatusOut_out(portNum, opcode, context, response);
}

void CommandIngressAuthority::handleSecureCommandV2(FwIndexType portNum,
                                                    U32 context,
                                                    const AuthorityConfig& config,
                                                    ActiveSessionState& activeSession,
                                                    Fw::CmdPacket& outerPacket) {
    Fw::ComBuffer innerCommand;
    const SecureCommandV2ParseResult secureCommand = parseSecureCommandV2(outerPacket, innerCommand);
    if (!secureCommand.valid) {
        this->rejectSecureCommand(portNum,
                                  secureCommand.responseOpcode,
                                  context,
                                  config,
                                  activeSession.open ? activeSession.sessionId : 0U,
                                  secureCommand.metadata.sequenceNumber,
                                  secureCommand.responseOpcode,
                                  secureCommand.reason,
                                  secureCommandRejectResponse(secureCommand.reason));
        return;
    }

    const U8 expectedServiceId = secureServiceIdForConfig(config);
    if (expectedServiceId == 0U || !activeSession.open || !activeSession.secureAuthenticated) {
        this->rejectSecureCommand(portNum,
                                  secureCommand.metadata.innerOpcode,
                                  context,
                                  config,
                                  activeSession.open ? activeSession.sessionId : 0U,
                                  secureCommand.metadata.sequenceNumber,
                                  secureCommand.metadata.innerOpcode,
                                  SecureCommandV2RejectReason::AUTH_REQUIRED,
                                  secureCommandRejectResponse(SecureCommandV2RejectReason::AUTH_REQUIRED));
        return;
    }
    if (activeSession.serviceId != expectedServiceId) {
        this->rejectSecureCommand(portNum,
                                  secureCommand.metadata.innerOpcode,
                                  context,
                                  config,
                                  activeSession.sessionId,
                                  secureCommand.metadata.sequenceNumber,
                                  secureCommand.metadata.innerOpcode,
                                  SecureCommandV2RejectReason::SERVICE_MISMATCH,
                                  secureCommandRejectResponse(SecureCommandV2RejectReason::SERVICE_MISMATCH));
        return;
    }

    const SecureCommandV2RejectReason authReject = verifySecureCommandV2Auth(
        secureCommand, innerCommand, activeSession.secureSessionKey.data());
    if (authReject != SecureCommandV2RejectReason::NONE) {
        this->rejectSecureCommand(portNum,
                                  secureCommand.metadata.innerOpcode,
                                  context,
                                  config,
                                  activeSession.sessionId,
                                  secureCommand.metadata.sequenceNumber,
                                  secureCommand.metadata.innerOpcode,
                                  authReject,
                                  secureCommandRejectResponse(authReject));
        return;
    }

    const AuthorityDecision innerDecision = evaluateCommandAuthority(config, secureCommand.metadata.innerOpcode);
    if (!innerDecision.allow) {
        this->rejectCommand(portNum, secureCommand.metadata.innerOpcode, context, innerDecision, config);
        return;
    }

    if (isDeniedDirectOfficialSequencingOpcode(secureCommand.metadata.innerOpcode)) {
        AuthorityDecision decision;
        decision.reason = AuthorityRejectReason::POLICY_DENIED;
        decision.response = Fw::CmdResponse::VALIDATION_ERROR;
        this->rejectCommand(portNum, secureCommand.metadata.innerOpcode, context, decision, config);
        return;
    }

    const bool isGovernedWrapper = isGovernedSequenceWrapperOpcode(secureCommand.metadata.innerOpcode);
    SequenceControlRequest request;
    if (isGovernedWrapper) {
        Fw::CmdPacket innerPacket;
        if (innerPacket.deserializeFrom(innerCommand) != Fw::FW_SERIALIZE_OK) {
            this->rejectSecureCommand(portNum,
                                      secureCommand.metadata.innerOpcode,
                                      context,
                                      config,
                                      activeSession.sessionId,
                                      secureCommand.metadata.sequenceNumber,
                                      secureCommand.metadata.innerOpcode,
                                      SecureCommandV2RejectReason::INVALID_INNER_COMMAND,
                                      secureCommandRejectResponse(SecureCommandV2RejectReason::INVALID_INNER_COMMAND));
            return;
        }
        if (!parseSequenceControlRequest(secureCommand.metadata.innerOpcode, innerPacket, portNum, config, context, request)) {
            this->rejectSecureCommand(portNum,
                                      secureCommand.metadata.innerOpcode,
                                      context,
                                      config,
                                      activeSession.sessionId,
                                      secureCommand.metadata.sequenceNumber,
                                      secureCommand.metadata.innerOpcode,
                                      SecureCommandV2RejectReason::INVALID_INNER_COMMAND,
                                      secureCommandRejectResponse(SecureCommandV2RejectReason::INVALID_INNER_COMMAND));
            return;
        }
    }

    CommandEnvelopeMetadata sequenceMetadata = {};
    sequenceMetadata.sessionId = activeSession.sessionId;
    sequenceMetadata.sequenceNumber = secureCommand.metadata.sequenceNumber;
    sequenceMetadata.innerOpcode = secureCommand.metadata.innerOpcode;
    const CommandSequenceResult sequenceResult =
        this->m_sequenceWindow.evaluateAndAccept(sessionKeyFor(portNum, config, sequenceMetadata),
                                                 secureCommand.metadata.sequenceNumber);
    if (sequenceResult != CommandSequenceResult::ACCEPTED) {
        const SecureCommandV2RejectReason reason =
            sequenceResult == CommandSequenceResult::REJECTED_NOT_INCREASING
                ? SecureCommandV2RejectReason::SEQUENCE_NOT_INCREASING
                : SecureCommandV2RejectReason::SEQUENCE_WINDOW_FULL;
        this->rejectSecureCommand(portNum,
                                  secureCommand.metadata.innerOpcode,
                                  context,
                                  config,
                                  activeSession.sessionId,
                                  secureCommand.metadata.sequenceNumber,
                                  secureCommand.metadata.innerOpcode,
                                  reason,
                                  secureCommandRejectResponse(reason));
        return;
    }

    if (isGovernedWrapper) {
        const SequenceControlResult result = this->sequenceControlOut_out(0, request);
        if (result.get_accepted()) {
            this->updateAcceptedSessionSequence(portNum, secureCommand.metadata.sequenceNumber);
            if (!result.get_deferred()) {
                this->seqCmdStatusOut_out(portNum,
                                          secureCommand.metadata.innerOpcode,
                                          context,
                                          Fw::CmdResponse(static_cast<Fw::CmdResponse::T>(result.get_cmdResponse())));
            }
        } else {
            this->seqCmdStatusOut_out(portNum,
                                      secureCommand.metadata.innerOpcode,
                                      context,
                                      Fw::CmdResponse(static_cast<Fw::CmdResponse::T>(result.get_cmdResponse())));
        }
        return;
    }

    this->updateAcceptedSessionSequence(portNum, secureCommand.metadata.sequenceNumber);
    this->rememberPendingDispatch(portNum, context);
    this->seqCmdBuffOut_out(portNum, innerCommand, context);
}

void CommandIngressAuthority::publishActiveSession(FwIndexType portNum, const ActiveSessionState& state) {
    this->m_lastPublishedSessionActive = state.open;
    this->m_lastPublishedSessionPort = portNum;
    this->tlmWrite_SESSION_ACTIVE(state.open ? 1U : 0U);
    this->tlmWrite_SESSION_ACTIVE_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SESSION_ACTIVE_IDENTITY(static_cast<U32>(state.linkIdentity));
    this->tlmWrite_SESSION_ACTIVE_ROLE(static_cast<U32>(state.linkRole));
    this->tlmWrite_SESSION_ACTIVE_ID(state.sessionId);
    this->tlmWrite_SESSION_LAST_ACCEPTED_SEQUENCE(state.lastAcceptedSequence);
    this->tlmWrite_SECURE_SESSION_ACTIVE_SERVICE(state.secureAuthenticated ? state.serviceId : 0U);
}

void CommandIngressAuthority::publishInactiveSession(FwIndexType portNum) {
    this->m_lastPublishedSessionActive = false;
    this->m_lastPublishedSessionPort = portNum;
    this->tlmWrite_SESSION_ACTIVE(0U);
    this->tlmWrite_SESSION_ACTIVE_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SESSION_ACTIVE_IDENTITY(static_cast<U32>(AuthorityLinkIdentity::UNKNOWN));
    this->tlmWrite_SESSION_ACTIVE_ROLE(static_cast<U32>(AuthorityLinkRole::UNKNOWN));
    this->tlmWrite_SESSION_ACTIVE_ID(0U);
    this->tlmWrite_SESSION_LAST_ACCEPTED_SEQUENCE(0U);
    this->tlmWrite_SECURE_SESSION_ACTIVE_SERVICE(0U);
}

void CommandIngressAuthority::publishRemainingActiveSessionOrInactive(FwIndexType clearedPortNum) {
    for (FwIndexType i = 0; i < CommandIngressAuthorityPorts; ++i) {
        if (this->m_activeSessions[i].open) {
            this->publishActiveSession(i, this->m_activeSessions[i]);
            return;
        }
    }
    this->publishInactiveSession(clearedPortNum);
}

void CommandIngressAuthority::clearActiveSession(FwIndexType portNum) {
    this->m_activeSessions[portNum] = ActiveSessionState();
    if (this->m_lastPublishedSessionActive && this->m_lastPublishedSessionPort == portNum) {
        this->publishRemainingActiveSessionOrInactive(portNum);
    }
}

void CommandIngressAuthority::updateAcceptedSessionSequence(FwIndexType portNum, U32 sequenceNumber) {
    ActiveSessionState& activeSession = this->m_activeSessions[portNum];
    if (!activeSession.open) {
        return;
    }
    activeSession.lastAcceptedSequence = sequenceNumber;
    this->publishActiveSession(portNum, activeSession);
    if (activeSession.secureAuthenticated && this->isConnected_authActivityOut_OutputPort(0)) {
        SecureAuthActivity activity;
        activity.set_ingressPort(static_cast<U32>(portNum));
        activity.set_serviceId(activeSession.serviceId);
        activity.set_sequenceNumber(sequenceNumber);
        this->authActivityOut_out(0, activity);
    }
    this->notifySessionActivityForRuntime(portNum, this->m_ingressConfigs[portNum], activeSession);
}

void CommandIngressAuthority::synthesizeSecureSessionFromAuthGrant(FwIndexType portNum,
                                                                   const AuthorityConfig& config,
                                                                   const SecureAuthGrant& grant) {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts, portNum);
    ActiveSessionState& activeSession = this->m_activeSessions[portNum];
    const bool replaced = activeSession.open;
    if (activeSession.open) {
        this->m_sequenceWindow.resetSource(portNum, activeSession.linkIdentity, activeSession.linkRole);
    }

    activeSession = ActiveSessionState();
    activeSession.open = true;
    activeSession.secureAuthenticated = true;
    activeSession.linkIdentity = config.identity;
    activeSession.linkRole = config.role;
    activeSession.serviceId = grant.get_serviceId();
    activeSession.sessionId = this->nextRuntimeSessionToken_();
    activeSession.lastAcceptedSequence = 0U;
    static_assert(SECURE_COMMAND_V2_SESSION_KEY_SIZE == SecureSessionKey::SIZE, "Secure session key size mismatch");
    for (FwSizeType i = 0; i < activeSession.secureSessionKey.size(); i++) {
        activeSession.secureSessionKey[i] = grant.get_sessionKey()[i];
    }

    this->m_sequenceWindow.resetSource(portNum, config.identity, config.role);
    this->m_sessionOpenTotal++;
    this->tlmWrite_SESSION_OPEN_TOTAL(this->m_sessionOpenTotal);
    this->publishActiveSession(portNum, activeSession);
    this->notifySessionOpenedForRuntime(portNum, config, activeSession, replaced);
    this->log_ACTIVITY_HI_COMMAND_SESSION_OPENED(static_cast<U32>(portNum),
                                                 static_cast<U32>(config.identity),
                                                 static_cast<U32>(config.role),
                                                 activeSession.sessionId,
                                                 replaced ? 1U : 0U);
}

void CommandIngressAuthority::revokeSecureSession(FwIndexType portNum, U8 serviceId, U32 reason, bool notifyObserver) {
    FW_ASSERT(portNum < CommandIngressAuthorityPorts, portNum);
    ActiveSessionState& session = this->m_activeSessions[portNum];
    if (!session.open || !session.secureAuthenticated || session.serviceId != serviceId) {
        return;
    }

    this->m_sequenceWindow.resetSource(portNum, session.linkIdentity, session.linkRole);
    this->m_sessionRevokeTotal++;
    this->tlmWrite_SESSION_REVOKE_TOTAL(this->m_sessionRevokeTotal);
    this->tlmWrite_SESSION_LAST_REVOKE_PORT(static_cast<U32>(portNum));
    this->tlmWrite_SESSION_LAST_REVOKE_IDENTITY(static_cast<U32>(session.linkIdentity));
    this->tlmWrite_SESSION_LAST_REVOKE_ROLE(static_cast<U32>(session.linkRole));
    this->tlmWrite_SESSION_LAST_REVOKE_ID(session.sessionId);
    this->tlmWrite_SESSION_LAST_REVOKE_REASON(reason);
    this->log_ACTIVITY_HI_COMMAND_SESSION_REVOKED(static_cast<U32>(portNum),
                                                  static_cast<U32>(session.linkIdentity),
                                                  static_cast<U32>(session.linkRole),
                                                  session.sessionId,
                                                  reason);
    this->notifySessionRevokedForRuntime(portNum, session, reason, notifyObserver);
    this->clearActiveSession(portNum);
}

U32 CommandIngressAuthority::nextRuntimeSessionToken_() {
    const U32 token = this->m_nextRuntimeSessionToken == 0U ? 1U : this->m_nextRuntimeSessionToken;
    this->m_nextRuntimeSessionToken = token + 1U;
    if (this->m_nextRuntimeSessionToken == 0U) {
        this->m_nextRuntimeSessionToken = 1U;
    }
    return token;
}

void CommandIngressAuthority::rememberPendingDispatch(FwIndexType ingressPort, U32 context) {
    for (PendingDispatch& pending : this->m_pendingDispatches) {
        if (!pending.used) {
            pending.used = true;
            pending.context = context;
            pending.ingressPort = ingressPort;
            return;
        }
    }

    PendingDispatch& pending = this->m_pendingDispatches[this->m_nextPendingDispatchSlot];
    pending.used = true;
    pending.context = context;
    pending.ingressPort = ingressPort;
    this->m_nextPendingDispatchSlot =
        (this->m_nextPendingDispatchSlot + 1U) % FW_NUM_ARRAY_ELEMENTS(this->m_pendingDispatches);
}

bool CommandIngressAuthority::consumePendingDispatch(FwIndexType statusPort, U32 context, FwIndexType& ingressPort) {
    for (PendingDispatch& pending : this->m_pendingDispatches) {
        if (pending.used && pending.context == context && pending.ingressPort == statusPort) {
            ingressPort = pending.ingressPort;
            pending = PendingDispatch();
            return true;
        }
    }

    PendingDispatch* uniqueContextMatch = nullptr;
    for (PendingDispatch& pending : this->m_pendingDispatches) {
        if (pending.used && pending.context == context) {
            if (uniqueContextMatch != nullptr) {
                return false;
            }
            uniqueContextMatch = &pending;
        }
    }

    if (uniqueContextMatch != nullptr) {
        ingressPort = uniqueContextMatch->ingressPort;
        *uniqueContextMatch = PendingDispatch();
        return true;
    }

    return false;
}

void CommandIngressAuthority::observeEnvelope(FwIndexType portNum,
                                              const AuthorityConfig& config,
                                              const CommandEnvelopeMetadata& metadata) {
    this->m_envelopeObservedTotal++;
    this->tlmWrite_ENVELOPE_OBSERVED_TOTAL(this->m_envelopeObservedTotal);
    this->tlmWrite_ENVELOPE_LAST_SESSION_ID(metadata.sessionId);
    this->tlmWrite_ENVELOPE_LAST_SEQUENCE_NUMBER(metadata.sequenceNumber);
    this->tlmWrite_ENVELOPE_LAST_INNER_OPCODE(static_cast<U32>(metadata.innerOpcode));
    this->log_ACTIVITY_HI_COMMAND_ENVELOPE_OBSERVED(
        static_cast<U32>(portNum),
        static_cast<U32>(config.identity),
        static_cast<U32>(config.role),
        metadata.sessionId,
        metadata.sequenceNumber,
        static_cast<U32>(metadata.innerOpcode));
}

void CommandIngressAuthority::notifySessionOpenedForRuntime(FwIndexType portNum,
                                                            const AuthorityConfig& config,
                                                            const ActiveSessionState& state,
                                                            bool replaced) {
    if (this->m_sessionRuntimeObserver == nullptr) {
        return;
    }
    this->m_sessionRuntimeObserver->onCommandSessionOpenedForRuntime(
        portNum, config, state.sessionId, state.lastAcceptedSequence, state.secureAuthenticated, replaced);
}

void CommandIngressAuthority::notifySessionActivityForRuntime(FwIndexType portNum,
                                                              const AuthorityConfig& config,
                                                              const ActiveSessionState& state) {
    if (this->m_sessionRuntimeObserver == nullptr) {
        return;
    }
    this->m_sessionRuntimeObserver->onCommandSessionActivityForRuntime(
        portNum, config, state.sessionId, state.lastAcceptedSequence);
}

void CommandIngressAuthority::notifySessionRevokedForRuntime(FwIndexType portNum,
                                                             const ActiveSessionState& state,
                                                             U32 reason,
                                                             bool notifyObserver) {
    if (!notifyObserver || this->m_sessionRuntimeObserver == nullptr) {
        return;
    }
    const AuthorityConfig& config = this->m_ingressConfigs[portNum];
    this->m_sessionRuntimeObserver->onCommandSessionRevokedForRuntime(
        portNum, config, state.sessionId, state.lastAcceptedSequence, reason);
}

CommandSequenceRejectReason CommandIngressAuthority::sequenceRejectReason(CommandSequenceResult result) {
    switch (result) {
        case CommandSequenceResult::REJECTED_NOT_INCREASING:
            return CommandSequenceRejectReason::NOT_INCREASING;
        case CommandSequenceResult::REJECTED_TABLE_FULL:
            return CommandSequenceRejectReason::WINDOW_FULL;
        case CommandSequenceResult::ACCEPTED:
            FW_ASSERT(false, static_cast<FwAssertArgType>(result));
            return CommandSequenceRejectReason::NONE;
    }
    FW_ASSERT(false, static_cast<FwAssertArgType>(result));
    return CommandSequenceRejectReason::NONE;
}

Fw::CmdResponse CommandIngressAuthority::sequenceRejectResponse(CommandSequenceResult result) {
    switch (result) {
        case CommandSequenceResult::REJECTED_NOT_INCREASING:
            return Fw::CmdResponse::VALIDATION_ERROR;
        case CommandSequenceResult::REJECTED_TABLE_FULL:
            return Fw::CmdResponse::EXECUTION_ERROR;
        case CommandSequenceResult::ACCEPTED:
            FW_ASSERT(false, static_cast<FwAssertArgType>(result));
            return Fw::CmdResponse::OK;
    }
    FW_ASSERT(false, static_cast<FwAssertArgType>(result));
    return Fw::CmdResponse::EXECUTION_ERROR;
}

Fw::CmdResponse CommandIngressAuthority::authRejectResponse(CommandEnvelopeAuthRejectReason reason) {
    switch (reason) {
        case CommandEnvelopeAuthRejectReason::INVALID_CONFIG:
            return Fw::CmdResponse::EXECUTION_ERROR;
        case CommandEnvelopeAuthRejectReason::SOURCE_MISMATCH:
        case CommandEnvelopeAuthRejectReason::UNKNOWN_KEY_SLOT:
        case CommandEnvelopeAuthRejectReason::BAD_MAC:
            return Fw::CmdResponse::VALIDATION_ERROR;
        case CommandEnvelopeAuthRejectReason::NONE:
            FW_ASSERT(false, static_cast<FwAssertArgType>(reason));
            return Fw::CmdResponse::OK;
    }
    FW_ASSERT(false, static_cast<FwAssertArgType>(reason));
    return Fw::CmdResponse::EXECUTION_ERROR;
}

void CommandIngressAuthority::writeRejectTelemetry(FwIndexType portNum,
                                                   FwOpcodeType opcode,
                                                   const AuthorityDecision& decision,
                                                   const AuthorityConfig& config) {
    this->m_rejectTotal++;
    switch (decision.reason) {
        case AuthorityRejectReason::POLICY_DENIED:
        case AuthorityRejectReason::ENVELOPE_REQUIRED:
            this->m_rejectPolicy++;
            break;
        case AuthorityRejectReason::MALFORMED_RESTRICTED:
            this->m_rejectMalformed++;
            break;
        case AuthorityRejectReason::INVALID_CONFIG:
            this->m_rejectConfig++;
            break;
        case AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED:
            this->m_rejectUnknownOpcode++;
            break;
        case AuthorityRejectReason::NONE:
            break;
    }
    if (config.identity == AuthorityLinkIdentity::UHF && config.role == AuthorityLinkRole::BACKUP) {
        this->m_rejectUhfBackup++;
    }

    this->tlmWrite_AUTH_REJECT_TOTAL(this->m_rejectTotal);
    this->tlmWrite_AUTH_REJECT_POLICY(this->m_rejectPolicy);
    this->tlmWrite_AUTH_REJECT_MALFORMED(this->m_rejectMalformed);
    this->tlmWrite_AUTH_REJECT_UNKNOWN_OPCODE(this->m_rejectUnknownOpcode);
    this->tlmWrite_AUTH_REJECT_CONFIG(this->m_rejectConfig);
    this->tlmWrite_AUTH_REJECT_UHF_BACKUP(this->m_rejectUhfBackup);
    this->tlmWrite_AUTH_LAST_REJECT_OPCODE(static_cast<U32>(opcode));
    this->tlmWrite_AUTH_LAST_REJECT_REASON(static_cast<U32>(decision.reason));
    this->tlmWrite_AUTH_LAST_REJECT_CLASS(static_cast<U32>(decision.commandClass));
    this->tlmWrite_AUTH_LAST_REJECT_ROLE(static_cast<U32>(config.role));
    this->tlmWrite_AUTH_LAST_REJECT_PORT(static_cast<U32>(portNum));
    this->tlmWrite_AUTH_LAST_REJECT_IDENTITY(static_cast<U32>(config.identity));
}

void CommandIngressAuthority::writeEnvelopeAuthRejectTelemetry(FwIndexType portNum,
                                                               const AuthorityConfig& config,
                                                               const CommandEnvelopeMetadata& metadata,
                                                               FwOpcodeType opcode,
                                                               CommandEnvelopeAuthRejectReason reason) {
    this->m_envelopeAuthRejectTotal++;
    switch (reason) {
        case CommandEnvelopeAuthRejectReason::INVALID_CONFIG:
            this->m_envelopeAuthRejectConfig++;
            break;
        case CommandEnvelopeAuthRejectReason::SOURCE_MISMATCH:
            this->m_envelopeAuthRejectSourceMismatch++;
            break;
        case CommandEnvelopeAuthRejectReason::UNKNOWN_KEY_SLOT:
            this->m_envelopeAuthRejectUnknownKeySlot++;
            break;
        case CommandEnvelopeAuthRejectReason::BAD_MAC:
            this->m_envelopeAuthRejectBadMac++;
            break;
        case CommandEnvelopeAuthRejectReason::NONE:
            FW_ASSERT(false, static_cast<FwAssertArgType>(reason));
            break;
    }

    this->tlmWrite_ENVELOPE_AUTH_REJECT_TOTAL(this->m_envelopeAuthRejectTotal);
    this->tlmWrite_ENVELOPE_AUTH_REJECT_CONFIG(this->m_envelopeAuthRejectConfig);
    this->tlmWrite_ENVELOPE_AUTH_REJECT_SOURCE_MISMATCH(this->m_envelopeAuthRejectSourceMismatch);
    this->tlmWrite_ENVELOPE_AUTH_REJECT_UNKNOWN_KEY_SLOT(this->m_envelopeAuthRejectUnknownKeySlot);
    this->tlmWrite_ENVELOPE_AUTH_REJECT_BAD_MAC(this->m_envelopeAuthRejectBadMac);
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_PORT(static_cast<U32>(portNum));
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_IDENTITY(static_cast<U32>(config.identity));
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_ROLE(static_cast<U32>(config.role));
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_SOURCE_ID(metadata.sourceId);
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_KEY_SLOT(metadata.keySlot);
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_SESSION_ID(metadata.sessionId);
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_SEQUENCE_NUMBER(metadata.sequenceNumber);
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_INNER_OPCODE(static_cast<U32>(opcode));
    this->tlmWrite_ENVELOPE_AUTH_LAST_REJECT_REASON(static_cast<U32>(reason));
}

}  // namespace OBC

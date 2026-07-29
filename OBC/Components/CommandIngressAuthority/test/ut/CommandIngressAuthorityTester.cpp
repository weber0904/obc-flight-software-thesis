#include "CommandIngressAuthorityTester.hpp"

#include <cstring>

#include <Fw/Com/ComPacket.hpp>
#include <Fw/Cmd/CmdString.hpp>
#include <Fw/Types/WaitEnumAc.hpp>

#include "OBC/Components/CommandIngressAuthority/CommandAuthKeystore.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp"
#include "OBC/Components/CommandIngressAuthority/SecureCommandV2Metadata.hpp"
#include "OBC/Components/SequenceAdmissionController/OfficialSequenceOpcodes.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/FppConstantsAc.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/SecureAuthRevocationReasonEnumAc.hpp"

namespace OBC {

namespace {

constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
constexpr FwOpcodeType OPCODE_MODE_GET = 268632065U;
constexpr FwOpcodeType OPCODE_MALFORMED = 0xFFFFFFFFU;
constexpr U32 TEST_CONTEXT = 0x11223344U;
constexpr U32 TEST_SESSION_ID = 42U;
constexpr U32 SBAND_SOURCE_ID = 1U;
constexpr U16 SBAND_KEY_SLOT = 1U;
constexpr U32 DEV_SOURCE_ID = 3U;
constexpr U16 DEV_KEY_SLOT = 3U;
constexpr U8 SBAND_KEY_BYTES[] = {
    0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
    0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
};
constexpr U8 DEV_KEY_BYTES[] = {
    0x50U, 0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U, 0x58U, 0x59U, 0x5AU, 0x5BU, 0x5CU, 0x5DU, 0x5EU, 0x5FU,
};
constexpr U8 TEST_SECURE_SESSION_KEY_BYTES[] = {
    0x70U, 0x71U, 0x72U, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U, 0x79U, 0x7AU, 0x7BU, 0x7CU, 0x7DU, 0x7EU, 0x7FU,
    0x80U, 0x81U, 0x82U, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U, 0x88U, 0x89U, 0x8AU, 0x8BU, 0x8CU, 0x8DU, 0x8EU, 0x8FU,
};

struct AuthMaterial {
    U32 sourceId;
    U16 keySlot;
    const U8* keyBytes;
    FwSizeType keyLength;
};

AuthMaterial authMaterialForProfile(const char* profile) {
    if (std::strcmp(profile, "sband-primary") == 0) {
        return {SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES, FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES)};
    }
    return {DEV_SOURCE_ID, DEV_KEY_SLOT, DEV_KEY_BYTES, FW_NUM_ARRAY_ELEMENTS(DEV_KEY_BYTES)};
}

AuthorityConfig authorityConfigForProfileWithAuth(const char* profile) {
    AuthorityConfig config = authorityConfigFromProfile(profile);
    const AuthMaterial auth = authMaterialForProfile(profile);
    EXPECT_TRUE(configureAuthorityAuth(config, auth.sourceId, auth.keySlot, auth.keyBytes, auth.keyLength));
    return config;
}

}  // namespace

CommandIngressAuthorityTester::CommandIngressAuthorityTester()
    : CommandIngressAuthorityGTestBase("CommandIngressAuthorityTester", MAX_HISTORY_SIZE),
      component("CommandIngressAuthority"),
      m_runtimeObserver(this->m_runtimeObservations) {
    this->initComponents();
    this->connectPorts();
    this->component.setSessionRuntimeObserverForRuntime(&this->m_runtimeObserver);
    EXPECT_TRUE(this->component.configurePersistentRootForRuntime("/tmp/command-ingress-authority-ut-unused"));
}

CommandIngressAuthorityTester::~CommandIngressAuthorityTester() = default;

CommandIngressAuthorityTester::FakeRuntimeObserver::FakeRuntimeObserver(
    std::vector<RuntimeObservation>& observations)
    : m_observations(observations) {}

void CommandIngressAuthorityTester::FakeRuntimeObserver::onCommandSessionOpenedForRuntime(FwIndexType ingressPort,
                                                                                          const AuthorityConfig& config,
                                                                                          U32 sessionId,
                                                                                          U32 sequenceNumber,
                                                                                          bool secureAuthenticated,
                                                                                          bool replaced) {
    this->m_observations.push_back(
        {RuntimeObservation::Kind::OPEN, ingressPort, config, sessionId, sequenceNumber, secureAuthenticated, replaced, 0U});
}

void CommandIngressAuthorityTester::FakeRuntimeObserver::onCommandSessionActivityForRuntime(FwIndexType ingressPort,
                                                                                            const AuthorityConfig& config,
                                                                                            U32 sessionId,
                                                                                            U32 sequenceNumber) {
    this->m_observations.push_back(
        {RuntimeObservation::Kind::ACTIVITY, ingressPort, config, sessionId, sequenceNumber, true, false, 0U});
}

void CommandIngressAuthorityTester::FakeRuntimeObserver::onCommandSessionRevokedForRuntime(FwIndexType ingressPort,
                                                                                           const AuthorityConfig& config,
                                                                                           U32 sessionId,
                                                                                           U32 lastAcceptedSequence,
                                                                                           U32 reason) {
    this->m_observations.push_back(
        {RuntimeObservation::Kind::REVOKE, ingressPort, config, sessionId, lastAcceptedSequence, false, false, reason});
}

void CommandIngressAuthorityTester::testAllowedCommandForwardsExactlyOnce() {
    this->component.configure(authorityConfigForProfileWithAuth("dev-direct"));
    Fw::ComBuffer command = this->makeCommand(OPCODE_MODE_SET);

    this->clearObservations();
    this->invoke_to_seqCmdBuffIn(0, command, TEST_CONTEXT);

    ASSERT_EQ(this->m_forwardedCommands.size(), 1U);
    EXPECT_EQ(this->m_forwardedCommands[0].portNum, 0);
    EXPECT_EQ(this->m_forwardedCommands[0].context, TEST_CONTEXT);
    ASSERT_TRUE(this->m_forwardedStatuses.empty());
    ASSERT_EVENTS_COMMAND_AUTHORITY_REJECTED_SIZE(0);
}

void CommandIngressAuthorityTester::testUnconfiguredPortFailsClosed() {
    Fw::ComBuffer command = this->makeCommand(OPCODE_MODE_GET);

    this->clearObservations();
    this->invoke_to_seqCmdBuffIn(0, command, TEST_CONTEXT);

    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, OPCODE_MODE_GET);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_TLM_AUTH_REJECT_CONFIG(0, 1U);
}

void CommandIngressAuthorityTester::testRestrictedMalformedFailsClosed() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));
    Fw::ComBuffer malformed;

    this->clearObservations();
    this->invoke_to_seqCmdBuffIn(0, malformed, TEST_CONTEXT);

    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, OPCODE_MALFORMED);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::FORMAT_ERROR);
    ASSERT_TLM_AUTH_REJECT_MALFORMED(0, 1U);
}

void CommandIngressAuthorityTester::testPrimaryIngressRejectsLegacyCommandWithoutEnvelope() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));
    Fw::ComBuffer command = this->makeCommand(OPCODE_MODE_GET);

    this->clearObservations();
    this->invoke_to_seqCmdBuffIn(0, command, TEST_CONTEXT);

    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, OPCODE_MODE_GET);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_COMMAND_AUTHORITY_REJECTED(0,
                                             OPCODE_MODE_GET,
                                             0U,
                                             static_cast<U32>(AuthorityLinkIdentity::SBAND),
                                             static_cast<U32>(AuthorityLinkRole::PRIMARY),
                                             static_cast<U32>(AuthorityCommandClass::READ_STATUS),
                                             static_cast<U32>(AuthorityRejectReason::ENVELOPE_REQUIRED),
                                             Fw::CmdResponse::VALIDATION_ERROR);
}

void CommandIngressAuthorityTester::testLegacyEnvelopeFailsClosedBeforeSessionOrSequenceMutation() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));

    this->clearObservations();
    Fw::ComBuffer legacy = this->makeEnvelopeCommand("sband-primary", OPCODE_MODE_GET, TEST_SESSION_ID, 1U);
    this->invoke_to_seqCmdBuffIn(0, legacy, TEST_CONTEXT);
    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, OBC_COMMAND_ENVELOPE_V1_OPCODE);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::FORMAT_ERROR);
    ASSERT_EVENTS_COMMAND_ENVELOPE_REJECTED(
        0, 0U, static_cast<U32>(CommandEnvelopeRejectReason::LEGACY_UNSUPPORTED), Fw::CmdResponse::FORMAT_ERROR);
    EXPECT_FALSE(this->component.hasOpenSessionForRuntime(0));

    this->clearObservations();
    this->establishSecureAuth(0, OBC::SECURE_SERVICE_SBAND);
    const U32 secureSessionId = this->m_runtimeObservations[0].sessionId;
    Fw::ComBuffer secure = this->makeSecureCommand(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, secure, TEST_CONTEXT + 1U);

    ASSERT_EQ(this->m_forwardedCommands.size(), 1U);
    EXPECT_EQ(this->deserializeOpcode(this->m_forwardedCommands[0].data), OPCODE_MODE_GET);
    ASSERT_TLM_SESSION_ACTIVE_ID(0, secureSessionId);
    ASSERT_TLM_SESSION_LAST_ACCEPTED_SEQUENCE(0, 1U);
}

void CommandIngressAuthorityTester::testDirectOfficialSeqDispatcherRunDenied() {
    this->component.configure(authorityConfigForProfileWithAuth("dev-direct"));
    Fw::ComBuffer command = this->makeCommand(OBC_SEQ_DISPATCHER_RUN_OPCODE);

    this->clearObservations();
    this->invoke_to_seqCmdBuffIn(0, command, TEST_CONTEXT);

    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_TRUE(this->m_sequenceControlCalls.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::VALIDATION_ERROR);
}

void CommandIngressAuthorityTester::testWrapperSequenceRunRoutesToSequenceController() {
    this->component.configure(authorityConfigForProfileWithAuth("dev-direct"));
    this->m_sequenceControlResult = SequenceControlResult(true, false, static_cast<U32>(Fw::CmdResponse::OK), 0U, 7U);
    Fw::ComBuffer command = this->makeSequenceRunCommand(".sequence-staging/test.seq", Fw::Wait::NO_WAIT);

    this->clearObservations();
    this->invoke_to_seqCmdBuffIn(0, command, TEST_CONTEXT);

    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_sequenceControlCalls.size(), 1U);
    EXPECT_EQ(this->m_sequenceControlCalls[0].request.get_operation(), SequenceControlAction::SEQ_RUN);
    EXPECT_STREQ(this->m_sequenceControlCalls[0].request.get_fileName().toChar(), ".sequence-staging/test.seq");
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, OBC_SEQ_RUN_OPCODE);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::OK);
}

void CommandIngressAuthorityTester::testAuthGrantSynthesizesSecureSessionAndAcceptsSecureCommand() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));

    this->clearObservations();
    this->establishSecureAuth(0, OBC::SECURE_SERVICE_SBAND);

    ASSERT_EQ(this->m_runtimeObservations.size(), 1U);
    EXPECT_EQ(this->m_runtimeObservations[0].kind, RuntimeObservation::Kind::OPEN);
    EXPECT_TRUE(this->m_runtimeObservations[0].secureAuthenticated);
    EXPECT_FALSE(this->m_runtimeObservations[0].replaced);
    ASSERT_TLM_SESSION_OPEN_TOTAL(0, 1U);
    ASSERT_TLM_SECURE_SESSION_ACTIVE_SERVICE(0, OBC::SECURE_SERVICE_SBAND);

    this->clearObservations();
    Fw::ComBuffer secure = this->makeSecureCommand(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, secure, TEST_CONTEXT);

    ASSERT_EQ(this->m_forwardedCommands.size(), 1U);
    EXPECT_EQ(this->deserializeOpcode(this->m_forwardedCommands[0].data), OPCODE_MODE_GET);
    ASSERT_TRUE(this->m_forwardedStatuses.empty());
    ASSERT_EQ(this->m_secureAuthActivities.size(), 1U);
    EXPECT_EQ(this->m_secureAuthActivities[0].activity.get_sequenceNumber(), 1U);
    ASSERT_EQ(this->m_runtimeObservations.size(), 1U);
    EXPECT_EQ(this->m_runtimeObservations[0].kind, RuntimeObservation::Kind::ACTIVITY);
    EXPECT_EQ(this->m_runtimeObservations[0].sequenceNumber, 1U);
}

void CommandIngressAuthorityTester::testSecureCommandRejectsWithoutAuth() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));

    this->clearObservations();
    Fw::ComBuffer secure = this->makeSecureCommand(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, secure, TEST_CONTEXT);

    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].opcode, OPCODE_MODE_GET);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_SECURE_COMMAND_REJECTED(0,
                                         0U,
                                         static_cast<U32>(AuthorityLinkIdentity::SBAND),
                                         static_cast<U32>(AuthorityLinkRole::PRIMARY),
                                         0U,
                                         1U,
                                         OPCODE_MODE_GET,
                                         static_cast<U32>(SecureCommandV2RejectReason::AUTH_REQUIRED),
                                         Fw::CmdResponse::VALIDATION_ERROR);
}

void CommandIngressAuthorityTester::testSecureCommandRejectsBadMacWithoutMutatingSequence() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));

    this->clearObservations();
    this->establishSecureAuth(0, OBC::SECURE_SERVICE_SBAND);
    const U32 runtimeSessionId = this->m_runtimeObservations[0].sessionId;

    this->clearObservations();
    Fw::ComBuffer badMac = this->makeSecureCommandWithMutatedAuthTag(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, badMac, TEST_CONTEXT);
    Fw::ComBuffer accepted = this->makeSecureCommand(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, accepted, TEST_CONTEXT + 1U);

    ASSERT_EQ(this->m_forwardedCommands.size(), 1U);
    EXPECT_EQ(this->deserializeOpcode(this->m_forwardedCommands[0].data), OPCODE_MODE_GET);
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_SECURE_COMMAND_REJECTED(0,
                                         0U,
                                         static_cast<U32>(AuthorityLinkIdentity::SBAND),
                                         static_cast<U32>(AuthorityLinkRole::PRIMARY),
                                         runtimeSessionId,
                                         1U,
                                         OPCODE_MODE_GET,
                                         static_cast<U32>(SecureCommandV2RejectReason::BAD_MAC),
                                         Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->m_secureAuthActivities.size(), 1U);
    EXPECT_EQ(this->m_secureAuthActivities[0].activity.get_sequenceNumber(), 1U);
    ASSERT_TLM_SESSION_LAST_ACCEPTED_SEQUENCE(0, 1U);
}

void CommandIngressAuthorityTester::testAuthRevokedClearsSecureSessionAndBlocksFurtherCommands() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));

    this->clearObservations();
    this->establishSecureAuth(0, OBC::SECURE_SERVICE_SBAND);
    const U32 runtimeSessionId = this->m_runtimeObservations[0].sessionId;

    this->clearObservations();
    SecureAuthRevocation revocation;
    revocation.set_ingressPort(0U);
    revocation.set_serviceId(OBC::SECURE_SERVICE_SBAND);
    revocation.set_reason(static_cast<U32>(SecureAuthRevocationReason::TIMEOUT));
    this->invoke_to_authRevokedIn(0, revocation);

    ASSERT_EQ(this->m_runtimeObservations.size(), 1U);
    EXPECT_EQ(this->m_runtimeObservations[0].kind, RuntimeObservation::Kind::REVOKE);
    EXPECT_EQ(this->m_runtimeObservations[0].sessionId, runtimeSessionId);
    EXPECT_EQ(this->m_runtimeObservations[0].reason, static_cast<U32>(SecureAuthRevocationReason::TIMEOUT));
    ASSERT_TLM_SECURE_SESSION_ACTIVE_SERVICE(0, 0U);

    this->clearObservations();
    Fw::ComBuffer secure = this->makeSecureCommand(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, secure, TEST_CONTEXT);
    ASSERT_TRUE(this->m_forwardedCommands.empty());
    ASSERT_EQ(this->m_forwardedStatuses.size(), 1U);
    EXPECT_EQ(this->m_forwardedStatuses[0].response, Fw::CmdResponse::VALIDATION_ERROR);
}

void CommandIngressAuthorityTester::testRuntimeObserverTracksSecureSessionLifecycle() {
    this->component.configure(authorityConfigForProfileWithAuth("sband-primary"));

    this->clearObservations();
    this->establishSecureAuth(0, OBC::SECURE_SERVICE_SBAND);
    ASSERT_EQ(this->m_runtimeObservations.size(), 1U);
    EXPECT_EQ(this->m_runtimeObservations[0].kind, RuntimeObservation::Kind::OPEN);
    EXPECT_TRUE(this->m_runtimeObservations[0].secureAuthenticated);

    this->clearObservations();
    Fw::ComBuffer secure = this->makeSecureCommand(OPCODE_MODE_GET, 1U);
    this->invoke_to_seqCmdBuffIn(0, secure, TEST_CONTEXT);
    ASSERT_EQ(this->m_runtimeObservations.size(), 1U);
    EXPECT_EQ(this->m_runtimeObservations[0].kind, RuntimeObservation::Kind::ACTIVITY);
    EXPECT_EQ(this->m_runtimeObservations[0].sequenceNumber, 1U);

    this->clearObservations();
    SecureAuthRevocation revocation;
    revocation.set_ingressPort(0U);
    revocation.set_serviceId(OBC::SECURE_SERVICE_SBAND);
    revocation.set_reason(99U);
    this->invoke_to_authRevokedIn(0, revocation);
    ASSERT_EQ(this->m_runtimeObservations.size(), 1U);
    EXPECT_EQ(this->m_runtimeObservations[0].kind, RuntimeObservation::Kind::REVOKE);
    EXPECT_EQ(this->m_runtimeObservations[0].reason, 99U);
}

void CommandIngressAuthorityTester::from_seqCmdBuffOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    this->pushFromPortEntry_seqCmdBuffOut(data, context);
    this->m_forwardedCommands.push_back({portNum, data, context});
}

void CommandIngressAuthorityTester::from_seqCmdStatusOut_handler(FwIndexType portNum,
                                                                 FwOpcodeType opCode,
                                                                 U32 cmdSeq,
                                                                 const Fw::CmdResponse& response) {
    this->pushFromPortEntry_seqCmdStatusOut(opCode, cmdSeq, response);
    this->m_forwardedStatuses.push_back({portNum, opCode, cmdSeq, response});
}

void CommandIngressAuthorityTester::from_authActivityOut_handler(FwIndexType portNum, const SecureAuthActivity& activity) {
    this->pushFromPortEntry_authActivityOut(activity);
    this->m_secureAuthActivities.push_back({portNum, activity});
}

SequenceControlResult CommandIngressAuthorityTester::from_sequenceControlOut_handler(
    FwIndexType portNum,
    const SequenceControlRequest& controlReq) {
    this->pushFromPortEntry_sequenceControlOut(controlReq);
    this->m_sequenceControlCalls.push_back({portNum, controlReq});
    return this->m_sequenceControlResult;
}

Fw::ComBuffer CommandIngressAuthorityTester::makeCommand(FwOpcodeType opcode) const {
    Fw::ComBuffer buffer;
    EXPECT_EQ(buffer.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(buffer.serializeFrom(opcode), Fw::FW_SERIALIZE_OK);
    return buffer;
}

Fw::ComBuffer CommandIngressAuthorityTester::makeSequenceRunCommand(const char* fileName, Fw::Wait waitMode) const {
    Fw::ComBuffer buffer = this->makeCommand(OBC_SEQ_RUN_OPCODE);
    EXPECT_EQ(buffer.serializeFrom(Fw::CmdStringArg(fileName)), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(buffer.serializeFrom(waitMode), Fw::FW_SERIALIZE_OK);
    return buffer;
}

Fw::ComBuffer CommandIngressAuthorityTester::makeEnvelopeCommand(const char* profile,
                                                                 FwOpcodeType innerOpcode,
                                                                 U32 sessionId,
                                                                 U32 sequenceNumber) const {
    const AuthMaterial auth = authMaterialForProfile(profile);
    return makeCommandEnvelopeV1(
        innerOpcode, auth.sourceId, auth.keySlot, auth.keyBytes, auth.keyLength, sessionId, sequenceNumber);
}

Fw::ComBuffer CommandIngressAuthorityTester::makeEnvelopeWithMutatedAuthTag(const char* profile,
                                                                            FwOpcodeType innerOpcode,
                                                                            U32 sessionId,
                                                                            U32 sequenceNumber) const {
    Fw::ComBuffer buffer = this->makeEnvelopeCommand(profile, innerOpcode, sessionId, sequenceNumber);
    U8* bytes = buffer.getBuffAddr();
    bytes[buffer.getSize() - 1U] ^= 0x5AU;
    return buffer;
}

Fw::ComBuffer CommandIngressAuthorityTester::makeSecureCommand(FwOpcodeType innerOpcode, U32 sequenceNumber) const {
    return makeSecureCommandV2(innerOpcode, TEST_SECURE_SESSION_KEY_BYTES, sequenceNumber);
}

Fw::ComBuffer CommandIngressAuthorityTester::makeSecureCommandWithMutatedAuthTag(FwOpcodeType innerOpcode,
                                                                                  U32 sequenceNumber) const {
    Fw::ComBuffer buffer = this->makeSecureCommand(innerOpcode, sequenceNumber);
    U8* bytes = buffer.getBuffAddr();
    bytes[buffer.getSize() - 1U] ^= 0xA5U;
    return buffer;
}

SecureAuthGrant CommandIngressAuthorityTester::makeSecureAuthGrant(FwIndexType ingressPort, U8 serviceId) const {
    SecureAuthGrant grant;
    grant.set_ingressPort(static_cast<U32>(ingressPort));
    grant.set_serviceId(serviceId);
    SecureSessionKey key;
    for (FwSizeType i = 0; i < key.SIZE; ++i) {
        key[i] = TEST_SECURE_SESSION_KEY_BYTES[i];
    }
    grant.set_sessionKey(key);
    return grant;
}

void CommandIngressAuthorityTester::establishSecureAuth(FwIndexType ingressPort, U8 serviceId) {
    const SecureAuthGrant grant = this->makeSecureAuthGrant(ingressPort, serviceId);
    this->invoke_to_authGrantedIn(0, grant);
}

FwOpcodeType CommandIngressAuthorityTester::deserializeOpcode(Fw::ComBuffer buffer) const {
    Fw::CmdPacket packet;
    EXPECT_EQ(packet.deserializeFrom(buffer), Fw::FW_SERIALIZE_OK);
    return packet.getOpCode();
}

void CommandIngressAuthorityTester::clearObservations() {
    this->clearHistory();
    this->m_forwardedCommands.clear();
    this->m_forwardedStatuses.clear();
    this->m_sequenceControlCalls.clear();
    this->m_runtimeObservations.clear();
    this->m_secureAuthActivities.clear();
    this->m_sequenceControlResult = SequenceControlResult();
}

}  // namespace OBC

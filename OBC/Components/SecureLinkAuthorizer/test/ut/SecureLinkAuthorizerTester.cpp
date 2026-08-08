#include "SecureLinkAuthorizerTester.hpp"

#include <cstring>

#include "Fw/Com/ComPacket.hpp"

namespace OBC {

namespace {

void storeU16(U8* dest, U16 value) {
    dest[0] = static_cast<U8>((value >> 8U) & 0xFFU);
    dest[1] = static_cast<U8>(value & 0xFFU);
}

void storeU32(U8* dest, U32 value) {
    dest[0] = static_cast<U8>((value >> 24U) & 0xFFU);
    dest[1] = static_cast<U8>((value >> 16U) & 0xFFU);
    dest[2] = static_cast<U8>((value >> 8U) & 0xFFU);
    dest[3] = static_cast<U8>(value & 0xFFU);
}

}  // namespace

SecureLinkAuthorizerTester::SecureLinkAuthorizerTester()
    : SecureLinkAuthorizerGTestBase("SecureLinkAuthorizerTester", MAX_HISTORY_SIZE), component("SecureLinkAuthorizer") {
    this->initComponents();
    this->connectPorts();
    EXPECT_TRUE(
        this->component.configureIngressService(0, OBC::SECURE_SERVICE_SBAND, this->m_sbandKey.data(), this->m_sbandKey.size()));
}

SecureLinkAuthorizerTester::~SecureLinkAuthorizerTester() = default;

void SecureLinkAuthorizerTester::testReqAuthProducesChallengeOnConfiguredIngress() {
    this->clearCaptures();
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U));

    Fw::Buffer payload = this->makeReqAuthPayload(OBC::SECURE_SERVICE_SBAND);
    ComCfg::FrameContext context;
    this->invoke_to_handshakeUplinkIn(0, payload, context);

    ASSERT_EQ(this->m_packets.size(), 1U);
    const SecureHandshakeParseResult parsed = this->parseHandshakeDownlink(this->m_packets[0]);
    ASSERT_TRUE(parsed.valid);
    EXPECT_EQ(parsed.message.type, SecureHandshakeMessageType::CHALLENGE);
    EXPECT_EQ(parsed.message.serviceId, OBC::SECURE_SERVICE_SBAND);
    ASSERT_EQ(this->m_returns.size(), 1U);
}

void SecureLinkAuthorizerTester::testMatchingResponseProducesAuthGrantAndAuthenticatedStatus() {
    this->clearCaptures();
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U));
    Fw::Buffer req = this->makeReqAuthPayload(OBC::SECURE_SERVICE_SBAND);
    ComCfg::FrameContext context;
    this->invoke_to_handshakeUplinkIn(0, req, context);
    ASSERT_EQ(this->m_packets.size(), 1U);
    const SecureHandshakeParseResult challengePacket = this->parseHandshakeDownlink(this->m_packets[0]);
    ASSERT_TRUE(challengePacket.valid);

    U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE] = {};
    ASSERT_TRUE(deriveSecureSessionKey(this->m_sbandKey.data(),
                                       this->m_sbandKey.size(),
                                       OBC::SECURE_SERVICE_SBAND,
                                       challengePacket.message.challenge.data(),
                                       sessionKey));
    U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE] = {};
    ASSERT_TRUE(computeSecureAuthResponse(sessionKey, response));

    Fw::Buffer responsePayload = this->makeResponsePayload(OBC::SECURE_SERVICE_SBAND, response);
    this->invoke_to_handshakeUplinkIn(0, responsePayload, context);

    ASSERT_EQ(this->m_grants.size(), 2U);
    for (const auto& capture : this->m_grants) {
        EXPECT_EQ(capture.grant.get_ingressPort(), 0U);
        EXPECT_EQ(capture.grant.get_serviceId(), OBC::SECURE_SERVICE_SBAND);
    }
    ASSERT_EQ(this->m_packets.size(), 2U);
    const SecureHandshakeParseResult statusPacket = this->parseHandshakeDownlink(this->m_packets[1]);
    ASSERT_TRUE(statusPacket.valid);
    EXPECT_EQ(statusPacket.message.type, SecureHandshakeMessageType::AUTH_STATUS);
    EXPECT_EQ(statusPacket.message.statusCode, static_cast<U32>(SecureAuthStatusCode::AUTHENTICATED));
}

void SecureLinkAuthorizerTester::testBadResponseReturnsNotAuthenticatedWithoutGrant() {
    this->clearCaptures();
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U));
    Fw::Buffer req = this->makeReqAuthPayload(OBC::SECURE_SERVICE_SBAND);
    ComCfg::FrameContext context;
    this->invoke_to_handshakeUplinkIn(0, req, context);
    ASSERT_EQ(this->m_packets.size(), 1U);

    U8 badResponse[SECURE_LINK_AUTH_RESPONSE_SIZE] = {};
    Fw::Buffer responsePayload = this->makeResponsePayload(OBC::SECURE_SERVICE_SBAND, badResponse);
    this->invoke_to_handshakeUplinkIn(0, responsePayload, context);

    ASSERT_EQ(this->m_grants.size(), 0U);
    ASSERT_EQ(this->m_packets.size(), 2U);
    const SecureHandshakeParseResult statusPacket = this->parseHandshakeDownlink(this->m_packets[1]);
    ASSERT_TRUE(statusPacket.valid);
    EXPECT_EQ(statusPacket.message.type, SecureHandshakeMessageType::AUTH_STATUS);
    EXPECT_EQ(statusPacket.message.statusCode, static_cast<U32>(SecureAuthStatusCode::NOT_AUTHENTICATED));
}

void SecureLinkAuthorizerTester::testUnsupportedServiceReturnsStatusWithoutGrant() {
    this->clearCaptures();
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U));

    Fw::Buffer req = this->makeReqAuthPayload(OBC::SECURE_SERVICE_UHF);
    ComCfg::FrameContext context;
    this->invoke_to_handshakeUplinkIn(0, req, context);

    ASSERT_TRUE(this->m_grants.empty());
    ASSERT_EQ(this->m_packets.size(), 1U);
    const SecureHandshakeParseResult statusPacket = this->parseHandshakeDownlink(this->m_packets[0]);
    ASSERT_TRUE(statusPacket.valid);
    EXPECT_EQ(statusPacket.message.type, SecureHandshakeMessageType::AUTH_STATUS);
    EXPECT_EQ(statusPacket.message.serviceId, OBC::SECURE_SERVICE_UHF);
    EXPECT_EQ(statusPacket.message.statusCode, static_cast<U32>(SecureAuthStatusCode::UNSUPPORTED_SERVICE));
    ASSERT_EQ(this->m_returns.size(), 1U);
}

void SecureLinkAuthorizerTester::testMalformedPacketRejectsWithoutStatusOrGrant() {
    this->clearCaptures();
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U));

    Fw::Buffer malformed = this->makeMalformedPayload();
    ComCfg::FrameContext context;
    this->invoke_to_handshakeUplinkIn(0, malformed, context);

    ASSERT_TRUE(this->m_grants.empty());
    ASSERT_TRUE(this->m_packets.empty());
    ASSERT_EQ(this->m_returns.size(), 1U);
}

void SecureLinkAuthorizerTester::testTimeoutRevokesActiveAuth() {
    this->clearCaptures();
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U));
    Fw::Buffer req = this->makeReqAuthPayload(OBC::SECURE_SERVICE_SBAND);
    ComCfg::FrameContext context;
    this->invoke_to_handshakeUplinkIn(0, req, context);
    const SecureHandshakeParseResult challengePacket = this->parseHandshakeDownlink(this->m_packets[0]);

    U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE] = {};
    ASSERT_TRUE(deriveSecureSessionKey(this->m_sbandKey.data(),
                                       this->m_sbandKey.size(),
                                       OBC::SECURE_SERVICE_SBAND,
                                       challengePacket.message.challenge.data(),
                                       sessionKey));
    U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE] = {};
    ASSERT_TRUE(computeSecureAuthResponse(sessionKey, response));
    Fw::Buffer responsePayload = this->makeResponsePayload(OBC::SECURE_SERVICE_SBAND, response);
    this->invoke_to_handshakeUplinkIn(0, responsePayload, context);
    ASSERT_EQ(this->m_grants.size(), 2U);

    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 279U, 0U));
    this->invoke_to_schedIn(0, 0U);
    ASSERT_TRUE(this->m_revokes.empty());

    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 280U, 0U));
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->m_revokes.size(), 2U);
    for (const auto& capture : this->m_revokes) {
        EXPECT_EQ(capture.revocation.get_ingressPort(), 0U);
        EXPECT_EQ(capture.revocation.get_serviceId(), OBC::SECURE_SERVICE_SBAND);
        EXPECT_EQ(capture.revocation.get_reason(), static_cast<U32>(SecureAuthRevocationReason::TIMEOUT));
    }
}

void SecureLinkAuthorizerTester::from_bufferReturnOut_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    this->pushFromPortEntry_bufferReturnOut(buffer);
    this->m_returns.push_back(
        {portNum, std::vector<U8>(buffer.getData(), buffer.getData() + static_cast<std::size_t>(buffer.getSize()))});
}

void SecureLinkAuthorizerTester::from_handshakePacketOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    static_cast<void>(context);
    this->pushFromPortEntry_handshakePacketOut(data, context);
    this->m_packets.push_back({portNum, data});
}

void SecureLinkAuthorizerTester::from_authGrantedOut_handler(FwIndexType portNum, const SecureAuthGrant& grant) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_authGrantedOut(grant);
    this->m_grants.push_back({grant});
}

void SecureLinkAuthorizerTester::from_authRevokedOut_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_authRevokedOut(revocation);
    this->m_revokes.push_back({revocation});
}

Fw::Buffer SecureLinkAuthorizerTester::makeReqAuthPayload(U8 serviceId) {
    this->m_storage.push_back({});
    std::array<U8, 64>& storage = this->m_storage.back();
    std::fill(storage.begin(), storage.end(), 0U);
    storeU16(storage.data(), static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND));
    storeU32(storage.data() + sizeof(FwPacketDescriptorType), SECURE_LINK_HANDSHAKE_MAGIC);
    storage[sizeof(FwPacketDescriptorType) + 4U] = SECURE_LINK_HANDSHAKE_VERSION;
    storage[sizeof(FwPacketDescriptorType) + 5U] = static_cast<U8>(SecureHandshakeMessageType::REQ_AUTH);
    storage[sizeof(FwPacketDescriptorType) + 6U] = serviceId;
    storage[sizeof(FwPacketDescriptorType) + 7U] = 0U;
    return Fw::Buffer(storage.data(), sizeof(FwPacketDescriptorType) + SECURE_LINK_HANDSHAKE_HEADER_SIZE);
}

Fw::Buffer SecureLinkAuthorizerTester::makeResponsePayload(U8 serviceId, const U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]) {
    this->m_storage.push_back({});
    std::array<U8, 64>& storage = this->m_storage.back();
    std::fill(storage.begin(), storage.end(), 0U);
    storeU16(storage.data(), static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND));
    storeU32(storage.data() + sizeof(FwPacketDescriptorType), SECURE_LINK_HANDSHAKE_MAGIC);
    storage[sizeof(FwPacketDescriptorType) + 4U] = SECURE_LINK_HANDSHAKE_VERSION;
    storage[sizeof(FwPacketDescriptorType) + 5U] = static_cast<U8>(SecureHandshakeMessageType::RESPONSE);
    storage[sizeof(FwPacketDescriptorType) + 6U] = serviceId;
    storage[sizeof(FwPacketDescriptorType) + 7U] = 0U;
    std::memcpy(storage.data() + sizeof(FwPacketDescriptorType) + SECURE_LINK_HANDSHAKE_HEADER_SIZE,
                response,
                SECURE_LINK_AUTH_RESPONSE_SIZE);
    return Fw::Buffer(storage.data(),
                      sizeof(FwPacketDescriptorType) + SECURE_LINK_HANDSHAKE_HEADER_SIZE +
                          SECURE_LINK_AUTH_RESPONSE_SIZE);
}

Fw::Buffer SecureLinkAuthorizerTester::makeMalformedPayload() {
    this->m_storage.push_back({});
    std::array<U8, 64>& storage = this->m_storage.back();
    std::fill(storage.begin(), storage.end(), 0U);
    storeU16(storage.data(), static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND));
    storeU32(storage.data() + sizeof(FwPacketDescriptorType), SECURE_LINK_HANDSHAKE_MAGIC ^ 0x1U);
    storage[sizeof(FwPacketDescriptorType) + 4U] = SECURE_LINK_HANDSHAKE_VERSION;
    storage[sizeof(FwPacketDescriptorType) + 5U] = static_cast<U8>(SecureHandshakeMessageType::REQ_AUTH);
    storage[sizeof(FwPacketDescriptorType) + 6U] = OBC::SECURE_SERVICE_SBAND;
    storage[sizeof(FwPacketDescriptorType) + 7U] = 0U;
    return Fw::Buffer(storage.data(), sizeof(FwPacketDescriptorType) + SECURE_LINK_HANDSHAKE_HEADER_SIZE);
}

SecureHandshakeParseResult SecureLinkAuthorizerTester::parseHandshakeDownlink(const HandshakePacketCapture& capture) const {
    Fw::ComBuffer packet = capture.packet;
    FwPacketDescriptorType descriptor = 0;
    EXPECT_EQ(packet.deserializeTo(descriptor), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(descriptor, static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND));
    return parseSecureHandshakePacket(packet.getBuffAddr() + sizeof(FwPacketDescriptorType),
                                      packet.getSize() - sizeof(FwPacketDescriptorType));
}

void SecureLinkAuthorizerTester::clearCaptures() {
    this->clearHistory();
    this->m_packets.clear();
    this->m_returns.clear();
    this->m_grants.clear();
    this->m_revokes.clear();
    this->m_storage.clear();
}

}  // namespace OBC

#include "FileIngressAuthorityTester.hpp"

#include "Fw/Com/ComPacket.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/FppConstantsAc.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/SecureAuthRevocationReasonEnumAc.hpp"

namespace OBC {

FileIngressAuthorityTester::FileIngressAuthorityTester()
    : FileIngressAuthorityGTestBase("FileIngressAuthorityTester", MAX_HISTORY_SIZE),
      component("FileIngressAuthority") {
    EXPECT_TRUE(this->m_runtimeRoot.valid());
    this->initComponents();
    this->connectPorts();
    EXPECT_TRUE(this->component.configureRuntime(this->m_runtimeRoot.path(), this->m_runtimeRoot.path()));
}

FileIngressAuthorityTester::~FileIngressAuthorityTester() = default;

void FileIngressAuthorityTester::testStartRequiresAuthAndAllowedPolicy() {
    this->clearCaptures();

    Fw::FilePacket start = this->makeStartPacket(".sequence-staging/allowed.seq");
    Fw::Buffer buffer = this->makeWireBuffer(start);
    this->invoke_to_bufferSendIn(0, buffer);

    ASSERT_EQ(this->m_forwarded.size(), 0U);
    ASSERT_EQ(this->m_returned.size(), 1U);
    ASSERT_TLM_FILE_INGRESS_REJECTED_STARTS(0, 1U);

    this->grantAuth(0, OBC::SECURE_SERVICE_SBAND);
    this->setPolicy(0, AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY, true);
    this->clearCaptures();

    buffer = this->makeWireBuffer(start);
    this->invoke_to_bufferSendIn(0, buffer);

    ASSERT_EQ(this->m_forwarded.size(), 1U);
    EXPECT_EQ(this->decodeStartDestination(this->m_forwarded[0]), this->component.getPolicy().getRuntimePrefix() + "allowed.seq");
    EXPECT_EQ(this->m_returned.size(), 0U);
    ASSERT_EQ(this->m_authActivity.size(), 1U);
    EXPECT_EQ(this->m_authActivity[0].activity.get_ingressPort(), 0U);
    EXPECT_EQ(this->m_authActivity[0].activity.get_serviceId(), OBC::SECURE_SERVICE_SBAND);
    ASSERT_TLM_FILE_INGRESS_ACCEPTED_STARTS(0, 1U);
    ASSERT_TLM_FILE_INGRESS_DROP_MODE(1, 0U);
    ASSERT_TLM_FILE_INGRESS_AUTH_ACTIVE(0, 1U);
    ASSERT_TLM_FILE_INGRESS_ALLOWED_PORTS(0, 1U);
    ASSERT_EVENTS_FILE_INGRESS_START_ACCEPTED_SIZE(1);
}

void FileIngressAuthorityTester::testUhfBackupDeniedWithActiveAuth() {
    this->clearCaptures();

    this->grantAuth(1, OBC::SECURE_SERVICE_UHF);
    this->setPolicy(1, AuthorityLinkIdentity::UHF, AuthorityLinkRole::BACKUP, false);

    Fw::FilePacket deniedStart = this->makeStartPacket(".sequence-staging/uhf-denied.seq");
    Fw::Buffer deniedBuffer = this->makeWireBuffer(deniedStart);
    this->invoke_to_bufferSendIn(1, deniedBuffer);

    ASSERT_EQ(this->m_forwarded.size(), 0U);
    ASSERT_EQ(this->m_returned.size(), 1U);
    ASSERT_TLM_FILE_INGRESS_REJECTED_STARTS(0, 1U);
    ASSERT_TLM_FILE_INGRESS_DROP_MODE(0, 1U);
}

void FileIngressAuthorityTester::testRevocationDropsPacketsUntilNewStart() {
    this->clearCaptures();

    this->grantAuth(0, OBC::SECURE_SERVICE_SBAND);
    this->setPolicy(0, AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY, true);

    Fw::FilePacket start = this->makeStartPacket(".sequence-staging/revoke.seq");
    Fw::Buffer startBuffer = this->makeWireBuffer(start);
    this->invoke_to_bufferSendIn(0, startBuffer);
    ASSERT_EQ(this->m_forwarded.size(), 1U);

    this->revokeAuth(0, OBC::SECURE_SERVICE_SBAND, static_cast<U32>(SecureAuthRevocationReason::INVALIDATED));
    this->clearCaptures();

    Fw::FilePacket dataPacket = this->makeDataPacket(1U);
    Fw::Buffer dataBuffer = this->makeWireBuffer(dataPacket);
    this->invoke_to_bufferSendIn(0, dataBuffer);

    ASSERT_EQ(this->m_forwarded.size(), 0U);
    ASSERT_EQ(this->m_returned.size(), 1U);
    ASSERT_TLM_FILE_INGRESS_DROPPED_PACKETS(0, 1U);
    ASSERT_TLM_FILE_INGRESS_DROP_MODE(0, 1U);

    this->grantAuth(0, OBC::SECURE_SERVICE_SBAND);
    this->setPolicy(0, AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY, true);
    this->clearCaptures();

    startBuffer = this->makeWireBuffer(start);
    this->invoke_to_bufferSendIn(0, startBuffer);
    ASSERT_EQ(this->m_forwarded.size(), 1U);
    ASSERT_EQ(this->m_returned.size(), 0U);
    ASSERT_TLM_FILE_INGRESS_DROP_MODE(1, 0U);
}

void FileIngressAuthorityTester::testNonStagingDestinationRejected() {
    this->clearCaptures();

    this->grantAuth(0, OBC::SECURE_SERVICE_SBAND);
    this->setPolicy(0, AuthorityLinkIdentity::SBAND, AuthorityLinkRole::PRIMARY, true);

    Fw::FilePacket deniedStart = this->makeStartPacket("../escape.seq");
    Fw::Buffer deniedBuffer = this->makeWireBuffer(deniedStart);
    this->invoke_to_bufferSendIn(0, deniedBuffer);

    ASSERT_EQ(this->m_forwarded.size(), 0U);
    ASSERT_EQ(this->m_returned.size(), 1U);
    ASSERT_TLM_FILE_INGRESS_REJECTED_STARTS(0, 1U);
    ASSERT_TLM_FILE_INGRESS_DROP_MODE(0, 1U);
}

void FileIngressAuthorityTester::testSecondIngressReturnRoutesToOriginalSource() {
    this->clearCaptures();

    this->grantAuth(1, OBC::SECURE_SERVICE_UHF);
    this->setPolicy(1, AuthorityLinkIdentity::UHF, AuthorityLinkRole::PRIMARY_AFTER_FAILOVER, true);

    Fw::FilePacket start = this->makeStartPacket(".sequence-staging/uhf.seq");
    Fw::Buffer buffer = this->makeWireBuffer(start);
    this->invoke_to_bufferSendIn(1, buffer);

    ASSERT_EQ(this->m_forwarded.size(), 1U);
    EXPECT_EQ(this->m_forwarded[0].portNum, 0U);
    EXPECT_EQ(this->decodeStartDestination(this->m_forwarded[0]), this->component.getPolicy().getRuntimePrefix() + "uhf.seq");
    ASSERT_EQ(this->m_returned.size(), 0U);

    this->invoke_to_bufferReturnIn(0, buffer);

    ASSERT_EQ(this->m_returned.size(), 1U);
    EXPECT_EQ(this->m_returned[0].portNum, 1U);
}

void FileIngressAuthorityTester::from_bufferSendOut_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    this->pushFromPortEntry_bufferSendOut(buffer);
    this->m_forwarded.push_back(
        {portNum, std::vector<U8>(buffer.getData(), buffer.getData() + static_cast<std::size_t>(buffer.getSize()))});
}

void FileIngressAuthorityTester::from_bufferReturnOut_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    this->pushFromPortEntry_bufferReturnOut(buffer);
    this->m_returned.push_back(
        {portNum, std::vector<U8>(buffer.getData(), buffer.getData() + static_cast<std::size_t>(buffer.getSize()))});
}

void FileIngressAuthorityTester::from_authActivityOut_handler(FwIndexType portNum, const SecureAuthActivity& activity) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_authActivityOut(activity);
    this->m_authActivity.push_back({activity});
}

Fw::Buffer FileIngressAuthorityTester::makeWireBuffer(const Fw::FilePacket& packet) {
    this->m_scratch.assign(packet.bufferSize() + sizeof(FwPacketDescriptorType), 0U);
    Fw::Buffer buffer(this->m_scratch.data(), static_cast<Fw::Buffer::SizeType>(this->m_scratch.size()));
    EXPECT_EQ(buffer.getSerializer().serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_FILE)),
              Fw::FW_SERIALIZE_OK);
    Fw::Buffer offsetBuffer(buffer.getData() + sizeof(FwPacketDescriptorType),
                            buffer.getSize() - static_cast<Fw::Buffer::SizeType>(sizeof(FwPacketDescriptorType)));
    EXPECT_EQ(packet.toBuffer(offsetBuffer), Fw::FW_SERIALIZE_OK);
    return buffer;
}

Fw::FilePacket FileIngressAuthorityTester::makeStartPacket(const char* destinationPath) const {
    Fw::FilePacket::StartPacket startPacket;
    startPacket.initialize(16U, "source.seq", destinationPath);
    Fw::FilePacket packet;
    packet.fromStartPacket(startPacket);
    return packet;
}

Fw::FilePacket FileIngressAuthorityTester::makeDataPacket(U32 sequenceIndex) const {
    Fw::FilePacket::DataPacket tempPacket;
    tempPacket.initialize(sequenceIndex, 0U, static_cast<U32>(this->m_dataBytes.size()), this->m_dataBytes.data());
    Fw::FilePacket packet;
    packet.fromDataPacket(tempPacket);
    return packet;
}

std::string FileIngressAuthorityTester::decodeStartDestination(const BufferCapture& capture) const {
    if (capture.bytes.size() < sizeof(FwPacketDescriptorType)) {
        return std::string();
    }
    Fw::Buffer packetBuffer(const_cast<U8*>(capture.bytes.data()) + sizeof(FwPacketDescriptorType),
                            static_cast<Fw::Buffer::SizeType>(capture.bytes.size() - sizeof(FwPacketDescriptorType)));
    Fw::FilePacket packet;
    if (packet.fromBuffer(packetBuffer) != Fw::FW_SERIALIZE_OK ||
        packet.asHeader().getType() != Fw::FilePacket::T_START) {
        return std::string();
    }
    const auto& destination = packet.asStartPacket().getDestinationPath();
    return std::string(destination.getValue(), destination.getValue() + destination.getLength());
}

void FileIngressAuthorityTester::grantAuth(FwIndexType ingressPort, U32 serviceId) {
    SecureAuthGrant grant = {};
    grant.set_ingressPort(static_cast<U32>(ingressPort));
    grant.set_serviceId(serviceId);
    this->invoke_to_authGrantedIn(0, grant);
}

void FileIngressAuthorityTester::revokeAuth(FwIndexType ingressPort, U32 serviceId, U32 reason) {
    SecureAuthRevocation revocation = {};
    revocation.set_ingressPort(static_cast<U32>(ingressPort));
    revocation.set_serviceId(serviceId);
    revocation.set_reason(reason);
    this->invoke_to_authRevokedIn(0, revocation);
}

void FileIngressAuthorityTester::setPolicy(FwIndexType ingressPort,
                                           AuthorityLinkIdentity identity,
                                           AuthorityLinkRole role,
                                           bool fileAllowed) {
    FileIngressPolicyState policy = {};
    policy.set_ingressPort(static_cast<U32>(ingressPort));
    policy.set_linkIdentity(static_cast<U32>(identity));
    policy.set_linkRole(static_cast<U32>(role));
    policy.set_fileAllowed(fileAllowed);
    this->invoke_to_filePolicyIn(0, policy);
}

void FileIngressAuthorityTester::clearCaptures() {
    this->m_forwarded.clear();
    this->m_returned.clear();
    this->m_authActivity.clear();
}

}  // namespace OBC

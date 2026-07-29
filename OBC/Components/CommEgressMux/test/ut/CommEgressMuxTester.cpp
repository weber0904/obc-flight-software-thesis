#include "CommEgressMuxTester.hpp"

#include <cstring>

namespace OBC {

CommEgressMuxTester::CommEgressMuxTester()
    : CommEgressMuxGTestBase("CommEgressMuxTester", MAX_HISTORY_SIZE), component("CommEgressMux") {
    this->initComponents();
    this->connectPorts();
}

CommEgressMuxTester::~CommEgressMuxTester() = default;

void CommEgressMuxTester::testDefaultsSuppressSbandUntilEnabled() {
    this->clearObservations();
    Fw::ComBuffer packet = makePacket_(0x11U);
    Fw::Buffer buffer = makeBuffer_(0x22U);

    this->invoke_to_packetIn(1, packet, 7U);
    this->invoke_to_fileBufferIn(0, buffer);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_EQ(this->m_sbandBuffers.size(), 1U);
    ASSERT_EQ(this->m_uhfBuffers.size(), 0U);
    EXPECT_FALSE(this->component.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommEgressMuxTester::testSbandLiveObservabilityRoutesAfterEnable() {
    this->component.setBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND, true);

    this->clearObservations();
    Fw::ComBuffer packet = makePacket_(0x11U);
    Fw::Buffer buffer = makeBuffer_(0x22U);

    this->invoke_to_packetIn(1, packet, 7U);
    this->invoke_to_fileBufferIn(0, buffer);

    ASSERT_EQ(this->m_sbandPackets.size(), 1U);
    EXPECT_EQ(this->m_sbandPackets[0].portNum, 1);
    EXPECT_EQ(this->m_sbandPackets[0].context, 7U);
    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_EQ(this->m_sbandBuffers.size(), 1U);
    ASSERT_EQ(this->m_uhfBuffers.size(), 0U);
    EXPECT_TRUE(this->component.getBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND));
}

void CommEgressMuxTester::testSwitchRoutesTelemetryAndFileToUhf() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);
    this->component.setPrimaryFileLinkForRuntime(OBC::CommBand::UHF);

    this->clearObservations();
    Fw::ComBuffer packet = makePacket_(0x33U);
    Fw::Buffer buffer = makeBuffer_(0x44U);

    this->invoke_to_packetIn(0, packet, 9U);
    this->invoke_to_fileBufferIn(0, buffer);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_EQ(this->m_uhfPackets.size(), 1U);
    EXPECT_EQ(this->m_uhfPackets[0].portNum, 0);
    EXPECT_EQ(this->m_uhfPackets[0].context, 9U);
    ASSERT_EQ(this->m_sbandBuffers.size(), 0U);
    ASSERT_EQ(this->m_uhfBuffers.size(), 1U);
}

void CommEgressMuxTester::testUhfFileTransferSuspendsAllPacketEgress() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);
    this->component.setPrimaryFileLinkForRuntime(OBC::CommBand::UHF);
    this->component.setUhfFileTransferActiveForRuntime(true);

    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x77U);
    Fw::ComBuffer telemetryPacket = makePacket_(0x79U);
    Fw::Buffer buffer = makeBuffer_(0x88U);

    this->invoke_to_packetIn(0, eventPacket, 12U);
    this->invoke_to_packetIn(1, telemetryPacket, 13U);
    this->invoke_to_fileBufferIn(0, buffer);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_EQ(this->m_sbandBuffers.size(), 0U);
    ASSERT_EQ(this->m_uhfBuffers.size(), 1U);
}

void CommEgressMuxTester::testQuietModeSuppressesPacketEgress() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);
    this->component.setDiagnosticQuietPacketEgressForRuntime(true);

    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x77U);
    Fw::ComBuffer telemetryPacket = makePacket_(0x79U);

    this->invoke_to_packetIn(0, eventPacket, 12U);
    this->invoke_to_packetIn(1, telemetryPacket, 13U);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_EQ(this->m_returnBuffers.size(), 0U);
}

void CommEgressMuxTester::testQuietModeSuppressesSbandPacketEgress() {
    this->component.setDiagnosticQuietPacketEgressForRuntime(true);

    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x17U);
    Fw::ComBuffer telemetryPacket = makePacket_(0x19U);

    this->invoke_to_packetIn(0, eventPacket, 14U);
    this->invoke_to_packetIn(1, telemetryPacket, 15U);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_EQ(this->m_returnBuffers.size(), 0U);
}

void CommEgressMuxTester::testQuietModeReturnsFileBuffersLocally() {
    this->component.setPrimaryFileLinkForRuntime(OBC::CommBand::UHF);
    this->component.setDiagnosticQuietPacketEgressForRuntime(true);

    this->clearObservations();
    Fw::Buffer buffer = makeBuffer_(0x88U);

    this->invoke_to_fileBufferIn(0, buffer);

    ASSERT_EQ(this->m_sbandBuffers.size(), 0U);
    ASSERT_EQ(this->m_uhfBuffers.size(), 0U);
    ASSERT_EQ(this->m_returnBuffers.size(), 1U);
    EXPECT_EQ(this->m_returnBuffers[0].portNum, 0);
    ASSERT_NE(this->m_returnBuffers[0].buffer.getData(), nullptr);
    EXPECT_EQ(this->m_returnBuffers[0].buffer.getData()[0], 0x88U);
}

void CommEgressMuxTester::testUhfPrimaryPacketQuietSuppressesPacketEgressOnly() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);
    this->component.setPrimaryFileLinkForRuntime(OBC::CommBand::UHF);
    this->component.setUhfPrimaryPacketQuietForRuntime(true);

    this->clearObservations();
    Fw::ComBuffer packet = makePacket_(0x51U);
    Fw::Buffer buffer = makeBuffer_(0x61U);

    this->invoke_to_packetIn(0, packet, 21U);
    this->invoke_to_fileBufferIn(0, buffer);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_EQ(this->m_sbandBuffers.size(), 0U);
    ASSERT_EQ(this->m_uhfBuffers.size(), 1U);
    EXPECT_EQ(this->m_uhfBuffers[0].portNum, 0);
    EXPECT_EQ(this->m_uhfBuffers[0].buffer.getData()[0], 0x61U);
    ASSERT_EQ(this->m_returnBuffers.size(), 0U);
    EXPECT_FALSE(this->component.getDiagnosticQuietPacketEgressForRuntime());
    EXPECT_TRUE(this->component.getUhfPrimaryPacketQuietForRuntime());
}

void CommEgressMuxTester::testUhfRouteCountersTrackEventAndTelemetryPackets() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);

    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x31U);
    Fw::ComBuffer telemetryPacket = makePacket_(0x41U);

    this->invoke_to_packetIn(0, eventPacket, 3U);
    this->invoke_to_packetIn(1, telemetryPacket, 4U);

    ASSERT_EQ(this->m_uhfPackets.size(), 2U);
    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_ROUTED_TLM_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_SUPPRESSED_EVENT_PACKETS_SIZE(1);
    ASSERT_TLM_UHF_SUPPRESSED_TLM_PACKETS_SIZE(1);
    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS(1, 1U);
    ASSERT_TLM_UHF_ROUTED_TLM_PACKETS(1, 1U);
    ASSERT_TLM_UHF_SUPPRESSED_EVENT_PACKETS(0, 0U);
    ASSERT_TLM_UHF_SUPPRESSED_TLM_PACKETS(0, 0U);
}

void CommEgressMuxTester::testUhfSuppressCountersTrackQuietPackets() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);
    this->component.setUhfPrimaryPacketQuietForRuntime(true);

    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x51U);
    Fw::ComBuffer telemetryPacket = makePacket_(0x61U);

    this->invoke_to_packetIn(0, eventPacket, 5U);
    this->invoke_to_packetIn(1, telemetryPacket, 6U);

    ASSERT_EQ(this->m_uhfPackets.size(), 0U);
    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS_SIZE(1);
    ASSERT_TLM_UHF_ROUTED_TLM_PACKETS_SIZE(1);
    ASSERT_TLM_UHF_SUPPRESSED_EVENT_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_SUPPRESSED_TLM_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS(0, 0U);
    ASSERT_TLM_UHF_ROUTED_TLM_PACKETS(0, 0U);
    ASSERT_TLM_UHF_SUPPRESSED_EVENT_PACKETS(1, 1U);
    ASSERT_TLM_UHF_SUPPRESSED_TLM_PACKETS(1, 1U);
}

void CommEgressMuxTester::testSbandCountersTrackSuppressedAndRoutedPackets() {
    this->clearHistory();
    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x15U);
    Fw::ComBuffer telemetryPacket = makePacket_(0x16U);

    this->invoke_to_packetIn(0, eventPacket, 1U);
    this->invoke_to_packetIn(1, telemetryPacket, 2U);

    ASSERT_EQ(this->m_sbandPackets.size(), 0U);
    ASSERT_TLM_SBAND_ROUTED_EVENT_PACKETS_SIZE(0);
    ASSERT_TLM_SBAND_ROUTED_TLM_PACKETS_SIZE(0);
    ASSERT_TLM_SBAND_SUPPRESSED_EVENT_PACKETS_SIZE(1);
    ASSERT_TLM_SBAND_SUPPRESSED_TLM_PACKETS_SIZE(1);
    ASSERT_TLM_SBAND_SUPPRESSED_EVENT_PACKETS(0, 1U);
    ASSERT_TLM_SBAND_SUPPRESSED_TLM_PACKETS(0, 1U);

    this->component.setBandLiveObservabilityEnabledForRuntime(OBC::CommBand::SBAND, true);
    this->invoke_to_packetIn(0, eventPacket, 3U);
    this->invoke_to_packetIn(1, telemetryPacket, 4U);

    ASSERT_EQ(this->m_sbandPackets.size(), 2U);
    ASSERT_TLM_SBAND_ROUTED_EVENT_PACKETS_SIZE(1);
    ASSERT_TLM_SBAND_ROUTED_TLM_PACKETS_SIZE(1);
    ASSERT_TLM_SBAND_ROUTED_EVENT_PACKETS(0, 1U);
    ASSERT_TLM_SBAND_ROUTED_TLM_PACKETS(0, 1U);
}

void CommEgressMuxTester::testTelemetryLinkSwitchRepublishesUhfCounters() {
    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::UHF);

    this->clearObservations();
    Fw::ComBuffer eventPacket = makePacket_(0x71U);
    this->invoke_to_packetIn(0, eventPacket, 8U);

    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS(1, 1U);

    this->component.setPrimaryTelemetryLinkForRuntime(OBC::CommBand::SBAND);

    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS_SIZE(3);
    ASSERT_TLM_UHF_ROUTED_TLM_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_SUPPRESSED_EVENT_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_SUPPRESSED_TLM_PACKETS_SIZE(2);
    ASSERT_TLM_UHF_ROUTED_EVENT_PACKETS(2, 1U);
    ASSERT_TLM_UHF_ROUTED_TLM_PACKETS(1, 0U);
    ASSERT_TLM_UHF_SUPPRESSED_EVENT_PACKETS(1, 0U);
    ASSERT_TLM_UHF_SUPPRESSED_TLM_PACKETS(1, 0U);
}

void CommEgressMuxTester::testReturnPathMergesBothBranches() {
    this->clearObservations();
    Fw::Buffer sbandReturn = makeBuffer_(0x55U);
    Fw::Buffer uhfReturn = makeBuffer_(0x66U);

    this->invoke_to_sbandFileBufferReturnIn(0, sbandReturn);
    this->invoke_to_uhfFileBufferReturnIn(0, uhfReturn);

    ASSERT_EQ(this->m_returnBuffers.size(), 2U);
    EXPECT_EQ(this->m_returnBuffers[0].buffer.getData()[0], 0x55U);
    EXPECT_EQ(this->m_returnBuffers[1].buffer.getData()[0], 0x66U);
}

void CommEgressMuxTester::from_sbandPacketOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    this->m_sbandPackets.push_back({portNum, data, context});
}

void CommEgressMuxTester::from_uhfPacketOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    this->m_uhfPackets.push_back({portNum, data, context});
}

void CommEgressMuxTester::from_sbandFileBufferOut_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    this->m_sbandBuffers.push_back({portNum, fwBuffer});
}

void CommEgressMuxTester::from_uhfFileBufferOut_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    this->m_uhfBuffers.push_back({portNum, fwBuffer});
}

void CommEgressMuxTester::from_fileBufferReturnOut_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    this->m_returnBuffers.push_back({portNum, fwBuffer});
}

void CommEgressMuxTester::clearObservations() {
    this->m_sbandPackets.clear();
    this->m_uhfPackets.clear();
    this->m_sbandBuffers.clear();
    this->m_uhfBuffers.clear();
    this->m_returnBuffers.clear();
    this->m_ownedBuffers.clear();
}

Fw::ComBuffer CommEgressMuxTester::makePacket_(U8 value) const {
    Fw::ComBuffer buffer;
    EXPECT_EQ(buffer.serializeFrom(value), Fw::FW_SERIALIZE_OK);
    return buffer;
}

Fw::Buffer CommEgressMuxTester::makeBuffer_(U8 value, U32 size) {
    std::unique_ptr<U8[]> storage(new U8[size]);
    std::memset(storage.get(), value, size);
    Fw::Buffer buffer(storage.get(), size);
    this->m_ownedBuffers.push_back(std::move(storage));
    return buffer;
}

}  // namespace OBC

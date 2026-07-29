#include "BeaconPublisherTester.hpp"

namespace OBC {

bool BeaconPublisherTester::FakeReducedSource::getReducedStateForRuntime(OBC::StateData::ReducedStateV1& output) const {
    if (!this->available) {
        return false;
    }
    output = this->state;
    return true;
}

bool BeaconPublisherTester::FakeBeaconSink::sendBeacon(const U8* data, U32 size) {
    if (!this->allowSend) {
        return false;
    }
    this->packets.emplace_back(data, data + size);
    return true;
}

BeaconPublisherTester::BeaconPublisherTester()
    : BeaconPublisherGTestBase("BeaconPublisherTester", MAX_HISTORY_SIZE),
      m_source(),
      m_sink(),
      component("BeaconPublisher") {
    this->initComponents();
    this->connectPorts();
    this->configureNominal_();
    this->component.configureRuntime(&this->m_source, &this->m_sink);
}

BeaconPublisherTester::~BeaconPublisherTester() = default;

void BeaconPublisherTester::testPublishEmitsPacket() {
    this->clearHistory();
    ASSERT_TRUE(this->component.publishNow());

    ASSERT_EQ(this->m_sink.packets.size(), 1U);
    OBC::StateData::DecodedBeaconV1 decoded = {};
    ASSERT_EQ(OBC::StateData::decodeBeaconV1(this->m_sink.packets[0].data(),
                                             static_cast<U32>(this->m_sink.packets[0].size()),
                                             decoded),
              OBC::StateData::DecodeStatus::OK);
    ASSERT_EQ(decoded.sequence, 0U);
    ASSERT_FLOAT_EQ(decoded.batteryCurrent, 1.2F);
    ASSERT_FLOAT_EQ(decoded.batteryTempC, 23.5F);
    ASSERT_EVENTS_BEACON_PACKET_EMITTED_SIZE(1);
    ASSERT_TLM_BEACON_EMITTED_COUNT_SIZE(1);
    ASSERT_TLM_BEACON_EMITTED_COUNT(0, 1U);
    ASSERT_TLM_BEACON_SEQUENCE_SIZE(1);
    ASSERT_TLM_BEACON_SEQUENCE(0, 1U);
}

void BeaconPublisherTester::testCadenceWaitsBetweenPackets() {
    this->component.configurePeriodForTest(2U);
    this->clearHistory();
    this->m_sink.packets.clear();
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->m_sink.packets.size(), 2U);
    ASSERT_EVENTS_BEACON_PACKET_EMITTED_SIZE(2);
}

void BeaconPublisherTester::testUnavailableSourceReportsError() {
    this->m_source.available = false;
    this->clearHistory();
    ASSERT_FALSE(this->component.publishNow());

    ASSERT_EVENTS_BEACON_SOURCE_UNAVAILABLE_SIZE(1);
    ASSERT_TLM_BEACON_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BEACON_LAST_ERROR(0, 2U);
    ASSERT_EQ(this->m_sink.packets.size(), 0U);
}

void BeaconPublisherTester::testNotConfiguredReportsDistinctError() {
    this->component.configureRuntime(nullptr, &this->m_sink);
    this->clearHistory();
    ASSERT_FALSE(this->component.publishNow());

    ASSERT_EVENTS_BEACON_NOT_CONFIGURED_SIZE(1);
    ASSERT_EVENTS_BEACON_SOURCE_UNAVAILABLE_SIZE(0);
    ASSERT_TLM_BEACON_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BEACON_LAST_ERROR(0, 1U);
    ASSERT_EQ(this->m_sink.packets.size(), 0U);
}

void BeaconPublisherTester::testDisabledSchedulerIsSilent() {
    this->component.configureRuntime(&this->m_source, &this->m_sink, false);
    this->component.configurePeriodForTest(1U);
    this->clearHistory();
    ASSERT_FALSE(this->component.publishNow());
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EVENTS_BEACON_NOT_CONFIGURED_SIZE(0);
    ASSERT_EVENTS_BEACON_SOURCE_UNAVAILABLE_SIZE(0);
    ASSERT_EVENTS_BEACON_SINK_ERROR_SIZE(0);
    ASSERT_EVENTS_BEACON_PACKET_EMITTED_SIZE(0);
    ASSERT_TLM_BEACON_LAST_ERROR_SIZE(0);
    ASSERT_EQ(this->m_sink.packets.size(), 0U);
}

void BeaconPublisherTester::testSinkFailureReportsError() {
    this->m_sink.allowSend = false;
    this->clearHistory();
    ASSERT_FALSE(this->component.publishNow());

    ASSERT_EVENTS_BEACON_SINK_ERROR_SIZE(1);
    ASSERT_TLM_BEACON_LAST_ERROR_SIZE(1);
    ASSERT_TLM_BEACON_LAST_ERROR(0, 3U);
    ASSERT_TLM_BEACON_SEQUENCE_SIZE(1);
    ASSERT_TLM_BEACON_SEQUENCE(0, 0U);

    this->m_sink.allowSend = true;
    this->clearHistory();
    ASSERT_TRUE(this->component.publishNow());
    OBC::StateData::DecodedBeaconV1 decoded = {};
    ASSERT_EQ(OBC::StateData::decodeBeaconV1(this->m_sink.packets[0].data(),
                                             static_cast<U32>(this->m_sink.packets[0].size()),
                                             decoded),
              OBC::StateData::DecodeStatus::OK);
    ASSERT_EQ(decoded.sequence, 0U);
}

void BeaconPublisherTester::testRuntimeSuppressIsSilent() {
    this->component.configurePeriodForTest(1U);
    this->component.setUhfBeaconSuppressedForRuntime(true);
    this->clearHistory();
    this->m_sink.packets.clear();

    ASSERT_FALSE(this->component.publishNow());
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_TRUE(this->m_sink.packets.empty());
    ASSERT_EVENTS_BEACON_NOT_CONFIGURED_SIZE(0);
    ASSERT_EVENTS_BEACON_SOURCE_UNAVAILABLE_SIZE(0);
    ASSERT_EVENTS_BEACON_SINK_ERROR_SIZE(0);
    ASSERT_EVENTS_BEACON_PACKET_EMITTED_SIZE(0);
}

void BeaconPublisherTester::testRuntimeSuppressClearResumesOnNextTick() {
    this->component.configurePeriodForTest(5U);
    this->component.setUhfBeaconSuppressedForRuntime(true);
    this->clearHistory();
    this->m_sink.packets.clear();
    this->invoke_to_schedIn(0, 0U);
    ASSERT_TRUE(this->m_sink.packets.empty());

    this->component.setUhfBeaconSuppressedForRuntime(false);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->m_sink.packets.size(), 1U);
    ASSERT_EVENTS_BEACON_PACKET_EMITTED_SIZE(1);
}

void BeaconPublisherTester::configureNominal_() {
    this->m_source.available = true;
    this->m_source.state = {};
    this->m_source.state.timestamp = Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 200U, 0U);
    this->m_source.state.mode = OBC::SatMode::IDLE;
    this->m_source.state.batterySoc = 76.0F;
    this->m_source.state.batteryVoltage = 8.1F;
    this->m_source.state.batteryCurrent = 1.2F;
    this->m_source.state.batteryTempC = 23.5F;
    this->m_source.state.activeBootSlot = OBC::BootSlot::SLOT_A;
    this->m_sink.allowSend = true;
    this->m_sink.packets.clear();
}

}  // namespace OBC

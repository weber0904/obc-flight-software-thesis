#include "GroundLinkHealthProviderTester.hpp"

namespace OBC {

FakeGroundLinkHealthBackend::FakeGroundLinkHealthBackend(OBC::COMM::GroundLinkBackendMode mode)
    : m_stats(),
      m_healthSemantics(mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP
                            ? OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP
                        : (mode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP
                               ? OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK
                               : OBC::COMM::GroundLinkHealthSemantics::DISABLED)),
      m_successfulStatusObservations(0U),
      m_healthReplies() {
    this->m_stats.mode = mode;
}

bool FakeGroundLinkHealthBackend::start() {
    return true;
}

void FakeGroundLinkHealthBackend::stop() {
    this->m_stats.connected = false;
}

OBC::COMM::GroundLinkReceiveStatus FakeGroundLinkHealthBackend::receive(std::string& outChunk, std::uint32_t timeoutMs) {
    static_cast<void>(timeoutMs);
    outChunk.clear();
    return OBC::COMM::GroundLinkReceiveStatus::IDLE;
}

OBC::COMM::GroundLinkSendStatus FakeGroundLinkHealthBackend::send(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U) {
        this->m_stats.txErrors += 1U;
        return OBC::COMM::GroundLinkSendStatus::ERROR;
    }

    this->m_stats.txChunks += 1U;
    this->m_stats.txBytes += static_cast<U32>(size);
    return OBC::COMM::GroundLinkSendStatus::OK;
}

OBC::COMM::GroundLinkStats FakeGroundLinkHealthBackend::getStats() const {
    return this->m_stats;
}

OBC::COMM::GroundLinkObservationState FakeGroundLinkHealthBackend::getObservationState() const {
    OBC::COMM::GroundLinkObservationState observation = {};
    observation.mode = this->m_stats.mode;
    observation.healthSemantics = this->m_healthSemantics;
    observation.connected = this->m_stats.connected;
    observation.txChunks = this->m_stats.txChunks;
    observation.rxChunks = this->m_stats.rxChunks;
    observation.txBytes = this->m_stats.txBytes;
    observation.rxBytes = this->m_stats.rxBytes;
    observation.txErrors = this->m_stats.txErrors;
    observation.rxErrors = this->m_stats.rxErrors;
    observation.successfulStatusObservations = this->m_successfulStatusObservations;
    return observation;
}

bool FakeGroundLinkHealthBackend::observeHealth() {
    if (!this->m_healthReplies.empty()) {
        const HealthReply reply = this->m_healthReplies.front();
        this->m_healthReplies.pop_front();
        this->m_stats.connected = reply.connected;
        if (reply.success) {
            this->m_successfulStatusObservations += 1U;
        }
        return reply.success;
    }

    if (this->m_stats.mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP && this->m_stats.connected) {
        this->m_successfulStatusObservations += 1U;
        return true;
    }

    return this->m_stats.connected;
}

void FakeGroundLinkHealthBackend::setConnected(bool connected) {
    this->m_stats.connected = connected;
}

void FakeGroundLinkHealthBackend::setMode(OBC::COMM::GroundLinkBackendMode mode) {
    this->m_stats.mode = mode;
    this->m_healthSemantics = mode == OBC::COMM::GroundLinkBackendMode::COMM_CSP
                                  ? OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP
                              : (mode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP
                                     ? OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK
                                     : OBC::COMM::GroundLinkHealthSemantics::DISABLED);
}

void FakeGroundLinkHealthBackend::setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics healthSemantics) {
    this->m_healthSemantics = healthSemantics;
}

void FakeGroundLinkHealthBackend::addRxChunk() {
    this->m_stats.rxChunks += 1U;
}

void FakeGroundLinkHealthBackend::addTxChunk() {
    this->m_stats.txChunks += 1U;
}

void FakeGroundLinkHealthBackend::addErrors(U32 txErrors, U32 rxErrors) {
    this->m_stats.txErrors += txErrors;
    this->m_stats.rxErrors += rxErrors;
}

void FakeGroundLinkHealthBackend::queueHealthObservation(bool success, bool connected) {
    this->m_healthReplies.push_back({success, connected});
}

GroundLinkHealthProviderTester::GroundLinkHealthProviderTester()
    : GroundLinkHealthProviderGTestBase("GroundLinkHealthProviderTester", MAX_HISTORY_SIZE),
      component("GroundLinkHealthProvider"),
      m_sbandGroundLinkDriver("SbandGroundLinkDriver"),
      m_uhfGroundLinkDriver("UhfGroundLinkDriver"),
      m_sbandBackend(new FakeGroundLinkHealthBackend()),
      m_uhfBackend(new FakeGroundLinkHealthBackend()) {
    this->initComponents();
    this->connectPorts();
    this->m_sbandGroundLinkDriver.init(TEST_INSTANCE_ID);
    this->m_uhfGroundLinkDriver.init(TEST_INSTANCE_ID);
    this->m_sbandGroundLinkDriver.setBackendForTest(this->m_sbandBackend.get());
    this->m_uhfGroundLinkDriver.setBackendForTest(this->m_uhfBackend.get());
    this->component.configureRuntime(&this->m_sbandGroundLinkDriver, &this->m_uhfGroundLinkDriver);
}

GroundLinkHealthProviderTester::~GroundLinkHealthProviderTester() {
    this->m_sbandGroundLinkDriver.clearConfiguration();
    this->m_uhfGroundLinkDriver.clearConfiguration();
}

void GroundLinkHealthProviderTester::testCommCspStatusObservationKeepsAvailabilityHealthy() {
    this->m_sbandBackend->setConnected(true);
    this->clearHistory();
    this->component.tickForTest();

    const OBC::CommLinkHealthView view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_TRUE(view.available);
    ASSERT_EQ(view.availabilityReason, OBC::CommLinkAvailabilityReason::HEALTHY_ACTIVITY);
    ASSERT_EQ(view.activityAgeTicks, 0U);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_AVAILABLE_SIZE(1);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_AVAILABLE(0, true);
}

void GroundLinkHealthProviderTester::testCommCspStaleTransitionAndRecovery() {
    this->m_sbandBackend->queueHealthObservation(true, true);
    this->component.tickForTest();

    this->m_sbandBackend->queueHealthObservation(false, true);
    this->component.tickForTest();
    this->m_sbandBackend->queueHealthObservation(false, true);
    this->clearHistory();
    this->component.tickForTest();

    OBC::CommLinkHealthView view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_FALSE(view.available);
    ASSERT_EQ(view.availabilityReason, OBC::CommLinkAvailabilityReason::STALE_ACTIVITY);
    ASSERT_EQ(view.activityAgeTicks, 2U);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_REASON_SIZE(1);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_REASON(0, static_cast<U32>(OBC::CommLinkAvailabilityReason::STALE_ACTIVITY));

    this->m_sbandBackend->queueHealthObservation(true, true);
    this->component.tickForTest();
    view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_TRUE(view.available);
    ASSERT_EQ(view.activityAgeTicks, 0U);
    ASSERT_EQ(view.availabilityReason, OBC::CommLinkAvailabilityReason::HEALTHY_ACTIVITY);
}

void GroundLinkHealthProviderTester::testCommCspStaleAgeTelemetryKeepsAdvancing() {
    this->m_sbandBackend->queueHealthObservation(true, true);
    this->component.tickForTest();

    this->m_sbandBackend->queueHealthObservation(false, true);
    this->component.tickForTest();
    this->m_sbandBackend->queueHealthObservation(false, true);
    this->component.tickForTest();

    this->clearHistory();
    this->m_sbandBackend->queueHealthObservation(false, true);
    this->component.tickForTest();

    const OBC::CommLinkHealthView view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_FALSE(view.available);
    ASSERT_EQ(view.availabilityReason, OBC::CommLinkAvailabilityReason::STALE_ACTIVITY);
    ASSERT_EQ(view.activityAgeTicks, 3U);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS_SIZE(1);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS(0, 3U);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_REASON_SIZE(0);
}

void GroundLinkHealthProviderTester::testMarkTelemetryDirtyRepublishesActivityAgeTelemetry() {
    this->m_sbandBackend->setConnected(true);
    this->component.tickForTest();

    this->clearHistory();
    this->component.tickForTest();
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_AVAILABLE_SIZE(0);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS_SIZE(0);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_REASON_SIZE(0);

    this->component.markTelemetryDirtyForRuntime(OBC::CommBand::SBAND);
    this->component.tickForTest();

    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS_SIZE(1);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS(0, 0U);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_AVAILABLE_SIZE(0);
    ASSERT_TLM_GROUND_LINK_HEALTH_S_BAND_REASON_SIZE(0);
}

void GroundLinkHealthProviderTester::testErrorGrowthTracksByCycle() {
    this->m_sbandBackend->setConnected(true);
    this->component.tickForTest();

    this->m_sbandBackend->addErrors(1U, 0U);
    this->component.tickForTest();
    OBC::CommLinkHealthView view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_TRUE(view.errorGrowthThisCycle);

    this->component.tickForTest();
    view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_FALSE(view.errorGrowthThisCycle);
}

void GroundLinkHealthProviderTester::testConnectedOnlyCompatibilitySuppressesTransportGrowthAndStale() {
    this->m_sbandBackend->setConnected(true);
    this->m_sbandBackend->setHealthSemantics(OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK);
    this->component.tickForTest();

    this->m_sbandBackend->addErrors(1U, 1U);
    this->component.tickForTest();
    this->component.tickForTest();
    this->component.tickForTest();

    const OBC::CommLinkHealthView view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_FALSE(view.errorGrowthThisCycle);
    ASSERT_TRUE(view.available);
    ASSERT_EQ(view.availabilityReason, OBC::CommLinkAvailabilityReason::CONNECTED_ONLY_FALLBACK);
}

void GroundLinkHealthProviderTester::testDirectTcpConnectedFallbackDoesNotGoStale() {
    this->m_sbandBackend->setMode(OBC::COMM::GroundLinkBackendMode::DIRECT_TCP);
    this->m_sbandBackend->setConnected(true);
    this->component.tickForTest();
    this->component.tickForTest();
    this->component.tickForTest();

    const OBC::CommLinkHealthView view = this->component.getHealthForRuntime(OBC::CommBand::SBAND);
    ASSERT_TRUE(view.available);
    ASSERT_EQ(view.availabilityReason, OBC::CommLinkAvailabilityReason::CONNECTED_ONLY_FALLBACK);
    ASSERT_GE(view.activityAgeTicks, 2U);
}

}  // namespace OBC

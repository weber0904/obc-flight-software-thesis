#include "AdcsFdirControllerTester.hpp"

namespace OBC {

bool AdcsFdirControllerTester::FakeHealthProvider::getPollHealthForRuntime(OBC::ADCS::PollHealthState& output) const {
    if (!this->available) {
        return false;
    }
    output = this->state;
    return true;
}

void AdcsFdirControllerTester::FakeRecoverySink::submitAdcsPollTransportFault(U32 failureCount) {
    this->adcsTransportFaultCount++;
    this->lastFailureCount = failureCount;
}

void AdcsFdirControllerTester::FakeRecoverySink::clearAdcsPollTransportFault(U32 failureCount) {
    this->adcsTransportClearCount++;
    this->lastFailureCount = failureCount;
}

void AdcsFdirControllerTester::FakeRecoverySink::submitAdcsPollFreshnessFault(U32 failureCount) {
    this->adcsFreshnessFaultCount++;
    this->lastFailureCount = failureCount;
}

void AdcsFdirControllerTester::FakeRecoverySink::clearAdcsPollFreshnessFault(U32 failureCount) {
    this->adcsFreshnessClearCount++;
    this->lastFailureCount = failureCount;
}

AdcsFdirControllerTester::AdcsFdirControllerTester()
    : AdcsFdirControllerGTestBase("AdcsFdirControllerTester", MAX_HISTORY_SIZE),
      m_healthProvider(),
      m_recoverySink(),
      component("AdcsFdirController"),
      m_watchdogBeatCount(0U),
      m_lastWatchdogCode(0U) {
    this->initComponents();
    this->connectPorts();
    this->component.configureRuntime(&this->m_healthProvider, &this->m_recoverySink);
}

AdcsFdirControllerTester::~AdcsFdirControllerTester() = default;

void AdcsFdirControllerTester::testTransportRetriesBeforeLatch() {
    this->setTransportHealth(1U);

    this->clearHistory();
    OBC::AdcsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldEnterRetry);
    ASSERT_FALSE(decision.shouldLatchFault);
    ASSERT_EQ(this->m_recoverySink.adcsTransportFaultCount, 0U);
    ASSERT_EVENTS_ADCS_FDIR_RETRYING_SIZE(1);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_ENTERED_SIZE(0);

    this->setTransportHealth(2U);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldEnterRetry);
    ASSERT_FALSE(decision.shouldLatchFault);
    ASSERT_EQ(this->m_recoverySink.adcsTransportFaultCount, 0U);
    ASSERT_EVENTS_ADCS_FDIR_RETRYING_SIZE(1);
    ASSERT_EVENTS_ADCS_FDIR_RETRYING(0, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT, 2U);
}

void AdcsFdirControllerTester::testFreshnessFaultLatchesAtThreshold() {
    this->setFreshnessHealth(3U);

    this->clearHistory();
    const OBC::AdcsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldLatchFault);
    ASSERT_EQ(decision.activeSource, OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS);
    ASSERT_EQ(this->m_recoverySink.adcsFreshnessFaultCount, 1U);
    ASSERT_EQ(this->m_recoverySink.lastFailureCount, 3U);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_ENTERED_SIZE(1);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_ENTERED(0, OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS, 3U);
    ASSERT_TLM_ADCS_FDIR_FAULT_LATCHED_SIZE(1);
    ASSERT_TLM_ADCS_FDIR_FAULT_LATCHED(0, true);
}

void AdcsFdirControllerTester::testHealthyScheduledCycleClearsFault() {
    this->setTransportHealth(3U);
    static_cast<void>(this->component.runCycle());
    ASSERT_EQ(this->m_recoverySink.adcsTransportFaultCount, 1U);

    this->setHealthyRefresh();
    this->clearHistory();
    const OBC::AdcsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldClearFault);
    ASSERT_EQ(this->m_recoverySink.adcsTransportClearCount, 1U);
    ASSERT_EQ(this->m_recoverySink.lastFailureCount, 3U);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_CLEARED_SIZE(1);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_CLEARED(0, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT, 3U);
    ASSERT_TLM_ADCS_FDIR_FAULT_LATCHED_SIZE(1);
    ASSERT_TLM_ADCS_FDIR_FAULT_LATCHED(0, false);
}

void AdcsFdirControllerTester::testLatchedFaultDoesNotSwitchSourceBeforeHealthyClear() {
    this->setFreshnessHealth(3U);
    static_cast<void>(this->component.runCycle());
    ASSERT_EQ(this->m_recoverySink.adcsFreshnessFaultCount, 1U);
    ASSERT_EQ(this->m_recoverySink.adcsTransportFaultCount, 0U);

    this->setTransportHealth(3U);
    this->clearHistory();
    const OBC::AdcsFdirDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldLatchFault);
    ASSERT_FALSE(decision.shouldClearFault);
    ASSERT_EQ(decision.activeSource, OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS);
    ASSERT_EQ(this->m_recoverySink.adcsFreshnessFaultCount, 1U);
    ASSERT_EQ(this->m_recoverySink.adcsTransportFaultCount, 0U);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_ENTERED_SIZE(0);
    ASSERT_EVENTS_ADCS_FDIR_FAULT_CLEARED_SIZE(0);
}

void AdcsFdirControllerTester::testSchedEmitsWatchdogBeat() {
    this->setHealthyRefresh();

    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->m_watchdogBeatCount, 1U);
    ASSERT_EQ(this->m_lastWatchdogCode, static_cast<U32>(OBC::WatchdogSource::ADCS_FDIR));
    ASSERT_from_watchdogBeatOut_SIZE(1);
    ASSERT_from_watchdogBeatOut(0, static_cast<U32>(OBC::WatchdogSource::ADCS_FDIR));
}

void AdcsFdirControllerTester::setTransportHealth(U32 consecutiveFailures) {
    this->m_healthProvider.available = true;
    this->m_healthProvider.state = {};
    this->m_healthProvider.state.consecutiveTransportFailures = consecutiveFailures;
    this->m_healthProvider.state.cumulativeTransportErrors = consecutiveFailures;
    this->m_healthProvider.state.lastScheduledTransportOk = false;
    this->m_healthProvider.state.lastScheduledValidRefresh = false;
}

void AdcsFdirControllerTester::setFreshnessHealth(U32 consecutiveFailures) {
    this->m_healthProvider.available = true;
    this->m_healthProvider.state = {};
    this->m_healthProvider.state.consecutiveNoValidRefresh = consecutiveFailures;
    this->m_healthProvider.state.cumulativeNoValidRefresh = consecutiveFailures;
    this->m_healthProvider.state.lastScheduledTransportOk = true;
    this->m_healthProvider.state.lastScheduledValidRefresh = false;
}

void AdcsFdirControllerTester::setHealthyRefresh() {
    this->m_healthProvider.available = true;
    this->m_healthProvider.state = {};
    this->m_healthProvider.state.hasValidState = true;
    this->m_healthProvider.state.lastScheduledTransportOk = true;
    this->m_healthProvider.state.lastScheduledValidRefresh = true;
}

void AdcsFdirControllerTester::from_watchdogBeatOut_handler(FwIndexType portNum, U32 code) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_watchdogBeatOut(code);
    this->m_watchdogBeatCount++;
    this->m_lastWatchdogCode = code;
}

}  // namespace OBC

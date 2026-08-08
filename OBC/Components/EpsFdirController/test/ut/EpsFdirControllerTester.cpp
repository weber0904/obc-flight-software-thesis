#include "EpsFdirControllerTester.hpp"

#include <limits>

namespace OBC {

OBC::SatMode EpsFdirControllerTester::FakeModeControl::getModeForRuntime() const {
    return this->mode;
}

void EpsFdirControllerTester::FakeModeControl::applyModeForInternalSource(OBC::SatMode requestedMode,
                                                                          OBC::ModeApplySource source) {
    if (this->mode != requestedMode) {
        this->mode = requestedMode;
        this->transitionCount++;
        this->lastRequestedMode = requestedMode;
        this->lastSource = source;
    }
}

bool EpsFdirControllerTester::FakeHealthProvider::getPollHealthForRuntime(OBC::EPS::PollHealthState& output) const {
    if (!this->available) {
        return false;
    }
    output = this->state;
    return true;
}

void EpsFdirControllerTester::FakeRecoverySink::submitEpsTimeoutFault(U32 failureCount) {
    this->lastFailureCount = failureCount;
    this->epsFaultCount++;
}

void EpsFdirControllerTester::FakeRecoverySink::clearEpsTimeoutFault(U32 failureCount) {
    this->lastFailureCount = failureCount;
    this->epsClearCount++;
}

EpsFdirControllerTester::EpsFdirControllerTester()
    : EpsFdirControllerGTestBase("EpsFdirControllerTester", MAX_HISTORY_SIZE),
      m_modeControl(),
      m_healthProvider(),
      m_recoverySink(),
      component("EpsFdirController") {
    this->initComponents();
    this->connectPorts();
    this->component.configureRuntime(&this->m_modeControl, &this->m_healthProvider, &this->m_recoverySink);
}

EpsFdirControllerTester::~EpsFdirControllerTester() = default;

void EpsFdirControllerTester::testNoEscalationBeforeThreshold() {
    this->resetMode(OBC::SatMode::IDLE);
    this->setHealth(false, 1U);

    this->clearHistory();
    OBC::EpsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldEnterRetry);
    ASSERT_FALSE(decision.shouldLatchFault);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_EPS_FDIR_RETRYING_SIZE(1);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(0);
    ASSERT_TLM_EPS_FDIR_FAULT_LATCHED_SIZE(1);
    ASSERT_TLM_EPS_FDIR_FAULT_LATCHED(0, false);

    this->setHealth(false, 2U);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldEnterRetry);
    ASSERT_FALSE(decision.shouldLatchFault);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_EPS_FDIR_RETRYING_SIZE(1);
    ASSERT_EVENTS_EPS_FDIR_RETRYING(0, 2U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(0);
}

void EpsFdirControllerTester::testFaultEscalatesOnceAtThreshold() {
    this->resetMode(OBC::SatMode::PAYLOAD);
    this->setHealth(false, 3U);

    this->clearHistory();
    OBC::EpsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldLatchFault);
    ASSERT_TRUE(decision.shouldRequestSafe);
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::PAYLOAD);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EQ(this->m_recoverySink.epsFaultCount, 1U);
    ASSERT_EQ(this->m_recoverySink.lastFailureCount, 3U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(1);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED(0, 3U, OBC::SatMode::PAYLOAD, true);
    ASSERT_TLM_EPS_FDIR_FAULT_LATCHED_SIZE(1);
    ASSERT_TLM_EPS_FDIR_FAULT_LATCHED(0, true);
    ASSERT_TLM_EPS_FDIR_ESCALATION_COUNT_SIZE(1);
    ASSERT_TLM_EPS_FDIR_ESCALATION_COUNT(0, 1U);
    ASSERT_TLM_EPS_FDIR_LAST_REQUESTED_SAFE_SIZE(1);
    ASSERT_TLM_EPS_FDIR_LAST_REQUESTED_SAFE(0, true);

    this->setHealth(false, 4U);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldLatchFault);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EQ(this->m_recoverySink.epsFaultCount, 1U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(0);
    ASSERT_TLM_EPS_FDIR_ESCALATION_COUNT_SIZE(1);
    ASSERT_TLM_EPS_FDIR_ESCALATION_COUNT(0, 1U);
}

void EpsFdirControllerTester::testRecoveryClearsFaultOnFirstSuccess() {
    this->resetMode(OBC::SatMode::IDLE);
    this->setHealth(false, 3U);
    static_cast<void>(this->component.runCycle());

    this->setHealth(true, 0U, true);
    this->clearHistory();
    const OBC::EpsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldClearFault);
    ASSERT_EQ(this->m_recoverySink.epsClearCount, 1U);
    ASSERT_EQ(this->m_recoverySink.lastFailureCount, 3U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_CLEARED_SIZE(1);
    ASSERT_EVENTS_EPS_FDIR_FAULT_CLEARED(0, 3U);
    ASSERT_TLM_EPS_FDIR_FAULT_LATCHED_SIZE(1);
    ASSERT_TLM_EPS_FDIR_FAULT_LATCHED(0, false);
    ASSERT_TLM_EPS_FDIR_RECOVERY_COUNT_SIZE(1);
    ASSERT_TLM_EPS_FDIR_RECOVERY_COUNT(0, 1U);
    ASSERT_TLM_EPS_FDIR_LAST_REQUESTED_SAFE_SIZE(1);
    ASSERT_TLM_EPS_FDIR_LAST_REQUESTED_SAFE(0, false);
}

void EpsFdirControllerTester::testSafeAndHellRecordFaultWithoutModeRequest() {
    this->resetMode(OBC::SatMode::SAFE);
    this->setHealth(false, 3U);

    this->clearHistory();
    OBC::EpsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldLatchFault);
    ASSERT_FALSE(decision.shouldRequestSafe);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EQ(this->m_recoverySink.epsFaultCount, 1U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(1);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED(0, 3U, OBC::SatMode::SAFE, false);

    this->setHealth(true, 0U, true);
    static_cast<void>(this->component.runCycle());

    this->resetMode(OBC::SatMode::HELL);
    this->setHealth(false, 3U);

    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldLatchFault);
    ASSERT_FALSE(decision.shouldRequestSafe);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EQ(this->m_recoverySink.epsFaultCount, 2U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(1);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED(0, 3U, OBC::SatMode::HELL, false);
}

void EpsFdirControllerTester::testCountersSaturateAtMax() {
    this->resetMode(OBC::SatMode::IDLE);
    this->component.setCountersForTest(std::numeric_limits<U32>::max(), std::numeric_limits<U32>::max());

    this->setHealth(false, 3U);
    this->clearHistory();
    OBC::EpsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldLatchFault);
    ASSERT_TLM_EPS_FDIR_ESCALATION_COUNT_SIZE(1);
    ASSERT_TLM_EPS_FDIR_ESCALATION_COUNT(0, std::numeric_limits<U32>::max());

    this->setHealth(true, 0U, true);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldClearFault);
    ASSERT_TLM_EPS_FDIR_RECOVERY_COUNT_SIZE(1);
    ASSERT_TLM_EPS_FDIR_RECOVERY_COUNT(0, std::numeric_limits<U32>::max());
}

void EpsFdirControllerTester::testRelatchReportsSecondFault() {
    this->resetMode(OBC::SatMode::IDLE);
    this->setHealth(false, 3U);
    static_cast<void>(this->component.runCycle());
    ASSERT_EQ(this->m_recoverySink.epsFaultCount, 1U);

    this->setHealth(true, 0U, true);
    static_cast<void>(this->component.runCycle());
    ASSERT_EQ(this->m_recoverySink.epsClearCount, 1U);

    this->setHealth(false, 3U);
    this->clearHistory();
    const OBC::EpsFdirDecision decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldLatchFault);
    ASSERT_EQ(this->m_recoverySink.epsFaultCount, 2U);
    ASSERT_EQ(this->m_recoverySink.lastFailureCount, 3U);
    ASSERT_EVENTS_EPS_FDIR_FAULT_ENTERED_SIZE(1);
}

void EpsFdirControllerTester::setHealth(const bool lastPollSucceeded,
                                        const U32 consecutiveFailures,
                                        const bool cacheValid) {
    this->m_healthProvider.available = true;
    this->m_healthProvider.state = {};
    this->m_healthProvider.state.cacheValid = cacheValid;
    this->m_healthProvider.state.lastPollSucceeded = lastPollSucceeded;
    this->m_healthProvider.state.consecutivePollFailures = consecutiveFailures;
    this->m_healthProvider.state.cumulativePollErrors = consecutiveFailures;
}

void EpsFdirControllerTester::resetMode(OBC::SatMode mode) {
    this->m_modeControl.mode = mode;
    this->m_modeControl.transitionCount = 0U;
    this->m_modeControl.lastRequestedMode = mode;
    this->m_modeControl.lastSource = OBC::ModeApplySource::TestSetup;
}

}  // namespace OBC

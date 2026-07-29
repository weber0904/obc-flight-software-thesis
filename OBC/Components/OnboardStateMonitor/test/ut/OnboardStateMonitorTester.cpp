#include "OnboardStateMonitorTester.hpp"

namespace OBC {

bool OnboardStateMonitorTester::FakeSource::readStateSnapshot(OBC::StateData::StateSnapshot& output) const {
    if (!this->available) {
        return false;
    }
    output = this->snapshot;
    this->calls++;
    return true;
}

OnboardStateMonitorTester::OnboardStateMonitorTester()
    : OnboardStateMonitorGTestBase("OnboardStateMonitorTester", MAX_HISTORY_SIZE),
      m_source(),
      component("OnboardStateMonitor") {
    this->initComponents();
    this->connectPorts();
    this->configureNominalSource_();
    this->component.configureRuntime(&this->m_source);
}

OnboardStateMonitorTester::~OnboardStateMonitorTester() = default;

void OnboardStateMonitorTester::testSchedPublishesReducedState() {
    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);

    OBC::StateData::ReducedStateV1 state = {};
    ASSERT_TRUE(this->component.getReducedStateForRuntime(state));
    ASSERT_EQ(this->m_source.calls, 1U);
    ASSERT_EQ(state.healthMask, 0U);
    ASSERT_EQ(state.qualityMask, 0U);
    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(0);
    ASSERT_TLM_STATE_MONITOR_HAVE_STATE_SIZE(1);
    ASSERT_TLM_STATE_MONITOR_HAVE_STATE(0, 1U);
    ASSERT_TLM_STATE_MONITOR_REDUCTION_COUNT_SIZE(1);
    ASSERT_TLM_STATE_MONITOR_REDUCTION_COUNT(0, 1U);
}

void OnboardStateMonitorTester::testStateMonitorUpdatedRequiresMaskChange() {
    this->clearHistory();
    ASSERT_TRUE(this->component.reduceNow());
    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(0);

    this->clearHistory();
    ASSERT_TRUE(this->component.reduceNow());
    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(0);

    this->m_source.snapshot.haveGpsState = false;
    this->clearHistory();
    ASSERT_TRUE(this->component.reduceNow());

    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(1);
}

void OnboardStateMonitorTester::testStateMonitorUpdatedRemainsQuietAcrossSourceRecovery() {
    ASSERT_TRUE(this->component.reduceNow());

    this->m_source.available = false;
    this->clearHistory();
    ASSERT_FALSE(this->component.reduceNow());
    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(0);

    this->m_source.available = true;
    this->clearHistory();
    ASSERT_TRUE(this->component.reduceNow());
    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(0);
}

void OnboardStateMonitorTester::testMissingSourceRaisesWarning() {
    this->m_source.available = false;
    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);

    OBC::StateData::ReducedStateV1 state = {};
    ASSERT_FALSE(this->component.getReducedStateForRuntime(state));
    ASSERT_EVENTS_STATE_MONITOR_SOURCE_UNAVAILABLE_SIZE(1);
    ASSERT_EVENTS_STATE_MONITOR_UPDATED_SIZE(0);
}

void OnboardStateMonitorTester::testSourceFailureInvalidatesCachedState() {
    this->clearHistory();
    ASSERT_TRUE(this->component.reduceNow());

    OBC::StateData::ReducedStateV1 state = {};
    ASSERT_TRUE(this->component.getReducedStateForRuntime(state));

    this->m_source.available = false;
    this->clearHistory();
    ASSERT_FALSE(this->component.reduceNow());

    ASSERT_FALSE(this->component.getReducedStateForRuntime(state));
    ASSERT_EVENTS_STATE_MONITOR_SOURCE_UNAVAILABLE_SIZE(1);
    ASSERT_TLM_STATE_MONITOR_HAVE_STATE_SIZE(1);
    ASSERT_TLM_STATE_MONITOR_HAVE_STATE(0, 0U);
}

void OnboardStateMonitorTester::testRecentReducedStateRingIsRuntimeReadable() {
    this->m_source.snapshot.eps.soc = 70.0F;
    ASSERT_TRUE(this->component.reduceNow());
    this->m_source.snapshot.eps.soc = 71.0F;
    ASSERT_TRUE(this->component.reduceNow());

    OBC::StateData::ReducedStateV1 newest = {};
    OBC::StateData::ReducedStateV1 previous = {};
    OBC::StateData::ReducedStateV1 unavailable = {};
    ASSERT_TRUE(this->component.getRecentReducedStateForRuntime(0U, newest));
    ASSERT_TRUE(this->component.getRecentReducedStateForRuntime(1U, previous));
    ASSERT_FALSE(this->component.getRecentReducedStateForRuntime(2U, unavailable));
    ASSERT_EQ(newest.batterySoc, 71.0F);
    ASSERT_EQ(previous.batterySoc, 70.0F);
}

void OnboardStateMonitorTester::configureNominalSource_() {
    this->m_source.available = true;
    this->m_source.calls = 0U;
    this->m_source.snapshot = {};
    this->m_source.snapshot.timestamp = Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U);
    this->m_source.snapshot.mode = OBC::SatMode::IDLE;
    this->m_source.snapshot.uptimeSec = 10U;
    this->m_source.snapshot.rebootCount = 1U;
    this->m_source.snapshot.haveEpsStatus = true;
    this->m_source.snapshot.eps.soc = 80.0F;
    this->m_source.snapshot.eps.vbat = 8.0F;
    this->m_source.snapshot.haveAdcsState = true;
    this->m_source.snapshot.adcs.omega_x = 0.01F;
    this->m_source.snapshot.adcs.omega_y = 0.01F;
    this->m_source.snapshot.haveGpsState = true;
    this->m_source.snapshot.gps.fixValid = true;
    this->m_source.snapshot.haveStorageHealth = true;
    this->m_source.snapshot.storage.warningActive = false;
    this->m_source.snapshot.uartConnected = true;
    this->m_source.snapshot.radioLinkConnected = true;
    this->m_source.snapshot.haveRadioStatus = true;
}

}  // namespace OBC

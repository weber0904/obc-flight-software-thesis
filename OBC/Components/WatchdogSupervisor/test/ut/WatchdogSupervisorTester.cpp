#include "WatchdogSupervisorTester.hpp"

namespace OBC {

void WatchdogSupervisorTester::FakeRecoverySink::submitWatchdogFault(OBC::WatchdogSource source) {
    this->lastWatchdogSource = source;
    this->watchdogFaultCount++;
}

void WatchdogSupervisorTester::FakeRecoverySink::submitWatchdogSuppression(OBC::WatchdogSource source) {
    this->lastWatchdogSource = source;
    this->watchdogSuppressionCount++;
}

void WatchdogSupervisorTester::FakeRecoverySink::clearWatchdogFault(OBC::WatchdogSource source) {
    this->lastWatchdogSource = source;
    this->watchdogClearCount++;
}

void WatchdogSupervisorTester::FakeRecoverySink::submitEpsTimeoutFault(U32 failureCount) {
    this->lastEpsFailureCount = failureCount;
    this->epsFaultCount++;
}

void WatchdogSupervisorTester::FakeRecoverySink::clearEpsTimeoutFault(U32 failureCount) {
    this->lastEpsFailureCount = failureCount;
    this->epsClearCount++;
}

WatchdogSupervisorTester::WatchdogSupervisorTester(bool connectWatchdogFeedPort)
    : WatchdogSupervisorGTestBase("WatchdogSupervisorTester", MAX_HISTORY_SIZE),
      component("WatchdogSupervisor"),
      modeControl(),
      recoverySink(),
      feedStrokeCount(0U),
      lastFeedCode(0U) {
    this->initComponents();
    this->connectComponentPorts_(connectWatchdogFeedPort);
    this->component.configureRuntime(&this->modeControl, &this->recoverySink);
}

WatchdogSupervisorTester::~WatchdogSupervisorTester() = default;

void WatchdogSupervisorTester::testResourceMonitoringMigration() {
    this->clearHistory();
    this->sendCmd_HEALTH_ENABLE(TEST_INSTANCE_ID, 0, true);
    this->sendCmd_HEALTH_SET_THRESHOLD(TEST_INSTANCE_ID, 1, OBC::HealthItem::CPU_USAGE, 75.0F);
    this->sendCmd_HEALTH_SET_THRESHOLD(TEST_INSTANCE_ID, 2, OBC::HealthItem::MEM_RSS_MB, 128.0F);
    ASSERT_CMD_RESPONSE_SIZE(3);

    this->clearHistory();
    this->component.updateResourceSample(80.0F, 129.0F);

    ASSERT_TLM_SYS_CPU_USAGE_SIZE(1);
    ASSERT_TLM_SYS_MEM_RSS_MB_SIZE(1);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(1);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(1);
}

void WatchdogSupervisorTester::testResourceMonitoringWarningsOnlyOnThresholdCrossing() {
    this->clearHistory();
    this->sendCmd_HEALTH_ENABLE(TEST_INSTANCE_ID, 0, true);
    this->sendCmd_HEALTH_SET_THRESHOLD(TEST_INSTANCE_ID, 1, OBC::HealthItem::CPU_USAGE, 75.0F);
    this->sendCmd_HEALTH_SET_THRESHOLD(TEST_INSTANCE_ID, 2, OBC::HealthItem::MEM_RSS_MB, 128.0F);

    this->clearHistory();
    this->component.updateResourceSample(74.0F, 127.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(0);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(0);

    this->component.updateResourceSample(80.0F, 129.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(1);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(1);

    this->clearHistory();
    this->component.updateResourceSample(85.0F, 140.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(0);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(0);

    this->component.updateResourceSample(70.0F, 120.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(0);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(0);

    this->component.updateResourceSample(90.0F, 140.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(1);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(1);
}

void WatchdogSupervisorTester::testResourceMonitoringEnableReplaysActiveThresholdState() {
    this->clearHistory();
    this->sendCmd_HEALTH_SET_THRESHOLD(TEST_INSTANCE_ID, 0, OBC::HealthItem::CPU_USAGE, 75.0F);
    this->sendCmd_HEALTH_SET_THRESHOLD(TEST_INSTANCE_ID, 1, OBC::HealthItem::MEM_RSS_MB, 128.0F);
    ASSERT_CMD_RESPONSE_SIZE(2);

    this->clearHistory();
    this->component.updateResourceSample(80.0F, 129.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(0);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(0);

    this->sendCmd_HEALTH_ENABLE(TEST_INSTANCE_ID, 2, true);
    ASSERT_CMD_RESPONSE_SIZE(1);

    this->clearHistory();
    this->component.updateResourceSample(81.0F, 130.0F);
    ASSERT_EVENTS_SYS_RESOURCE_DEGRADED_SIZE(1);
    ASSERT_EVENTS_SYS_LOW_MEMORY_SIZE(1);
}

void WatchdogSupervisorTester::testHealthyBeatsKeepFeedEligible() {
    this->clearHistory();
    this->tickAllHealthy_();

    const OBC::WatchdogRuntimeSnapshot snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::HEALTHY);
    ASSERT_TRUE(snapshot.feedEligible);
    ASSERT_EQ(snapshot.feedStrokeAttemptCount, 1U);
    ASSERT_EQ(this->feedStrokeCount, 1U);
    ASSERT_from_watchdogFeedOut_SIZE(1);
}

void WatchdogSupervisorTester::testFaultEscalationAndRecovery() {
    this->clearHistory();

    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    OBC::WatchdogRuntimeSnapshot snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::LATCHED_FAULT);
    ASSERT_EQ(snapshot.recoveryLevel, OBC::WatchdogRecoveryLevel::SAFE_REQUESTED);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    ASSERT_EQ(this->recoverySink.watchdogFaultCount, 5U);
    ASSERT_EQ(snapshot.faultMask, 0x1FU);

    this->invoke_to_beatIn(0, 0U);
    this->invoke_to_beatIn(1, 1U);
    this->invoke_to_beatIn(2, 2U);
    this->invoke_to_beatIn(3, 3U);
    this->invoke_to_beatIn(4, 4U);
    this->invoke_to_schedIn(0, 0U);

    snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::HEALTHY);
    ASSERT_EQ(snapshot.recoveryLevel, OBC::WatchdogRecoveryLevel::NONE);
    ASSERT_TRUE(snapshot.feedEligible);
    ASSERT_EQ(this->recoverySink.watchdogClearCount, 5U);
    ASSERT_EVENTS_WATCHDOG_SOURCE_RECOVERED_SIZE(5);
}

void WatchdogSupervisorTester::testFaultLatchedInSafeDefersSafeUntilRequestableMode() {
    this->modeControl.currentMode = OBC::SatMode::SAFE;
    this->clearHistory();

    this->tickAllExcept_(OBC::WatchdogSource::EPS_BRIDGE);
    this->tickAllExcept_(OBC::WatchdogSource::EPS_BRIDGE);
    this->tickAllExcept_(OBC::WatchdogSource::EPS_BRIDGE);

    OBC::WatchdogRuntimeSnapshot snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::LATCHED_FAULT);
    ASSERT_EQ(snapshot.recoveryLevel, OBC::WatchdogRecoveryLevel::LATCHED_FAULT);
    ASSERT_EQ(snapshot.safeRequestCount, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    ASSERT_EQ(this->recoverySink.watchdogFaultCount, 1U);
    ASSERT_EVENTS_WATCHDOG_SOURCE_FAULT_ENTERED_SIZE(1);
    ASSERT_EVENTS_WATCHDOG_SOURCE_FAULT_ENTERED(0, OBC::WatchdogSource::EPS_BRIDGE, 3U, OBC::SatMode::SAFE, false);

    this->clearHistory();
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->tickAllExcept_(OBC::WatchdogSource::EPS_BRIDGE);

    snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::LATCHED_FAULT);
    ASSERT_EQ(snapshot.recoveryLevel, OBC::WatchdogRecoveryLevel::SAFE_REQUESTED);
    ASSERT_EQ(snapshot.safeRequestCount, 1U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    ASSERT_EQ(this->recoverySink.watchdogFaultCount, 2U);
    ASSERT_EVENTS_WATCHDOG_SOURCE_FAULT_ENTERED_SIZE(0);

    this->tickAllExcept_(OBC::WatchdogSource::EPS_BRIDGE);
    snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::FEED_SUPPRESSED);
    ASSERT_EQ(snapshot.recoveryLevel, OBC::WatchdogRecoveryLevel::FEED_SUPPRESSED);
    ASSERT_EQ(snapshot.safeRequestCount, 1U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    ASSERT_EQ(this->recoverySink.watchdogSuppressionCount, 1U);
}

void WatchdogSupervisorTester::testFeedStrokeAttemptRequiresConnectedFeedPort() {
    this->clearHistory();
    this->tickAllHealthy_();

    const OBC::WatchdogRuntimeSnapshot snapshot = this->component.getStatusForRuntime();
    ASSERT_EQ(snapshot.feedStrokeAttemptCount, 0U);
    ASSERT_TRUE(snapshot.feedEligible);
    ASSERT_EQ(this->feedStrokeCount, 0U);
    ASSERT_from_watchdogFeedOut_SIZE(0);
    ASSERT_TLM_WATCHDOG_FEED_STROKE_ATTEMPT_TOTAL_SIZE(1);
    ASSERT_TLM_WATCHDOG_FEED_STROKE_ATTEMPT_TOTAL(0, 0U);
}

void WatchdogSupervisorTester::testProbeSuppressionCommandUpdatesRuntimeState() {
    this->clearHistory();
    this->sendCmd_SET_WATCHDOG_PROBE_SUPPRESSION(TEST_INSTANCE_ID, 0U, OBC::WatchdogSource::ADCS_FDIR, true);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_SET_WATCHDOG_PROBE_SUPPRESSION, 0U, Fw::CmdResponse::OK);
    ASSERT_EVENTS_WATCHDOG_PROBE_SUPPRESSION_UPDATED_SIZE(1);
    ASSERT_EVENTS_WATCHDOG_PROBE_SUPPRESSION_UPDATED(0, OBC::WatchdogSource::ADCS_FDIR, true);

    OBC::WatchdogRuntimeSnapshot snapshot = this->component.getStatusForRuntime();
    ASSERT_TRUE(snapshot.sources.at(OBC::watchdogSourceIndex(OBC::WatchdogSource::ADCS_FDIR)).probeSuppressed);

    this->clearHistory();
    this->sendCmd_SET_WATCHDOG_PROBE_SUPPRESSION(TEST_INSTANCE_ID, 1U, OBC::WatchdogSource::ADCS_FDIR, false);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_SET_WATCHDOG_PROBE_SUPPRESSION, 1U, Fw::CmdResponse::OK);
    snapshot = this->component.getStatusForRuntime();
    ASSERT_FALSE(snapshot.sources.at(OBC::watchdogSourceIndex(OBC::WatchdogSource::ADCS_FDIR)).probeSuppressed);
}

void WatchdogSupervisorTester::testProbeSuppressionImmediatelyFeedsSuppressedState() {
    this->clearHistory();
    this->sendCmd_SET_WATCHDOG_PROBE_SUPPRESSION(TEST_INSTANCE_ID, 0U, OBC::WatchdogSource::ADCS_FDIR, true);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_SET_WATCHDOG_PROBE_SUPPRESSION, 0U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->recoverySink.watchdogSuppressionCount, 1U);
    ASSERT_EQ(this->recoverySink.watchdogFaultCount, 0U);
    ASSERT_EQ(this->recoverySink.lastWatchdogSource, OBC::WatchdogSource::ADCS_FDIR);
    ASSERT_EVENTS_WATCHDOG_SOURCE_FEED_SUPPRESSED_SIZE(1);
    ASSERT_EVENTS_WATCHDOG_SOURCE_FEED_SUPPRESSED(0, OBC::WatchdogSource::ADCS_FDIR, 5U, 5U);
    ASSERT_EVENTS_WATCHDOG_FEED_ELIGIBILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_WATCHDOG_FEED_ELIGIBILITY_CHANGED(
        0, false, watchdogSourceMask(OBC::WatchdogSource::ADCS_FDIR), watchdogSourceMask(OBC::WatchdogSource::ADCS_FDIR),
        OBC::WatchdogRecoveryLevel::FEED_SUPPRESSED);

    OBC::WatchdogRuntimeSnapshot snapshot = this->component.getStatusForRuntime();
    const auto& source = snapshot.sources.at(OBC::watchdogSourceIndex(OBC::WatchdogSource::ADCS_FDIR));
    ASSERT_TRUE(source.probeSuppressed);
    ASSERT_EQ(source.state, OBC::WatchdogState::FEED_SUPPRESSED);
    ASSERT_EQ(source.ageTicks, 5U);
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::FEED_SUPPRESSED);
    ASSERT_EQ(snapshot.recoveryLevel, OBC::WatchdogRecoveryLevel::FEED_SUPPRESSED);
    ASSERT_FALSE(snapshot.feedEligible);
    ASSERT_EQ(snapshot.suppressMask, watchdogSourceMask(OBC::WatchdogSource::ADCS_FDIR));
    ASSERT_EQ(snapshot.faultMask, watchdogSourceMask(OBC::WatchdogSource::ADCS_FDIR));

    this->clearHistory();
    this->sendCmd_SET_WATCHDOG_PROBE_SUPPRESSION(TEST_INSTANCE_ID, 1U, OBC::WatchdogSource::ADCS_FDIR, false);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_SET_WATCHDOG_PROBE_SUPPRESSION, 1U, Fw::CmdResponse::OK);
    ASSERT_EQ(this->recoverySink.watchdogClearCount, 1U);
    ASSERT_EVENTS_WATCHDOG_FEED_ELIGIBILITY_CHANGED_SIZE(1);
    ASSERT_EVENTS_WATCHDOG_FEED_ELIGIBILITY_CHANGED(0, true, 0U, 0U, OBC::WatchdogRecoveryLevel::NONE);

    snapshot = this->component.getStatusForRuntime();
    const auto& cleared = snapshot.sources.at(OBC::watchdogSourceIndex(OBC::WatchdogSource::ADCS_FDIR));
    ASSERT_FALSE(cleared.probeSuppressed);
    ASSERT_EQ(cleared.state, OBC::WatchdogState::HEALTHY);
    ASSERT_EQ(cleared.ageTicks, 0U);
    ASSERT_EQ(snapshot.aggregateState, OBC::WatchdogState::HEALTHY);
    ASSERT_TRUE(snapshot.feedEligible);
    ASSERT_EQ(snapshot.suppressMask, 0U);
    ASSERT_EQ(snapshot.faultMask, 0U);
}

void WatchdogSupervisorTester::connectComponentPorts_(bool connectWatchdogFeedPort) {
    this->connect_to_CmdDisp(0, this->component.get_CmdDisp_InputPort(0));

    this->component.set_CmdReg_OutputPort(0, this->get_from_CmdReg(0));
    this->component.set_CmdStatus_OutputPort(0, this->get_from_CmdStatus(0));
    this->component.set_Log_OutputPort(0, this->get_from_Log(0));
    this->component.set_LogText_OutputPort(0, this->get_from_LogText(0));
    this->component.set_Time_OutputPort(0, this->get_from_Time(0));
    this->component.set_Tlm_OutputPort(0, this->get_from_Tlm(0));

    for (FwIndexType port = 0; port < static_cast<FwIndexType>(OBC::WatchdogSource::NUM_CONSTANTS); ++port) {
        this->connect_to_beatIn(port, this->component.get_beatIn_InputPort(port));
    }
    this->connect_to_schedIn(0, this->component.get_schedIn_InputPort(0));

    if (connectWatchdogFeedPort) {
        this->component.set_watchdogFeedOut_OutputPort(0, this->get_from_watchdogFeedOut(0));
    }
}

void WatchdogSupervisorTester::tickAllHealthy_() {
    this->invoke_to_beatIn(0, 0U);
    this->invoke_to_beatIn(1, 1U);
    this->invoke_to_beatIn(2, 2U);
    this->invoke_to_beatIn(3, 3U);
    this->invoke_to_beatIn(4, 4U);
    this->invoke_to_schedIn(0, 0U);
}

void WatchdogSupervisorTester::tickAllExcept_(OBC::WatchdogSource source) {
    for (FwIndexType port = 0; port < static_cast<FwIndexType>(OBC::WatchdogSource::NUM_CONSTANTS); ++port) {
        if (static_cast<std::size_t>(port) == OBC::watchdogSourceIndex(source)) {
            continue;
        }
        this->invoke_to_beatIn(port, static_cast<U32>(port));
    }
    this->invoke_to_schedIn(0, 0U);
}

void WatchdogSupervisorTester::from_watchdogFeedOut_handler(FwIndexType portNum, U32 code) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_watchdogFeedOut(code);
    this->feedStrokeCount++;
    this->lastFeedCode = code;
}

}  // namespace OBC

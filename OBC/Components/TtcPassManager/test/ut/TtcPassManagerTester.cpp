#include "TtcPassManagerTester.hpp"

#include "Fw/Time/Time.hpp"

#include <cstdint>

namespace OBC {

void TtcPassManagerTester::FakeModeControl::applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) {
    this->requestedMode = mode;
    this->lastSource = source;
    this->currentMode = mode;
    this->applyCount++;
}

TtcPassManagerTester::TtcPassManagerTester()
    : TtcPassManagerGTestBase("TtcPassManagerTester", MAX_HISTORY_SIZE),
      component("TtcPassManager"),
      modeControl(),
      gpsProvider(),
      commProvider(),
      adcsControl() {
    this->initComponents();
    this->connectPorts();
    this->component.configureRuntime(&this->modeControl, &this->gpsProvider, &this->commProvider, &this->adcsControl);
    this->commProvider.state.sbandAvailable = true;
    this->gpsProvider.state.hasSample = true;
}

TtcPassManagerTester::~TtcPassManagerTester() = default;

void TtcPassManagerTester::testConfigAndWindowCommands() {
    this->clearHistory();
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 4U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->sendCmd_TTC_GET_STATUS(TEST_INSTANCE_ID, 2);

    ASSERT_CMD_RESPONSE_SIZE(3);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_TTC_SET_POLICY, 0, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(1, this->component.OPCODE_TTC_SET_PASS_WINDOW, 1, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(2, this->component.OPCODE_TTC_GET_STATUS, 2, Fw::CmdResponse::OK);
    ASSERT_EVENTS_TTC_POLICY_CONFIG_UPDATED_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_CONFIG_UPDATED(0, 1U, 4U);
    ASSERT_EVENTS_TTC_PASS_WINDOW_SET_SIZE(1);
    ASSERT_EVENTS_TTC_PASS_WINDOW_SET(0, 100U, 200U);
    ASSERT_TLM_TTC_POLICY_ENABLED_SIZE(3);
    ASSERT_TLM_TTC_POLICY_ENABLED(2, true);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED_SIZE(3);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED(0, false);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED(1, true);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED(2, true);
}

void TtcPassManagerTester::testGetStatusReplaysTelemetryWhenValuesUnchanged() {
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 4U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->sendCmd_TTC_GET_STATUS(TEST_INSTANCE_ID, 2);
    this->clearHistory();

    this->sendCmd_TTC_GET_STATUS(TEST_INSTANCE_ID, 3);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_TTC_GET_STATUS, 3, Fw::CmdResponse::OK);
    ASSERT_TLM_TTC_POLICY_ENABLED_SIZE(1);
    ASSERT_TLM_TTC_POLICY_ENABLED(0, true);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED_SIZE(1);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED(0, true);
    ASSERT_TLM_TTC_POLICY_WINDOW_ACTIVE_SIZE(1);
    ASSERT_TLM_TTC_POLICY_WINDOW_ACTIVE(0, false);
    ASSERT_TLM_TTC_POLICY_LOSS_TIMEOUT_SEC_SIZE(1);
    ASSERT_TLM_TTC_POLICY_LOSS_TIMEOUT_SEC(0, 4U);
    ASSERT_TLM_TTC_POLICY_WINDOW_START_UNIX_SEC_SIZE(1);
    ASSERT_TLM_TTC_POLICY_WINDOW_START_UNIX_SEC(0, 100U);
    ASSERT_TLM_TTC_POLICY_WINDOW_END_UNIX_SEC_SIZE(1);
    ASSERT_TLM_TTC_POLICY_WINDOW_END_UNIX_SEC(0, 200U);
    ASSERT_TLM_TTC_POLICY_GPS_TIME_VALID_SIZE(1);
    ASSERT_TLM_TTC_POLICY_GPS_TIME_VALID(0, false);
    ASSERT_TLM_TTC_POLICY_TTC_ACTIVE_SIZE(1);
    ASSERT_TLM_TTC_POLICY_TTC_ACTIVE(0, false);
}

void TtcPassManagerTester::testSetPolicyReplaysTelemetryWhenValuesUnchanged() {
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 4U);
    this->clearHistory();

    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 1, true, 4U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_TTC_SET_POLICY, 1, Fw::CmdResponse::OK);
    ASSERT_TLM_TTC_POLICY_ENABLED_SIZE(1);
    ASSERT_TLM_TTC_POLICY_ENABLED(0, true);
    ASSERT_TLM_TTC_POLICY_LOSS_TIMEOUT_SEC_SIZE(1);
    ASSERT_TLM_TTC_POLICY_LOSS_TIMEOUT_SEC(0, 4U);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED_SIZE(1);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED(0, false);
    ASSERT_TLM_TTC_POLICY_GPS_TIME_VALID_SIZE(1);
    ASSERT_TLM_TTC_POLICY_GPS_TIME_VALID(0, false);
    ASSERT_TLM_TTC_POLICY_TTC_ACTIVE_SIZE(1);
    ASSERT_TLM_TTC_POLICY_TTC_ACTIVE(0, false);
}

void TtcPassManagerTester::testSchedInPublishesOnlyChangeDrivenOperatorStatus() {
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(1);
    ASSERT_TLM_TTC_POLICY_ENABLED_SIZE(0);
    ASSERT_TLM_TTC_POLICY_LOSS_TIMEOUT_SEC_SIZE(0);
    ASSERT_TLM_TTC_POLICY_WINDOW_CONFIGURED_SIZE(0);
    ASSERT_TLM_TTC_POLICY_WINDOW_START_UNIX_SEC_SIZE(0);
    ASSERT_TLM_TTC_POLICY_WINDOW_END_UNIX_SEC_SIZE(0);
    ASSERT_TLM_TTC_POLICY_CURRENT_GPS_UNIX_SEC_SIZE(0);
    ASSERT_TLM_TTC_POLICY_LOSS_TIMER_SEC_SIZE(0);
    ASSERT_TLM_TTC_POLICY_LAST_ENTRY_REASON_SIZE(0);
    ASSERT_TLM_TTC_POLICY_LAST_EXIT_REASON_SIZE(0);
    ASSERT_TLM_TTC_POLICY_WINDOW_ACTIVE_SIZE(1);
    ASSERT_TLM_TTC_POLICY_WINDOW_ACTIVE(0, true);
    ASSERT_TLM_TTC_POLICY_GPS_TIME_VALID_SIZE(1);
    ASSERT_TLM_TTC_POLICY_GPS_TIME_VALID(0, true);
    ASSERT_TLM_TTC_POLICY_TTC_ACTIVE_SIZE(1);
    ASSERT_TLM_TTC_POLICY_TTC_ACTIVE(0, true);
    ASSERT_TLM_TTC_POLICY_ENTRY_COUNT_SIZE(1);
    ASSERT_TLM_TTC_POLICY_ENTRY_COUNT(0, 1U);
    ASSERT_TLM_TTC_POLICY_EXIT_COUNT_SIZE(0);
}

void TtcPassManagerTester::testInvalidWindowRejectedFailClosed() {
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 0, 100U, 200U);
    this->clearHistory();

    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 200U, 200U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_TTC_SET_PASS_WINDOW, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_TTC_PASS_WINDOW_REJECTED_SIZE(1);
    ASSERT_EVENTS_TTC_PASS_WINDOW_REJECTED(0, 200U, 200U);
    const OBC::TtcPassRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_FALSE(status.windowConfigured);
}

void TtcPassManagerTester::testAutoEntryRequiresEnabledAndActiveWindow() {
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::TTC);
    ASSERT_EQ(this->modeControl.lastSource, OBC::ModeApplySource::TtcPassPolicy);
    ASSERT_EQ(this->adcsControl.requestCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST(0, static_cast<U32>(OBC::TtcPassPolicyReason::WINDOW_ACTIVE));
    const OBC::TtcPassRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.entryCount, 1U);
    ASSERT_EQ(status.lastEntryReason, OBC::TtcPassPolicyReason::WINDOW_ACTIVE);
}

void TtcPassManagerTester::testAutoEntryTriggersAdcsPointingOnce() {
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);

    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->adcsControl.requestCount, 1U);

    this->clearHistory();
    this->makeGpsValidAtEpoch_(151U, 11U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->adcsControl.requestCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(0);
}

void TtcPassManagerTester::testAdcsTriggerFailureDoesNotBlockTtcEntry() {
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->adcsControl.shouldSucceed = false;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::TTC);
    ASSERT_EQ(this->adcsControl.requestCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_ADCS_POINTING_REQUEST_FAILED_SIZE(1);
    ASSERT_EQ(this->component.getStatusForRuntime().currentMode, OBC::SatMode::TTC);
}

void TtcPassManagerTester::testWindowInactiveExitsTtc() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(201U, 10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::IDLE);
    ASSERT_EQ(this->modeControl.lastSource, OBC::ModeApplySource::TtcPassPolicy);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST(0, static_cast<U32>(OBC::TtcPassPolicyReason::WINDOW_INACTIVE));
    const OBC::TtcPassRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.exitCount, 1U);
    ASSERT_EQ(status.lastExitReason, OBC::TtcPassPolicyReason::WINDOW_INACTIVE);
}

void TtcPassManagerTester::testGpsInvalidExitsTtc() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 200U);
    this->gpsProvider.state.hasSample = false;
    this->clearHistory();
    this->setTestTimeSec_(10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST(0, static_cast<U32>(OBC::TtcPassPolicyReason::GPS_INVALID));
}

void TtcPassManagerTester::testCommLossTimeoutExitsTtc() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 2U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 300U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);
    this->commProvider.state.sbandAvailable = false;
    this->commProvider.state.uhfAvailable = false;

    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    this->makeGpsValidAtEpoch_(151U, 11U);
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    this->makeGpsValidAtEpoch_(152U, 12U);
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);
    this->makeGpsValidAtEpoch_(153U, 13U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST(0, static_cast<U32>(OBC::TtcPassPolicyReason::COMM_LOSS_TIMEOUT));
}

void TtcPassManagerTester::testCommLossTimeoutUsesElapsedWallclockSeconds() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 1U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 300U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);
    this->commProvider.state.sbandAvailable = false;
    this->commProvider.state.uhfAvailable = false;

    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->component.getStatusForRuntime().lossOfLockTimerSec, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);

    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->component.getStatusForRuntime().lossOfLockTimerSec, 0U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);

    this->makeGpsValidAtEpoch_(151U, 11U);
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->component.getStatusForRuntime().lossOfLockTimerSec, 1U);
    ASSERT_EQ(this->modeControl.applyCount, 0U);

    this->makeGpsValidAtEpoch_(152U, 12U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST(0, static_cast<U32>(OBC::TtcPassPolicyReason::COMM_LOSS_TIMEOUT));
}

void TtcPassManagerTester::testCommLossTimeoutSuppressesReentryUntilWindowChanges() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 1U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 300U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(150U, 10U);
    this->commProvider.state.sbandAvailable = false;
    this->commProvider.state.uhfAvailable = false;

    this->invoke_to_schedIn(0, 0U);
    this->makeGpsValidAtEpoch_(151U, 11U);
    this->invoke_to_schedIn(0, 0U);
    this->makeGpsValidAtEpoch_(152U, 12U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::IDLE);

    this->clearHistory();
    this->commProvider.state.sbandAvailable = true;
    this->makeGpsValidAtEpoch_(153U, 13U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(0);

    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 2, 140U, 260U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(154U, 14U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 2U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::TTC);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(1);
}

void TtcPassManagerTester::testManualTtcStillSubjectToPolicy() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, false, 0U);
    this->clearHistory();
    this->gpsProvider.state.hasSample = false;
    this->setTestTimeSec_(10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::IDLE);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST_SIZE(1);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST(0, static_cast<U32>(OBC::TtcPassPolicyReason::TTC_DISABLED));
}

void TtcPassManagerTester::testManualIdleSuppressesReentryUntilWindowChanges() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 5U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 300U);
    this->makeGpsValidAtEpoch_(150U, 10U);
    this->invoke_to_schedIn(0, 0U);

    this->clearHistory();
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->makeGpsValidAtEpoch_(151U, 11U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 0U);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(0);
    ASSERT_EQ(this->component.getStatusForRuntime().currentMode, OBC::SatMode::IDLE);

    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 2, 140U, 260U);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(152U, 12U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::TTC);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(1);
}

void TtcPassManagerTester::testMidnightGpsSampleStillAllowsEntry() {
    this->modeControl.currentMode = OBC::SatMode::IDLE;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 3U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 1747094400ULL, 1747094460ULL);
    this->clearHistory();
    this->makeGpsValidAtEpoch_(1747094400ULL, 10U);

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 1U);
    ASSERT_EQ(this->modeControl.requestedMode, OBC::SatMode::TTC);
    ASSERT_TRUE(this->component.getStatusForRuntime().gpsTimeValid);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(1);
}

void TtcPassManagerTester::testSafetyOverrideClearsLossTimerWithoutRestoringTtc() {
    this->modeControl.currentMode = OBC::SatMode::TTC;
    this->sendCmd_TTC_SET_POLICY(TEST_INSTANCE_ID, 0, true, 5U);
    this->sendCmd_TTC_SET_PASS_WINDOW(TEST_INSTANCE_ID, 1, 100U, 300U);
    this->makeGpsValidAtEpoch_(150U, 10U);
    this->commProvider.state.sbandAvailable = false;
    this->commProvider.state.uhfAvailable = false;
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->component.getStatusForRuntime().lossOfLockTimerSec, 0U);

    this->clearHistory();
    this->modeControl.currentMode = OBC::SatMode::SAFE;
    this->makeGpsValidAtEpoch_(151U, 11U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->modeControl.applyCount, 0U);
    ASSERT_EQ(this->component.getStatusForRuntime().lossOfLockTimerSec, 0U);
    ASSERT_EQ(this->adcsControl.requestCount, 0U);
    ASSERT_EVENTS_TTC_POLICY_ENTRY_REQUEST_SIZE(0);
    ASSERT_EVENTS_TTC_POLICY_EXIT_REQUEST_SIZE(0);
}

void TtcPassManagerTester::setTestTimeSec_(const U32 seconds) {
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, seconds, 0U));
}

void TtcPassManagerTester::makeGpsValidAtEpoch_(const U64 epochSec, const U32 wallclockSec) {
    this->setTestTimeSec_(wallclockSec);
    this->gpsProvider.available = true;
    this->gpsProvider.state.hasSample = true;
    this->gpsProvider.state.fixValid = true;
    this->gpsProvider.state.acceptedSentenceCount++;
    const U32 secOfDay = static_cast<U32>(epochSec % 86400ULL);
    const U64 daysSinceEpoch = epochSec / 86400ULL;

    I64 z = static_cast<I64>(daysSinceEpoch) + 719468;
    const I64 era = (z >= 0 ? z : z - 146096) / 146097;
    const U32 doe = static_cast<U32>(z - era * 146097);
    const U32 yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    I64 year = static_cast<I64>(yoe) + era * 400;
    const U32 doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    const U32 mp = (5U * doy + 2U) / 153U;
    const U32 day = doy - (153U * mp + 2U) / 5U + 1U;
    const U32 month = mp < 10U ? mp + 3U : mp - 9U;
    year += month <= 2U ? 1 : 0;

    this->gpsProvider.state.utcDateYmd =
        static_cast<U32>(year) * 10000U + month * 100U + day;
    this->gpsProvider.state.utcSecondsOfDay = secOfDay;
}

}  // namespace OBC

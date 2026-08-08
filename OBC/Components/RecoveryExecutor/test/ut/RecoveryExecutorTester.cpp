#include "RecoveryExecutorTester.hpp"

#include <cstdlib>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Os/FileSystem.hpp"

namespace OBC {

void RecoveryExecutorTester::FakeModeControl::applyModeForInternalSource(OBC::SatMode mode,
                                                                         OBC::ModeApplySource source) {
    this->requestedMode = mode;
    this->lastSource = source;
    this->applyCount++;
    this->currentMode = mode;
}

OBC::EPS::TransportStatus RecoveryExecutorTester::FakeEpsTransport::getStatus(OBC::EPS::StatusData& outStatus) {
    outStatus = this->lastStatus;
    return OBC::EPS::TransportStatus::OK;
}

OBC::EPS::TransportStatus RecoveryExecutorTester::FakeEpsTransport::setPdu(std::uint8_t,
                                                                            bool,
                                                                            OBC::EPS::StatusData& outStatus) {
    outStatus = this->lastStatus;
    return OBC::EPS::TransportStatus::OK;
}

OBC::EPS::TransportStatus RecoveryExecutorTester::FakeEpsTransport::setHeater(bool,
                                                                               OBC::EPS::StatusData& outStatus) {
    outStatus = this->lastStatus;
    return OBC::EPS::TransportStatus::OK;
}

OBC::EPS::TransportStatus RecoveryExecutorTester::FakeEpsTransport::reset(OBC::EPS::StatusData& outStatus) {
    this->resetCalls++;
    outStatus = this->lastStatus;
    return this->resetStatus;
}

OBC::ADCS::TransportStatus RecoveryExecutorTester::FakeAdcsTransport::getState(OBC::ADCS::StateData& outState) {
    outState = this->lastState;
    return OBC::ADCS::TransportStatus::OK;
}

OBC::ADCS::TransportStatus RecoveryExecutorTester::FakeAdcsTransport::setMode(std::uint8_t,
                                                                               OBC::ADCS::StateData& outState) {
    outState = this->lastState;
    return OBC::ADCS::TransportStatus::OK;
}

OBC::ADCS::TransportStatus RecoveryExecutorTester::FakeAdcsTransport::setTarget(double,
                                                                                 double,
                                                                                 double,
                                                                                 double,
                                                                                 OBC::ADCS::StateData& outState) {
    outState = this->lastState;
    return OBC::ADCS::TransportStatus::OK;
}

OBC::ADCS::TransportStatus RecoveryExecutorTester::FakeAdcsTransport::calibrate(std::uint8_t,
                                                                                 OBC::ADCS::StateData& outState) {
    outState = this->lastState;
    return OBC::ADCS::TransportStatus::OK;
}

OBC::ADCS::TransportStatus RecoveryExecutorTester::FakeAdcsTransport::reset(OBC::ADCS::StateData& outState) {
    this->resetCalls++;
    outState = this->lastState;
    return this->resetStatus;
}

OBC::RecoveryCommActionResult RecoveryExecutorTester::FakeCommControl::performRecoveryLinkFailoverForRuntime() {
    this->actionCount++;
    if (this->owner != nullptr) {
        this->lastObservedStatus = this->owner->getStatusForRuntime();
    }
    return this->nextResult;
}

RecoveryExecutorTester::RecoveryExecutorTester()
    : RecoveryExecutorGTestBase("RecoveryExecutorTester", MAX_HISTORY_SIZE),
      m_tempRoot(this->makeTempRoot_()),
      m_modeControl(),
      m_epsTransport(),
      m_adcsTransport(),
      m_commControl(),
      m_faultRecorder(),
      m_bootManager("RecoveryExecutorBootManager"),
      m_adcsBridge("RecoveryExecutorAdcsBridge"),
      m_epsBridge("RecoveryExecutorEpsBridge"),
      component("RecoveryExecutor") {
    this->initComponents();
    this->connectPorts();
    this->m_bootManager.init(0);
    this->m_adcsBridge.init(0);
    this->m_epsBridge.init(0);
    this->m_bootManager.configurePersistentFaultRecorderForRuntime(&this->m_faultRecorder);
    this->m_bootManager.configureStorageRootForTest(this->m_tempRoot + "/persistent-data/boot");
    this->m_adcsTransport.lastState = {};
    this->m_adcsTransport.lastState.q0 = 1.0;
    this->m_adcsTransport.lastState.mode = 0U;
    this->m_adcsTransport.lastState.sensor_valid = 1U;
    this->m_adcsBridge.set_adcsStatusRefreshTlmOut_OutputPort(0, this->get_from_Tlm(0));
    this->m_epsBridge.set_epsStatusRefreshTlmOut_OutputPort(0, this->get_from_Tlm(0));
    this->m_adcsBridge.setTransportForTest(&this->m_adcsTransport);
    this->m_epsBridge.setTransportForTest(&this->m_epsTransport);
    this->m_commControl.owner = &this->component;
    this->component.configureRuntime(
        &this->m_modeControl, &this->m_epsBridge, &this->m_adcsBridge, &this->m_bootManager, &this->m_commControl);
    this->component.configurePersistentFaultRecorderForRuntime(&this->m_faultRecorder);
    this->m_faultRecorder.records.clear();
    this->clearHistory();
}

RecoveryExecutorTester::~RecoveryExecutorTester() {
    (void)Os::FileSystem::removeFile((this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data/boot").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data").c_str());
    (void)Os::FileSystem::removeDirectory(this->m_tempRoot.c_str());
}

void RecoveryExecutorTester::testWatchdogFaultQueuesProcessRestartAndClears() {
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;

    this->clearHistory();
    this->component.submitWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::PROCESS_RESTART);
    ASSERT_TRUE(status.pendingProcessRestart);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 1U);
    ASSERT_EQ(status.totalReboots, 0U);
    ASSERT_EQ(this->m_modeControl.applyCount, 0U);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoverySource,
              OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoveryLevel,
              OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_EVENTS_RECOVERY_INCIDENT_OPENED_SIZE(1);
    ASSERT_EVENTS_RECOVERY_ACTION_REQUESTED_SIZE(1);
    ASSERT_EVENTS_RECOVERY_ACTION_EXECUTED_SIZE(1);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::PROCESS_RESTART);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);

    this->clearHistory();
    this->component.clearWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);
    const OBC::RecoveryRuntimeStatus cleared = this->component.getStatusForRuntime();
    ASSERT_EQ(cleared.activeIncidentCount, 0U);
    ASSERT_EQ(cleared.activeSource, OBC::RecoveryIncidentSource::NONE);
    ASSERT_EQ(cleared.currentLevel, OBC::RecoveryLevel::R0_RECORD_ONLY);
    ASSERT_EQ(cleared.highestLevel, OBC::RecoveryLevel::R0_RECORD_ONLY);
    ASSERT_EQ(cleared.lastAction, OBC::RecoveryAction::NONE);
    ASSERT_FALSE(cleared.pendingProcessRestart);
    ASSERT_FALSE(cleared.pendingReboot);
    ASSERT_EVENTS_RECOVERY_INCIDENT_CLEARED_SIZE(1);
}

void RecoveryExecutorTester::testWatchdogSuppressionEscalatesToReboot() {
    this->clearHistory();
    this->component.submitWatchdogSuppression(OBC::WatchdogSource::EPS_BRIDGE);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::OBC_REBOOT);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::OBC_REBOOT);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoverySource,
              OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoveryLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EVENTS_RECOVERY_REBOOT_PENDING_SIZE(1);
    ASSERT_EVENTS_RECOVERY_REBOOT_ISSUED_SIZE(1);
}

void RecoveryExecutorTester::testWatchdogSuppressionWithHardwareWatchdogLeavesPendingRebootWithoutRuntimeExit() {
    this->component.configureHardwareWatchdogModeForRuntime(true);

    this->clearHistory();
    this->component.submitWatchdogSuppression(OBC::WatchdogSource::EPS_BRIDGE);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::OBC_REBOOT);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.totalReboots, 1U);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoverySource,
              OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoveryLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
}

void RecoveryExecutorTester::testWatchdogFaultInHardwareWatchdogModeStillQueuesProcessRestart() {
    this->component.configureHardwareWatchdogModeForRuntime(true);
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;

    this->clearHistory();
    this->component.submitWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);

    OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::PROCESS_RESTART);
    ASSERT_TRUE(status.pendingProcessRestart);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 1U);
    ASSERT_EQ(status.totalReboots, 0U);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_WATCHDOG);
    ASSERT_EVENTS_RECOVERY_INCIDENT_OPENED_SIZE(1);
    ASSERT_EVENTS_RECOVERY_ACTION_REQUESTED_SIZE(1);
    ASSERT_EVENTS_RECOVERY_ACTION_EXECUTED_SIZE(1);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::PROCESS_RESTART);
}

void RecoveryExecutorTester::testEpsFaultResetThenTimeoutEscalatesToReboot() {
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;

    this->clearHistory();
    this->component.submitEpsTimeoutFault(3U);

    OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::EPS_TIMEOUT);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R5_MODE_FALLBACK);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R5_MODE_FALLBACK);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::SAFE_FALLBACK);
    ASSERT_EQ(status.totalResetActions, 1U);
    ASSERT_EQ(this->m_epsTransport.resetCalls, 1U);
    ASSERT_EQ(this->m_modeControl.applyCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::FdirSubsystemFault);

    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_EPS_TIMEOUT);
    ASSERT_TRUE(this->component.consumeRebootRequestForRuntime());
    ASSERT_EVENTS_RECOVERY_REBOOT_PENDING_SIZE(1);
    ASSERT_EVENTS_RECOVERY_REBOOT_ISSUED_SIZE(1);
}

void RecoveryExecutorTester::testNonWatchdogRebootStillRequestsRuntimeExitWhenHardwareWatchdogEnabled() {
    this->component.configureHardwareWatchdogModeForRuntime(true);
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;

    this->clearHistory();
    this->component.submitEpsTimeoutFault(3U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::EPS_TIMEOUT);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::OBC_REBOOT);
}

void RecoveryExecutorTester::testExistingRuntimeRebootRequestSurvivesWatchdogRebootIntent() {
    this->component.configureHardwareWatchdogModeForRuntime(true);
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;

    this->clearHistory();
    this->component.submitEpsTimeoutFault(3U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::EPS_TIMEOUT);

    this->component.submitWatchdogSuppression(OBC::WatchdogSource::EPS_BRIDGE);

    status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.totalReboots, 2U);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::OBC_REBOOT);
}

void RecoveryExecutorTester::testBootSafeFallbackAndStableAck() {
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->m_bootManager.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                             OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                             OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->m_bootManager.simulateRuntimeBootForTest());
    }
    ASSERT_EQ(this->m_bootManager.getConsecutiveResetCountForRuntime(), 3U);
    ASSERT_TRUE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());

    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->m_modeControl.applyCount = 0U;
    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->m_modeControl.applyCount, 1U);
    ASSERT_EQ(this->m_modeControl.requestedMode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::RecoveryBootFallback);

    this->clearHistory();
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->m_bootManager.getConsecutiveResetCountForRuntime(), 0U);
    ASSERT_FALSE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());
}

void RecoveryExecutorTester::testBootSafeFallbackClampSuppressesR2RestartLoop() {
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->m_bootManager.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                             OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                             OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->m_bootManager.simulateRuntimeBootForTest());
    }
    ASSERT_TRUE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());

    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->m_modeControl.applyCount = 0U;
    this->clearHistory();
    this->component.submitWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R5_MODE_FALLBACK);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::SAFE_FALLBACK);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 0U);
    ASSERT_EQ(this->m_modeControl.applyCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::WatchdogFault);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
}

void RecoveryExecutorTester::testBootSafeFallbackClampAlreadySafeQueuesSafeAction() {
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->m_bootManager.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                             OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                             OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->m_bootManager.simulateRuntimeBootForTest());
    }
    ASSERT_TRUE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());

    this->m_modeControl.currentMode = OBC::SatMode::SAFE;
    this->m_modeControl.applyCount = 0U;
    this->clearHistory();
    this->component.submitAdcsPollTransportFault(3U);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R5_MODE_FALLBACK);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::SAFE_FALLBACK);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 0U);
    ASSERT_EQ(status.totalResetActions, 1U);
    ASSERT_EQ(status.totalSafeFallbacks, 1U);
    ASSERT_EQ(this->m_adcsTransport.resetCalls, 1U);
    ASSERT_EQ(this->m_modeControl.applyCount, 1U);
    ASSERT_EQ(this->m_modeControl.requestedMode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::FdirSubsystemFault);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
    ASSERT_EVENTS_RECOVERY_ACTION_REQUESTED_SIZE(2);
    ASSERT_EVENTS_RECOVERY_ACTION_EXECUTED_SIZE(2);
}

void RecoveryExecutorTester::testGetRecoveryStatusCommandReportsState() {
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->component.submitWatchdogFault(OBC::WatchdogSource::COMM_CONTROLLER);

    this->clearHistory();
    this->sendCmd_GET_RECOVERY_STATUS(TEST_INSTANCE_ID, 3U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GET_RECOVERY_STATUS, 3U, Fw::CmdResponse::OK);
    ASSERT_EVENTS_RECOVERY_STATUS_SIZE(1);
    ASSERT_EVENTS_RECOVERY_STATUS(0,
                                  1U,
                                  OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER,
                                  OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT,
                                  OBC::RecoveryAction::PROCESS_RESTART,
                                  true,
                                  false,
                                  0U);
}

void RecoveryExecutorTester::testRebootRequestWaitsForBootMetadataPersistence() {
    this->clearHistory();
    this->m_bootManager.configureStorageRootForTest(this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt");
    this->component.configureRuntime(
        &this->m_modeControl, &this->m_epsBridge, &this->m_adcsBridge, &this->m_bootManager, &this->m_commControl);

    this->component.submitWatchdogSuppression(OBC::WatchdogSource::EPS_BRIDGE);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_FALSE(this->component.consumeRebootRequestForRuntime());
    ASSERT_EQ(status.totalReboots, 0U);
}

void RecoveryExecutorTester::testProcessRestartPersistenceFailureFallsBackToSafe() {
    this->clearHistory();
    this->m_bootManager.configureStorageRootForTest(this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt");
    this->component.configureRuntime(
        &this->m_modeControl, &this->m_epsBridge, &this->m_adcsBridge, &this->m_bootManager, &this->m_commControl);
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->m_modeControl.applyCount = 0U;

    this->component.submitWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);

    OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R5_MODE_FALLBACK);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R5_MODE_FALLBACK);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::SAFE_FALLBACK);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 0U);
    ASSERT_EQ(status.totalSafeFallbacks, 1U);
    ASSERT_EQ(this->m_modeControl.applyCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::WatchdogFault);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
    ASSERT_EVENTS_RECOVERY_ACTION_REQUESTED_SIZE(2);
    ASSERT_EVENTS_RECOVERY_ACTION_EXECUTED_SIZE(2);

    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::OBC_REBOOT);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
}

void RecoveryExecutorTester::testStableAckFailureRetriesAfterPersistenceReturns() {
    const std::string metadataPath = this->m_tempRoot + "/persistent-data/boot/metadata-v1.txt";
    for (U32 i = 0U; i < 3U; i++) {
        ASSERT_TRUE(this->m_bootManager.recordRecoveryRebootIntentForRuntime(OBC::ResetCause::RECOVERY_WATCHDOG,
                                                                             OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE,
                                                                             OBC::RecoveryLevel::R6_OBC_REBOOT));
        ASSERT_TRUE(this->m_bootManager.simulateRuntimeBootForTest());
    }
    ASSERT_EQ(this->m_bootManager.getConsecutiveResetCountForRuntime(), 3U);
    ASSERT_TRUE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());

    ASSERT_EQ(::chmod(metadataPath.c_str(), 0444), 0);
    for (U32 i = 0U; i < 5U; i++) {
        this->invoke_to_schedIn(0, 0U);
    }
    ASSERT_EQ(this->m_bootManager.getConsecutiveResetCountForRuntime(), 3U);
    ASSERT_TRUE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());

    ASSERT_EQ(::chmod(metadataPath.c_str(), 0644), 0);
    this->invoke_to_schedIn(0, 0U);
    ASSERT_EQ(this->m_bootManager.getConsecutiveResetCountForRuntime(), 0U);
    ASSERT_FALSE(this->m_bootManager.isBootSafeFallbackRequiredForRuntime());
}

void RecoveryExecutorTester::testClearDoesNotDropPendingRebootBeforeConsume() {
    this->clearHistory();
    this->component.submitWatchdogSuppression(OBC::WatchdogSource::EPS_BRIDGE);

    OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::OBC_REBOOT);

    this->component.clearWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);
    status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.activeIncidentCount, 0U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::OBC_REBOOT);

    ASSERT_TRUE(this->component.consumeRebootRequestForRuntime());
    status = this->component.getStatusForRuntime();
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.activeIncidentCount, 0U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::NONE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R0_RECORD_ONLY);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::NONE);
}

void RecoveryExecutorTester::testAdcsFaultQueuesResetWithoutProcessRestart() {
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->m_adcsTransport.resetCalls = 0U;
    this->m_adcsTransport.resetStatus = OBC::ADCS::TransportStatus::OK;

    this->clearHistory();
    this->component.submitAdcsPollTransportFault(3U);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeIncidentCount, 1U);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 0U);
    ASSERT_EQ(status.totalResetActions, 1U);
    ASSERT_EQ(this->m_adcsTransport.resetCalls, 1U);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::UNKNOWN);
    ASSERT_EQ(this->m_modeControl.applyCount, 0U);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoverySource, OBC::RecoveryIncidentSource::NONE);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoveryLevel, OBC::RecoveryLevel::R0_RECORD_ONLY);
    ASSERT_FALSE(this->m_bootManager.getMetadataForRuntime().recoveryResetPending);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);
}

void RecoveryExecutorTester::testAdcsRelatchEscalatesThroughExistingRebootPath() {
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->m_adcsTransport.resetCalls = 0U;
    this->m_adcsTransport.resetStatus = OBC::ADCS::TransportStatus::OK;

    this->clearHistory();
    this->component.submitAdcsPollTransportFault(3U);
    ASSERT_EQ(this->m_adcsTransport.resetCalls, 1U);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::NONE);

    this->component.clearAdcsPollTransportFault(3U);
    this->clearHistory();
    this->component.submitAdcsPollTransportFault(3U);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.highestLevel, OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::OBC_REBOOT);
    ASSERT_FALSE(status.pendingProcessRestart);
    ASSERT_TRUE(status.pendingReboot);
    ASSERT_EQ(status.totalProcessRestarts, 0U);
    ASSERT_EQ(status.totalResetActions, 1U);
    ASSERT_EQ(this->m_adcsTransport.resetCalls, 1U);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::RECOVERY_ADCS_FDIR);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoverySource,
              OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ASSERT_EQ(this->m_bootManager.getMetadataForRuntime().lastRecoveryLevel,
              OBC::RecoveryLevel::R6_OBC_REBOOT);
    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::OBC_REBOOT);
}

void RecoveryExecutorTester::testCommFailoverRunsOutsideExecutorLock() {
    this->m_modeControl.currentMode = OBC::SatMode::IDLE;
    this->m_commControl.nextResult = {};
    this->m_commControl.nextResult.switched = true;
    this->m_commControl.nextResult.finalPrimaryCommandLink = OBC::CommBand::UHF;
    this->m_commControl.nextResult.finalPrimaryTelemetryLink = OBC::CommBand::UHF;
    this->m_commControl.nextResult.finalPrimaryFileLink = OBC::CommBand::UHF;

    this->clearHistory();
    this->component.submitCommPrimaryUnavailableFault(3U);

    const OBC::RecoveryRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_EQ(this->m_commControl.actionCount, 1U);
    ASSERT_EQ(this->m_commControl.lastObservedStatus.activeSource, OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE);
    ASSERT_EQ(this->m_commControl.lastObservedStatus.currentLevel, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE);
    ASSERT_EQ(status.activeSource, OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE);
    ASSERT_EQ(status.currentLevel, OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE);
    ASSERT_EQ(status.lastAction, OBC::RecoveryAction::COMM_LINK_FAILOVER);
    ASSERT_FALSE(status.pendingReboot);
    ASSERT_EQ(this->m_bootManager.getResetCauseForRuntime(), OBC::ResetCause::UNKNOWN);
}

void RecoveryExecutorTester::testPersistentFaultLifecycleBreadcrumbs() {
    this->setTestTime(Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 321U, 0U));
    for (U32 i = 0U; i < 9U; i++) {
        this->m_bootManager.tickForTest();
    }
    this->clearHistory();
    this->component.submitWatchdogSuppression(OBC::WatchdogSource::EPS_BRIDGE);

    ASSERT_EQ(this->m_faultRecorder.records.size(), 5U);
    ASSERT_EQ(this->m_faultRecorder.records.at(0).kind, OBC::PersistentFaultRecordKind::INCIDENT_OPENED);
    ASSERT_EQ(this->m_faultRecorder.records.at(0).timestampSec, 321U);
    ASSERT_EQ(this->m_faultRecorder.records.at(0).uptimeSec, 9U);
    ASSERT_EQ(this->m_faultRecorder.records.at(1).kind, OBC::PersistentFaultRecordKind::ACTION_REQUESTED);
    ASSERT_EQ(this->m_faultRecorder.records.at(1).action, OBC::RecoveryAction::OBC_REBOOT);
    ASSERT_EQ(this->m_faultRecorder.records.at(2).kind, OBC::PersistentFaultRecordKind::REBOOT_PENDING);
    ASSERT_EQ(this->m_faultRecorder.records.at(3).kind, OBC::PersistentFaultRecordKind::ACTION_EXECUTED);
    ASSERT_EQ(this->m_faultRecorder.records.at(3).action, OBC::RecoveryAction::OBC_REBOOT);
    ASSERT_EQ(this->m_faultRecorder.records.at(4).kind, OBC::PersistentFaultRecordKind::REBOOT_ISSUED);

    ASSERT_EQ(this->component.consumeRecoveryExitRequestForRuntime(), OBC::RecoveryExitRequest::OBC_REBOOT);
    this->component.clearWatchdogFault(OBC::WatchdogSource::EPS_BRIDGE);
    ASSERT_EQ(this->m_faultRecorder.records.size(), 6U);
    ASSERT_EQ(this->m_faultRecorder.records.at(5).kind, OBC::PersistentFaultRecordKind::INCIDENT_CLEARED);
    ASSERT_EQ(this->m_faultRecorder.records.at(5).timestampSec, 321U);
    ASSERT_EQ(this->m_faultRecorder.records.at(5).uptimeSec, 9U);
}

std::string RecoveryExecutorTester::makeTempRoot_() const {
    char buffer[] = "/tmp/recovery-executor-ut-XXXXXX";
    const char* const path = ::mkdtemp(buffer);
    EXPECT_NE(path, nullptr);
    return path == nullptr ? "/tmp/recovery-executor-ut-fallback" : std::string(path);
}

}  // namespace OBC

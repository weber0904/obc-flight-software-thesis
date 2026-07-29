#include "ModeManagerTester.hpp"

namespace OBC {

namespace {

const OBC::SatMode MODES[] = {OBC::SatMode::SAFE,
                              OBC::SatMode::IDLE,
                              OBC::SatMode::HELL,
                              OBC::SatMode::PAYLOAD,
                              OBC::SatMode::TTC};

bool isAcceptedByV1Matrix(OBC::SatMode from, OBC::SatMode to) {
    if (from == to) {
        return true;
    }
    switch (from.e) {
        case OBC::SatMode::SAFE:
            return to == OBC::SatMode::IDLE;
        case OBC::SatMode::IDLE:
            return to == OBC::SatMode::SAFE || to == OBC::SatMode::PAYLOAD || to == OBC::SatMode::TTC;
        case OBC::SatMode::PAYLOAD:
            return to == OBC::SatMode::SAFE || to == OBC::SatMode::IDLE;
        case OBC::SatMode::TTC:
            return to == OBC::SatMode::SAFE || to == OBC::SatMode::IDLE;
        case OBC::SatMode::HELL:
            return to == OBC::SatMode::SAFE;
        default:
            return false;
    }
}

OBC::ModeTransitionRejectionReason rejectionReasonForV1Matrix(OBC::SatMode to) {
    return to == OBC::SatMode::HELL ? OBC::ModeTransitionRejectionReason::INTERNAL_ONLY_TARGET
                                    : OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION;
}

}  // namespace

ModeManagerTester::ModeManagerTester()
    : ModeManagerGTestBase("ModeManagerTester", MAX_HISTORY_SIZE), m_guard(), component("ModeManager") {
    this->initComponents();
    this->connectPorts();
    this->component.configureOperatorTransitionGuard(&this->m_guard);
}

ModeManagerTester::~ModeManagerTester() = default;

void ModeManagerTester::from_modeGetTlmOut_handler(FwIndexType portNum,
                                                   FwChanIdType id,
                                                   Fw::Time& timeTag,
                                                   Fw::TlmBuffer& val) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_modeGetTlmOut(id, timeTag, val);
    this->dispatchTlm(id, timeTag, val);
}

OBC::OperatorModeTransitionDecision ModeManagerTester::FakeOperatorTransitionGuard::evaluateOperatorTransition(
    OBC::SatMode currentMode,
    OBC::SatMode requestedMode) const {
    FakeOperatorTransitionGuard* self = const_cast<FakeOperatorTransitionGuard*>(this);
    self->calls++;
    this->lastCurrentMode = currentMode;
    this->lastRequestedMode = requestedMode;
    OBC::OperatorModeTransitionDecision decision = {};
    decision.accepted = this->accepted;
    decision.reason = this->reason;
    return decision;
}

void ModeManagerTester::testModeSetPublishesTelemetry() {
    this->m_guard.accepted = true;
    this->clearHistory();
    this->sendCmd_MODE_SET(TEST_INSTANCE_ID, 0, OBC::SatMode::IDLE);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_MODE_SET, 0, Fw::CmdResponse::OK);
    ASSERT_EQ(this->m_guard.calls, 1U);
    ASSERT_EQ(this->m_guard.lastCurrentMode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_guard.lastRequestedMode, OBC::SatMode::IDLE);
    ASSERT_EVENTS_SYS_BOOT_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_CHANGE_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_CHANGE(0, OBC::SatMode::IDLE);
    ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED_SIZE(0);
    ASSERT_TLM_SYS_MODE_SIZE(2);
    ASSERT_TLM_SYS_MODE(0, OBC::SatMode::SAFE);
    ASSERT_TLM_SYS_MODE(1, OBC::SatMode::IDLE);
    ASSERT_TLM_SYS_UPTIME_SEC_SIZE(2);
    ASSERT_TLM_SYS_REBOOT_COUNT_SIZE(1);
    ASSERT_TLM_SYS_REBOOT_COUNT(0, 1);
}

void ModeManagerTester::testModeSetRejectedPreservesMode() {
    this->m_guard.accepted = false;
    this->m_guard.reason = OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION;

    this->clearHistory();
    this->sendCmd_MODE_SET(TEST_INSTANCE_ID, 7, OBC::SatMode::PAYLOAD);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_MODE_SET, 7, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_SYS_BOOT_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_CHANGE_SIZE(0);
    ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED(0,
                                               OBC::SatMode::SAFE,
                                               OBC::SatMode::PAYLOAD,
                                               static_cast<U32>(OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION));
    ASSERT_TLM_SYS_MODE_SIZE(1);
    ASSERT_TLM_SYS_MODE(0, OBC::SatMode::SAFE);
    ASSERT_EQ(this->component.getModeForRuntime(), OBC::SatMode::SAFE);
}

void ModeManagerTester::testSameModeNoOpDoesNotEmitModeChange() {
    this->m_guard.accepted = true;

    this->clearHistory();
    this->sendCmd_MODE_SET(TEST_INSTANCE_ID, 8, OBC::SatMode::SAFE);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_MODE_SET, 8, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SYS_BOOT_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_CHANGE_SIZE(0);
    ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED_SIZE(0);
    ASSERT_TLM_SYS_MODE_SIZE(1);
    ASSERT_TLM_SYS_MODE(0, OBC::SatMode::SAFE);
}

void ModeManagerTester::testGuardUnconfiguredReturnsExecutionError() {
    this->component.configureOperatorTransitionGuard(nullptr);

    this->clearHistory();
    this->sendCmd_MODE_SET(TEST_INSTANCE_ID, 9, OBC::SatMode::IDLE);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_MODE_SET, 9, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_SYS_BOOT_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_CHANGE_SIZE(0);
    ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED_SIZE(1);
    ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED(0,
                                               OBC::SatMode::SAFE,
                                               OBC::SatMode::IDLE,
                                               static_cast<U32>(OBC::ModeTransitionRejectionReason::GUARD_UNCONFIGURED));
    ASSERT_TLM_SYS_MODE_SIZE(1);
    ASSERT_TLM_SYS_MODE(0, OBC::SatMode::SAFE);
}

void ModeManagerTester::testAllOperatorPairsUseGuardedPath() {
    U32 sequence = 20U;
    for (const OBC::SatMode from : MODES) {
        for (const OBC::SatMode to : MODES) {
            this->component.applyModeForInternalSource(from, OBC::ModeApplySource::TestSetup);
            this->clearHistory();
            this->m_guard.calls = 0U;
            this->m_guard.accepted = isAcceptedByV1Matrix(from, to);
            this->m_guard.reason = rejectionReasonForV1Matrix(to);

            this->sendCmd_MODE_SET(TEST_INSTANCE_ID, sequence++, to);

            ASSERT_EQ(this->m_guard.calls, 1U);
            ASSERT_EQ(this->m_guard.lastCurrentMode, from);
            ASSERT_EQ(this->m_guard.lastRequestedMode, to);
            ASSERT_CMD_RESPONSE_SIZE(1);
            if (this->m_guard.accepted) {
                ASSERT_CMD_RESPONSE(0, this->component.OPCODE_MODE_SET, sequence - 1U, Fw::CmdResponse::OK);
                ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED_SIZE(0);
                ASSERT_EQ(this->component.getModeForRuntime(), to);
            } else {
                ASSERT_CMD_RESPONSE(0,
                                    this->component.OPCODE_MODE_SET,
                                    sequence - 1U,
                                    Fw::CmdResponse::VALIDATION_ERROR);
                ASSERT_EVENTS_SYS_MODE_CHANGE_SIZE(0);
                ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED_SIZE(1);
                ASSERT_EVENTS_SYS_MODE_TRANSITION_REJECTED(0, from, to, static_cast<U32>(this->m_guard.reason));
                ASSERT_EQ(this->component.getModeForRuntime(), from);
            }
        }
    }
}

void ModeManagerTester::testModeGetRepublishesStateWhenUnchanged() {
    this->clearHistory();
    this->component.setUptimeForTest(42);
    this->sendCmd_MODE_GET(TEST_INSTANCE_ID, 3);
    this->sendCmd_MODE_GET(TEST_INSTANCE_ID, 4);

    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_MODE_GET, 3, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(1, this->component.OPCODE_MODE_GET, 4, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SYS_BOOT_SIZE(1);
    ASSERT_TLM_SYS_MODE_SIZE(3);
    ASSERT_TLM_SYS_MODE(0, OBC::SatMode::SAFE);
    ASSERT_TLM_SYS_MODE(1, OBC::SatMode::SAFE);
    ASSERT_TLM_SYS_MODE(2, OBC::SatMode::SAFE);
    ASSERT_TLM_SYS_UPTIME_SEC_SIZE(3);
    ASSERT_TLM_SYS_UPTIME_SEC(0, 42);
    ASSERT_TLM_SYS_UPTIME_SEC(1, 42);
    ASSERT_TLM_SYS_UPTIME_SEC(2, 42);
    ASSERT_TLM_SYS_REBOOT_COUNT_SIZE(3);
    ASSERT_TLM_SYS_REBOOT_COUNT(0, 1);
    ASSERT_TLM_SYS_REBOOT_COUNT(1, 1);
    ASSERT_TLM_SYS_REBOOT_COUNT(2, 1);
}

}  // namespace OBC

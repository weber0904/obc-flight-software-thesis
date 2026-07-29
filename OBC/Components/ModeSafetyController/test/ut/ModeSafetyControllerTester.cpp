#include "ModeSafetyControllerTester.hpp"

namespace OBC {

OBC::SatMode ModeSafetyControllerTester::FakeModeControl::getModeForRuntime() const {
    return this->mode;
}

void ModeSafetyControllerTester::FakeModeControl::applyModeForInternalSource(OBC::SatMode requestedMode,
                                                                             OBC::ModeApplySource source) {
    if (this->mode != requestedMode) {
        this->mode = requestedMode;
        this->transitionCount++;
        this->lastRequestedMode = requestedMode;
        this->lastSource = source;
    }
}

bool ModeSafetyControllerTester::FakeEpsStatus::getCachedStatusForRuntime(OBC::EPS::StatusData& output) const {
    if (!this->available) {
        return false;
    }
    output = this->status;
    return true;
}

ModeSafetyControllerTester::ModeSafetyControllerTester()
    : ModeSafetyControllerGTestBase("ModeSafetyControllerTester", MAX_HISTORY_SIZE),
      m_modeControl(),
      m_epsStatus(),
      component("ModeSafetyController") {
    this->initComponents();
    this->connectPorts();
    this->component.configureRuntime(&this->m_modeControl, &this->m_epsStatus);
}

ModeSafetyControllerTester::~ModeSafetyControllerTester() = default;

void ModeSafetyControllerTester::testNoCachedEpsStatusDoesNotTransition() {
    this->configure(OBC::SatMode::SAFE, false, 0.0F);

    this->clearHistory();
    const OBC::ModeSafetyDecision decision = this->component.runCycle();

    ASSERT_FALSE(decision.hasValidEpsStatus);
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_EPS_UNAVAILABLE_SIZE(1);
    ASSERT_TLM_MODE_SAFETY_EPS_VALID_SIZE(1);
    ASSERT_TLM_MODE_SAFETY_EPS_VALID(0, false);

    this->clearHistory();
    static_cast<void>(this->component.runCycle());
    ASSERT_EVENTS_MODE_SAFETY_EPS_UNAVAILABLE_SIZE(0);
}

void ModeSafetyControllerTester::testUnconfiguredRuntimeDoesNotEvaluate() {
    this->configure(OBC::SatMode::PAYLOAD, true, 39.0F);

    this->component.configureRuntime(nullptr, &this->m_epsStatus);
    this->clearHistory();
    OBC::ModeSafetyDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::PAYLOAD);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);
    ASSERT_EVENTS_MODE_SAFETY_EPS_UNAVAILABLE_SIZE(0);
    ASSERT_TLM_MODE_SAFETY_EPS_VALID_SIZE(0);

    this->component.configureRuntime(&this->m_modeControl, nullptr);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::PAYLOAD);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);
    ASSERT_EVENTS_MODE_SAFETY_EPS_UNAVAILABLE_SIZE(0);
    ASSERT_TLM_MODE_SAFETY_EPS_VALID_SIZE(0);

    this->component.configureRuntime(&this->m_modeControl, &this->m_epsStatus);
}

void ModeSafetyControllerTester::testSafeToHellThresholdIsStrict() {
    this->configure(OBC::SatMode::SAFE, true, 10.0F);
    this->clearHistory();
    OBC::ModeSafetyDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);

    this->configure(OBC::SatMode::SAFE, true, 9.99F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(decision.action, OBC::ModeSafetyAction::SAFE_TO_HELL);
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::HELL);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::SafetyFallback);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(1);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION(0, OBC::SatMode::SAFE, OBC::SatMode::HELL, 9.99F);
    ASSERT_TLM_MODE_SAFETY_LAST_SOC_SIZE(1);
    ASSERT_TLM_MODE_SAFETY_LAST_SOC(0, 9.99F);
    ASSERT_TLM_MODE_SAFETY_LAST_TARGET_MODE(0, OBC::SatMode::HELL);
}

void ModeSafetyControllerTester::testHellToSafeThresholdIsStrict() {
    this->configure(OBC::SatMode::HELL, true, 15.0F);
    this->clearHistory();
    OBC::ModeSafetyDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);

    this->configure(OBC::SatMode::HELL, true, 15.01F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(decision.action, OBC::ModeSafetyAction::HELL_TO_SAFE);
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::SafetyRecovery);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(1);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION(0, OBC::SatMode::HELL, OBC::SatMode::SAFE, 15.01F);
}

void ModeSafetyControllerTester::testActiveModesToSafeThresholdIsStrict() {
    this->configure(OBC::SatMode::IDLE, true, 40.0F);
    this->clearHistory();
    OBC::ModeSafetyDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);

    this->configure(OBC::SatMode::IDLE, true, 39.99F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(decision.action, OBC::ModeSafetyAction::ACTIVE_TO_SAFE);
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::SafetyFallback);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(1);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION(0, OBC::SatMode::IDLE, OBC::SatMode::SAFE, 39.99F);

    this->configure(OBC::SatMode::PAYLOAD, true, 39.0F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);

    this->configure(OBC::SatMode::TTC, true, 39.0F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);
}

void ModeSafetyControllerTester::testPayloadToIdleThresholdAndPriority() {
    this->configure(OBC::SatMode::PAYLOAD, true, 60.0F);
    this->clearHistory();
    OBC::ModeSafetyDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::PAYLOAD);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);

    this->configure(OBC::SatMode::PAYLOAD, true, 59.99F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(decision.action, OBC::ModeSafetyAction::PAYLOAD_TO_IDLE);
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::IDLE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::SafetyPayloadExit);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(1);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION(0, OBC::SatMode::PAYLOAD, OBC::SatMode::IDLE, 59.99F);

    this->configure(OBC::SatMode::PAYLOAD, true, 39.99F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_TRUE(decision.shouldTransition());
    ASSERT_EQ(decision.action, OBC::ModeSafetyAction::ACTIVE_TO_SAFE);
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 1U);
    ASSERT_EQ(this->m_modeControl.lastSource, OBC::ModeApplySource::SafetyFallback);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(1);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION(0, OBC::SatMode::PAYLOAD, OBC::SatMode::SAFE, 39.99F);
}

void ModeSafetyControllerTester::testAlreadyTargetAndHighSocSafeAreNoOps() {
    this->configure(OBC::SatMode::HELL, true, 5.0F);
    this->clearHistory();
    OBC::ModeSafetyDecision decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::HELL);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);

    this->configure(OBC::SatMode::SAFE, true, 60.0F);
    this->clearHistory();
    decision = this->component.runCycle();
    ASSERT_FALSE(decision.shouldTransition());
    ASSERT_EQ(this->m_modeControl.mode, OBC::SatMode::SAFE);
    ASSERT_EQ(this->m_modeControl.transitionCount, 0U);
    ASSERT_EVENTS_MODE_SAFETY_TRANSITION_SIZE(0);
}

void ModeSafetyControllerTester::testOperatorGuardMatrix() {
    struct Case {
        OBC::SatMode from;
        OBC::SatMode to;
        bool accepted;
        OBC::ModeTransitionRejectionReason reason;
    };

    const Case cases[] = {
        {OBC::SatMode::SAFE, OBC::SatMode::SAFE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::SAFE, OBC::SatMode::PAYLOAD, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::SAFE, OBC::SatMode::HELL, false, OBC::ModeTransitionRejectionReason::INTERNAL_ONLY_TARGET},
        {OBC::SatMode::SAFE, OBC::SatMode::TTC, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::IDLE, OBC::SatMode::SAFE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::IDLE, OBC::SatMode::IDLE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::IDLE, OBC::SatMode::HELL, false, OBC::ModeTransitionRejectionReason::INTERNAL_ONLY_TARGET},
        {OBC::SatMode::IDLE, OBC::SatMode::TTC, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::HELL, OBC::SatMode::HELL, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::HELL, OBC::SatMode::IDLE, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::HELL, OBC::SatMode::PAYLOAD, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::HELL, OBC::SatMode::TTC, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::PAYLOAD, OBC::SatMode::SAFE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::PAYLOAD, OBC::SatMode::IDLE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::PAYLOAD, OBC::SatMode::HELL, false, OBC::ModeTransitionRejectionReason::INTERNAL_ONLY_TARGET},
        {OBC::SatMode::PAYLOAD, OBC::SatMode::PAYLOAD, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::PAYLOAD, OBC::SatMode::TTC, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::TTC, OBC::SatMode::SAFE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::TTC, OBC::SatMode::IDLE, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::TTC, OBC::SatMode::HELL, false, OBC::ModeTransitionRejectionReason::INTERNAL_ONLY_TARGET},
        {OBC::SatMode::TTC, OBC::SatMode::PAYLOAD, false, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
        {OBC::SatMode::TTC, OBC::SatMode::TTC, true, OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION},
    };

    this->m_epsStatus.available = true;
    this->m_epsStatus.status.soc = 16.0F;
    const OBC::OperatorModeTransitionDecision hellToSafe =
        this->component.evaluateOperatorTransition(OBC::SatMode::HELL, OBC::SatMode::SAFE);
    ASSERT_TRUE(hellToSafe.accepted);

    for (const Case& testCase : cases) {
        const OBC::OperatorModeTransitionDecision decision =
            this->component.evaluateOperatorTransition(testCase.from, testCase.to);
        ASSERT_EQ(decision.accepted, testCase.accepted);
        if (!testCase.accepted) {
            ASSERT_EQ(decision.reason, testCase.reason);
        }
    }
}

void ModeSafetyControllerTester::testOperatorHellToSafeGuardUsesCachedEps() {
    this->m_epsStatus.available = true;

    this->m_epsStatus.status.soc = 15.01F;
    OBC::OperatorModeTransitionDecision decision =
        this->component.evaluateOperatorTransition(OBC::SatMode::HELL, OBC::SatMode::SAFE);
    ASSERT_TRUE(decision.accepted);

    this->m_epsStatus.status.soc = 15.0F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::HELL, OBC::SatMode::SAFE);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET);

    this->m_epsStatus.status.soc = 14.99F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::HELL, OBC::SatMode::SAFE);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET);

    this->m_epsStatus.available = false;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::HELL, OBC::SatMode::SAFE);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_UNAVAILABLE);
}

void ModeSafetyControllerTester::testOperatorAdmissionGuardsUseCachedEps() {
    this->m_epsStatus.available = true;

    this->m_epsStatus.status.soc = 50.01F;
    OBC::OperatorModeTransitionDecision decision =
        this->component.evaluateOperatorTransition(OBC::SatMode::SAFE, OBC::SatMode::IDLE);
    ASSERT_TRUE(decision.accepted);

    this->m_epsStatus.status.soc = 50.0F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::SAFE, OBC::SatMode::IDLE);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET);

    this->m_epsStatus.status.soc = 49.99F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::SAFE, OBC::SatMode::IDLE);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET);

    this->m_epsStatus.available = false;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::SAFE, OBC::SatMode::IDLE);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_UNAVAILABLE);

    this->m_epsStatus.available = true;
    this->m_epsStatus.status.soc = 70.01F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::IDLE, OBC::SatMode::PAYLOAD);
    ASSERT_TRUE(decision.accepted);

    this->m_epsStatus.status.soc = 70.0F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::IDLE, OBC::SatMode::PAYLOAD);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET);

    this->m_epsStatus.status.soc = 69.99F;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::IDLE, OBC::SatMode::PAYLOAD);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET);

    this->m_epsStatus.available = false;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::IDLE, OBC::SatMode::PAYLOAD);
    ASSERT_FALSE(decision.accepted);
    ASSERT_EQ(decision.reason, OBC::ModeTransitionRejectionReason::SOC_GUARD_UNAVAILABLE);

    this->m_epsStatus.available = false;
    decision = this->component.evaluateOperatorTransition(OBC::SatMode::IDLE, OBC::SatMode::TTC);
    ASSERT_TRUE(decision.accepted);
}

void ModeSafetyControllerTester::configure(OBC::SatMode mode, bool epsAvailable, F32 soc) {
    this->m_modeControl.mode = mode;
    this->m_modeControl.transitionCount = 0U;
    this->m_modeControl.lastRequestedMode = mode;
    this->m_modeControl.lastSource = OBC::ModeApplySource::TestSetup;

    this->m_epsStatus.available = epsAvailable;
    this->m_epsStatus.status = {};
    this->m_epsStatus.status.soc = soc;
    this->m_epsStatus.status.vbat = 8.0F;
    this->m_epsStatus.status.temp_bat = 25.0F;
}

}  // namespace OBC

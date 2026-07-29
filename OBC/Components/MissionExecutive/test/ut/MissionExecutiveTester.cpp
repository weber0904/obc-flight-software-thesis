#include "MissionExecutiveTester.hpp"

#include <cmath>

namespace OBC {

namespace {

constexpr std::uint8_t ADCS_MODE_IDLE = 0U;
constexpr std::uint8_t ADCS_MODE_DETUMBLE = 1U;
constexpr std::uint8_t ADCS_MODE_POINTING = 2U;

class FakeModeControl final : public OBC::ILowPowerModeControl {
  public:
    bool isLowPowerModeActiveForRuntime() const override {
        return this->m_lowPowerActive;
    }

    void enterLowPowerModeForRuntime() override {
        this->m_lowPowerActive = true;
        this->m_entryCount++;
    }

    bool m_lowPowerActive = false;
    std::size_t m_entryCount = 0U;
};

class FakeEpsStatus final : public OBC::IEpsAutonomyStatus {
  public:
    bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override {
        if (!this->m_valid) {
            return false;
        }
        status = this->m_status;
        return true;
    }

    bool m_valid = true;
    OBC::EPS::StatusData m_status = {};
};

class FakeAdcsControl final : public OBC::IAdcsSunPointingControl {
  public:
    bool getCachedStateForRuntime(OBC::ADCS::StateData& state) const {
        if (!this->m_validState) {
            return false;
        }

        state = this->m_state;
        return true;
    }

    bool commandDetumbleForRuntime() {
        this->m_state.mode = ADCS_MODE_DETUMBLE;
        this->m_detumbleCommandCount++;
        return this->m_detumbleShouldSucceed;
    }

    bool commandSunSafePointingForRuntime(double q0, double q1, double q2, double q3) override {
        this->m_lastQ0 = q0;
        this->m_lastQ1 = q1;
        this->m_lastQ2 = q2;
        this->m_lastQ3 = q3;
        this->m_commandCount++;
        this->m_state.mode = ADCS_MODE_POINTING;
        return this->m_shouldSucceed;
    }

    bool m_validState = true;
    bool m_detumbleShouldSucceed = true;
    bool m_shouldSucceed = true;
    std::size_t m_detumbleCommandCount = 0U;
    std::size_t m_commandCount = 0U;
    double m_lastQ0 = 0.0;
    double m_lastQ1 = 0.0;
    double m_lastQ2 = 0.0;
    double m_lastQ3 = 0.0;
    OBC::ADCS::StateData m_state = {};
};

class FakeAdcsStatus final : public OBC::IAdcsAutonomyStatus {
  public:
    explicit FakeAdcsStatus(const FakeAdcsControl& control) : m_control(control) {}

    bool getCachedStateForRuntime(OBC::ADCS::StateData& state) const override {
        return this->m_control.getCachedStateForRuntime(state);
    }

  private:
    const FakeAdcsControl& m_control;
};

class FakeAdcsDetumbleControl final : public OBC::IAdcsDetumbleControl {
  public:
    explicit FakeAdcsDetumbleControl(FakeAdcsControl& control) : m_control(control) {}

    bool commandDetumbleForRuntime() override {
        return this->m_control.commandDetumbleForRuntime();
    }

  private:
    FakeAdcsControl& m_control;
};

float omegaNorm(float x, float y, float z) {
    return std::sqrt(x * x + y * y + z * z);
}

OBC::EPS::StatusData makeStatus(float soc) {
    OBC::EPS::StatusData status = {};
    status.soc = soc;
    status.sunlight = 0U;
    return status;
}

OBC::ADCS::StateData makeAdcsState(float omegaX, float omegaY, float omegaZ, std::uint8_t mode = ADCS_MODE_IDLE) {
    OBC::ADCS::StateData state = {};
    state.q0 = 1.0;
    state.sensor_valid = 1U;
    state.mode = mode;
    state.omega_x = omegaX;
    state.omega_y = omegaY;
    state.omega_z = omegaZ;
    return state;
}

}  // namespace

MissionExecutiveTester::MissionExecutiveTester()
    : MissionExecutiveGTestBase("MissionExecutiveTester", MAX_HISTORY_SIZE), component("MissionExecutive") {
    this->initComponents();
    this->connectPorts();
}

MissionExecutiveTester::~MissionExecutiveTester() = default;

void MissionExecutiveTester::testHighAngularRateTriggersDetumble() {
    FakeModeControl modeControl;
    FakeEpsStatus epsStatus;
    FakeAdcsControl adcsControl;
    FakeAdcsStatus adcsStatus(adcsControl);
    FakeAdcsDetumbleControl detumbleControl(adcsControl);
    epsStatus.m_status = makeStatus(72.0F);
    adcsControl.m_state = makeAdcsState(0.12F, -0.08F, 0.06F);
    this->component.configureRuntimeBindings(&modeControl, &epsStatus, &adcsStatus, &detumbleControl, &adcsControl);

    this->clearHistory();
    const MissionExecutiveStepResult result = this->component.runCycleForTest();

    ASSERT_TRUE(result.hadValidAdcsStatus);
    ASSERT_TRUE(result.highAngularRateCondition);
    ASSERT_TRUE(result.detumbleActive);
    ASSERT_TRUE(result.issuedDetumbleThisCycle);
    ASSERT_FALSE(result.sunSafePointingActive);
    ASSERT_EQ(adcsControl.m_detumbleCommandCount, 1U);
    ASSERT_EQ(adcsControl.m_commandCount, 0U);
    ASSERT_EVENTS_MISSION_DETUMBLE_COMMAND_SIZE(1);
    ASSERT_TLM_MISSION_LAST_ANGULAR_RATE_NORM_SIZE(1);
    ASSERT_TLM_MISSION_LAST_ANGULAR_RATE_NORM(0, omegaNorm(0.12F, -0.08F, 0.06F));
    ASSERT_TLM_MISSION_DETUMBLE_ACTIVE_SIZE(1);
    ASSERT_TLM_MISSION_DETUMBLE_ACTIVE(0, true);
}

void MissionExecutiveTester::testHighRateSuppressesSunSafePointingAtLowBattery() {
    FakeModeControl modeControl;
    FakeEpsStatus epsStatus;
    FakeAdcsControl adcsControl;
    FakeAdcsStatus adcsStatus(adcsControl);
    FakeAdcsDetumbleControl detumbleControl(adcsControl);
    epsStatus.m_status = makeStatus(15.0F);
    adcsControl.m_state = makeAdcsState(0.20F, 0.0F, 0.0F);
    this->component.configureRuntimeBindings(&modeControl, &epsStatus, &adcsStatus, &detumbleControl, &adcsControl);

    this->clearHistory();
    const MissionExecutiveStepResult result = this->component.runCycleForTest();

    ASSERT_TRUE(result.lowBatteryCondition);
    ASSERT_TRUE(result.lowPowerActive);
    ASSERT_TRUE(result.highAngularRateCondition);
    ASSERT_TRUE(result.detumbleActive);
    ASSERT_FALSE(result.sunSafePointingActive);
    ASSERT_EQ(modeControl.m_entryCount, 1U);
    ASSERT_EQ(adcsControl.m_detumbleCommandCount, 1U);
    ASSERT_EQ(adcsControl.m_commandCount, 0U);
    ASSERT_EVENTS_MISSION_LOW_POWER_ENTER_SIZE(1);
    ASSERT_EVENTS_MISSION_SUN_SAFE_POINTING_COMMAND_SIZE(0);
    ASSERT_EVENTS_MISSION_DETUMBLE_COMMAND_SIZE(1);
}

void MissionExecutiveTester::testLowBatteryTriggersLowPowerAndSunPointing() {
    FakeModeControl modeControl;
    FakeEpsStatus epsStatus;
    FakeAdcsControl adcsControl;
    FakeAdcsStatus adcsStatus(adcsControl);
    FakeAdcsDetumbleControl detumbleControl(adcsControl);
    epsStatus.m_status = makeStatus(15.0F);
    adcsControl.m_state = makeAdcsState(0.01F, 0.0F, 0.0F);
    this->component.configureRuntimeBindings(&modeControl, &epsStatus, &adcsStatus, &detumbleControl, &adcsControl);

    this->clearHistory();
    const MissionExecutiveStepResult result = this->component.runCycleForTest();

    ASSERT_TRUE(result.lowBatteryCondition);
    ASSERT_TRUE(result.lowPowerActive);
    ASSERT_TRUE(result.sunSafePointingActive);
    ASSERT_TRUE(modeControl.m_lowPowerActive);
    ASSERT_EQ(modeControl.m_entryCount, 1U);
    ASSERT_EQ(adcsControl.m_detumbleCommandCount, 0U);
    ASSERT_EQ(adcsControl.m_commandCount, 1U);
    ASSERT_EVENTS_MISSION_LOW_POWER_ENTER_SIZE(1);
    ASSERT_EVENTS_MISSION_SUN_SAFE_POINTING_COMMAND_SIZE(1);
    ASSERT_EVENTS_MISSION_DETUMBLE_COMMAND_SIZE(0);
    ASSERT_EVENTS_MISSION_ADCS_COMMAND_ERROR_SIZE(0);
    ASSERT_TLM_MISSION_LAST_SOC_SIZE(1);
    ASSERT_TLM_MISSION_LAST_SOC(0, 15.0F);
    ASSERT_TLM_MISSION_LOW_POWER_ACTIVE_SIZE(1);
    ASSERT_TLM_MISSION_LOW_POWER_ACTIVE(0, true);
    ASSERT_TLM_MISSION_SUN_SAFE_POINTING_ACTIVE_SIZE(1);
    ASSERT_TLM_MISSION_SUN_SAFE_POINTING_ACTIVE(0, true);
}

void MissionExecutiveTester::testPolicyLatchesAfterEntry() {
    FakeModeControl modeControl;
    FakeEpsStatus epsStatus;
    FakeAdcsControl adcsControl;
    FakeAdcsStatus adcsStatus(adcsControl);
    FakeAdcsDetumbleControl detumbleControl(adcsControl);
    epsStatus.m_status = makeStatus(12.0F);
    adcsControl.m_state = makeAdcsState(0.01F, 0.0F, 0.0F);
    this->component.configureRuntimeBindings(&modeControl, &epsStatus, &adcsStatus, &detumbleControl, &adcsControl);

    static_cast<void>(this->component.runCycleForTest());
    this->clearHistory();

    const MissionExecutiveStepResult result = this->component.runCycleForTest();

    ASSERT_TRUE(result.lowPowerActive);
    ASSERT_TRUE(result.sunSafePointingActive);
    ASSERT_EQ(modeControl.m_entryCount, 1U);
    ASSERT_EQ(adcsControl.m_detumbleCommandCount, 0U);
    ASSERT_EQ(adcsControl.m_commandCount, 1U);
    ASSERT_EVENTS_MISSION_LOW_POWER_ENTER_SIZE(0);
    ASSERT_EVENTS_MISSION_SUN_SAFE_POINTING_COMMAND_SIZE(0);
}

void MissionExecutiveTester::testNominalBatteryDoesNothing() {
    FakeModeControl modeControl;
    FakeEpsStatus epsStatus;
    FakeAdcsControl adcsControl;
    FakeAdcsStatus adcsStatus(adcsControl);
    FakeAdcsDetumbleControl detumbleControl(adcsControl);
    epsStatus.m_status = makeStatus(72.0F);
    adcsControl.m_state = makeAdcsState(0.01F, 0.0F, 0.0F);
    this->component.configureRuntimeBindings(&modeControl, &epsStatus, &adcsStatus, &detumbleControl, &adcsControl);

    this->clearHistory();
    const MissionExecutiveStepResult result = this->component.runCycleForTest();

    ASSERT_FALSE(result.lowBatteryCondition);
    ASSERT_FALSE(result.highAngularRateCondition);
    ASSERT_FALSE(result.lowPowerActive);
    ASSERT_FALSE(result.detumbleActive);
    ASSERT_FALSE(result.sunSafePointingActive);
    ASSERT_FALSE(modeControl.m_lowPowerActive);
    ASSERT_EQ(adcsControl.m_detumbleCommandCount, 0U);
    ASSERT_EQ(adcsControl.m_commandCount, 0U);
    ASSERT_EVENTS_MISSION_LOW_POWER_ENTER_SIZE(0);
    ASSERT_EVENTS_MISSION_SUN_SAFE_POINTING_COMMAND_SIZE(0);
    ASSERT_EVENTS_MISSION_DETUMBLE_COMMAND_SIZE(0);
    ASSERT_TLM_MISSION_LAST_SOC_SIZE(1);
    ASSERT_TLM_MISSION_LAST_SOC(0, 72.0F);
    ASSERT_TLM_MISSION_LOW_POWER_ACTIVE_SIZE(1);
    ASSERT_TLM_MISSION_LOW_POWER_ACTIVE(0, false);
}

}  // namespace OBC

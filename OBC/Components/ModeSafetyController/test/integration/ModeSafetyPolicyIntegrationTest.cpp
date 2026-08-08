#include "OBC/Components/ModeManager/ModeManager.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyController.hpp"

#include <iostream>
#include <string>

namespace {

class FakeEpsStatus final : public OBC::IModeSafetyEpsStatus {
  public:
    bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override {
        if (!this->available) {
            return false;
        }
        status = this->status;
        return true;
    }

    bool available = true;
    OBC::EPS::StatusData status = {};
};

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

OBC::EPS::StatusData makeStatus(F32 soc) {
    OBC::EPS::StatusData status = {};
    status.soc = soc;
    status.vbat = 8.0F;
    status.temp_bat = 25.0F;
    return status;
}

}  // namespace

int main() {
    OBC::ModeManager modeManager("ModeManagerIntegration");
    OBC::ModeSafetyController controller("ModeSafetyControllerIntegration");
    FakeEpsStatus epsStatus;

    modeManager.init(0);
    controller.init(0);
    controller.configureRuntime(&modeManager, &epsStatus);

    bool ok = true;

    modeManager.applyModeForInternalSource(OBC::SatMode::SAFE, OBC::ModeApplySource::TestSetup);
    epsStatus.status = makeStatus(9.0F);
    OBC::ModeSafetyDecision decision = controller.runCycle();
    ok = check(decision.shouldTransition(), "SAFE low SoC should request transition") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::HELL,
               "SAFE low SoC should drive ModeManager to HELL") &&
         ok;

    epsStatus.status = makeStatus(16.0F);
    decision = controller.runCycle();
    ok = check(decision.shouldTransition(), "HELL recovery SoC should request transition") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::SAFE,
               "HELL recovery SoC should drive ModeManager to SAFE") &&
         ok;

    modeManager.applyModeForInternalSource(OBC::SatMode::PAYLOAD, OBC::ModeApplySource::TestSetup);
    epsStatus.status = makeStatus(39.0F);
    decision = controller.runCycle();
    ok = check(decision.shouldTransition(), "PAYLOAD low SoC should request transition") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::SAFE,
               "PAYLOAD low SoC should drive ModeManager to SAFE") &&
         ok;

    modeManager.applyModeForInternalSource(OBC::SatMode::PAYLOAD, OBC::ModeApplySource::TestSetup);
    epsStatus.status = makeStatus(59.0F);
    decision = controller.runCycle();
    ok = check(decision.shouldTransition(), "PAYLOAD mid-low SoC should request transition") && ok;
    ok = check(decision.action == OBC::ModeSafetyAction::PAYLOAD_TO_IDLE,
               "PAYLOAD mid-low SoC should request PAYLOAD_TO_IDLE") &&
         ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::IDLE,
               "PAYLOAD mid-low SoC should drive ModeManager to IDLE") &&
         ok;

    modeManager.applyModeForInternalSource(OBC::SatMode::SAFE, OBC::ModeApplySource::TestSetup);
    epsStatus.status = makeStatus(60.0F);
    decision = controller.runCycle();
    ok = check(!decision.shouldTransition(), "SAFE high SoC should remain manual") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::SAFE,
               "SAFE high SoC should remain SAFE") &&
         ok;

    return ok ? 0 : 1;
}

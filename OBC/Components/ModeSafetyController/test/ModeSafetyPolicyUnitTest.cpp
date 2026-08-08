#include "OBC/Components/ModeSafetyController/ModeSafetyPolicy.hpp"

#include <iostream>
#include <string>

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

bool expect(OBC::SatMode currentMode,
            bool hasEps,
            F32 soc,
            bool transition,
            OBC::SatMode targetMode,
            OBC::ModeSafetyAction action,
            const std::string& message) {
    const OBC::ModeSafetyDecision decision = OBC::ModeSafetyPolicy::evaluate(currentMode, hasEps, soc);
    bool ok = true;
    ok = check(decision.hasValidEpsStatus == hasEps, message + ": EPS validity mismatch") && ok;
    ok = check(decision.currentMode == currentMode, message + ": current mode mismatch") && ok;
    ok = check(decision.targetMode == targetMode, message + ": target mode mismatch") && ok;
    ok = check(decision.action == action, message + ": action mismatch") && ok;
    ok = check(decision.shouldTransition() == transition, message + ": transition mismatch") && ok;
    return ok;
}

}  // namespace

int main() {
    bool ok = true;

    ok = expect(OBC::SatMode::SAFE, false, 0.0F, false, OBC::SatMode::SAFE, OBC::ModeSafetyAction::NONE,
                "missing EPS") &&
         ok;
    ok = expect(OBC::SatMode::SAFE, true, 9.99F, true, OBC::SatMode::HELL,
                OBC::ModeSafetyAction::SAFE_TO_HELL, "SAFE 9.99") &&
         ok;
    ok = expect(OBC::SatMode::SAFE, true, 10.0F, false, OBC::SatMode::SAFE, OBC::ModeSafetyAction::NONE,
                "SAFE 10 boundary") &&
         ok;
    ok = expect(OBC::SatMode::HELL, true, 15.0F, false, OBC::SatMode::HELL, OBC::ModeSafetyAction::NONE,
                "HELL 15 boundary") &&
         ok;
    ok = expect(OBC::SatMode::HELL, true, 15.01F, true, OBC::SatMode::SAFE,
                OBC::ModeSafetyAction::HELL_TO_SAFE, "HELL 15.01") &&
         ok;
    ok = expect(OBC::SatMode::IDLE, true, 39.99F, true, OBC::SatMode::SAFE,
                OBC::ModeSafetyAction::ACTIVE_TO_SAFE, "IDLE 39.99") &&
         ok;
    ok = expect(OBC::SatMode::IDLE, true, 40.0F, false, OBC::SatMode::IDLE, OBC::ModeSafetyAction::NONE,
                "IDLE 40 boundary") &&
         ok;
    ok = expect(OBC::SatMode::PAYLOAD, true, 39.99F, true, OBC::SatMode::SAFE,
                OBC::ModeSafetyAction::ACTIVE_TO_SAFE, "PAYLOAD fallback") &&
         ok;
    ok = expect(OBC::SatMode::PAYLOAD, true, 40.0F, true, OBC::SatMode::IDLE,
                OBC::ModeSafetyAction::PAYLOAD_TO_IDLE, "PAYLOAD exit at 40") &&
         ok;
    ok = expect(OBC::SatMode::PAYLOAD, true, 59.99F, true, OBC::SatMode::IDLE,
                OBC::ModeSafetyAction::PAYLOAD_TO_IDLE, "PAYLOAD 59.99 exits to IDLE") &&
         ok;
    ok = expect(OBC::SatMode::PAYLOAD, true, 60.0F, false, OBC::SatMode::PAYLOAD, OBC::ModeSafetyAction::NONE,
                "PAYLOAD 60 boundary") &&
         ok;
    ok = expect(OBC::SatMode::TTC, true, 39.99F, true, OBC::SatMode::SAFE,
                OBC::ModeSafetyAction::ACTIVE_TO_SAFE, "TTC fallback") &&
         ok;
    ok = expect(OBC::SatMode::SAFE, true, 60.0F, false, OBC::SatMode::SAFE, OBC::ModeSafetyAction::NONE,
                "SAFE high SoC no auto IDLE") &&
         ok;

    return ok ? 0 : 1;
}

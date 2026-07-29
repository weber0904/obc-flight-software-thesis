#include <cmath>
#include <iostream>

#include "simulators/eps/EpsSimModel.hpp"

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

bool approxEqual(float lhs, float rhs, float tolerance) {
    return std::fabs(lhs - rhs) <= tolerance;
}

OBC::EPS::StatusData requestStatus(OBC::EPS::EpsSimModel& model) {
    OBC::EPS::CSP::Request request =
        OBC::EPS::CSP::makeBlankRequest(OBC::EPS::CSP::ServicePort::STATUS, 1U);
    OBC::EPS::CSP::Reply reply = {};
    const OBC::EPS::ResultCode result = model.processCspRequest(request, reply);
    if (result != OBC::EPS::ResultCode::OK) {
        std::cerr << "FAIL: status request failed\n";
    }
    return reply.status;
}

void setPdu(OBC::EPS::EpsSimModel& model, std::uint8_t channel, bool enabled) {
    OBC::EPS::CSP::Request request =
        OBC::EPS::CSP::makeBlankRequest(OBC::EPS::CSP::ServicePort::PDU, 2U);
    request.payload.pdu.channel = channel;
    request.payload.pdu.enabled = enabled ? 1U : 0U;
    OBC::EPS::CSP::Reply reply = {};
    const OBC::EPS::ResultCode result = model.processCspRequest(request, reply);
    if (result != OBC::EPS::ResultCode::OK) {
        std::cerr << "FAIL: PDU request failed\n";
    }
}

}  // namespace

int main() {
    bool ok = true;

    OBC::EPS::EpsSimModel baseModel;
    const OBC::EPS::StatusData base = requestStatus(baseModel);
    ok = check(base.pdu_status == 0x03U, "default EPS PDU status should stay 0x03") && ok;

    OBC::EPS::EpsSimModel mappedModel;
    const OBC::EPS::StatusData mappedBefore = requestStatus(mappedModel);
    setPdu(mappedModel, 5U, true);
    const OBC::EPS::StatusData mappedAfter = requestStatus(mappedModel);
    ok = check(mappedAfter.ibat < mappedBefore.ibat, "mapped channel should make ibat more negative") && ok;

    OBC::EPS::EpsSimModel spareModel;
    const OBC::EPS::StatusData spareBefore = requestStatus(spareModel);
    setPdu(spareModel, 2U, true);
    const OBC::EPS::StatusData spareAfter = requestStatus(spareModel);
    ok = check((spareAfter.pdu_status & 0x04U) != 0U, "spare channel bit should still update") && ok;
    ok = check(approxEqual(spareAfter.ibat, spareBefore.ibat, 1e-6F), "spare channel should not change ibat") && ok;

    OBC::EPS::EpsSimModel noiseModelA;
    OBC::EPS::EpsSimModel noiseModelB;
    noiseModelA.advanceForTest(std::chrono::milliseconds(1100));
    noiseModelB.advanceForTest(std::chrono::milliseconds(1100));
    const OBC::EPS::StatusData noiseA1 = noiseModelA.snapshotStateForRuntime();
    const OBC::EPS::StatusData noiseB1 = noiseModelB.snapshotStateForRuntime();
    ok = check(approxEqual(noiseA1.vbat, noiseB1.vbat, 1e-6F), "same seeded progression should reproduce vbat") && ok;
    ok = check(approxEqual(noiseA1.temp_bat, noiseB1.temp_bat, 1e-6F), "same seeded progression should reproduce temperature") &&
         ok;

    noiseModelA.advanceForTest(std::chrono::milliseconds(1000));
    const OBC::EPS::StatusData noiseA2 = noiseModelA.snapshotStateForRuntime();
    ok = check(!approxEqual(noiseA1.vbat, noiseA2.vbat, 1e-6F), "normal mode should not stay perfectly flat") && ok;
    ok = check(std::fabs(noiseA2.vbat - noiseA1.vbat) <= 0.05F, "normal vbat jitter should stay bounded") && ok;
    ok = check(std::fabs(noiseA2.temp_bat - noiseA1.temp_bat) <= 0.60F, "normal temperature jitter should stay bounded") &&
         ok;

    OBC::EPS::EpsSimModel normalSocModel;
    OBC::EPS::EpsSimModel highDrawSocModel;
    normalSocModel.advanceForTest(std::chrono::milliseconds(60000));
    const OBC::EPS::StatusData normalAfter = normalSocModel.snapshotStateForRuntime();
    highDrawSocModel.setLoadModeForRuntime(OBC::EPS::LoadMode::HIGH_DRAW);
    highDrawSocModel.advanceForTest(std::chrono::milliseconds(60000));
    const OBC::EPS::StatusData highDrawAfter = highDrawSocModel.snapshotStateForRuntime();
    ok = check(highDrawAfter.ibat < normalAfter.ibat, "high-draw should make ibat more negative than normal") && ok;
    ok = check(highDrawAfter.vbat < normalAfter.vbat, "high-draw should lower vbat relative to normal") && ok;
    ok = check(highDrawAfter.soc < normalAfter.soc, "high-draw should discharge SoC faster than normal") && ok;

    OBC::EPS::EpsSimModel rampModel;
    rampModel.setSocForRuntime(80.0F, 0.20F);
    rampModel.setLoadModeForRuntime(OBC::EPS::LoadMode::HIGH_DRAW);
    rampModel.advanceForTest(std::chrono::milliseconds(100));
    const OBC::EPS::StatusData rampMid = rampModel.snapshotStateForRuntime();
    ok = check(rampMid.soc > 76.0F && rampMid.soc < 80.0F, "timed ramp should report an intermediate SoC under load") &&
         ok;
    rampModel.advanceForTest(std::chrono::milliseconds(150));
    const OBC::EPS::StatusData rampEnd = rampModel.snapshotStateForRuntime();
    ok = check(rampEnd.soc <= 80.0F && rampEnd.soc > 70.0F, "post-ramp SoC should stay clamped and continue evolving") &&
         ok;

    return ok ? 0 : 1;
}

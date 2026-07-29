#include "OBC/Components/WatchdogSupervisor/WatchdogPolicy.hpp"

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

OBC::WatchdogSourceSnapshot makeSource(OBC::WatchdogSource source,
                                       OBC::WatchdogState state,
                                       U32 ageTicks,
                                       const OBC::WatchdogSourceConfig& config) {
    OBC::WatchdogSourceSnapshot snapshot = {};
    snapshot.source = source;
    snapshot.state = state;
    snapshot.ageTicks = ageTicks;
    snapshot.config = config;
    return snapshot;
}

}  // namespace

int main() {
    bool ok = true;

    OBC::WatchdogSourceConfig config = {};
    ok = check(OBC::WatchdogPolicy::isValidConfig(config), "default config should be valid") && ok;
    config.warningTicks = 0U;
    ok = check(!OBC::WatchdogPolicy::isValidConfig(config), "warning tick 0 should be invalid") && ok;

    config = OBC::WatchdogSourceConfig();
    OBC::WatchdogSourceSnapshot current =
        makeSource(OBC::WatchdogSource::EPS_BRIDGE, OBC::WatchdogState::HEALTHY, 1U, config);
    OBC::WatchdogSourceEvaluation evaluation =
        OBC::WatchdogPolicy::evaluateSource(config, current, false, OBC::SatMode::PAYLOAD);
    ok = check(evaluation.crossedWarning, "age crossing warning should emit warning") && ok;
    ok = check(!evaluation.crossedFault, "warning-only crossing should not latch fault") && ok;

    current.state = OBC::WatchdogState::WARNING;
    current.ageTicks = 2U;
    evaluation = OBC::WatchdogPolicy::evaluateSource(config, current, false, OBC::SatMode::IDLE);
    ok = check(evaluation.crossedFault, "age crossing safe should latch fault") && ok;
    ok = check(evaluation.shouldRequestSafe, "IDLE fault should request SAFE") && ok;

    current.state = OBC::WatchdogState::FEED_SUPPRESSED;
    current.ageTicks = 6U;
    evaluation = OBC::WatchdogPolicy::evaluateSource(config, current, true, OBC::SatMode::SAFE);
    ok = check(evaluation.recovered, "first beat after suppression should recover") && ok;
    ok = check(evaluation.nextState == OBC::WatchdogState::HEALTHY, "recovered source should return healthy") && ok;

    std::array<OBC::WatchdogSourceSnapshot, OBC::WatchdogSource::NUM_CONSTANTS> sources = {
        makeSource(OBC::WatchdogSource::EPS_BRIDGE, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::EPS_FDIR, OBC::WatchdogState::LATCHED_FAULT, 3U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::MODE_SAFETY, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::COMM_CONTROLLER, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::ADCS_FDIR, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
    };
    OBC::WatchdogAggregateRollup rollup = OBC::WatchdogPolicy::rollup(sources, false);
    ok = check(rollup.aggregateState == OBC::WatchdogState::LATCHED_FAULT,
               "latched fault source should drive aggregate fault") && ok;
    ok = check(rollup.recoveryLevel == OBC::WatchdogRecoveryLevel::LATCHED_FAULT,
               "fault without SAFE request should report latched-fault recovery level") && ok;
    ok = check(rollup.feedEligible, "latched fault should keep feed eligible before suppression") && ok;

    rollup = OBC::WatchdogPolicy::rollup(sources, true);
    ok = check(rollup.recoveryLevel == OBC::WatchdogRecoveryLevel::SAFE_REQUESTED,
               "fault after SAFE request should report SAFE_REQUESTED recovery level") && ok;

    sources = {
        makeSource(OBC::WatchdogSource::EPS_BRIDGE, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::EPS_FDIR, OBC::WatchdogState::LATCHED_FAULT, 3U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::MODE_SAFETY, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::COMM_CONTROLLER, OBC::WatchdogState::FEED_SUPPRESSED, 5U, OBC::WatchdogSourceConfig()),
        makeSource(OBC::WatchdogSource::ADCS_FDIR, OBC::WatchdogState::HEALTHY, 0U, OBC::WatchdogSourceConfig()),
    };
    rollup = OBC::WatchdogPolicy::rollup(sources, true);
    ok = check(rollup.aggregateState == OBC::WatchdogState::FEED_SUPPRESSED,
               "suppressed source should drive aggregate suppressed") && ok;
    ok = check(!rollup.feedEligible, "suppressed source should disable feed eligibility") && ok;
    ok = check((rollup.faultMask & OBC::watchdogSourceMask(OBC::WatchdogSource::EPS_FDIR)) != 0U,
               "fault mask should include latched fault source") && ok;

    return ok ? 0 : 1;
}

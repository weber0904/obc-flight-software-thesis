#include "OBC/Components/WatchdogSupervisor/WatchdogPolicy.hpp"

#include <limits>

namespace OBC {

namespace {

U32 saturatingIncrement(U32 value) {
    return value == std::numeric_limits<U32>::max() ? value : value + 1U;
}

U32 stateSeverity(const OBC::WatchdogState state) {
    switch (state.e) {
        case OBC::WatchdogState::DISABLED:
            return 0U;
        case OBC::WatchdogState::HEALTHY:
            return 1U;
        case OBC::WatchdogState::WARNING:
            return 2U;
        case OBC::WatchdogState::LATCHED_FAULT:
            return 3U;
        case OBC::WatchdogState::FEED_SUPPRESSED:
            return 4U;
        default:
            return 0U;
    }
}

}  // namespace

OBC::WatchdogSourceConfig WatchdogPolicy::defaultConfig(const OBC::WatchdogSource source) {
    static_cast<void>(source);
    return OBC::WatchdogSourceConfig();
}

bool WatchdogPolicy::isValidConfig(const OBC::WatchdogSourceConfig& config) {
    return config.warningTicks > 0U && config.warningTicks <= config.safeTicks &&
           config.safeTicks <= config.suppressTicks;
}

bool WatchdogPolicy::shouldRequestSafeForMode(const OBC::SatMode mode) {
    return mode == OBC::SatMode::IDLE || mode == OBC::SatMode::PAYLOAD || mode == OBC::SatMode::TTC;
}

OBC::WatchdogState WatchdogPolicy::stateForAge(const OBC::WatchdogSourceConfig& config, U32 ageTicks) {
    if (!config.enabled) {
        return OBC::WatchdogState::DISABLED;
    }
    if (ageTicks >= config.suppressTicks) {
        return OBC::WatchdogState::FEED_SUPPRESSED;
    }
    if (ageTicks >= config.safeTicks) {
        return OBC::WatchdogState::LATCHED_FAULT;
    }
    if (ageTicks >= config.warningTicks) {
        return OBC::WatchdogState::WARNING;
    }
    return OBC::WatchdogState::HEALTHY;
}

OBC::WatchdogSourceEvaluation WatchdogPolicy::evaluateSource(const OBC::WatchdogSourceConfig& config,
                                                             const OBC::WatchdogSourceSnapshot& current,
                                                             bool beatObserved,
                                                             OBC::SatMode currentMode) {
    OBC::WatchdogSourceEvaluation evaluation = {};

    if (!config.enabled) {
        evaluation.nextAgeTicks = 0U;
        evaluation.nextState = OBC::WatchdogState::DISABLED;
        evaluation.recovered = stateSeverity(current.state) > stateSeverity(OBC::WatchdogState::HEALTHY);
        return evaluation;
    }

    evaluation.nextAgeTicks = beatObserved ? 0U : saturatingIncrement(current.ageTicks);
    evaluation.nextState = stateForAge(config, evaluation.nextAgeTicks);
    evaluation.recovered = beatObserved && stateSeverity(current.state) > stateSeverity(OBC::WatchdogState::HEALTHY);
    evaluation.crossedWarning = !beatObserved && stateSeverity(current.state) < stateSeverity(OBC::WatchdogState::WARNING) &&
                                stateSeverity(evaluation.nextState) >= stateSeverity(OBC::WatchdogState::WARNING);
    evaluation.crossedFault = !beatObserved &&
                              stateSeverity(current.state) < stateSeverity(OBC::WatchdogState::LATCHED_FAULT) &&
                              stateSeverity(evaluation.nextState) >= stateSeverity(OBC::WatchdogState::LATCHED_FAULT);
    evaluation.crossedSuppression =
        !beatObserved && current.state != OBC::WatchdogState::FEED_SUPPRESSED &&
        evaluation.nextState == OBC::WatchdogState::FEED_SUPPRESSED;
    evaluation.shouldRequestSafe = evaluation.crossedFault && shouldRequestSafeForMode(currentMode);
    return evaluation;
}

OBC::WatchdogAggregateRollup WatchdogPolicy::rollup(
    const std::array<OBC::WatchdogSourceSnapshot, OBC::WatchdogSource::NUM_CONSTANTS>& sources,
    bool safeRequestIssuedForCurrentFault) {
    OBC::WatchdogAggregateRollup rollup = {};
    bool anyEnabled = false;

    for (const auto& source : sources) {
        if (source.state == OBC::WatchdogState::DISABLED || !source.config.enabled) {
            continue;
        }

        anyEnabled = true;
        if (source.state == OBC::WatchdogState::WARNING) {
            rollup.warningMask |= watchdogSourceMask(source.source);
        }
        if (source.state == OBC::WatchdogState::LATCHED_FAULT ||
            source.state == OBC::WatchdogState::FEED_SUPPRESSED) {
            rollup.faultMask |= watchdogSourceMask(source.source);
        }
        if (source.state == OBC::WatchdogState::FEED_SUPPRESSED) {
            rollup.suppressMask |= watchdogSourceMask(source.source);
        }
    }

    if (!anyEnabled) {
        rollup.aggregateState = OBC::WatchdogState::DISABLED;
        rollup.feedEligible = false;
        return rollup;
    }

    if (rollup.suppressMask != 0U) {
        rollup.aggregateState = OBC::WatchdogState::FEED_SUPPRESSED;
        rollup.recoveryLevel = OBC::WatchdogRecoveryLevel::FEED_SUPPRESSED;
        rollup.feedEligible = false;
        return rollup;
    }

    if (rollup.faultMask != 0U) {
        rollup.aggregateState = OBC::WatchdogState::LATCHED_FAULT;
        rollup.recoveryLevel = safeRequestIssuedForCurrentFault ? OBC::WatchdogRecoveryLevel::SAFE_REQUESTED
                                                                : OBC::WatchdogRecoveryLevel::LATCHED_FAULT;
        rollup.feedEligible = true;
        return rollup;
    }

    if (rollup.warningMask != 0U) {
        rollup.aggregateState = OBC::WatchdogState::WARNING;
        rollup.recoveryLevel = OBC::WatchdogRecoveryLevel::WARNING;
        rollup.feedEligible = true;
        return rollup;
    }

    rollup.aggregateState = OBC::WatchdogState::HEALTHY;
    rollup.recoveryLevel = OBC::WatchdogRecoveryLevel::NONE;
    rollup.feedEligible = true;
    return rollup;
}

}  // namespace OBC

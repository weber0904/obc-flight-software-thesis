#include "OBC/Components/GroundLinkHealthProvider/GroundLinkHealthProvider.hpp"

#include <limits>

namespace OBC {

namespace {

U32 saturatingIncrement(const U32 value) {
    return value == std::numeric_limits<U32>::max() ? value : value + 1U;
}

bool changed(const U32 current, const U32 previous) {
    return current > previous;
}

}  // namespace

GroundLinkHealthProvider::GroundLinkHealthProvider(const char* const compName)
    : GroundLinkHealthProviderComponentBase(compName),
      m_sbandGroundLinkDriver(nullptr),
      m_uhfGroundLinkDriver(nullptr),
      m_sbandState(),
      m_uhfState() {
    this->m_sbandState.view.band = OBC::CommBand::SBAND;
    this->m_uhfState.view.band = OBC::CommBand::UHF;
}

GroundLinkHealthProvider::~GroundLinkHealthProvider() = default;

void GroundLinkHealthProvider::configureRuntime(GroundLinkDriver* sbandGroundLinkDriver, GroundLinkDriver* uhfGroundLinkDriver) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_sbandGroundLinkDriver = sbandGroundLinkDriver;
    this->m_uhfGroundLinkDriver = uhfGroundLinkDriver;
    this->m_sbandState = {};
    this->m_sbandState.view.band = OBC::CommBand::SBAND;
    this->m_uhfState = {};
    this->m_uhfState.view.band = OBC::CommBand::UHF;
}

void GroundLinkHealthProvider::tickForTest() {
    if (this->m_sbandGroundLinkDriver != nullptr && !this->m_sbandGroundLinkDriver->isRunningForRuntime()) {
        static_cast<void>(this->m_sbandGroundLinkDriver->observeHealthForRuntime());
    }

    if (this->m_uhfGroundLinkDriver != nullptr && !this->m_uhfGroundLinkDriver->isRunningForRuntime()) {
        static_cast<void>(this->m_uhfGroundLinkDriver->observeHealthForRuntime());
    }

    this->schedIn_handler(0, 0U);
}

OBC::CommLinkHealthView GroundLinkHealthProvider::getHealthForRuntime(OBC::CommBand band) const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return band == OBC::CommBand::UHF ? this->m_uhfState.view : this->m_sbandState.view;
}

void GroundLinkHealthProvider::markTelemetryDirtyForRuntime(OBC::CommBand band) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    TrackedLinkState& tracked = band == OBC::CommBand::UHF ? this->m_uhfState : this->m_sbandState;
    tracked.havePublishedTelemetry = false;
}

void GroundLinkHealthProvider::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->updateTrackedState_(OBC::CommBand::SBAND, this->m_sbandGroundLinkDriver, this->m_sbandState);
        this->updateTrackedState_(OBC::CommBand::UHF, this->m_uhfGroundLinkDriver, this->m_uhfState);
    }

    this->publishTelemetry_();
}

void GroundLinkHealthProvider::updateTrackedState_(OBC::CommBand band,
                                                   GroundLinkDriver* driver,
                                                   TrackedLinkState& trackedState) {
    const OBC::COMM::GroundLinkObservationState observation =
        driver != nullptr ? driver->getObservationForRuntime() : OBC::COMM::GroundLinkObservationState{};
    const bool previouslyInitialized = trackedState.initialized;
    const bool rxChanged = previouslyInitialized && changed(observation.rxChunks, trackedState.lastObservation.rxChunks);
    const bool txChanged = previouslyInitialized && changed(observation.txChunks, trackedState.lastObservation.txChunks);
    const bool statusChanged =
        previouslyInitialized &&
        changed(observation.successfulStatusObservations, trackedState.lastObservation.successfulStatusObservations);
    const bool anyActivityChanged =
        (!previouslyInitialized && observation.connected) || rxChanged || txChanged || statusChanged;
    const bool errorGrowthThisCycle =
        previouslyInitialized && observation.healthSemantics == OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP &&
        (changed(observation.txErrors, trackedState.lastObservation.txErrors) ||
         changed(observation.rxErrors, trackedState.lastObservation.rxErrors));

    trackedState.view.band = band;
    trackedState.view.backendMode = observation.mode;
    trackedState.view.connected = observation.connected;
    trackedState.view.errorGrowthThisCycle = errorGrowthThisCycle;

    if (!previouslyInitialized) {
        trackedState.view.activityAgeTicks = 0U;
        trackedState.view.rxAgeTicks = 0U;
        trackedState.view.txAgeTicks = 0U;
    } else {
        trackedState.view.activityAgeTicks =
            anyActivityChanged ? 0U : saturatingIncrement(trackedState.view.activityAgeTicks);
        trackedState.view.rxAgeTicks = rxChanged ? 0U : saturatingIncrement(trackedState.view.rxAgeTicks);
        trackedState.view.txAgeTicks = txChanged ? 0U : saturatingIncrement(trackedState.view.txAgeTicks);
    }

    switch (observation.healthSemantics) {
        case OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP:
            if (!observation.connected) {
                trackedState.view.available = false;
                trackedState.view.availabilityReason = OBC::CommLinkAvailabilityReason::DISCONNECTED;
            } else if (trackedState.view.activityAgeTicks <= 1U) {
                trackedState.view.available = true;
                trackedState.view.availabilityReason = OBC::CommLinkAvailabilityReason::HEALTHY_ACTIVITY;
            } else {
                trackedState.view.available = false;
                trackedState.view.availabilityReason = OBC::CommLinkAvailabilityReason::STALE_ACTIVITY;
            }
            break;
        case OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK:
            trackedState.view.available = observation.connected;
            trackedState.view.availabilityReason = observation.connected
                                                       ? OBC::CommLinkAvailabilityReason::CONNECTED_ONLY_FALLBACK
                                                       : OBC::CommLinkAvailabilityReason::DISCONNECTED;
            break;
        case OBC::COMM::GroundLinkHealthSemantics::DISABLED:
        default:
            trackedState.view.available = false;
            trackedState.view.availabilityReason = OBC::CommLinkAvailabilityReason::DISCONNECTED;
            break;
    }

    trackedState.lastObservation = observation;
    trackedState.initialized = true;
}

void GroundLinkHealthProvider::publishTelemetry_() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    auto publishIfChanged = [this](TrackedLinkState& state,
                                   auto writeAvailable,
                                   auto writeAge,
                                   auto writeReason) {
        const bool changed = !state.havePublishedTelemetry ||
                             state.view.available != state.lastPublishedView.available ||
                             state.view.activityAgeTicks != state.lastPublishedView.activityAgeTicks ||
                             state.view.availabilityReason != state.lastPublishedView.availabilityReason;
        if (!changed) {
            return;
        }
        writeAvailable(state.view.available);
        writeAge(state.view.activityAgeTicks);
        writeReason(static_cast<U32>(state.view.availabilityReason));
        state.lastPublishedView = state.view;
        state.havePublishedTelemetry = true;
    };

    publishIfChanged(
        this->m_sbandState,
        [this](bool value) { this->tlmWrite_GROUND_LINK_HEALTH_S_BAND_AVAILABLE(value); },
        [this](U32 value) { this->tlmWrite_GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS(value); },
        [this](U32 value) { this->tlmWrite_GROUND_LINK_HEALTH_S_BAND_REASON(value); });
    publishIfChanged(
        this->m_uhfState,
        [this](bool value) { this->tlmWrite_GROUND_LINK_HEALTH_UHF_AVAILABLE(value); },
        [this](U32 value) { this->tlmWrite_GROUND_LINK_HEALTH_UHF_ACTIVITY_AGE_TICKS(value); },
        [this](U32 value) { this->tlmWrite_GROUND_LINK_HEALTH_UHF_REASON(value); });
}

}  // namespace OBC

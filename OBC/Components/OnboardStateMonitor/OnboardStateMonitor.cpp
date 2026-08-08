#include "OBC/Components/OnboardStateMonitor/OnboardStateMonitor.hpp"

namespace OBC {

OnboardStateMonitor::OnboardStateMonitor(const char* const compName)
    : OnboardStateMonitorComponentBase(compName),
      m_source(nullptr),
      m_haveState(false),
      m_reducedState(),
      m_recentReducedRing(),
      m_recentReducedWriteIndex(0U),
      m_reductionCount(0U) {}

OnboardStateMonitor::~OnboardStateMonitor() = default;

void OnboardStateMonitor::configureRuntime(const OBC::StateData::IStateSnapshotSource* source) {
    this->m_source = source;
    this->m_haveState = false;
    this->m_reductionCount = 0U;
    this->m_recentReducedWriteIndex = 0U;
    this->publishState_();
}

bool OnboardStateMonitor::getReducedStateForRuntime(OBC::StateData::ReducedStateV1& state) const {
    if (!this->m_haveState) {
        return false;
    }
    state = this->m_reducedState;
    return true;
}

bool OnboardStateMonitor::getRecentReducedStateForRuntime(U32 newestOffset,
                                                          OBC::StateData::ReducedStateV1& state) const {
    const U32 available = this->m_reductionCount < RECENT_REDUCED_RING_SIZE ? this->m_reductionCount
                                                                            : RECENT_REDUCED_RING_SIZE;
    if (newestOffset >= available) {
        return false;
    }

    const U32 index =
        (this->m_recentReducedWriteIndex + RECENT_REDUCED_RING_SIZE - 1U - newestOffset) % RECENT_REDUCED_RING_SIZE;
    state = this->m_recentReducedRing[index];
    return true;
}

bool OnboardStateMonitor::reduceNow() {
    if (this->m_source == nullptr) {
        this->invalidateState_();
        this->publishState_();
        this->log_WARNING_HI_STATE_MONITOR_SOURCE_UNAVAILABLE();
        return false;
    }

    OBC::StateData::StateSnapshot snapshot = {};
    snapshot.timestamp = this->getTime();
    if (!this->m_source->readStateSnapshot(snapshot)) {
        this->invalidateState_();
        this->publishState_();
        this->log_WARNING_HI_STATE_MONITOR_SOURCE_UNAVAILABLE();
        return false;
    }

    this->m_reducedState = OBC::StateData::reduceStateSnapshot(snapshot);
    this->m_haveState = true;
    this->m_reductionCount++;
    this->storeRecentReducedSample_(this->m_reducedState);
    this->publishState_();
    if (this->shouldEmitStateUpdated_(this->m_reducedState)) {
        this->log_ACTIVITY_LO_STATE_MONITOR_UPDATED(this->m_reducedState.healthMask,
                                                    this->m_reducedState.faultMask,
                                                    this->m_reducedState.qualityMask);
    }
    return true;
}

void OnboardStateMonitor::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    static_cast<void>(this->reduceNow());
}

void OnboardStateMonitor::publishState_() {
    this->tlmWrite_STATE_MONITOR_HAVE_STATE(this->m_haveState ? 1U : 0U);
    this->tlmWrite_STATE_MONITOR_HEALTH_MASK(this->m_reducedState.healthMask);
    this->tlmWrite_STATE_MONITOR_FAULT_MASK(this->m_reducedState.faultMask);
    this->tlmWrite_STATE_MONITOR_QUALITY_MASK(this->m_reducedState.qualityMask);
    this->tlmWrite_STATE_MONITOR_REDUCTION_COUNT(this->m_reductionCount);
    this->tlmWrite_STATE_MONITOR_LAST_SOC(this->m_reducedState.batterySoc);
    this->tlmWrite_STATE_MONITOR_LAST_RATE_NORM(this->m_reducedState.adcsRateNorm);
}

void OnboardStateMonitor::storeRecentReducedSample_(const OBC::StateData::ReducedStateV1& state) {
    this->m_recentReducedRing[this->m_recentReducedWriteIndex] = state;
    this->m_recentReducedWriteIndex = (this->m_recentReducedWriteIndex + 1U) % RECENT_REDUCED_RING_SIZE;
}

void OnboardStateMonitor::invalidateState_() {
    this->m_haveState = false;
    this->m_reducedState = {};
}

bool OnboardStateMonitor::shouldEmitStateUpdated_(const OBC::StateData::ReducedStateV1& state) const {
    if (!this->havePreviousSuccessfulState_()) {
        return false;
    }

    const OBC::StateData::ReducedStateV1 previous = this->previousSuccessfulState_();
    return previous.healthMask != state.healthMask || previous.faultMask != state.faultMask ||
           previous.qualityMask != state.qualityMask;
}

bool OnboardStateMonitor::havePreviousSuccessfulState_() const {
    return this->m_reductionCount > 1U;
}

OBC::StateData::ReducedStateV1 OnboardStateMonitor::previousSuccessfulState_() const {
    const U32 index = (this->m_recentReducedWriteIndex + RECENT_REDUCED_RING_SIZE - 2U) % RECENT_REDUCED_RING_SIZE;
    return this->m_recentReducedRing[index];
}

}  // namespace OBC

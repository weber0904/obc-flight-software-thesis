#include "OBC/Components/RateGroupTimingProbe/RateGroupTimingProbe.hpp"

#include <limits>

namespace OBC {

RateGroupTimingProbe::RateGroupTimingProbe(const char* const compName)
    : RateGroupTimingProbeComponentBase(compName),
      m_enabled(false),
      m_cycleThresholdUsec(900000U),
      m_thresholdExceedTotal(0U),
      m_maxCycleUsec(0U),
      m_cycleWindow() {}

RateGroupTimingProbe::~RateGroupTimingProbe() = default;

void RateGroupTimingProbe::configureForRuntime(bool enabled, U32 cycleThresholdUsec) {
    this->m_enabled = enabled;
    this->m_cycleThresholdUsec = cycleThresholdUsec == 0U ? 1U : cycleThresholdUsec;
    this->log_ACTIVITY_HI_RG_TIMING_CONFIG(this->m_enabled, this->m_cycleThresholdUsec);
}

void RateGroupTimingProbe::schedIn_handler(FwIndexType portNum, U32 context) {
    this->beginCycle_(portNum);

    const Clock::time_point started = Clock::now();
    if (this->isConnected_schedOut_OutputPort(portNum)) {
        this->schedOut_out(portNum, context);
    }
    const U32 elapsedUsec = elapsedUsec_(started, Clock::now());

    this->m_cycleWindow.totalUsec = saturatingAdd_(this->m_cycleWindow.totalUsec, elapsedUsec);
    if (elapsedUsec >= this->m_cycleWindow.slowestUsec) {
        this->m_cycleWindow.slowestUsec = elapsedUsec;
        this->m_cycleWindow.slowestSlot = static_cast<U32>(portNum);
    }

    this->finishCycleIfNeeded_(portNum);
}

U32 RateGroupTimingProbe::saturatingAdd_(U32 lhs, U32 rhs) {
    if (std::numeric_limits<U32>::max() - lhs < rhs) {
        return std::numeric_limits<U32>::max();
    }
    return lhs + rhs;
}

U32 RateGroupTimingProbe::elapsedUsec_(const Clock::time_point& started, const Clock::time_point& finished) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finished - started).count();
    if (elapsed <= 0) {
        return 0U;
    }
    if (elapsed >= static_cast<long long>(std::numeric_limits<U32>::max())) {
        return std::numeric_limits<U32>::max();
    }
    return static_cast<U32>(elapsed);
}

void RateGroupTimingProbe::beginCycle_(FwIndexType portNum) {
    if (!this->m_cycleWindow.active || portNum == 0) {
        this->m_cycleWindow.active = true;
        this->m_cycleWindow.cycle = this->m_cycleWindow.cycle + 1U;
        this->m_cycleWindow.totalUsec = 0U;
        this->m_cycleWindow.slowestSlot = 0U;
        this->m_cycleWindow.slowestUsec = 0U;
    }
}

void RateGroupTimingProbe::finishCycleIfNeeded_(FwIndexType portNum) {
    if (!this->m_cycleWindow.active || portNum + 1U != SLOT_COUNT) {
        return;
    }

    this->tlmWrite_RG_TIMING_LAST_SLOW_SLOT(this->m_cycleWindow.slowestSlot);
    this->tlmWrite_RG_TIMING_LAST_SLOW_USEC(this->m_cycleWindow.slowestUsec);
    this->tlmWrite_RG_TIMING_LAST_CYCLE_USEC(this->m_cycleWindow.totalUsec);

    if (this->m_cycleWindow.totalUsec > this->m_maxCycleUsec) {
        this->m_maxCycleUsec = this->m_cycleWindow.totalUsec;
        this->tlmWrite_RG_TIMING_MAX_CYCLE_USEC(this->m_maxCycleUsec);
    }

    if (this->m_enabled && this->m_cycleWindow.totalUsec >= this->m_cycleThresholdUsec) {
        if (this->m_thresholdExceedTotal < std::numeric_limits<U32>::max()) {
            this->m_thresholdExceedTotal++;
        }
        this->tlmWrite_RG_TIMING_THRESHOLD_EXCEED_TOTAL(this->m_thresholdExceedTotal);
        this->log_WARNING_LO_RG_TIMING_CYCLE_THRESHOLD_EXCEEDED(this->m_cycleWindow.cycle,
                                                                this->m_cycleWindow.totalUsec,
                                                                this->m_cycleWindow.slowestSlot,
                                                                this->m_cycleWindow.slowestUsec);
    }

    this->m_cycleWindow.active = false;
}

}  // namespace OBC

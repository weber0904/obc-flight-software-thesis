#include "RateGroupTimingProbeTester.hpp"

#include <chrono>
#include <thread>

namespace OBC {

RateGroupTimingProbeTester::RateGroupTimingProbeTester()
    : RateGroupTimingProbeGTestBase("RateGroupTimingProbeTester", MAX_HISTORY_SIZE),
      component("RateGroupTimingProbe"),
      m_forwardCount(),
      m_lastContext(),
      m_sleepUsec() {
    this->initComponents();
    this->connectPorts();
}

RateGroupTimingProbeTester::~RateGroupTimingProbeTester() = default;

void RateGroupTimingProbeTester::testForwardsScheduledContexts() {
    this->component.configureForRuntime(false, 1000U);
    this->clearHistory();
    this->runFullCycle_(100U);

    ASSERT_EVENTS_RG_TIMING_CYCLE_THRESHOLD_EXCEEDED_SIZE(0);
    for (FwIndexType port = 0; port < RateGroupTimingProbe::SLOT_COUNT; port++) {
        ASSERT_EQ(this->m_forwardCount.at(static_cast<std::size_t>(port)), 1U);
        ASSERT_EQ(this->m_lastContext.at(static_cast<std::size_t>(port)), 100U + static_cast<U32>(port));
    }
}

void RateGroupTimingProbeTester::testEmitsSlowCycleSummary() {
    this->component.configureForRuntime(true, 1000U);
    this->m_sleepUsec.fill(0U);
    this->m_sleepUsec.at(5) = 5000U;

    this->clearHistory();
    this->runFullCycle_(200U);

    ASSERT_EVENTS_RG_TIMING_CYCLE_THRESHOLD_EXCEEDED_SIZE(1);
    ASSERT_TLM_RG_TIMING_LAST_SLOW_SLOT_SIZE(1);
    ASSERT_TLM_RG_TIMING_LAST_SLOW_SLOT(0, 5U);
    ASSERT_TLM_RG_TIMING_LAST_SLOW_USEC_SIZE(1);
    ASSERT_GE(this->tlmHistory_RG_TIMING_LAST_SLOW_USEC->at(0).arg, 1000U);
    ASSERT_TLM_RG_TIMING_LAST_CYCLE_USEC_SIZE(1);
    ASSERT_GE(this->tlmHistory_RG_TIMING_LAST_CYCLE_USEC->at(0).arg, 1000U);
    ASSERT_TLM_RG_TIMING_THRESHOLD_EXCEED_TOTAL_SIZE(1);
    ASSERT_TLM_RG_TIMING_THRESHOLD_EXCEED_TOTAL(0, 1U);
}

void RateGroupTimingProbeTester::runFullCycle_(U32 baseContext) {
    for (FwIndexType port = 0; port < RateGroupTimingProbe::SLOT_COUNT; port++) {
        this->invoke_to_schedIn(port, baseContext + static_cast<U32>(port));
    }
}

void RateGroupTimingProbeTester::from_schedOut_handler(FwIndexType portNum, U32 context) {
    const std::size_t index = static_cast<std::size_t>(portNum);
    this->m_forwardCount.at(index)++;
    this->m_lastContext.at(index) = context;
    const U32 sleepUsec = this->m_sleepUsec.at(index);
    if (sleepUsec > 0U) {
        std::this_thread::sleep_for(std::chrono::microseconds(sleepUsec));
    }
}

}  // namespace OBC

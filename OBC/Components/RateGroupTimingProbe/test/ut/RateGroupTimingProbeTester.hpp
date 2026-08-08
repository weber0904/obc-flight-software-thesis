#ifndef OBC_COMPONENTS_RATEGROUPTIMINGPROBE_TEST_UT_RATEGROUPTIMINGPROBETESTER_HPP
#define OBC_COMPONENTS_RATEGROUPTIMINGPROBE_TEST_UT_RATEGROUPTIMINGPROBETESTER_HPP

#include <array>

#include "OBC/Components/RateGroupTimingProbe/RateGroupTimingProbe.hpp"
#include "OBC/Components/RateGroupTimingProbe/RateGroupTimingProbeGTestBase.hpp"

namespace OBC {

class RateGroupTimingProbeTester final : public RateGroupTimingProbeGTestBase {
  public:
    static constexpr FwSizeType MAX_HISTORY_SIZE = 20;
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;

    RateGroupTimingProbeTester();

    ~RateGroupTimingProbeTester() override;

    void testForwardsScheduledContexts();
    void testEmitsSlowCycleSummary();

  private:
    void initComponents();
    void connectPorts();
    void runFullCycle_(U32 baseContext);

    void from_schedOut_handler(FwIndexType portNum, U32 context) override;

  private:
    RateGroupTimingProbe component;
    std::array<U32, RateGroupTimingProbe::SLOT_COUNT> m_forwardCount;
    std::array<U32, RateGroupTimingProbe::SLOT_COUNT> m_lastContext;
    std::array<U32, RateGroupTimingProbe::SLOT_COUNT> m_sleepUsec;
};

}  // namespace OBC

#endif

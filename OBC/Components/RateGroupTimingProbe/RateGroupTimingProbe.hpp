#ifndef OBC_COMPONENTS_RATEGROUPTIMINGPROBE_RATEGROUPTIMINGPROBE_HPP
#define OBC_COMPONENTS_RATEGROUPTIMINGPROBE_RATEGROUPTIMINGPROBE_HPP

#include <array>
#include <chrono>

#include "OBC/Components/RateGroupTimingProbe/RateGroupTimingProbeComponentAc.hpp"

namespace OBC {

class RateGroupTimingProbe final : public RateGroupTimingProbeComponentBase {
  public:
    static constexpr FwIndexType SLOT_COUNT = 16;

    explicit RateGroupTimingProbe(const char* const compName);

    ~RateGroupTimingProbe() override;

    void configureForRuntime(bool enabled, U32 cycleThresholdUsec);

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    struct CycleWindow {
        bool active = false;
        U32 cycle = 0U;
        U32 totalUsec = 0U;
        U32 slowestSlot = 0U;
        U32 slowestUsec = 0U;
    };

    using Clock = std::chrono::steady_clock;

    static U32 saturatingAdd_(U32 lhs, U32 rhs);
    static U32 elapsedUsec_(const Clock::time_point& started, const Clock::time_point& finished);

    void beginCycle_(FwIndexType portNum);
    void finishCycleIfNeeded_(FwIndexType portNum);

  private:
    bool m_enabled;
    U32 m_cycleThresholdUsec;
    U32 m_thresholdExceedTotal;
    U32 m_maxCycleUsec;
    CycleWindow m_cycleWindow;
};

}  // namespace OBC

#endif

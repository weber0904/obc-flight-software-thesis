#ifndef OBC_COMPONENTS_ONBOARDSTATEMONITOR_ONBOARDSTATEMONITOR_HPP
#define OBC_COMPONENTS_ONBOARDSTATEMONITOR_ONBOARDSTATEMONITOR_HPP

#include <array>

#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"
#include "OBC/Components/OnboardStateMonitor/OnboardStateMonitorComponentAc.hpp"

namespace OBC {

class OnboardStateMonitor final : public OnboardStateMonitorComponentBase,
                                  public OBC::StateData::IReducedStateSource {
  public:
    static constexpr U32 RECENT_REDUCED_RING_SIZE = 16U;

    explicit OnboardStateMonitor(const char* const compName);

    ~OnboardStateMonitor() override;

    void configureRuntime(const OBC::StateData::IStateSnapshotSource* source);

    bool getReducedStateForRuntime(OBC::StateData::ReducedStateV1& state) const override;

    bool getRecentReducedStateForRuntime(U32 newestOffset, OBC::StateData::ReducedStateV1& state) const;

    bool reduceNow();

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

  private:
    void publishState_();
    void storeRecentReducedSample_(const OBC::StateData::ReducedStateV1& state);
    void invalidateState_();
    bool shouldEmitStateUpdated_(const OBC::StateData::ReducedStateV1& state) const;
    bool havePreviousSuccessfulState_() const;
    OBC::StateData::ReducedStateV1 previousSuccessfulState_() const;

  private:
    const OBC::StateData::IStateSnapshotSource* m_source;
    bool m_haveState;
    OBC::StateData::ReducedStateV1 m_reducedState;
    std::array<OBC::StateData::ReducedStateV1, RECENT_REDUCED_RING_SIZE> m_recentReducedRing;
    U32 m_recentReducedWriteIndex;
    U32 m_reductionCount;
};

}  // namespace OBC

#endif

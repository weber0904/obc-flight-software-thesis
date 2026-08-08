#ifndef OBC_ONBOARDSTATEMONITOR_TESTER_HPP
#define OBC_ONBOARDSTATEMONITOR_TESTER_HPP

#include "OBC/Components/OnboardStateMonitor/OnboardStateMonitor.hpp"
#include "OBC/Components/OnboardStateMonitor/OnboardStateMonitorGTestBase.hpp"

namespace OBC {

class OnboardStateMonitorTester final : public OnboardStateMonitorGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    OnboardStateMonitorTester();
    ~OnboardStateMonitorTester() override;

    void testSchedPublishesReducedState();
    void testStateMonitorUpdatedRequiresMaskChange();
    void testStateMonitorUpdatedRemainsQuietAcrossSourceRecovery();
    void testMissingSourceRaisesWarning();
    void testSourceFailureInvalidatesCachedState();
    void testRecentReducedStateRingIsRuntimeReadable();

  private:
    class FakeSource final : public OBC::StateData::IStateSnapshotSource {
      public:
        bool readStateSnapshot(OBC::StateData::StateSnapshot& snapshot) const override;

        bool available = true;
        mutable U32 calls = 0U;
        OBC::StateData::StateSnapshot snapshot = {};
    };

  private:
    void connectPorts();
    void initComponents();
    void configureNominalSource_();

  private:
    FakeSource m_source;
    OBC::OnboardStateMonitor component;
};

}  // namespace OBC

#endif

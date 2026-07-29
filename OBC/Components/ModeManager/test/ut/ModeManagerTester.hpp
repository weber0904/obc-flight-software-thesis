#ifndef OBC_ModeManagerTester_HPP
#define OBC_ModeManagerTester_HPP

#include "OBC/Components/ModeManager/ModeManager.hpp"
#include "OBC/Components/ModeManager/ModeManagerGTestBase.hpp"

namespace OBC {

class ModeManagerTester final : public ModeManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    ModeManagerTester();

    ~ModeManagerTester() override;

    void testModeSetPublishesTelemetry();

    void testModeSetRejectedPreservesMode();

    void testSameModeNoOpDoesNotEmitModeChange();

    void testGuardUnconfiguredReturnsExecutionError();

    void testAllOperatorPairsUseGuardedPath();

    void testModeGetRepublishesStateWhenUnchanged();

  private:
    class FakeOperatorTransitionGuard final : public OBC::IModeOperatorTransitionGuard {
      public:
        OBC::OperatorModeTransitionDecision evaluateOperatorTransition(OBC::SatMode currentMode,
                                                                       OBC::SatMode requestedMode) const override;

        bool accepted = true;
        OBC::ModeTransitionRejectionReason reason =
            OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION;
        U32 calls = 0U;
        mutable OBC::SatMode lastCurrentMode = OBC::SatMode::SAFE;
        mutable OBC::SatMode lastRequestedMode = OBC::SatMode::SAFE;
    };

    void connectPorts();

    void initComponents();

  protected:
    void from_modeGetTlmOut_handler(FwIndexType portNum,
                                    FwChanIdType id,
                                    Fw::Time& timeTag,
                                    Fw::TlmBuffer& val) override;

  private:
    FakeOperatorTransitionGuard m_guard;
    OBC::ModeManager component;
};

}  // namespace OBC

#endif

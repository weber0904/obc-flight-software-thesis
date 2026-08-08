#ifndef OBC_COMPONENTS_EPSFDIRCONTROLLER_EPSFDIRCONTROLLER_HPP
#define OBC_COMPONENTS_EPSFDIRCONTROLLER_EPSFDIRCONTROLLER_HPP

#include "OBC/Components/EpsFdirController/EpsFdirControllerComponentAc.hpp"
#include "OBC/Components/EpsFdirController/EpsFdirPolicy.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"

namespace OBC {

class EpsFdirController final : public EpsFdirControllerComponentBase {
  public:
    explicit EpsFdirController(const char* const compName);

    ~EpsFdirController() override;

    void configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                          const OBC::IEpsFdirHealthProvider* healthProvider,
                          OBC::IRecoveryRequestSink* recoverySink);

    OBC::EpsFdirDecision runCycle();

#ifdef BUILD_UT
    void setCountersForTest(U32 escalationCount, U32 recoveryCount);
#endif

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void publishState_(const OBC::EpsFdirDecision& decision);

    static void saturatingIncrement_(U32& value);

  private:
    OBC::IModeSafetyModeControl* m_modeControl;
    const OBC::IEpsFdirHealthProvider* m_healthProvider;
    OBC::IRecoveryRequestSink* m_recoverySink;
    bool m_faultLatched;
    bool m_lastRequestedSafe;
    U32 m_lastFailureCount;
    U32 m_escalationCount;
    U32 m_recoveryCount;
};

}  // namespace OBC

#endif

#ifndef OBC_COMPONENTS_ADCSFDIRCONTROLLER_ADCSFDIRCONTROLLER_HPP
#define OBC_COMPONENTS_ADCSFDIRCONTROLLER_ADCSFDIRCONTROLLER_HPP

#include "OBC/Components/AdcsBridge/AdcsRuntime.hpp"
#include "OBC/Components/AdcsFdirController/AdcsFdirControllerComponentAc.hpp"
#include "OBC/Components/AdcsFdirController/AdcsFdirPolicy.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"

namespace OBC {

class AdcsFdirController final : public AdcsFdirControllerComponentBase {
  public:
    explicit AdcsFdirController(const char* const compName);

    ~AdcsFdirController() override;

    void configureRuntime(const OBC::IAdcsFdirHealthProvider* healthProvider, OBC::IRecoveryRequestSink* recoverySink);

    OBC::AdcsFdirDecision runCycle();

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void publishState_(const OBC::AdcsFdirDecision& decision);

    static void saturatingIncrement_(U32& value);

    void submitFault_(const OBC::AdcsFdirDecision& decision);

    void clearFault_();

  private:
    const OBC::IAdcsFdirHealthProvider* m_healthProvider;
    OBC::IRecoveryRequestSink* m_recoverySink;
    bool m_faultLatched;
    OBC::RecoveryIncidentSource m_latchedSource;
    U32 m_lastFailureCount;
    U32 m_escalationCount;
    U32 m_recoveryCount;
};

}  // namespace OBC

#endif

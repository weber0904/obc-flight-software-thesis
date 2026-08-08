#ifndef OBC_COMPONENTS_TTCPASSMANAGER_TTCPASSMANAGER_HPP
#define OBC_COMPONENTS_TTCPASSMANAGER_TTCPASSMANAGER_HPP

#include <mutex>

#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/TtcPassManager/TtcPassManagerComponentAc.hpp"
#include "OBC/Components/TtcPassManager/TtcPassPolicy.hpp"

namespace OBC {

class TtcPassManager final : public TtcPassManagerComponentBase {
  public:
    explicit TtcPassManager(const char* const compName);

    ~TtcPassManager() override;

    void configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                          const OBC::ITtcPassGpsProvider* gpsProvider,
                          const OBC::ITtcPassCommStateProvider* commProvider,
                          OBC::ITtcPassAdcsControl* adcsControl);

    Fw::CmdResponse setPolicyForRuntime(bool enabled, U32 lossOfLockTimeoutSec);

    Fw::CmdResponse setPassWindowForRuntime(U64 startUnixSec, U64 endUnixSec);

    void clearPassWindowForRuntime();

    OBC::TtcPassRuntimeStatus getStatusForRuntime() const;

    void tickForTest();

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void TTC_SET_POLICY_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enabled, U32 lossOfLockTimeoutSec) override;

    void TTC_SET_PASS_WINDOW_cmdHandler(FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        U64 startUnixSec,
                                        U64 endUnixSec) override;

    void TTC_CLEAR_PASS_WINDOW_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void TTC_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void publishChangeDrivenStatus_(const OBC::TtcPassRuntimeStatus& previous,
                                    const OBC::TtcPassRuntimeStatus& current);

    void publishExplicitRefreshStatus_(const OBC::TtcPassRuntimeStatus& status);

    static void clearWindow_(OBC::TtcPassWindow& window);
    static bool windowsEqual_(const OBC::TtcPassWindow& lhs, const OBC::TtcPassWindow& rhs);

  private:
    mutable std::mutex m_mutex;
    OBC::IModeSafetyModeControl* m_modeControl;
    const OBC::ITtcPassGpsProvider* m_gpsProvider;
    const OBC::ITtcPassCommStateProvider* m_commProvider;
    OBC::ITtcPassAdcsControl* m_adcsControl;
    OBC::TtcPassConfig m_config;
    OBC::TtcPassWindow m_window;
    OBC::TtcPassWindow m_suppressedWindow;
    OBC::TtcGpsFreshnessTracker m_gpsFreshness;
    OBC::TtcCommLossTracker m_commLoss;
    OBC::TtcPassRuntimeStatus m_status;
};

}  // namespace OBC

#endif

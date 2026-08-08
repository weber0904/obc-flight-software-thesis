#ifndef OBC_Components_AdcsBridge_HPP
#define OBC_Components_AdcsBridge_HPP

#include <cstdint>
#include <memory>

#include "Fw/Types/Assert.hpp"
#include "Fw/Tlm/TlmBuffer.hpp"
#include "OBC/Components/CspRuntimeOwner/AsyncCspRuntimeOwner.hpp"
#include "OBC/Components/AdcsBridge/AdcsRuntime.hpp"
#include "OBC/Components/AdcsBridge/AdcsBridgeComponentAc.hpp"
#include "OBC/Components/MissionExecutive/MissionExecutiveRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"
#include "OBC/Components/TtcPassManager/TtcPassRuntime.hpp"
#include "simulators/adcs/AdcsTransport.hpp"

namespace OBC {

class AdcsBridge final : public AdcsBridgeComponentBase,
                         public IAdcsFdirHealthProvider,
                         public IAdcsAutonomyStatus,
                         public IAdcsDetumbleControl,
                         public IAdcsSunPointingControl,
                         public ITtcPassAdcsControl,
                         public OBC::IRecoveryAdcsControl {
  public:
    explicit AdcsBridge(const char* const compName, U32 timeoutMs = 200U);

    ~AdcsBridge() override;

    void setTransportForTest(OBC::ADCS::IAdcsTransport* transport);

    bool pollStateForTest();

    void tickScheduledPollForTest();

    bool getStateForRuntime(OBC::ADCS::StateData& state);

    bool getCachedStateForRuntime(OBC::ADCS::StateData& state) const override;

    bool getPollHealthForRuntime(OBC::ADCS::PollHealthState& state) const override;

    void configureCspRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime);

    void configureScheduledPollPeriodForTest(U32 periodTicks);
    void configureOperatorTelemetryPeriodForTest(U32 periodTicks);

    Fw::CmdResponse setModeForRuntime(OBC::AdcsMode mode, OBC::ADCS::StateData& state);

    Fw::CmdResponse setTargetForRuntime(F64 q0, F64 q1, F64 q2, F64 q3, OBC::ADCS::StateData& state);

    Fw::CmdResponse calibrateForRuntime(U8 sensorId, OBC::ADCS::StateData& state);

    Fw::CmdResponse resetForRuntime(OBC::ADCS::StateData& state);

    Fw::CmdResponse resetAdcsForRecovery(OBC::ADCS::StateData& state) override {
        return this->resetForRuntime(state);
    }

    bool commandDetumbleForRuntime() override;

    bool commandSunSafePointingForRuntime(double q0, double q1, double q2, double q3) override;

    bool requestPointingForTtcEntry() override;

  private:
    struct AsyncScheduledPollState {
        bool inFlight = false;
        std::uint64_t handle = 0U;
        std::uint16_t seq = 0U;
    };

    struct ExplicitRefreshState {
        bool active = false;
        std::uint8_t nextField = 0U;
        OBC::ADCS::StateData state = {};
    };

    static constexpr F32 DETUMBLE_THRESHOLD = 0.05F;
    static constexpr F32 POINTING_THRESHOLD_DEG = 5.0F;
    static constexpr std::uint8_t EXPLICIT_REFRESH_FIELDS_PER_TICK = 3U;
    static constexpr std::uint8_t EXPLICIT_REFRESH_FIELD_COUNT = 12U;

    void schedIn_handler(const FwIndexType portNum, U32 context) override;

    void ADCS_SET_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::AdcsMode mode) override;

    void ADCS_SET_TARGET_cmdHandler(FwOpcodeType opCode,
                                    U32 cmdSeq,
                                    F64 q0,
                                    F64 q1,
                                    F64 q2,
                                    F64 q3) override;

    void ADCS_GET_ATTITUDE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void ADCS_CALIBRATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 sensorId) override;

    OBC::ADCS::TransportStatus requestState_(OBC::ADCS::StateData& state);

    OBC::ADCS::TransportStatus requestSetMode_(OBC::AdcsMode mode, OBC::ADCS::StateData& state);

    OBC::ADCS::TransportStatus requestSetTarget_(F64 q0, F64 q1, F64 q2, F64 q3, OBC::ADCS::StateData& state);

    OBC::ADCS::TransportStatus requestCalibrate_(U8 sensorId, OBC::ADCS::StateData& state);

    OBC::ADCS::TransportStatus requestReset_(OBC::ADCS::StateData& state);

    bool pollScheduledStateAsync_();
    bool consumeAsyncScheduledState_();
    bool submitAsyncScheduledState_();
    std::uint16_t nextAsyncSeq_();
    bool shouldEmitScheduledOperatorTelemetry_();

    bool updateCachedState_(const OBC::ADCS::StateData& state, bool emitChangeDrivenTelemetry);

    void publishScheduledOperatorTelemetry_(const OBC::ADCS::StateData& state);

    void publishExplicitRefreshTelemetry_(const OBC::ADCS::StateData& state);

    void publishChangeDrivenTelemetry_(OBC::AdcsMode mode);

    void queueExplicitRefresh_(const OBC::ADCS::StateData& state);

    void drainExplicitRefresh_(std::uint8_t maxFields);

    void emitExplicitRefreshField_(std::uint8_t fieldIndex, const OBC::ADCS::StateData& state, Fw::Time& timeTag);

    void emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag);

    template <typename T>
    void emitStatusRefreshTelemetryValue_(FwChanIdType channelId, const T& value, Fw::Time& timeTag) {
        Fw::TlmBuffer buffer;
        const Fw::SerializeStatus status = buffer.serializeFrom(value);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));
        this->emitStatusRefreshTelemetry_(channelId, buffer, timeTag);
    }

    bool pollScheduledState_();

    void noteScheduledTransportFailure_();

    void noteScheduledNoValidRefresh_();

    void noteScheduledValidRefresh_();

    void emitCommError_(U32 code);

    Fw::CmdResponse mapTransportStatus_(OBC::ADCS::TransportStatus status) const;

  private:
    U32 m_timeoutMs;
    OBC::IAsyncCspRuntimeOwner* m_asyncRuntimeOwner;
    std::uint16_t m_cspTargetNode;
    std::uint16_t m_asyncSeq;
    U32 m_pollPeriodTicks;
    U32 m_ticksUntilNextPoll;
    U32 m_operatorTlmPeriodTicks;
    U32 m_ticksUntilOperatorTlm;
    AsyncScheduledPollState m_asyncScheduledPoll;
    std::unique_ptr<OBC::ADCS::IAdcsTransport> m_ownedTransport;
    OBC::ADCS::IAdcsTransport* m_transport;
    bool m_hasValidState;
    OBC::ADCS::StateData m_cachedState;
    ExplicitRefreshState m_explicitRefresh;
    OBC::ADCS::PollHealthState m_pollHealth;
    OBC::AdcsMode m_lastMode;
    bool m_detumbleLatched;
    bool m_pointingLatched;
};

}  // namespace OBC

#endif

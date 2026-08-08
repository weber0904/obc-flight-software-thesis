#ifndef OBC_Components_EpsBridge_HPP
#define OBC_Components_EpsBridge_HPP

#include <memory>

#include "Fw/Types/Assert.hpp"
#include "Fw/Tlm/TlmBuffer.hpp"
#include "OBC/Components/CspRuntimeOwner/AsyncCspRuntimeOwner.hpp"
#include "OBC/Components/EpsFdirController/EpsFdirRuntime.hpp"
#include "OBC/Components/EpsBridge/EpsBridgeComponentAc.hpp"
#include "OBC/Components/MissionExecutive/MissionExecutiveRuntime.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"
#include "simulators/eps/EpsTransport.hpp"

namespace OBC {

class EpsBridge final : public EpsBridgeComponentBase,
                        public IEpsAutonomyStatus,
                        public OBC::IModeSafetyEpsStatus,
                        public OBC::IEpsFdirHealthProvider,
                        public OBC::IRecoveryEpsControl {
  public:
    explicit EpsBridge(const char* const compName, U32 timeoutMs = 200U);

    ~EpsBridge() override;

    void setTransportForTest(OBC::EPS::IEpsTransport* transport);

    bool pollStatusForTest();

    void tickScheduledPollForTest();

    bool getStatusForRuntime(OBC::EPS::StatusData& status);

    bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override;

    bool getPollHealthForRuntime(OBC::EPS::PollHealthState& state) const override;

    void configureCspRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime);

    void configureScheduledPollPeriodForTest(U32 periodTicks);
    void configureOperatorTelemetryPeriodForTest(U32 periodTicks);

#ifdef BUILD_UT
    void setPollHealthForTest(const OBC::EPS::PollHealthState& state);

    void drainExplicitRefreshForTest(std::uint8_t maxFields = EXPLICIT_REFRESH_FIELD_COUNT);
#endif

    Fw::CmdResponse setPduForRuntime(U8 channel, bool enabled, OBC::EPS::StatusData& status);

    Fw::CmdResponse setHeaterForRuntime(bool enabled, OBC::EPS::StatusData& status);

    Fw::CmdResponse resetForRuntime(OBC::EPS::StatusData& status);

    Fw::CmdResponse resetEpsForRecovery(OBC::EPS::StatusData& status) override { return this->resetForRuntime(status); }

  private:
    struct AsyncScheduledPollState {
        bool inFlight = false;
        std::uint64_t handle = 0U;
        std::uint16_t seq = 0U;
    };

    struct ExplicitRefreshState {
        bool active = false;
        std::uint8_t nextField = 0U;
        OBC::EPS::StatusData status = {};
    };

    static constexpr F32 LOW_BATTERY_SOC = 20.0F;
    static constexpr F32 CRITICAL_BATTERY_SOC = 10.0F;
    static constexpr F32 OVERTEMP_THRESHOLD_C = 45.0F;
    static constexpr std::uint8_t EXPLICIT_REFRESH_FIELDS_PER_TICK = 2U;
    static constexpr std::uint8_t EXPLICIT_REFRESH_FIELD_COUNT = 10U;

    void schedIn_handler(const FwIndexType portNum, U32 context) override;

    void EPS_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void EPS_SET_PDU_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 channel, bool enabled) override;

    void EPS_SET_HEATER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enabled) override;

    void EPS_RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    OBC::EPS::TransportStatus requestStatus_(OBC::EPS::StatusData& status);

    OBC::EPS::TransportStatus requestSetPdu_(U8 channel, bool enabled, OBC::EPS::StatusData& status);

    OBC::EPS::TransportStatus requestSetHeater_(bool enabled, OBC::EPS::StatusData& status);

    OBC::EPS::TransportStatus requestReset_(OBC::EPS::StatusData& status);

    bool pollScheduledStatus_();
    bool consumeAsyncScheduledStatus_();
    bool submitAsyncScheduledStatus_();
    std::uint16_t nextAsyncSeq_();
    bool shouldEmitScheduledOperatorTelemetry_();

    void updateCachedStatus_(const OBC::EPS::StatusData& status, bool emitChangeDrivenTelemetry);

    void publishScheduledOperatorTelemetry_(const OBC::EPS::StatusData& status);

    void publishExplicitRefreshTelemetry_(const OBC::EPS::StatusData& status);

    void publishChangeDrivenTelemetry_(const OBC::EPS::StatusData& status);

    void queueExplicitRefresh_(const OBC::EPS::StatusData& status);

    void drainExplicitRefresh_(std::uint8_t maxFields);

    void emitExplicitRefreshField_(std::uint8_t fieldIndex, const OBC::EPS::StatusData& status, Fw::Time& timeTag);

    void emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag);

    template <typename T>
    void emitStatusRefreshTelemetryValue_(FwChanIdType channelId, const T& value, Fw::Time& timeTag) {
        Fw::TlmBuffer buffer;
        const Fw::SerializeStatus status = buffer.serializeFrom(value);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));
        this->emitStatusRefreshTelemetry_(channelId, buffer, timeTag);
    }

    void invalidateCachedStatus_();

    void notePollSuccess_();

    void notePollFailure_();

    static void saturatingIncrement_(U32& value);

    void emitCommError_(U32 code);

    Fw::CmdResponse mapTransportStatus_(OBC::EPS::TransportStatus status) const;

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
    std::unique_ptr<OBC::EPS::IEpsTransport> m_ownedTransport;
    OBC::EPS::IEpsTransport* m_transport;
    bool m_hasValidStatus;
    OBC::EPS::StatusData m_lastStatus;
    ExplicitRefreshState m_explicitRefresh;
    OBC::EPS::PollHealthState m_pollHealth;
    U8 m_lastPduStatus;
    bool m_lowBatteryLatched;
    bool m_criticalBatteryLatched;
    bool m_overtempLatched;
};

}  // namespace OBC

#endif

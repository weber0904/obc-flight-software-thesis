#ifndef OBC_ModeManager_HPP
#define OBC_ModeManager_HPP

#include <chrono>

#include "Fw/Types/Assert.hpp"
#include "Fw/Tlm/TlmBuffer.hpp"
#include "OBC/Components/ModeManager/ModeManagerComponentAc.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"

namespace OBC {

class ModeManager final : public ModeManagerComponentBase, public OBC::IModeSafetyModeControl {
  public:
    explicit ModeManager(const char* const compName);

    ~ModeManager() override;

    void announceBoot();
    void configureOperatorTransitionGuard(const OBC::IModeOperatorTransitionGuard* guard);

    Fw::CmdResponse requestModeFromOperator(OBC::SatMode mode);

    void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) override;

    void setUptimeForTest(U32 uptimeSec);

    OBC::SatMode getModeForRuntime() const override;

    U16 getRebootCountForRuntime() const;

    U32 getUptimeForRuntime() const;

  private:
    void MODE_SET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::SatMode mode) override;

    void MODE_GET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void applyMode_(OBC::SatMode mode);

    void rejectTransition_(OBC::SatMode fromMode, OBC::SatMode toMode, OBC::ModeTransitionRejectionReason reason);

    void publishState_();
    void publishStateForModeGet_();
    void emitModeGetTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag);

    template <typename T>
    void emitModeGetTelemetryValue_(FwChanIdType channelId, const T& value, Fw::Time& timeTag) {
        Fw::TlmBuffer buffer;
        const Fw::SerializeStatus status = buffer.serializeFrom(value);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));
        this->emitModeGetTelemetry_(channelId, buffer, timeTag);
    }

    U32 uptimeSec_() const;

  private:
    bool m_bootAnnounced;
    bool m_useManualUptime;
    OBC::SatMode m_currentMode;
    U16 m_rebootCount;
    U32 m_manualUptimeSec;
    std::chrono::steady_clock::time_point m_startTime;
    const OBC::IModeOperatorTransitionGuard* m_operatorTransitionGuard;
};

}  // namespace OBC

#endif

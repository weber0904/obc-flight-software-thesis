#ifndef OBC_Components_RadioController_HPP
#define OBC_Components_RadioController_HPP

#include <memory>

#include "OBC/Components/RadioController/RadioControllerComponentAc.hpp"
#include "OBC/Components/RadioController/RadioControllerRuntime.hpp"
#include "simulators/comm/RadioTransport.hpp"

namespace OBC {

class RadioController final : public RadioControllerComponentBase {
  public:
    explicit RadioController(const char* const compName);

    ~RadioController() override;

    void configureTransport(std::unique_ptr<OBC::COMM::IRadioTransport> transport);

    void setTransportForTest(OBC::COMM::IRadioTransport* transport);

    bool pollStatusForTest();

    bool getStatusForRuntime(OBC::COMM::RadioStatus& status);

    bool getCachedStatusForRuntime(OBC::COMM::RadioStatus& status) const;

    OBC::RadioObservationState getObservationForRuntime() const;

    Fw::CmdResponse enableForRuntime(bool enabled, OBC::COMM::RadioStatus& status);

    Fw::CmdResponse setPowerForRuntime(U8 powerDbm, OBC::COMM::RadioStatus& status);

    Fw::CmdResponse setFrequencyForRuntime(U32 freqHz, OBC::COMM::RadioStatus& status);

    OBC::COMM::ByteStreamStats getLinkStatsForRuntime() const;

  private:
    static constexpr F32 OVERTEMP_THRESHOLD_C = 70.0F;
    enum class PublishMode {
        SUMMARY,
        DETAILED,
    };

    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void RADIO_ENABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enable) override;

    void RADIO_SET_POWER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 powerDbm) override;

    void RADIO_SET_FREQ_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 freqHz) override;

    void RADIO_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    bool applyStatus_(const OBC::COMM::RadioStatus& status, PublishMode publishMode);

    void publishSummaryStatus_(const OBC::COMM::RadioStatus& status);

    void publishDetailedStatus_(const OBC::COMM::RadioStatus& status);

    void updateObservationResult_(OBC::COMM::RadioTransportStatus status);

    void resetObservation_();

    OBC::RadioObservationResult mapObservationResult_(OBC::COMM::RadioTransportStatus status) const;

    Fw::CmdResponse mapStatus_(OBC::COMM::RadioTransportStatus status) const;

  private:
    std::unique_ptr<OBC::COMM::IRadioTransport> m_ownedTransport;
    OBC::COMM::IRadioTransport* m_transport;
    bool m_haveStatus;
    bool m_lastEnabled;
    U32 m_statusAgeTicks;
    OBC::COMM::RadioStatus m_lastStatus;
    OBC::RadioObservationResult m_lastObservationResult;
};

}  // namespace OBC

#endif

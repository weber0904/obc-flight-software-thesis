#ifndef OBC_COMPONENTS_GPSBRIDGE_HPP
#define OBC_COMPONENTS_GPSBRIDGE_HPP

#include <memory>
#include <string>
#include <cstdint>

#include "OBC/Components/GpsBridge/GpsBridgeComponentAc.hpp"
#include "OBC/Components/TtcPassManager/TtcPassRuntime.hpp"
#include "simulators/gps/GpsSource.hpp"
#include "simulators/gps/GpsTypes.hpp"

namespace OBC {

class GpsBridge final : public GpsBridgeComponentBase, public OBC::ITtcPassGpsProvider {
  public:
    explicit GpsBridge(const char* const compName);

    ~GpsBridge() override;

    void configureRuntime(const std::string& runtimeRoot = std::string());

    void setSentenceSourceForTest(std::unique_ptr<OBC::GPS::IGpsSentenceSource> source, OBC::GpsSourceMode mode);

    bool pollStateForTest();

    bool getCachedStateForRuntime(OBC::GPS::StateData& state) const override;

    Fw::CmdResponse setSourceModeForRuntime(OBC::GpsSourceMode mode);

  private:
    enum class SourceError : U32 {
        NOT_CONFIGURED = 1U,
        NO_SOURCE_DATA = 2U,
        REPLAY_NOT_AVAILABLE = 3U,
        LIVE_UART_NOT_AVAILABLE = 4U,
    };
    enum class PollMode : U8 {
        SCHEDULED,
        EXPLICIT_REFRESH,
    };

    void schedIn_handler(const FwIndexType portNum, U32 context) override;

    void GPS_GET_STATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void GPS_SET_SOURCE_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::GpsSourceMode mode) override;

    bool poll_(PollMode pollMode);

    bool activateSource_(OBC::GpsSourceMode mode);

    void publishSummaryTelemetry_(const OBC::GPS::StateData& state);

    void publishChangeDrivenTelemetry_(const OBC::GPS::StateData& previous, const OBC::GPS::StateData& current);

    void publishExplicitRefreshTelemetry_(const OBC::GPS::StateData& state);

    void applyValidUpdate_(const OBC::GPS::SentenceUpdate& update);

    void applyNoFixUpdate_(const OBC::GPS::SentenceUpdate& update);

  private:
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> m_fakeSource;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> m_replaySource;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> m_liveSource;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> m_testSource;
    OBC::GPS::IGpsSentenceSource* m_activeSource;
    std::string m_liveSerialDevice;
    std::uint32_t m_liveBaudrate;
    OBC::GPS::StateData m_cachedState;
    bool m_fixLatched;
};

}  // namespace OBC

#endif

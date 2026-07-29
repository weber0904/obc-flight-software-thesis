#ifndef OBC_Components_CommEgressMux_HPP
#define OBC_Components_CommEgressMux_HPP

#include "OBC/Components/CommEgressMux/CommEgressMuxComponentAc.hpp"
#include "OBC/Types/CommBandEnumAc.hpp"

namespace OBC {

class CommEgressMux final : public CommEgressMuxComponentBase {
  public:
    explicit CommEgressMux(const char* const compName);

    ~CommEgressMux() override;

    void setPrimaryTelemetryLinkForRuntime(OBC::CommBand band);

    void setPrimaryFileLinkForRuntime(OBC::CommBand band);

    void setUhfFileTransferActiveForRuntime(bool active);

    void setDiagnosticQuietPacketEgressForRuntime(bool enabled);

    void setUhfPrimaryPacketQuietForRuntime(bool enabled);

    void setBandLiveObservabilityEnabledForRuntime(OBC::CommBand band, bool enabled);

    OBC::CommBand getPrimaryTelemetryLinkForRuntime() const;

    OBC::CommBand getPrimaryFileLinkForRuntime() const;

    bool getUhfFileTransferActiveForRuntime() const;

    bool getDiagnosticQuietPacketEgressForRuntime() const;

    bool getUhfPrimaryPacketQuietForRuntime() const;

    bool getBandLiveObservabilityEnabledForRuntime(OBC::CommBand band) const;

  private:
    void publishPacketCounters_();
    void recordPacketRouted_(OBC::CommBand band, FwIndexType portNum);
    void recordPacketSuppressed_(OBC::CommBand band, FwIndexType portNum);

    bool isPacketEgressQuiet_() const;

    void packetIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void fileBufferIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void sbandFileBufferReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    void uhfFileBufferReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

  private:
    OBC::CommBand m_primaryTelemetryLink;
    OBC::CommBand m_primaryFileLink;
    bool m_uhfFileTransferActive;
    bool m_diagnosticQuietPacketEgress;
    bool m_sbandLiveObservabilityEnabled;
    bool m_uhfLiveObservabilityEnabled;
    bool m_uhfPrimaryPacketQuiet;
    U32 m_sbandRoutedEventPackets;
    U32 m_sbandRoutedTelemetryPackets;
    U32 m_sbandSuppressedEventPackets;
    U32 m_sbandSuppressedTelemetryPackets;
    U32 m_uhfRoutedEventPackets;
    U32 m_uhfRoutedTelemetryPackets;
    U32 m_uhfSuppressedEventPackets;
    U32 m_uhfSuppressedTelemetryPackets;
};

}  // namespace OBC

#endif

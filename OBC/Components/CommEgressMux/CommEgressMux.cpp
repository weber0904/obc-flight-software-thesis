#include "OBC/Components/CommEgressMux/CommEgressMux.hpp"

namespace OBC {

CommEgressMux::CommEgressMux(const char* const compName)
    : CommEgressMuxComponentBase(compName),
      m_primaryTelemetryLink(OBC::CommBand::SBAND),
      m_primaryFileLink(OBC::CommBand::SBAND),
      m_uhfFileTransferActive(false),
      m_diagnosticQuietPacketEgress(false),
      m_sbandLiveObservabilityEnabled(false),
      m_uhfLiveObservabilityEnabled(true),
      m_uhfPrimaryPacketQuiet(false),
      m_sbandRoutedEventPackets(0U),
      m_sbandRoutedTelemetryPackets(0U),
      m_sbandSuppressedEventPackets(0U),
      m_sbandSuppressedTelemetryPackets(0U),
      m_uhfRoutedEventPackets(0U),
      m_uhfRoutedTelemetryPackets(0U),
      m_uhfSuppressedEventPackets(0U),
      m_uhfSuppressedTelemetryPackets(0U) {
    this->publishPacketCounters_();
}

CommEgressMux::~CommEgressMux() = default;

void CommEgressMux::setPrimaryTelemetryLinkForRuntime(OBC::CommBand band) {
    this->m_primaryTelemetryLink = band;
    this->publishPacketCounters_();
}

void CommEgressMux::setPrimaryFileLinkForRuntime(OBC::CommBand band) {
    this->m_primaryFileLink = band;
}

void CommEgressMux::setUhfFileTransferActiveForRuntime(bool active) {
    this->m_uhfFileTransferActive = active;
}

void CommEgressMux::setDiagnosticQuietPacketEgressForRuntime(bool enabled) {
    this->m_diagnosticQuietPacketEgress = enabled;
}

void CommEgressMux::setUhfPrimaryPacketQuietForRuntime(bool enabled) {
    this->m_uhfPrimaryPacketQuiet = enabled;
}

void CommEgressMux::setBandLiveObservabilityEnabledForRuntime(OBC::CommBand band, bool enabled) {
    if (band == OBC::CommBand::SBAND) {
        this->m_sbandLiveObservabilityEnabled = enabled;
    } else {
        this->m_uhfLiveObservabilityEnabled = enabled;
    }
}

OBC::CommBand CommEgressMux::getPrimaryTelemetryLinkForRuntime() const {
    return this->m_primaryTelemetryLink;
}

OBC::CommBand CommEgressMux::getPrimaryFileLinkForRuntime() const {
    return this->m_primaryFileLink;
}

bool CommEgressMux::getUhfFileTransferActiveForRuntime() const {
    return this->m_uhfFileTransferActive;
}

bool CommEgressMux::getDiagnosticQuietPacketEgressForRuntime() const {
    return this->m_diagnosticQuietPacketEgress;
}

bool CommEgressMux::getUhfPrimaryPacketQuietForRuntime() const {
    return this->m_uhfPrimaryPacketQuiet;
}

bool CommEgressMux::getBandLiveObservabilityEnabledForRuntime(OBC::CommBand band) const {
    return band == OBC::CommBand::SBAND ? this->m_sbandLiveObservabilityEnabled : this->m_uhfLiveObservabilityEnabled;
}

void CommEgressMux::publishPacketCounters_() {
    this->tlmWrite_SBAND_ROUTED_EVENT_PACKETS(this->m_sbandRoutedEventPackets);
    this->tlmWrite_SBAND_ROUTED_TLM_PACKETS(this->m_sbandRoutedTelemetryPackets);
    this->tlmWrite_SBAND_SUPPRESSED_EVENT_PACKETS(this->m_sbandSuppressedEventPackets);
    this->tlmWrite_SBAND_SUPPRESSED_TLM_PACKETS(this->m_sbandSuppressedTelemetryPackets);
    this->tlmWrite_UHF_ROUTED_EVENT_PACKETS(this->m_uhfRoutedEventPackets);
    this->tlmWrite_UHF_ROUTED_TLM_PACKETS(this->m_uhfRoutedTelemetryPackets);
    this->tlmWrite_UHF_SUPPRESSED_EVENT_PACKETS(this->m_uhfSuppressedEventPackets);
    this->tlmWrite_UHF_SUPPRESSED_TLM_PACKETS(this->m_uhfSuppressedTelemetryPackets);
}

void CommEgressMux::recordPacketRouted_(OBC::CommBand band, FwIndexType portNum) {
    if (band == OBC::CommBand::SBAND) {
        if (portNum == 0U) {
            this->m_sbandRoutedEventPackets += 1U;
            this->tlmWrite_SBAND_ROUTED_EVENT_PACKETS(this->m_sbandRoutedEventPackets);
        } else if (portNum == 1U) {
            this->m_sbandRoutedTelemetryPackets += 1U;
            this->tlmWrite_SBAND_ROUTED_TLM_PACKETS(this->m_sbandRoutedTelemetryPackets);
        }
    } else {
        if (portNum == 0U) {
            this->m_uhfRoutedEventPackets += 1U;
            this->tlmWrite_UHF_ROUTED_EVENT_PACKETS(this->m_uhfRoutedEventPackets);
        } else if (portNum == 1U) {
            this->m_uhfRoutedTelemetryPackets += 1U;
            this->tlmWrite_UHF_ROUTED_TLM_PACKETS(this->m_uhfRoutedTelemetryPackets);
        }
    }
}

void CommEgressMux::recordPacketSuppressed_(OBC::CommBand band, FwIndexType portNum) {
    if (band == OBC::CommBand::SBAND) {
        if (portNum == 0U) {
            this->m_sbandSuppressedEventPackets += 1U;
            this->tlmWrite_SBAND_SUPPRESSED_EVENT_PACKETS(this->m_sbandSuppressedEventPackets);
        } else if (portNum == 1U) {
            this->m_sbandSuppressedTelemetryPackets += 1U;
            this->tlmWrite_SBAND_SUPPRESSED_TLM_PACKETS(this->m_sbandSuppressedTelemetryPackets);
        }
    } else {
        if (portNum == 0U) {
            this->m_uhfSuppressedEventPackets += 1U;
            this->tlmWrite_UHF_SUPPRESSED_EVENT_PACKETS(this->m_uhfSuppressedEventPackets);
        } else if (portNum == 1U) {
            this->m_uhfSuppressedTelemetryPackets += 1U;
            this->tlmWrite_UHF_SUPPRESSED_TLM_PACKETS(this->m_uhfSuppressedTelemetryPackets);
        }
    }
}

bool CommEgressMux::isPacketEgressQuiet_() const {
    return this->m_diagnosticQuietPacketEgress || this->m_uhfPrimaryPacketQuiet;
}

void CommEgressMux::packetIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    const OBC::CommBand band = this->m_primaryTelemetryLink;
    if (this->m_diagnosticQuietPacketEgress) {
        this->recordPacketSuppressed_(band, portNum);
        return;
    }
    if (!this->getBandLiveObservabilityEnabledForRuntime(band)) {
        this->recordPacketSuppressed_(band, portNum);
        return;
    }
    if (band == OBC::CommBand::UHF && this->m_uhfPrimaryPacketQuiet) {
        this->recordPacketSuppressed_(band, portNum);
        return;
    }
    if (band == OBC::CommBand::UHF) {
        if (this->m_uhfFileTransferActive) {
            return;
        }
        this->recordPacketRouted_(band, portNum);
        this->uhfPacketOut_out(portNum, data, context);
    } else {
        this->recordPacketRouted_(band, portNum);
        this->sbandPacketOut_out(portNum, data, context);
    }
}

void CommEgressMux::fileBufferIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    static_cast<void>(portNum);
    if (this->m_diagnosticQuietPacketEgress) {
        this->fileBufferReturnOut_out(0, fwBuffer);
        return;
    }
    if (this->m_primaryFileLink == OBC::CommBand::UHF) {
        this->uhfFileBufferOut_out(0, fwBuffer);
    } else {
        this->sbandFileBufferOut_out(0, fwBuffer);
    }
}

void CommEgressMux::sbandFileBufferReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    static_cast<void>(portNum);
    this->fileBufferReturnOut_out(0, fwBuffer);
}

void CommEgressMux::uhfFileBufferReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    static_cast<void>(portNum);
    this->fileBufferReturnOut_out(0, fwBuffer);
}

}  // namespace OBC

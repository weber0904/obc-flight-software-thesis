#include "OBC/Components/DpCatalogFileDownlinkGate/DpCatalogFileDownlinkGate.hpp"

namespace OBC {

DpCatalogFileDownlinkGate::DpCatalogFileDownlinkGate(const char* const compName)
    : DpCatalogFileDownlinkGateComponentBase(compName), m_havePending(false), m_pendingContext(NO_CONTEXT) {}

DpCatalogFileDownlinkGate::~DpCatalogFileDownlinkGate() = default;

Svc::SendFileResponse DpCatalogFileDownlinkGate::sendFileIn_handler(FwIndexType portNum,
                                                                    const Fw::StringBase& sourceFileName,
                                                                    const Fw::StringBase& destFileName,
                                                                    U32 offset,
                                                                    U32 length) {
    static_cast<void>(portNum);

    if (this->m_havePending) {
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_BUSY, this->m_pendingContext);
    }

    if (!this->isConnected_sendFileOut_OutputPort(0)) {
        this->clearPending_();
        return Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, NO_CONTEXT);
    }

    const Svc::SendFileResponse response = this->sendFileOut_out(0, sourceFileName, destFileName, offset, length);
    if (response.get_status() == Svc::SendFileStatus::STATUS_OK) {
        this->m_havePending = true;
        this->m_pendingContext = response.get_context();
    } else {
        this->clearPending_();
    }
    return response;
}

void DpCatalogFileDownlinkGate::fileCompleteIn_handler(FwIndexType portNum, const Svc::SendFileResponse& resp) {
    static_cast<void>(portNum);

    if (!this->m_havePending || resp.get_context() != this->m_pendingContext) {
        return;
    }

    this->clearPending_();
    if (this->isConnected_fileCompleteOut_OutputPort(0)) {
        this->fileCompleteOut_out(0, resp);
    }
}

void DpCatalogFileDownlinkGate::clearPending_() {
    this->m_havePending = false;
    this->m_pendingContext = NO_CONTEXT;
}

}  // namespace OBC

#include "OBC/Components/CspBridge/CspBridge.hpp"

namespace OBC {

CspBridge::CspBridge(const char* const compName, OBC::CSP::ICspRuntime& runtime)
    : CspBridgeComponentBase(compName), m_runtime(&runtime), m_shutdownCalled(false) {}

CspBridge::~CspBridge() {
    this->shutdownForRuntime();
}

Fw::CmdResponse CspBridge::initForRuntime(U8 nodeId) {
    const OBC::CSP::RuntimeConfig config = OBC::CSP::runtimeConfigFromEnvironment(nodeId, "OBCCSP");
    const Fw::CmdResponse response = mapStatus_(this->m_runtime->init(config));
    if (response != Fw::CmdResponse::OK) {
        this->log_WARNING_HI_CSP_ERROR(nodeId, 1U);
        this->publishCounters_();
        return response;
    }

    this->m_shutdownCalled = false;
    this->log_ACTIVITY_HI_CSP_INIT_COMPLETE(nodeId);
    this->publishCounters_();
    return response;
}

Fw::CmdResponse CspBridge::pingForRuntime(U8 targetNode, U32 timeoutMs, bool& success) {
    const Fw::CmdResponse response = mapStatus_(this->m_runtime->ping(targetNode, timeoutMs, success));
    if (response != Fw::CmdResponse::OK) {
        this->log_WARNING_HI_CSP_ERROR(targetNode, 2U);
        this->publishCounters_();
        return response;
    }

    if (success) {
        this->log_DIAGNOSTIC_CSP_PING_SUCCESS_LOCAL(targetNode, timeoutMs);
    } else {
        this->log_ACTIVITY_LO_CSP_PING_RESULT(targetNode, success, timeoutMs);
        this->log_WARNING_HI_CSP_ERROR(targetNode, 3U);
    }

    this->publishCounters_();
    return response;
}

bool CspBridge::probeNodeResponsiveForRuntime(U16 nodeId, U32 timeoutMs) {
    bool success = false;
    return this->pingForRuntime(static_cast<U8>(nodeId), timeoutMs, success) == Fw::CmdResponse::OK && success;
}

Fw::CmdResponse CspBridge::sendRawForRuntime(U8 targetNode, U8 targetPort, const std::string& data) {
    const Fw::CmdResponse response = mapStatus_(this->m_runtime->sendRaw(targetNode, targetPort, data));
    if (response != Fw::CmdResponse::OK) {
        this->log_WARNING_HI_CSP_ERROR(targetNode, response == Fw::CmdResponse::VALIDATION_ERROR ? 5U : 4U);
        this->publishCounters_();
        return response;
    }

    this->log_ACTIVITY_LO_CSP_RAW_SENT(targetNode, targetPort, static_cast<U32>(data.size()));
    this->publishCounters_();
    return response;
}

OBC::CspRuntimeCounters CspBridge::getCountersForRuntime() const {
    const OBC::CSP::RuntimeMetrics metrics = this->m_runtime->metrics();
    return OBC::CspRuntimeCounters{
        metrics.initialized,
        static_cast<U8>(metrics.localNodeId),
        metrics.txPackets,
        metrics.rxPackets,
        metrics.errorCount,
        metrics.freeBuffers,
    };
}

void CspBridge::setRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime) {
    this->m_runtime = &runtime;
    this->m_shutdownCalled = false;
}

void CspBridge::shutdownForRuntime() {
    if (this->m_shutdownCalled) {
        return;
    }
    this->m_runtime->shutdown();
    this->m_shutdownCalled = true;
}

void CspBridge::CSP_INIT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 nodeId) {
    this->cmdResponse_out(opCode, cmdSeq, this->initForRuntime(nodeId));
}

void CspBridge::CSP_PING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 targetNode, U32 timeoutMs) {
    bool success = false;
    this->cmdResponse_out(opCode, cmdSeq, this->pingForRuntime(targetNode, timeoutMs, success));
    static_cast<void>(success);
}

void CspBridge::CSP_SEND_RAW_cmdHandler(FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        U8 targetNode,
                                        U8 targetPort,
                                        const Fw::CmdStringArg& data) {
    this->cmdResponse_out(opCode, cmdSeq, this->sendRawForRuntime(targetNode, targetPort, data.toChar()));
}

Fw::CmdResponse CspBridge::mapStatus_(OBC::CSP::RuntimeStatus status) {
    switch (status) {
        case OBC::CSP::RuntimeStatus::OK:
            return Fw::CmdResponse::OK;
        case OBC::CSP::RuntimeStatus::INVALID_ARGUMENT:
            return Fw::CmdResponse::VALIDATION_ERROR;
        case OBC::CSP::RuntimeStatus::TIMEOUT:
        case OBC::CSP::RuntimeStatus::EXECUTION_ERROR:
        default:
            return Fw::CmdResponse::EXECUTION_ERROR;
    }
}

void CspBridge::publishCounters_() {
    const OBC::CspRuntimeCounters counters = this->getCountersForRuntime();
    this->tlmWrite_CSP_TX_PACKETS(counters.txPackets);
    this->tlmWrite_CSP_RX_PACKETS(counters.rxPackets);
    this->tlmWrite_CSP_ERROR_COUNT(counters.errorCount);
    this->tlmWrite_CSP_FREE_BUFFERS(counters.freeBuffers);
}

}  // namespace OBC

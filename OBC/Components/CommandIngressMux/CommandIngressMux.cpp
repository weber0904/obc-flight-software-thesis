#include "OBC/Components/CommandIngressMux/CommandIngressMux.hpp"

#include <Fw/Cmd/CmdPacket.hpp>

namespace OBC {

CommandIngressMux::CommandIngressMux(const char* compName) : CommandIngressMuxComponentBase(compName) {}

CommandIngressMux::~CommandIngressMux() = default;

void CommandIngressMux::commandIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    U32 dispatchContext = 0U;
    if (!this->rememberPendingDispatch_(portNum, context, dispatchContext)) {
        this->commandStatusOut_out(portNum, this->commandOpcode_(data), context, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->commandOut_out(0, data, dispatchContext);
}

void CommandIngressMux::commandStatusIn_handler(FwIndexType portNum,
                                                FwOpcodeType opCode,
                                                U32 cmdSeq,
                                                const Fw::CmdResponse& response) {
    static_cast<void>(portNum);
    FwIndexType ingressPort = 0U;
    U32 originalContext = cmdSeq;
    static_cast<void>(this->consumePendingDispatch_(cmdSeq, ingressPort, originalContext));
    this->commandStatusOut_out(ingressPort, opCode, originalContext, response);
}

bool CommandIngressMux::rememberPendingDispatch_(FwIndexType ingressPort, U32 originalContext, U32& dispatchContext) {
    for (PendingDispatch& pending : this->m_pendingDispatches) {
        if (!pending.used) {
            pending.used = true;
            pending.dispatchContext = this->allocateDispatchContext_();
            pending.originalContext = originalContext;
            pending.ingressPort = ingressPort;
            dispatchContext = pending.dispatchContext;
            return true;
        }
    }

    return false;
}

bool CommandIngressMux::consumePendingDispatch_(U32 dispatchContext, FwIndexType& ingressPort, U32& originalContext) {
    for (PendingDispatch& pending : this->m_pendingDispatches) {
        if (pending.used && pending.dispatchContext == dispatchContext) {
            ingressPort = pending.ingressPort;
            originalContext = pending.originalContext;
            pending = PendingDispatch();
            return true;
        }
    }
    return false;
}

U32 CommandIngressMux::allocateDispatchContext_() {
    const U32 context = this->m_nextDispatchContext;
    this->m_nextDispatchContext++;
    if (this->m_nextDispatchContext == 0U) {
        this->m_nextDispatchContext = 1U;
    }
    return context;
}

FwOpcodeType CommandIngressMux::commandOpcode_(Fw::ComBuffer data) {
    Fw::CmdPacket packet;
    if (packet.deserializeFrom(data) == Fw::FW_SERIALIZE_OK) {
        return packet.getOpCode();
    }
    return 0U;
}

}  // namespace OBC

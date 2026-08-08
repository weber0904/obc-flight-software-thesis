#ifndef OBC_Components_CommandIngressMux_HPP
#define OBC_Components_CommandIngressMux_HPP

#include "OBC/Components/CommandIngressMux/FppConstantsAc.hpp"
#include "OBC/Components/CommandIngressMux/CommandIngressMuxComponentAc.hpp"

namespace OBC {

class CommandIngressMux final : public CommandIngressMuxComponentBase {
  public:
    explicit CommandIngressMux(const char* compName);

    ~CommandIngressMux() override;

  private:
    struct PendingDispatch {
        bool used = false;
        U32 dispatchContext = 0U;
        U32 originalContext = 0U;
        FwIndexType ingressPort = 0U;
    };

  private:
    void commandIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void commandStatusIn_handler(FwIndexType portNum,
                                 FwOpcodeType opCode,
                                 U32 cmdSeq,
                                 const Fw::CmdResponse& response) override;

    bool rememberPendingDispatch_(FwIndexType ingressPort, U32 originalContext, U32& dispatchContext);

    bool consumePendingDispatch_(U32 dispatchContext, FwIndexType& ingressPort, U32& originalContext);

    U32 allocateDispatchContext_();

    static FwOpcodeType commandOpcode_(Fw::ComBuffer data);

  private:
    PendingDispatch m_pendingDispatches[CommandIngressMuxIngressPorts * 4] = {};
    U32 m_nextDispatchContext = 1U;
};

}  // namespace OBC

#endif

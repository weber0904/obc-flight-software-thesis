#ifndef OBC_CspBridge_HPP
#define OBC_CspBridge_HPP

#include <string>

#include "Fw/Cmd/CmdString.hpp"
#include "OBC/Components/CommController/CommControllerRuntime.hpp"
#include "OBC/Components/CspBridge/CspBridgeComponentAc.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace OBC {

struct CspRuntimeCounters {
    bool initialized;
    U8 localNodeId;
    U32 txPackets;
    U32 rxPackets;
    U32 errorCount;
    U32 freeBuffers;
};

class CspBridge final : public CspBridgeComponentBase, public OBC::ICommSubsystemHealthProbe {
  public:
    explicit CspBridge(const char* const compName, OBC::CSP::ICspRuntime& runtime = OBC::CSP::defaultRuntime());

    ~CspBridge() override;

    Fw::CmdResponse initForRuntime(U8 nodeId);

    Fw::CmdResponse pingForRuntime(U8 targetNode, U32 timeoutMs, bool& success);

    bool probeNodeResponsiveForRuntime(U16 nodeId, U32 timeoutMs) override;

    Fw::CmdResponse sendRawForRuntime(U8 targetNode, U8 targetPort, const std::string& data);

    OBC::CspRuntimeCounters getCountersForRuntime() const;

    void setRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime);

    void shutdownForRuntime();

  private:
    void CSP_INIT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 nodeId) override;

    void CSP_PING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 targetNode, U32 timeoutMs) override;

    void CSP_SEND_RAW_cmdHandler(FwOpcodeType opCode,
                                 U32 cmdSeq,
                                 U8 targetNode,
                                 U8 targetPort,
                                 const Fw::CmdStringArg& data) override;

    void publishCounters_();

    static Fw::CmdResponse mapStatus_(OBC::CSP::RuntimeStatus status);

  private:
    OBC::CSP::ICspRuntime* m_runtime;
    bool m_shutdownCalled;
};

}  // namespace OBC

#endif

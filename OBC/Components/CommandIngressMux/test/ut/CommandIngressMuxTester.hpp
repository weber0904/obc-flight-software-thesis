#ifndef OBC_CommandIngressMuxTester_HPP
#define OBC_CommandIngressMuxTester_HPP

#include <vector>

#include "OBC/Components/CommandIngressMux/CommandIngressMux.hpp"
#include "OBC/Components/CommandIngressMux/CommandIngressMuxGTestBase.hpp"

namespace OBC {

class CommandIngressMuxTester final : public CommandIngressMuxGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 16;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct ForwardedCommand {
        FwIndexType portNum;
        Fw::ComBuffer data;
        U32 context;
    };

    struct ForwardedStatus {
        FwIndexType portNum;
        FwOpcodeType opcode;
        U32 context;
        Fw::CmdResponse response;
    };

    CommandIngressMuxTester();

    ~CommandIngressMuxTester() override;

    void testMergesBothIngressPorts();

    void testRoutesStatusesBackToRememberedIngress();

    void testDuplicateOriginalContextsUseMuxDispatchContext();

    void testFullTableFailsClosedWithoutOverwritingExistingMappings();

  private:
    void connectPorts();

    void initComponents();

    void from_commandOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    void from_commandStatusOut_handler(FwIndexType portNum,
                                       FwOpcodeType opCode,
                                       U32 cmdSeq,
                                       const Fw::CmdResponse& response) override;

    Fw::ComBuffer makeCommand_(FwOpcodeType opcode) const;

  private:
    OBC::CommandIngressMux component;
    std::vector<ForwardedCommand> m_forwardedCommands;
    std::vector<ForwardedStatus> m_forwardedStatuses;
};

}  // namespace OBC

#endif

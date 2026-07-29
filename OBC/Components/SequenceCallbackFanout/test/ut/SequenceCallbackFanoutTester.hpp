#ifndef OBC_SEQUENCECALLBACKFANOUT_TESTER_HPP
#define OBC_SEQUENCECALLBACKFANOUT_TESTER_HPP

#include <vector>

#include "OBC/Components/SequenceCallbackFanout/SequenceCallbackFanout.hpp"
#include "OBC/Components/SequenceCallbackFanout/SequenceCallbackFanoutGTestBase.hpp"

namespace OBC {

class SequenceCallbackFanoutTester final : public SequenceCallbackFanoutGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 16;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct StartCapture {
        FwIndexType portNum;
        std::string filename;
    };

    struct DoneCapture {
        FwIndexType portNum;
        FwOpcodeType opcode;
        U32 cmdSeq;
        Fw::CmdResponse response;
    };

    SequenceCallbackFanoutTester();
    ~SequenceCallbackFanoutTester() override;

    void testSeqStartFansOutToBothSinks();
    void testSeqDoneFansOutToBothSinks();

  private:
    void connectPorts();
    void initComponents();

    void from_dispatcherSeqStartOut_handler(FwIndexType portNum, const Fw::StringBase& filename) override;
    void from_controllerSeqStartOut_handler(FwIndexType portNum, const Fw::StringBase& filename) override;
    void from_dispatcherSeqDoneOut_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) override;
    void from_controllerSeqDoneOut_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) override;

  private:
    SequenceCallbackFanout component;
    std::vector<StartCapture> m_dispatcherStarts;
    std::vector<StartCapture> m_controllerStarts;
    std::vector<DoneCapture> m_dispatcherDone;
    std::vector<DoneCapture> m_controllerDone;
};

}  // namespace OBC

#endif

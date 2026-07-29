#include "SequenceCallbackFanoutTester.hpp"

namespace OBC {

SequenceCallbackFanoutTester::SequenceCallbackFanoutTester()
    : SequenceCallbackFanoutGTestBase("SequenceCallbackFanoutTester", MAX_HISTORY_SIZE),
      component("SequenceCallbackFanout") {
    this->initComponents();
    this->connectPorts();
}

SequenceCallbackFanoutTester::~SequenceCallbackFanoutTester() = default;

void SequenceCallbackFanoutTester::testSeqStartFansOutToBothSinks() {
    this->clearHistory();
    this->m_dispatcherStarts.clear();
    this->m_controllerStarts.clear();

    this->invoke_to_seqStartIn(0, Fw::String("runtime/sequences/admitted/ctx-1.seq"));

    ASSERT_EQ(this->m_dispatcherStarts.size(), 1U);
    ASSERT_EQ(this->m_controllerStarts.size(), 1U);
    EXPECT_EQ(this->m_dispatcherStarts[0].filename, "runtime/sequences/admitted/ctx-1.seq");
    EXPECT_EQ(this->m_controllerStarts[0].filename, "runtime/sequences/admitted/ctx-1.seq");
}

void SequenceCallbackFanoutTester::testSeqDoneFansOutToBothSinks() {
    this->clearHistory();
    this->m_dispatcherDone.clear();
    this->m_controllerDone.clear();

    this->invoke_to_seqDoneIn(0, 0x1234U, 77U, Fw::CmdResponse::OK);

    ASSERT_EQ(this->m_dispatcherDone.size(), 1U);
    ASSERT_EQ(this->m_controllerDone.size(), 1U);
    EXPECT_EQ(this->m_dispatcherDone[0].opcode, 0x1234U);
    EXPECT_EQ(this->m_controllerDone[0].cmdSeq, 77U);
    EXPECT_EQ(this->m_dispatcherDone[0].response, Fw::CmdResponse::OK);
    EXPECT_EQ(this->m_controllerDone[0].response, Fw::CmdResponse::OK);
}

void SequenceCallbackFanoutTester::from_dispatcherSeqStartOut_handler(FwIndexType portNum, const Fw::StringBase& filename) {
    this->pushFromPortEntry_dispatcherSeqStartOut(filename);
    this->m_dispatcherStarts.push_back({portNum, filename.toChar()});
}

void SequenceCallbackFanoutTester::from_controllerSeqStartOut_handler(FwIndexType portNum, const Fw::StringBase& filename) {
    this->pushFromPortEntry_controllerSeqStartOut(filename);
    this->m_controllerStarts.push_back({portNum, filename.toChar()});
}

void SequenceCallbackFanoutTester::from_dispatcherSeqDoneOut_handler(FwIndexType portNum,
                                                                     FwOpcodeType opCode,
                                                                     U32 cmdSeq,
                                                                     const Fw::CmdResponse& response) {
    this->pushFromPortEntry_dispatcherSeqDoneOut(opCode, cmdSeq, response);
    this->m_dispatcherDone.push_back({portNum, opCode, cmdSeq, response});
}

void SequenceCallbackFanoutTester::from_controllerSeqDoneOut_handler(FwIndexType portNum,
                                                                     FwOpcodeType opCode,
                                                                     U32 cmdSeq,
                                                                     const Fw::CmdResponse& response) {
    this->pushFromPortEntry_controllerSeqDoneOut(opCode, cmdSeq, response);
    this->m_controllerDone.push_back({portNum, opCode, cmdSeq, response});
}

}  // namespace OBC

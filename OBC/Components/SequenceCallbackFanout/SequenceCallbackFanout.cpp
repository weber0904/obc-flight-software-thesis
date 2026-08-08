#include "OBC/Components/SequenceCallbackFanout/SequenceCallbackFanout.hpp"

namespace OBC {

SequenceCallbackFanout::SequenceCallbackFanout(const char* compName) : SequenceCallbackFanoutComponentBase(compName) {}

SequenceCallbackFanout::~SequenceCallbackFanout() = default;

void SequenceCallbackFanout::seqStartIn_handler(FwIndexType portNum, const Fw::StringBase& filename) {
    if (this->isConnected_dispatcherSeqStartOut_OutputPort(0)) {
        this->dispatcherSeqStartOut_out(0, filename);
    }
    if (this->isConnected_controllerSeqStartOut_OutputPort(0)) {
        this->controllerSeqStartOut_out(0, filename);
    }
}

void SequenceCallbackFanout::seqDoneIn_handler(FwIndexType portNum,
                                               FwOpcodeType opCode,
                                               U32 cmdSeq,
                                               const Fw::CmdResponse& response) {
    if (this->isConnected_dispatcherSeqDoneOut_OutputPort(0)) {
        this->dispatcherSeqDoneOut_out(0, opCode, cmdSeq, response);
    }
    if (this->isConnected_controllerSeqDoneOut_OutputPort(0)) {
        this->controllerSeqDoneOut_out(0, opCode, cmdSeq, response);
    }
}

}  // namespace OBC

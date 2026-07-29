#ifndef OBC_SEQUENCE_CALLBACK_FANOUT_HPP
#define OBC_SEQUENCE_CALLBACK_FANOUT_HPP

#include "OBC/Components/SequenceCallbackFanout/SequenceCallbackFanoutComponentAc.hpp"

namespace OBC {

class SequenceCallbackFanout final : public SequenceCallbackFanoutComponentBase {
  public:
    explicit SequenceCallbackFanout(const char* compName);
    ~SequenceCallbackFanout() override;

  private:
    void seqStartIn_handler(FwIndexType portNum, const Fw::StringBase& filename) override;
    void seqDoneIn_handler(FwIndexType portNum,
                           FwOpcodeType opCode,
                           U32 cmdSeq,
                           const Fw::CmdResponse& response) override;
};

}  // namespace OBC

#endif

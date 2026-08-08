module OBC {

  passive component SequenceCallbackFanout {

    sync input port seqStartIn: Svc.CmdSeqIn
    sync input port seqDoneIn: Fw.CmdResponse

    output port dispatcherSeqStartOut: Svc.CmdSeqIn
    output port controllerSeqStartOut: Svc.CmdSeqIn
    output port dispatcherSeqDoneOut: Fw.CmdResponse
    output port controllerSeqDoneOut: Fw.CmdResponse

  }

}

module OBC {

  type SequenceFileName = string size 240

  enum SequenceControlAction : U8 {
    SEQ_VALIDATE = 0
    SEQ_RUN = 1
    SEQ_PREPARE_MANUAL = 2
    SEQ_START = 3
    SEQ_STEP = 4
    SEQ_CANCEL = 5
    SEQ_LOG_STATUS = 6
  }

  enum SequenceContextState : U8 {
    UNUSED = 0
    ADMITTED = 1
    AUTO_DISPATCHED = 2
    MANUAL_PREPARED = 3
    RUNNING = 4
    SUCCEEDED = 5
    FAILED = 6
    CANCELED = 7
  }

  struct SequenceControlRequest {
    operation: SequenceControlAction
    ingressPort: U32
    linkIdentity: U32
    linkRole: U32
    originalOpcode: U32
    originalCmdSeq: U32
    fileName: SequenceFileName
    waitMode: U32
    contextId: U32
  }

  struct SequenceControlResult {
    accepted: bool
    deferred: bool
    cmdResponse: U32
    reason: U32
    contextId: U32
  }

  struct SequenceControlStatus {
    ingressPort: U32
    originalOpcode: U32
    originalCmdSeq: U32
    cmdResponse: U32
  }

  port SequenceControl(controlReq: SequenceControlRequest) -> SequenceControlResult
  port SequenceControlStatus(controlStatusArg: SequenceControlStatus)

}

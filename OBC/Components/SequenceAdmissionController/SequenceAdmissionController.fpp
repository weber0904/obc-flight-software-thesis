module OBC {

  active component SequenceAdmissionController {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus

    sync input port controlIn: SequenceControl
    output port controlStatusOut: SequenceControlStatus

    output port internalCmdOut: Fw.Com
    async input port internalCmdStatusIn: Fw.CmdResponse

    async input port seqStartIn: [2] Svc.CmdSeqIn
    async input port seqDoneIn: [2] Fw.CmdResponse

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync command SEQ_VALIDATE(fileName: SequenceFileName) opcode 0x00
    sync command SEQ_RUN(fileName: SequenceFileName, waitMode: Fw.Wait) opcode 0x01
    sync command SEQ_PREPARE_MANUAL(fileName: SequenceFileName) opcode 0x02
    sync command SEQ_START(contextId: U32) opcode 0x03
    sync command SEQ_STEP(contextId: U32) opcode 0x04
    sync command SEQ_CANCEL(contextId: U32) opcode 0x05
    sync command SEQ_LOG_STATUS() opcode 0x06

    event SEQUENCE_CONTROL_REJECTED(seqAction: SequenceControlAction, rejectCode: U32, ingressPortNum: U32, linkIdentity: U32, linkRole: U32) \
      severity warning high \
      id 0x00 \
      format "Sequence control rejected action {} reason {} ingress {} identity {} role {}" \
      throttle 10

    event SEQUENCE_CONTEXT_UPDATED(contextId: U32, contextState: SequenceContextState, sequencerPort: U32, linkIdentity: U32, linkRole: U32) \
      severity activity high \
      id 0x01 \
      format "Sequence context {} state {} sequencer {} identity {} role {}" \
      throttle 20

    event SEQUENCE_STATUS_LOG(contextId: U32, contextState: SequenceContextState, sequencerPort: U32, linkIdentity: U32, linkRole: U32, waitMode: Fw.Wait, sequenceFileName: SequenceFileName) \
      severity activity low \
      id 0x02 \
      format "Sequence context {} state {} sequencer {} identity {} role {} wait {} file {}" \
      throttle 20

    telemetry SEQ_CONTEXTS_ACTIVE: U32 id 0x00 update on change
    telemetry SEQ_LAST_CONTEXT_ID: U32 id 0x01 update on change
    telemetry SEQ_LAST_STATE: SequenceContextState id 0x02 update on change
    telemetry SEQ_LAST_REASON: U32 id 0x03 update on change
    telemetry SEQ_LAST_SEQUENCER: U32 id 0x04 update on change
    telemetry SEQ_REJECT_TOTAL: U32 id 0x05 update on change

  }

}

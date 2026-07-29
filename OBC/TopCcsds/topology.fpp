module OBCApp {

  topology App {
    import CdhCore.Subtopology
    import ComCcsds.Subtopology
    import OBCComCcsds.UhfSubtopology
    import FileHandling.Subtopology

    instance posixTime
    instance rateGroup1Comp
    instance rateGroup2Comp
    instance rateGroup3Comp
    instance rateGroupDriverComp
    instance linuxTimer
    instance groundLinkDriver
    instance uhfGroundLinkDriver
    instance groundLinkHealthProvider
    instance secureLinkAuthorizer
    instance modeManager
    instance ttcPassManager
    instance modeSafetyController
    instance epsFdirController
    instance adcsFdirController
    instance watchdogSupervisor
    instance linuxWatchdogSink
    instance rateGroup1TimingProbe
    instance cspRuntimeOwner
    instance cspBridge
    instance epsBridge
    instance adcsBridge
    instance gpsBridge
    instance storageHealthBridge
    instance payloadOpsController
    instance commController
    instance radioController
    instance uartDriver
    instance bootManager
    instance persistentFaultManager
    instance onboardStateMonitor
    instance beaconPublisher
    instance hkTrendProductProducer
    instance dpCatalogFileDownlinkGate
    instance commandIngressAuthority
    instance commandIngressMux
    instance commEgressMux
    instance fileIngressAuthority
    instance sequenceAdmissionController
    instance cmdSeqA
    instance cmdSeqB
    instance seqDispatcher
    instance seqCallbackFanoutA
    instance seqCallbackFanoutB
    instance systemResources
    instance dpCatalog
    instance dpManager
    instance dpWriter
    instance dpBufferManager
    instance recoveryExecutor

    command connections instance CdhCore.cmdDisp
    event connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    param connections instance FileHandling.prmDb
    time connections instance posixTime

    connections RateGroups {
      linuxTimer.CycleOut -> rateGroupDriverComp.CycleIn

      rateGroupDriverComp.CycleOut[Ports_RateGroups.fast] -> rateGroup1Comp.CycleIn
      rateGroup1Comp.RateGroupMemberOut[0] -> rateGroup1TimingProbe.schedIn[0]
      rateGroup1Comp.RateGroupMemberOut[1] -> rateGroup1TimingProbe.schedIn[1]
      rateGroup1Comp.RateGroupMemberOut[2] -> rateGroup1TimingProbe.schedIn[2]
      rateGroup1Comp.RateGroupMemberOut[3] -> rateGroup1TimingProbe.schedIn[3]
      rateGroup1Comp.RateGroupMemberOut[4] -> rateGroup1TimingProbe.schedIn[4]
      rateGroup1Comp.RateGroupMemberOut[5] -> rateGroup1TimingProbe.schedIn[5]
      rateGroup1Comp.RateGroupMemberOut[6] -> rateGroup1TimingProbe.schedIn[6]
      rateGroup1Comp.RateGroupMemberOut[7] -> rateGroup1TimingProbe.schedIn[7]
      rateGroup1Comp.RateGroupMemberOut[8] -> rateGroup1TimingProbe.schedIn[8]
      rateGroup1Comp.RateGroupMemberOut[9] -> rateGroup1TimingProbe.schedIn[9]
      rateGroup1Comp.RateGroupMemberOut[10] -> rateGroup1TimingProbe.schedIn[10]
      rateGroup1Comp.RateGroupMemberOut[11] -> rateGroup1TimingProbe.schedIn[11]
      rateGroup1Comp.RateGroupMemberOut[12] -> rateGroup1TimingProbe.schedIn[12]
      rateGroup1Comp.RateGroupMemberOut[13] -> rateGroup1TimingProbe.schedIn[13]
      rateGroup1Comp.RateGroupMemberOut[14] -> rateGroup1TimingProbe.schedIn[14]
      rateGroup1Comp.RateGroupMemberOut[15] -> rateGroup1TimingProbe.schedIn[15]

      rateGroup1TimingProbe.schedOut[0] -> CdhCore.tlmSend.Run
      rateGroup1TimingProbe.schedOut[1] -> ComCcsds.comQueue.run
      rateGroup1TimingProbe.schedOut[2] -> CdhCore.cmdDisp.run
      rateGroup1TimingProbe.schedOut[3] -> epsBridge.schedIn
      rateGroup1TimingProbe.schedOut[4] -> epsFdirController.schedIn
      rateGroup1TimingProbe.schedOut[5] -> modeSafetyController.schedIn
      rateGroup1TimingProbe.schedOut[6] -> adcsBridge.schedIn
      rateGroup1TimingProbe.schedOut[7] -> adcsFdirController.schedIn
      rateGroup1TimingProbe.schedOut[8] -> radioController.schedIn
      rateGroup1TimingProbe.schedOut[9] -> uartDriver.schedIn
      rateGroup1TimingProbe.schedOut[10] -> groundLinkHealthProvider.schedIn
      rateGroup1TimingProbe.schedOut[11] -> commController.schedIn
      rateGroup1TimingProbe.schedOut[12] -> ttcPassManager.schedIn
      rateGroup1TimingProbe.schedOut[13] -> watchdogSupervisor.schedIn
      rateGroup1TimingProbe.schedOut[14] -> recoveryExecutor.schedIn
      rateGroup1TimingProbe.schedOut[15] -> payloadOpsController.schedIn

      rateGroupDriverComp.CycleOut[Ports_RateGroups.slow] -> rateGroup2Comp.CycleIn
      rateGroup2Comp.RateGroupMemberOut[0] -> ComCcsds.commsBufferManager.schedIn
      rateGroup2Comp.RateGroupMemberOut[2] -> gpsBridge.schedIn
      rateGroup2Comp.RateGroupMemberOut[3] -> storageHealthBridge.schedIn
      rateGroup2Comp.RateGroupMemberOut[4] -> OBCComCcsds.commsBufferManager.schedIn
      rateGroup2Comp.RateGroupMemberOut[5] -> bootManager.schedIn
      rateGroup2Comp.RateGroupMemberOut[6] -> secureLinkAuthorizer.schedIn
      rateGroup2Comp.RateGroupMemberOut[7] -> ComCcsds.aggregator.timeout
      rateGroup2Comp.RateGroupMemberOut[8] -> OBCComCcsds.aggregator.timeout

      rateGroupDriverComp.CycleOut[Ports_RateGroups.data] -> rateGroup3Comp.CycleIn
      rateGroup3Comp.RateGroupMemberOut[0] -> onboardStateMonitor.schedIn
      rateGroup3Comp.RateGroupMemberOut[1] -> beaconPublisher.schedIn
      rateGroup3Comp.RateGroupMemberOut[2] -> hkTrendProductProducer.schedIn
      rateGroup3Comp.RateGroupMemberOut[3] -> dpManager.schedIn
      rateGroup3Comp.RateGroupMemberOut[4] -> dpWriter.schedIn
      rateGroup3Comp.RateGroupMemberOut[5] -> dpBufferManager.schedIn
      rateGroup3Comp.RateGroupMemberOut[6] -> cmdSeqA.schedIn
      rateGroup3Comp.RateGroupMemberOut[7] -> cmdSeqB.schedIn
      rateGroup3Comp.RateGroupMemberOut[8] -> FileHandling.fileManager.schedIn
      rateGroup3Comp.RateGroupMemberOut[9] -> systemResources.run
      rateGroup3Comp.RateGroupMemberOut[10] -> OBCComCcsds.comQueue.run
      rateGroup3Comp.RateGroupMemberOut[11] -> FileHandling.fileDownlink.Run
    }

    connections Communications {
      groundLinkDriver.allocate -> ComCcsds.commsBufferManager.bufferGetCallee
      groundLinkDriver.deallocate -> ComCcsds.commsBufferManager.bufferSendIn

      groundLinkDriver.$recv -> ComCcsds.comStub.drvReceiveIn
      ComCcsds.comStub.drvReceiveReturnOut -> groundLinkDriver.recvReturnIn

      ComCcsds.comStub.drvSendOut -> groundLinkDriver.$send
      groundLinkDriver.ready -> ComCcsds.comStub.drvConnected

      uhfGroundLinkDriver.allocate -> OBCComCcsds.commsBufferManager.bufferGetCallee
      uhfGroundLinkDriver.deallocate -> OBCComCcsds.commsBufferManager.bufferSendIn

      uhfGroundLinkDriver.$recv -> OBCComCcsds.comStub.drvReceiveIn
      OBCComCcsds.comStub.drvReceiveReturnOut -> uhfGroundLinkDriver.recvReturnIn

      OBCComCcsds.comStub.drvSendOut -> uhfGroundLinkDriver.$send
      uhfGroundLinkDriver.ready -> OBCComCcsds.comStub.drvConnected
    }

    connections ComCcsds_CdhCore {
      ComCcsds.fprimeRouter.commandOut -> commandIngressAuthority.seqCmdBuffIn[0]
      ComCcsds.fprimeRouter.unknownDataOut -> secureLinkAuthorizer.handshakeUplinkIn[0]
      secureLinkAuthorizer.bufferReturnOut[0] -> ComCcsds.fprimeRouter.fileBufferReturnIn
      commandIngressAuthority.seqCmdStatusOut[0] -> ComCcsds.fprimeRouter.cmdResponseIn
    }

    connections UhfComCcsds_CdhCore {
      OBCComCcsds.fprimeRouter.commandOut -> commandIngressAuthority.seqCmdBuffIn[1]
      OBCComCcsds.fprimeRouter.unknownDataOut -> secureLinkAuthorizer.handshakeUplinkIn[1]
      secureLinkAuthorizer.bufferReturnOut[1] -> OBCComCcsds.fprimeRouter.fileBufferReturnIn
      commandIngressAuthority.seqCmdStatusOut[1] -> OBCComCcsds.fprimeRouter.cmdResponseIn
    }

    connections CommandIngressAuthority_CdhCore {
      secureLinkAuthorizer.authGrantedOut[0] -> commandIngressAuthority.authGrantedIn
      secureLinkAuthorizer.authRevokedOut[0] -> commandIngressAuthority.authRevokedIn
      commandIngressAuthority.authActivityOut -> secureLinkAuthorizer.authActivityIn[0]
      commController.secureAuthInvalidateOut -> secureLinkAuthorizer.authInvalidateIn
      commandIngressAuthority.seqCmdBuffOut[0] -> commandIngressMux.commandIn[0]
      commandIngressAuthority.seqCmdBuffOut[1] -> commandIngressMux.commandIn[1]
      commandIngressAuthority.sequenceControlOut -> sequenceAdmissionController.controlIn
      sequenceAdmissionController.controlStatusOut -> commandIngressAuthority.sequenceControlStatusIn
      commandIngressMux.commandOut -> CdhCore.cmdDisp.seqCmdBuff[0]
      CdhCore.cmdDisp.seqCmdStatus[0] -> commandIngressMux.commandStatusIn
      commandIngressMux.commandStatusOut[0] -> commandIngressAuthority.seqCmdStatusIn[0]
      commandIngressMux.commandStatusOut[1] -> commandIngressAuthority.seqCmdStatusIn[1]
      sequenceAdmissionController.internalCmdOut -> CdhCore.cmdDisp.seqCmdBuff[3]
      CdhCore.cmdDisp.seqCmdStatus[3] -> sequenceAdmissionController.internalCmdStatusIn
    }

    connections OfficialSequencing {
      cmdSeqA.comCmdOut -> CdhCore.cmdDisp.seqCmdBuff[1]
      CdhCore.cmdDisp.seqCmdStatus[1] -> cmdSeqA.cmdResponseIn
      cmdSeqB.comCmdOut -> CdhCore.cmdDisp.seqCmdBuff[2]
      CdhCore.cmdDisp.seqCmdStatus[2] -> cmdSeqB.cmdResponseIn

      cmdSeqA.seqStartOut -> seqCallbackFanoutA.seqStartIn
      cmdSeqA.seqDone -> seqCallbackFanoutA.seqDoneIn
      seqCallbackFanoutA.controllerSeqStartOut -> sequenceAdmissionController.seqStartIn[0]
      seqCallbackFanoutA.controllerSeqDoneOut -> sequenceAdmissionController.seqDoneIn[0]

      cmdSeqB.seqStartOut -> seqCallbackFanoutB.seqStartIn
      cmdSeqB.seqDone -> seqCallbackFanoutB.seqDoneIn
      seqCallbackFanoutB.controllerSeqStartOut -> sequenceAdmissionController.seqStartIn[1]
      seqCallbackFanoutB.controllerSeqDoneOut -> sequenceAdmissionController.seqDoneIn[1]

    }

    connections CommEgress_CdhCore {
      CdhCore.events.PktSend -> commEgressMux.packetIn[0]
      CdhCore.tlmSend.PktSend -> commEgressMux.packetIn[1]
      modeManager.modeGetTlmOut -> CdhCore.tlmSend.TlmRecv
      bootManager.bootStatusRefreshTlmOut -> CdhCore.tlmSend.TlmRecv
      epsBridge.epsStatusRefreshTlmOut -> CdhCore.tlmSend.TlmRecv
      adcsBridge.adcsStatusRefreshTlmOut -> CdhCore.tlmSend.TlmRecv
      commController.commStatusRefreshTlmOut -> CdhCore.tlmSend.TlmRecv

      commEgressMux.sbandPacketOut[0] -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.EVENTS]
      secureLinkAuthorizer.handshakePacketOut[0] -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.HANDSHAKE]
      commEgressMux.sbandPacketOut[1] -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.TELEMETRY]
      commEgressMux.uhfPacketOut[0] -> OBCComCcsds.comQueue.comPacketQueueIn[OBCComCcsds.UhfPorts_ComPacketQueue.EVENTS]
      secureLinkAuthorizer.handshakePacketOut[1] ->
        OBCComCcsds.comQueue.comPacketQueueIn[OBCComCcsds.UhfPorts_ComPacketQueue.HANDSHAKE]
      commEgressMux.uhfPacketOut[1] -> OBCComCcsds.comQueue.comPacketQueueIn[OBCComCcsds.UhfPorts_ComPacketQueue.TELEMETRY]
    }

    connections ComCcsds_FileHandling {
      ComCcsds.fprimeRouter.fileOut -> fileIngressAuthority.bufferSendIn[0]
      OBCComCcsds.fprimeRouter.fileOut -> fileIngressAuthority.bufferSendIn[1]
      secureLinkAuthorizer.authGrantedOut[1] -> fileIngressAuthority.authGrantedIn
      secureLinkAuthorizer.authRevokedOut[1] -> fileIngressAuthority.authRevokedIn
      fileIngressAuthority.authActivityOut -> secureLinkAuthorizer.authActivityIn[1]
      commController.filePolicyOut -> fileIngressAuthority.filePolicyIn
      fileIngressAuthority.bufferSendOut -> FileHandling.fileUplink.bufferSendIn
      FileHandling.fileUplink.bufferSendOut -> fileIngressAuthority.bufferReturnIn
      fileIngressAuthority.bufferReturnOut[0] -> ComCcsds.fprimeRouter.fileBufferReturnIn
      fileIngressAuthority.bufferReturnOut[1] -> OBCComCcsds.fprimeRouter.fileBufferReturnIn
    }

    connections CommEgress_FileHandling {
      FileHandling.fileDownlink.bufferSendOut -> commEgressMux.fileBufferIn
      commEgressMux.sbandFileBufferOut -> ComCcsds.comQueue.bufferQueueIn[ComCcsds.Ports_ComBufferQueue.FILE]
      commEgressMux.uhfFileBufferOut -> OBCComCcsds.comQueue.bufferQueueIn[OBCComCcsds.UhfPorts_ComBufferQueue.FILE]
      ComCcsds.comQueue.bufferReturnOut[ComCcsds.Ports_ComBufferQueue.FILE] -> commEgressMux.sbandFileBufferReturnIn
      OBCComCcsds.comQueue.bufferReturnOut[OBCComCcsds.UhfPorts_ComBufferQueue.FILE] -> commEgressMux.uhfFileBufferReturnIn
      commEgressMux.fileBufferReturnOut -> FileHandling.fileDownlink.bufferReturn
    }

    connections Watchdog {
      epsBridge.watchdogBeatOut -> watchdogSupervisor.beatIn[0]
      epsFdirController.watchdogBeatOut -> watchdogSupervisor.beatIn[1]
      modeSafetyController.watchdogBeatOut -> watchdogSupervisor.beatIn[2]
      commController.watchdogBeatOut -> watchdogSupervisor.beatIn[3]
      adcsFdirController.watchdogBeatOut -> watchdogSupervisor.beatIn[4]
      watchdogSupervisor.watchdogFeedOut -> linuxWatchdogSink.watchdogFeedIn
    }

    connections HkTrendDataProducts {
      hkTrendProductProducer.productGetOut -> dpManager.productGetIn[0]
      hkTrendProductProducer.productSendOut -> dpManager.productSendIn[0]
      hkTrendProductProducer.productBufferReturnOut -> dpBufferManager.bufferSendIn

      payloadOpsController.productGetOut -> dpManager.productGetIn[1]
      payloadOpsController.productSendOut -> dpManager.productSendIn[1]
      payloadOpsController.productBufferReturnOut -> dpBufferManager.bufferSendIn

      dpManager.bufferGetOut[0] -> dpBufferManager.bufferGetCallee
      dpManager.bufferGetOut[1] -> dpBufferManager.bufferGetCallee
      dpManager.productSendOut[0] -> dpWriter.bufferSendIn
      dpManager.productSendOut[1] -> dpWriter.bufferSendIn
      dpWriter.dpWrittenOut -> payloadOpsController.dpWrittenIn
      dpWriter.deallocBufferSendOut -> dpBufferManager.bufferSendIn

      dpCatalog.fileOut -> commController.dpFileRequestIn
      commController.sendFileOut -> FileHandling.fileDownlink.SendFile
      FileHandling.fileDownlink.FileComplete[0] -> commController.fileCompleteIn
      commController.dpFileCompleteOut -> dpCatalog.fileDone
    }
  }

}

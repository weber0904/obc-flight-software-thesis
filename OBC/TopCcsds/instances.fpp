module OBCApp {

  enum Ports_RateGroups {
    fast
    slow
    data
  }

  module Default {
    constant QUEUE_SIZE = 10
    constant DATA_PRODUCT_QUEUE_SIZE = 64
    constant STACK_SIZE = 64 * 1024
  }

  instance posixTime: Svc.PosixTime base id 0x10000000

  instance rateGroup1Comp: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  instance rateGroup2Comp: Svc.ActiveRateGroup base id 0x10002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  instance rateGroup3Comp: Svc.ActiveRateGroup base id 0x10006000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 41

  instance rateGroupDriverComp: Svc.RateGroupDriver base id 0x10003000

  instance linuxTimer: Svc.LinuxTimer base id 0x10004000

  instance groundLinkDriver: OBC.GroundLinkDriver base id 0x10005000
  instance uhfGroundLinkDriver: OBC.GroundLinkDriver base id 0x10005100
  instance groundLinkHealthProvider: OBC.GroundLinkHealthProvider base id 0x10005200
  instance secureLinkAuthorizer: OBC.SecureLinkAuthorizer base id 0x10005300

  instance modeManager: OBC.ModeManager base id 0x10030000
  instance ttcPassManager: OBC.TtcPassManager base id 0x10030500
  instance modeSafetyController: OBC.ModeSafetyController base id 0x10039000
  instance epsFdirController: OBC.EpsFdirController base id 0x10046000
  instance adcsFdirController: OBC.AdcsFdirController base id 0x10046500
  instance watchdogSupervisor: OBC.WatchdogSupervisor base id 0x10031000
  instance linuxWatchdogSink: OBC.LinuxWatchdogSink base id 0x10031500
  instance cspRuntimeOwner: OBC.CspRuntimeOwner base id 0x10031700
  instance rateGroup1TimingProbe: OBC.RateGroupTimingProbe base id 0x10031800
  instance cspBridge: OBC.CspBridge base id 0x10032000
  instance epsBridge: OBC.EpsBridge base id 0x10033000
  instance adcsBridge: OBC.AdcsBridge base id 0x10034000
  instance gpsBridge: OBC.GpsBridge base id 0x1003B000
  instance storageHealthBridge: OBC.StorageHealthBridge base id 0x1003C000
  instance payloadOpsController: OBC.PayloadOpsController base id 0x1003A000
  instance commController: OBC.CommController base id 0x10035000
  instance radioController: OBC.RadioController base id 0x10036000
  instance uartDriver: OBC.UartDriver base id 0x10037000
  instance bootManager: OBC.BootManager base id 0x10038000
  instance persistentFaultManager: OBC.PersistentFaultManager base id 0x10038500
  instance onboardStateMonitor: OBC.OnboardStateMonitor base id 0x1003D000
  instance beaconPublisher: OBC.BeaconPublisher base id 0x1003E000
  instance hkTrendProductProducer: OBC.HkTrendProductProducer base id 0x1003F000

  instance dpCatalogFileDownlinkGate: OBC.DpCatalogFileDownlinkGate base id 0x10044000
  instance commandIngressAuthority: OBC.CommandIngressAuthority base id 0x10045000
  instance commandIngressMux: OBC.CommandIngressMux base id 0x10045500
  instance commEgressMux: OBC.CommEgressMux base id 0x10047000
  instance recoveryExecutor: OBC.RecoveryExecutor base id 0x10048000
  instance fileIngressAuthority: OBC.FileIngressAuthority base id 0x10049000
  instance sequenceAdmissionController: OBC.SequenceAdmissionController base id 0x1004A000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 21
  instance cmdSeqA: Svc.CmdSequencer base id 0x1004B000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 20
  instance cmdSeqB: Svc.CmdSequencer base id 0x1004C000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 19
  instance seqDispatcher: Svc.SeqDispatcher base id 0x1004D000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 18
  instance seqCallbackFanoutA: OBC.SequenceCallbackFanout base id 0x1004E000
  instance seqCallbackFanoutB: OBC.SequenceCallbackFanout base id 0x1004F000
  instance systemResources: Svc.SystemResources base id 0x10050000

  instance dpCatalog: Svc.DpCatalog base id 0x10040000 \
    queue size Default.DATA_PRODUCT_QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 24

  instance dpManager: Svc.DpManager base id 0x10041000 \
    queue size Default.DATA_PRODUCT_QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 23

  instance dpWriter: Svc.DpWriter base id 0x10042000 \
    queue size Default.DATA_PRODUCT_QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 22

  instance dpBufferManager: Svc.BufferManager base id 0x10043000

}

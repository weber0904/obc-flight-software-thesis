# ======================================================================
# AcConstants.fpp
# Project-local override of F Prime configuration constants
# ======================================================================

@ Number of rate group member output ports for ActiveRateGroup.
@ The active baseline now schedules RecoveryExecutor and TtcPassManager
@ in the fast group without demoting existing runtime members to slower cadence.
@ payload-ops-contract-v1 adds PayloadOpsController to the same fast-group slice.
constant ActiveRateGroupOutputPorts = 16

@ Number of rate group member output ports for PassiveRateGroup
constant PassiveRateGroupOutputPorts = 10

@ Used to drive rate groups
constant RateGroupDriverRateGroupPorts = 3

@ Used for command and registration ports.
@ hk-trend-chunked-fdp-v1 adds HkTrendProductProducer as a formal command target
@ for manual flush/status control, so leave a small amount of command-surface headroom.
constant CmdDispatcherComponentCommandPorts = 36

@ Used for uplink/sequencer buffer/response ports
constant CmdDispatcherSequencePorts = 5

@ Used for dispatching sequences to command sequencers
constant SeqDispatcherSequencerPorts = 2

@ Used for sizing the command splitter input arrays
constant CmdSplitterPorts = CmdDispatcherSequencePorts

@ Number of static memory allocations
constant StaticMemoryAllocations = 4

@ Used to ping active components
constant HealthPingPorts = 25

@ Used for broadcasting completed file downlinks
constant FileDownCompletePorts = 1

@ Used for number of Fw::Com type ports supported by Svc::ComQueue.
@ challenge-handshake-secure-command-v1 adds a dedicated handshake packet queue
@ alongside the existing events and telemetry queues.
constant ComQueueComPorts = 3

@ Used for number of Fw::Buffer type ports supported by Svc::ComQueue
constant ComQueueBufferPorts = 1

@ Used for maximum number of connected buffer repeater consumers
constant BufferRepeaterOutputPorts = 10

@ Size of port array for DpManager
constant DpManagerNumPorts = 5

@ Size of processing port array for DpWriter
constant DpWriterNumProcPorts = 5

@ The size of a file name string
constant FileNameStringSize = 200

@ The size of an assert text string
constant FwAssertTextSize = 256

@ The size of a file name in an AssertFatalAdapter event
@ Note: File names in assertion failures are also truncated by
@ the constants FW_ASSERT_TEXT_SIZE and FW_LOG_STRING_MAX_SIZE, set
@ in FpConfig.h.
constant AssertFatalAdapterEventFileSize = FileNameStringSize

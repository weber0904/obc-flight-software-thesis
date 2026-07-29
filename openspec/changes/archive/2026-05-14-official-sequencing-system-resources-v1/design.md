## Context

Active `TopCcsds` currently routes command ingress through:

- `ComCcsds.fprimeRouter.commandOut -> CommandIngressAuthority`
- `OBCComFprime.fprimeRouter.commandOut -> CommandIngressAuthority`
- `CommandIngressAuthority -> CommandIngressMux -> CdhCore.cmdDisp.seqCmdBuff[0]`

The active file path already routes:

- `ComCcsds.fprimeRouter.fileOut -> FileHandling.fileUplink.bufferSendIn`

There is no active `CmdSequencer`, `SeqDispatcher`, or `SystemResources` instance today, and the repo documentation still says generic scheduler / mission-sequence behavior is absent. This change adopts official upstream sequencing instead of adding a repo-native timed-command table, but adds repo-owned control around sequence upload and execution so official stock controls do not bypass the repo's command authority model.

## Goals

- use official `CmdSequencer` and `SeqDispatcher` as the timed-command execution engine
- govern the current active CCSDS file-uplink path before it is used for sequence staging
- require repo-owned authority admission before any externally requested sequence execution
- preserve truthful ownership and completion state for manual and automatic sequence contexts
- integrate `SystemResources` as active standard resource telemetry

## Non-Goals

- add mission-time or GPS-time scheduling semantics
- provide persistent schedule storage
- arbitrate resource conflicts between concurrent sequences
- add UHF file upload or generic unknown-packet uplink governance
- revive `MissionExecutive`

## Design Decisions

### 1. `FileIngressAuthority` owns active sequence-staging ingress policy

Insert a repo-owned `FileIngressAuthority` component between `ComCcsds.fprimeRouter.fileOut` and `FileHandling.fileUplink.bufferSendIn`.

Behavior:

- parse each incoming `Fw::FilePacket`
- only `START` may establish an accepted transfer
- validate the logical destination path carried in the `START` packet
- reject:
  - absolute paths
  - `..`
  - paths outside the allowed logical prefix
  - symlink escape relative to the configured physical staging root
- on accepted `START`, rewrite the destination path to the configured physical staging directory under the runtime root and forward the rewritten packet
- on rejected `START`, remember the active transfer as denied and drop subsequent `DATA` / `END` / `CANCEL` until the next `START`

V1 keeps the logical destination family narrow: sequence staging only.

### 2. `SequenceAdmissionController` is the only external sequence-operations owner

External comm-managed ingress must not directly use:

- `SeqDispatcher.RUN`
- `SeqDispatcher.RUN_ARGS`
- `cmdSeqA.CS_RUN`, `cmdSeqB.CS_RUN`
- raw `CS_START/STEP/AUTO/MANUAL/JOIN_WAIT/CANCEL`

Instead the repo adds a new component `SequenceAdmissionController` with governed commands:

- `SEQ_VALIDATE(file)`
- `SEQ_RUN(file, block)`
- `SEQ_PREPARE_MANUAL(file)`
- `SEQ_START(contextId)`
- `SEQ_STEP(contextId)`
- `SEQ_CANCEL(contextId)`
- bounded read/status surfaces

Admission workflow:

1. resolve the requested staged path under the governed sequence staging root
2. read and validate the official sequence file format and CRC
3. deserialize every record into a full `Fw::CmdPacket`
4. verify opcode presence and argument deserialization
5. evaluate every inner command against the caller's authority profile
6. if admitted, copy the staged file to an admission-owned immutable path
7. record a bounded context stamp for later run/manual control

### 3. TOCTOU is prevented with admitted-copy execution

The controller never runs the mutable staged file directly.

Each admitted context stores:

- context id
- source staged path
- admitted immutable path
- file size
- CRC or content hash
- caller authority identity/role/profile
- admission mode (`auto` or `manual`)
- admission timestamp
- assigned sequencer once known
- current runtime state

Execution uses only the admitted copy.

### 4. `CmdSequencer` and `SeqDispatcher` use dedicated dispatcher indices

`CmdDispatcherSequencePorts = 5`, so active routing becomes:

- ingress mux remains on `CdhCore.cmdDisp.seqCmdBuff[0]`
- `cmdSeqA.comCmdOut -> CdhCore.cmdDisp.seqCmdBuff[1]`
- `CdhCore.cmdDisp.seqCmdStatus[1] -> cmdSeqA.cmdResponseIn`
- `cmdSeqB.comCmdOut -> CdhCore.cmdDisp.seqCmdBuff[2]`
- `CdhCore.cmdDisp.seqCmdStatus[2] -> cmdSeqB.cmdResponseIn`
- `SequenceAdmissionController.internalCmdOut -> CdhCore.cmdDisp.seqCmdBuff[3]`
- `CdhCore.cmdDisp.seqCmdStatus[3] -> SequenceAdmissionController.internalCmdStatusIn`

Index `4` stays unused for future work.

This keeps stock sequence execution out of the SBAND/UHF ingress mux and preserves truthful per-source `cmdSeq` status correlation.

### 5. Controller-driven stock command synthesis

The controller uses the reserved dispatcher source index to synthesize stock command packets for:

- `SeqDispatcher.RUN`
- `cmdSeqA` / `cmdSeqB` manual controls
- `cmdSeqA` / `cmdSeqB` cancel and mode controls when required

Direct comm-managed calls to those stock commands are denied by `CommandIngressAuthority`, but controller-issued internal packets bypass that deny by using the dedicated internal dispatcher source rather than the external authority path.

### 6. Sequencer lifecycle fanout keeps ownership truth

Each sequencer has one repo-owned fanout helper so its callbacks reach both:

- `SeqDispatcher`
- `SequenceAdmissionController`

The controller uses `seqStartOut` and `seqDone` to:

- bind a context to the sequencer that actually started it
- preserve owner/profile association
- update manual/auto run status truthfully
- enforce backup ownership limits on cancel/start/step

### 7. Backup semantics stay bounded

`uhf-backup` may:

- validate a sequence
- run a sequence only if every inner command is backup-allowed
- manually step only a context admitted under the same backup ownership

`uhf-backup` may not:

- run a sequence containing higher-authority opcodes
- mutate another profile's context
- directly invoke stock sequencer or dispatcher controls

### 8. Timing truth stays official and hosted-bounded

`CmdSequencer` uses the deployment time source from `posixTime`.

Supported semantics are the official upstream ones:

- immediate
- relative
- absolute

Truth statement for this repo:

- absolute-time correctness is only claimed for hosted environments where POSIX wallclock is already correct
- this is not GPS mission time, not RTC proof, and not TTC pass-window scheduling

### 9. Sequencer scheduling and timeout

Current hosted runtime default base tick is `1000 ms`.
Current active slow group uses divisor `5`, so it is too slow for truthful sequence timing claims.

V1 will:

- schedule `cmdSeqA.schedIn`, `cmdSeqB.schedIn`, `FileHandling.fileManager.schedIn`, and `systemResources.run` on a 1 Hz path
- declare the worst-case dispatch latency as one base tick
- configure both sequencers with a repo-owned nonzero command timeout during topology setup

### 10. `SystemResources` is additive but preferred

`SystemResources` becomes the standard resource telemetry surface in active topology.
It does not replace:

- `WatchdogSupervisor`
- custom runtime resource sampling
- storage-health policy owners

`SystemResources.ENABLE` is treated as a controlled configuration surface and is not backup-writable by default.

## Verification Strategy

- classic F' L2 harness for each new real component:
  - `FileIngressAuthority`
  - `SequenceAdmissionController`
  - lifecycle fanout helper if it derives from `*ComponentBase`
- L1 helper tests for:
  - sequence admission parsing/CRC/record decoding
  - admitted-copy state and TOCTOU protection
  - logical-path normalization and physical-root resolution
  - synthetic stock command packet construction
- hosted probe proving:
  - active CCSDS file upload into governed staging
  - validate/run/manual/cancel wrapper behavior
  - dispatcher use across both sequencers
  - timeout abort
  - backup authority limits
  - `SystemResources` telemetry presence

# official-sequencing-system-resources-v1 Test Record

## Scope

- Branch: `feature/official-sequencing-system-resources-v1`
- Base commit: `bce5504`
- Final local commit SHA: branch worktree closeout state before push
- OpenSpec change: `official-sequencing-system-resources-v1`
- Date: 2026-05-14

This record closes the first active-baseline official F Prime sequencing and system-resource telemetry integration on hosted `TopCcsds`.

The active runtime claims proven here are:

- official `Svc::CmdSequencer`, `Svc::SeqDispatcher`, and `Svc::SystemResources` are part of active `TopCcsds`
- the current active CCSDS S-band file-uplink path now has a repo-owned `FileIngressAuthority` gate before `Svc::FileUplink`
- only the governed sequence staging destination is enabled on that active file path in v1
- `SequenceAdmissionController` is the only external sequence-operations owner
- direct external `SeqDispatcher.RUN`, `SeqDispatcher.RUN_ARGS`, and raw `CmdSequencer` `CS_*` controls are denied on comm-managed ingress
- admitted sequence execution uses immutable admitted copies, not mutable staged files
- two official sequencers execute through dedicated `CmdDispatcher` source indices with truthful status routing
- manual sequence execution restores the selected sequencer back to `AUTO` before later auto-run work reuses it
- `uhf-backup` may run only backup-allowed sequences and may not mutate `SystemResources.ENABLE`
- hosted absolute-time sequence truth is bounded to POSIX wallclock correctness plus one base tick of dispatch latency

## Out Of Scope

- mission scheduler behavior
- persistent onboard schedule storage
- GPS mission-time scheduling or RTC proof
- payload planner or payload framework integration
- generic file-uplink governance for all links or unknown packet routes
- conflict-free multi-sequence arbitration
- target / Raspberry Pi deployment closure

## Commands

Commands run from `$REPO_ROOT`:

```bash
$REPO_ROOT/fprime-venv/bin/cmake --build $REPO_ROOT/build-fprime-automatic-native -j4 --target OBC
./build-fprime-automatic-native/bin/Darwin/OBC_Components_FileIngressAuthority_ut_exe
./build-fprime-automatic-native/bin/Darwin/OBC_Components_SequenceCallbackFanout_ut_exe
./build-fprime-automatic-native/bin/Darwin/OBC_Components_SequenceAdmissionController_ut_exe
./build-fprime-automatic-native/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
bash scripts/run_official_sequencing_system_resources_v1_probe.sh
openspec validate official-sequencing-system-resources-v1
openspec validate --specs
```

## Results

- Fresh local OBC build: PASS
- Focused component/helper tests: PASS for:
  - `OBC_Components_FileIngressAuthority_ut_exe`
  - `OBC_Components_SequenceCallbackFanout_ut_exe`
  - `OBC_Components_SequenceAdmissionController_ut_exe`
  - `OBC_Components_CommandIngressAuthority_ut_exe`
- Hosted official sequencing / system-resources probe: PASS at `/tmp/official-sequencing-system-resources-v1.oUHfai`
- `openspec validate official-sequencing-system-resources-v1`: PASS
- `openspec validate --specs`: PASS

## Focused Coverage

### 1. File ingress governance

`OBC_Components_FileIngressAuthority_ut_exe` and direct helper tests prove:

- allowed logical sequence-staging leaf paths are rewritten into the governed runtime staging root
- absolute-path, parent-traversal, nested-path, and symlink-escape attempts fail closed
- once a `START` packet is denied, later `DATA` / `END` / `CANCEL` packets are dropped until the next `START`

### 2. Sequence admission and lifecycle ownership

`OBC_Components_SequenceAdmissionController_ut_exe` proves:

- official sequence file CRC and record deserialization are enforced before admission
- unknown/high-authority inner commands are rejected before execution
- admitted-copy workflow protects execution from staged-file overwrite TOCTOU
- manual prepare/start/step uses the admitted copy, preserves owner checks, and restores the sequencer to `AUTO`
- failed inner command completion reports `FAILED` truthfully without corrupting controller state

### 3. Stock-control callback fanout

`OBC_Components_SequenceCallbackFanout_ut_exe` proves:

- sequencer `seqStart` and `seqDone` callbacks fan out to both `SeqDispatcher` and `SequenceAdmissionController`
- controller ownership/status tracking does not require direct rewiring around the official dispatcher

### 4. Command authority boundary

`OBC_Components_CommandIngressAuthority_ut_exe` proves:

- direct external `SeqDispatcher.RUN` is denied
- wrapper sequence commands route only to `SequenceAdmissionController`
- governed sequence wrapper commands still preserve envelope/session/sequence/auth behavior on the active command path
- `SystemResources.ENABLE` remains a controlled runtime surface and is not backup-writable

## Hosted Probe

Repository-owned proof entry point:

```bash
bash scripts/run_official_sequencing_system_resources_v1_probe.sh
```

Latest passing run after hosted harness stabilization:

```text
official_sequencing_system_resources_v1_probe: PASS
file-accept-attempt-1: PASS
file-accept-attempt-1-root=/tmp/official-sequencing-system-resources-v1.oUHfai/file-accept-attempt-1
file-accept-attempt-1-destination=.sequence-staging/readable.bin
file-accept-attempt-1-staged=/tmp/official-sequencing-system-resources-v1.oUHfai/file-accept-attempt-1/runtime/sequences/staging/readable.bin
file-reject-absolute: PASS
file-reject-absolute-root=/tmp/official-sequencing-system-resources-v1.oUHfai/file-reject-absolute
file-reject-absolute-destination=/tmp/abs-denied.txt
file-reject-parent: PASS
file-reject-parent-root=/tmp/official-sequencing-system-resources-v1.oUHfai/file-reject-parent
file-reject-parent-destination=.sequence-staging/../parent-denied.txt
file-reject-prefix: PASS
file-reject-prefix-root=/tmp/official-sequencing-system-resources-v1.oUHfai/file-reject-prefix
file-reject-prefix-destination=other-prefix/denied.txt
sband-stock-denies: PASS
sband-stock-denies-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-stock-denies
sband-stock-denies-attempts=1
sband-stock-denies-result=direct stock seqDispatcher/cmdSeq controls denied on hosted node-5 ingress
sband-dual-dispatch: PASS
sband-dual-dispatch-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-dual-dispatch
sband-dual-dispatch-attempts=2
sband-dual-dispatch-runtime-sec=3.334
sband-dual-dispatch-result=wrapper validate/run and dual-sequencer dispatch succeeded
sband-dual-dispatch-system-resources=CPU and MEMORY_TOTAL channels observed
sband-manual-smoke: PASS
sband-manual-smoke-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-manual-smoke
sband-manual-smoke-attempts=3
sband-manual-smoke-context=1
sband-manual-smoke-sequence=direct-staged manual prepare/start/step completed on hosted node-5 path
sband-manual-toctou: PASS
sband-manual-toctou-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-manual-toctou
sband-manual-toctou-attempts=1
sband-manual-toctou-context=1
sband-manual-toctou-result=manual admitted copy still executed after overwriting the staged manual leaf
sband-failure: PASS
sband-failure-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-failure
sband-failure-attempts=2
sband-failure-result=COMM_START_PASS 0 inner command failed truthfully without corrupting controller state
sband-cancel: PASS
sband-cancel-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-cancel
sband-cancel-attempts=1
sband-cancel-result=wrapper cancel completed on active hosted node-5 path
sband-absolute: PASS
sband-absolute-root=/tmp/official-sequencing-system-resources-v1.oUHfai/sband-absolute
sband-absolute-attempts=1
sband-absolute-result=hosted POSIX absolute sequence fired within the declared one-tick bound
uhf-backup: PASS
uhf-root=/tmp/official-sequencing-system-resources-v1.oUHfai/uhf-backup
uhf-read-only-sequence=MODE_GET admitted and completed through wrapper
uhf-high-authority-sequence=MODE_SET rejected before execution
uhf-system-resources-enable=backup write denied
```

This passing rerun keeps the same formal claim as the original change, but it
also records the shared hosted harness stabilization that later payload v2
probes now reuse:

- `SEQ_VALIDATE` on the wrapper surface is treated as a bounded non-reject
  preflight; `SEQ_RUN` is the formal end-to-end sequence proof
- command send and file uplink now wait for `GROUND_LINK_UP` plus a short
  `GROUND_LINK_*` quiet window before each bounded subcase
- the repo-owned probe no longer tears down and reconnects the GDS API between
  every file-ingress subcase

This probe proves:

- active CCSDS file upload into the governed sequence staging path
- runtime-scoped internal staging/admission aliases are used; no legacy checkout-root `.sequence-staging` or `.sequence-admitted` alias is created
- direct stock sequence controls are denied on the comm-managed ingress path
- official dispatcher distribution across both sequencers is active and reviewable
- manual sequence stepping restores the selected sequencer to `AUTO`
- a deterministic inner command failure (`COMM_START_PASS 0`) produces truthful `CS_CommandError` / failed context state
- hosted absolute-time sequencing uses POSIX wallclock and stays within the declared one-tick bound
- official `SystemResources` channels are present while existing custom telemetry continues to coexist

This probe does **not** prove:

- target/Pi sequencing behavior
- GPS mission-time scheduling
- resource arbitration between concurrent sequences
- generic file-uplink governance for non-sequence destinations or other links

## Verification Path Use

- Reused path: current hosted CCSDS S-band `OBC -> GDS` baseline and hosted authenticated command ingress path
- Newly proven path: active hosted CCSDS file-uplink to governed sequence staging plus admitted official sequence execution through `SequenceAdmissionController` / `SeqDispatcher` / `CmdSequencer`

## Non-Claims

- No repo-native timed-command table was added.
- No persistent mission planner or onboard schedule database was added.
- No payload-specific scheduler consumer was added.
- No UHF file-uplink governance path was added.

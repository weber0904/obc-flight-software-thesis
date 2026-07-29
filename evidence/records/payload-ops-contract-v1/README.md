# payload-ops-contract-v1 Test Record

> Historical evidence note: this record captures the accepted `2026-05-20`
> payload-contract-v1 baseline. The repository-owned compatibility probes under
> `scripts/run_payload_ops_contract_v1_{hosted,target}_probe.sh` are retained
> only as historical wrappers. Current `main` no longer exposes
> `PAYLOAD_CAPTURE_STILL`, so do not treat live reruns of those wrappers as
> current maintained payload truth. Treat the concrete
> `capture-<boot>-<count>` excerpts below as historical evidence for that
> earlier boundary, not as the current payload local-artifact contract.

Current-note:

- current maintained payload normal still-capture policies are `AUTO` and
  `DETERMINISTIC` only
- current persistent-session lifecycle and `PAYLOAD_CAPTURE_STILL` retirement
  closure are governed by
  [payload-persistent-session-still-retirement-v1](../payload-persistent-session-still-retirement-v1/README.md)
- current target source-image validity and actual `AUTO` metadata truth are
  governed by
  [payload-target-capture-sanity-v1](../payload-target-capture-sanity-v1/README.md)

## Scope

- Branch: `feature/payload-ops-contract-v1`
- Base commit: `d412f8279f67e1977303b5e0944aaf695be8e5b4`
- Final local commit SHA: branch worktree closeout state before push
- OpenSpec change: `payload-ops-contract-v1`
- Date: 2026-05-20

This record closes the first active-baseline camera payload contract for the
OV5647-based Raspberry Pi CSI camera.

The active runtime claims proven here are:

- `PayloadOpsController` is the only public payload command/event/telemetry
  owner on the active `TopCcsds` baseline
- payload execution reuses the existing official sequencing surface; it does
  not add a scheduler or a second control plane
- `PAYLOAD` mode entry itself remains side-effect free; actual camera work
  starts only from explicit `PAYLOAD_*` commands
- payload power is modeled explicitly as a lab proxy through EPS simulator PDU
  channel `3`; this does not claim a physically switched EPS rail
- payload capture artifacts are stored under
  `<runtime-root>/persistent-data/payload/camera/` using deterministic
  `capture-<bootCount>-<captureCount>.jpg` naming
- hosted proof and target proof are recorded separately

## Out Of Scope

- onboard scheduler or time-tagged mission planner
- generic multi-payload registry/framework
- payload autonomy or ADCS pointing logic
- payload data-product or downlink closure
- final storage-retention policy
- target timing/WCET closure
- physically switched EPS payload rail proof

## Commands

Commands run from `$REPO_ROOT` unless noted otherwise:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j 8
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut -j 8
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
bash scripts/run_payload_ops_contract_v1_hosted_probe.sh
/bin/zsh -lc "RPI_REMOTE_DIR=$OBC_HOME/lab/fprime/v0-hk-verify bash scripts/sync_rpi_workspace.sh"
ssh operator@<private-lab-host> 'cd $OBC_HOME/lab/fprime/v0-hk-verify && export PATH=$PWD/fprime-venv/bin:$PATH && fprime-util generate -f && fprime-util build'
cd $REPO_ROOT && RPI_REMOTE_DIR=$OBC_HOME/lab/fprime/v0-hk-verify bash scripts/run_payload_ops_contract_v1_target_probe.sh
openspec validate payload-ops-contract-v1
openspec validate --specs
```

## Results

- Fresh local native build: PASS
- Fresh local unit-test build: PASS
- `OBC_Components_PayloadOpsController_ut_exe`: PASS
- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS
- `hosted_runtime_unit_test`: PASS
- Hosted CCSDS node-`5` payload probe:
  - canonical completed PASS at `/tmp/payload-ops-contract-v1-hosted.nTwF7H`
  - fresh rebuild-session rerun reproduced the same payload sequence, status,
    proxy-channel, and capture-file evidence at
    `/tmp/payload-ops-contract-v1-hosted.83UyG8`
  - cleanup-hardened rerun PASS at
    `/tmp/payload-ops-contract-v1-hosted.M6jjfh`
  - closeout fresh-binary rerun PASS at
    `/tmp/payload-ops-contract-v1-hosted.6UR32d`
  - review-fix rerun PASS at
    `/tmp/payload-ops-contract-v1-hosted.3a0ZM6`
- Raspberry Pi target payload-ops rerun on the Pi-local direct adapter path:
  - canonical completed PASS at
    `obc.local:/tmp/payload-ops-contract-v1-target.VCkaHs`
  - cleanup-hardened rerun PASS at
    `obc.local:/tmp/payload-ops-contract-v1-target.w1U09D`
- `openspec validate payload-ops-contract-v1`: PASS
- `openspec validate --specs`: PASS

## Focused Coverage

### 1. Payload component and manager behavior

`OBC_Components_PayloadOpsController_ut_exe` proves:

- explicit-prepare requirement before `PAYLOAD_CAPTURE_STILL`
- `PAYLOAD`-mode gating and rejection outside `PAYLOAD`
- deterministic capture-path reporting through
  `persistent-data/payload/camera/capture-<boot>-<count>.jpg`
- forced mode-exit cleanup back to `OFF`
- manager-level abort-during-prepare and capture-failure cleanup behavior

### 2. Authority labeling and readback boundary

`OBC_Components_CommandIngressAuthority_ut_exe` proves:

- payload mutating commands use governed payload-control classification and the
  `PAYLOAD` resource label
- `PAYLOAD_GET_STATUS` remains a read/status surface
- restricted backup ingress may read payload status but may not mutate payload
  state

### 3. Hosted detector defaults used by the governed CCSDS proof path

`hosted_runtime_unit_test` proves the hosted runtime now defaults to:

- `commSubsystemPingTimeoutMs = 500`
- `commPrimaryUnavailableFailureThreshold = 10`

This keeps the default hosted CCSDS node-`5` path aligned with the current
proof configuration for this payload change.

## Hosted Contract Proof

Repository-owned proof entry point:

```bash
bash scripts/run_payload_ops_contract_v1_hosted_probe.sh
```

Hosted proof path:

```text
fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> OBC
```

Canonical completed PASS directory:

```text
/tmp/payload-ops-contract-v1-hosted.nTwF7H
```

Fresh rebuild-session corroborating rerun:

```text
/tmp/payload-ops-contract-v1-hosted.83UyG8
```

Cleanup-hardened corroborating rerun:

```text
/tmp/payload-ops-contract-v1-hosted.M6jjfh
```

Closeout fresh-binary rerun:

```text
/tmp/payload-ops-contract-v1-hosted.6UR32d
```

Review-fix rerun:

```text
/tmp/payload-ops-contract-v1-hosted.3a0ZM6
```

Observed hosted payload sequence excerpts from the fresh rerun:

```text
COMMAND_SESSION_OPENED : ... session 9101
SYS_MODE_CHANGE : System mode changed to IDLE
SYS_MODE_CHANGE : System mode changed to PAYLOAD
SEQUENCE_CONTEXT_UPDATED : Sequence context 1 state RUNNING
PAYLOAD_PROXY_POWER_CHANGED : Payload proxy power 1 channel 3
PAYLOAD_STATUS : Payload status state PSTATE_READY ... capture 0
PAYLOAD_CAPTURED : Payload captured still 1 path persistent-data/payload/camera/capture-1-1.jpg
PAYLOAD_STATUS : Payload status state PSTATE_READY ... capture 1 path persistent-data/payload/camera/capture-1-1.jpg
PAYLOAD_PROXY_POWER_CHANGED : Payload proxy power 0 channel 3
PAYLOAD_STATUS : Payload status state PSTATE_OFF ... capture 1 path persistent-data/payload/camera/capture-1-1.jpg
SEQUENCE_CONTEXT_UPDATED : Sequence context 1 state SUCCEEDED
```

Hosted proof proves:

- authenticated session-open plus official sequencing can consume
  `PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_STILL -> PAYLOAD_SHUTDOWN`
- payload proxy power channel `3` is surfaced as governed observability
- status/event readback reports the deterministic relative capture path
- the capture artifact exists under the governed runtime root
- the hosted capture artifact is generated by the stub payload driver, not by a
  real Raspberry Pi camera sensor

Hosted probe hygiene note:

- the hosted probe now preflight-cleans stale
  `payload-ops-contract-v1-hosted.*` GDS children before launch
- teardown now terminates full spawned process groups and escalates to
  `SIGKILL` if graceful shutdown fails
- teardown also removes detached hosted-workspace `fprime_gds` `comm` and
  `tcpserver` children that can survive after `fprime-gds` exits

Hosted proof does **not** prove:

- real `libcamera` sensor interaction
- Raspberry Pi hardware power/init timing
- a physically switched EPS camera rail

Current target-backend caveat:

- the real `libcamera` prepare path now enforces `initTimeoutMs` as a
  stage-by-stage deadline across the normal initialization sequence
- this does not claim recovery from a lower-level hard hang inside a single
  blocking `libcamera` or kernel call that never returns

## Raspberry Pi Target Payload-ops Rerun On The Pi-local Adapter Path

Repository-owned proof entry point:

```bash
cd $REPO_ROOT && RPI_REMOTE_DIR=$OBC_HOME/lab/fprime/v0-hk-verify bash scripts/run_payload_ops_contract_v1_target_probe.sh
```

Target proof path:

```text
fprime-cli -> local fprime-gds on the Raspberry Pi -> direct target OBC adapter path
```

Verification workspace note:

- the `RPI_REMOTE_DIR=$OBC_HOME/lab/fprime/v0-hk-verify` override is an
  operator-chosen isolated Raspberry Pi workspace name inherited from older
  housekeeping verification, not the payload probe script default

Canonical completed PASS directory on the Pi:

```text
obc.local:/tmp/payload-ops-contract-v1-target.VCkaHs
```

Cleanup-hardened corroborating rerun:

```text
obc.local:/tmp/payload-ops-contract-v1-target.w1U09D
```

Observed target payload excerpts:

```text
COMMAND_SESSION_OPENED : ... session 9201
SYS_MODE_CHANGE : System mode changed to IDLE
SYS_MODE_CHANGE : System mode changed to PAYLOAD
SEQUENCE_CONTEXT_UPDATED : Sequence context 1 state RUNNING
PAYLOAD_PROXY_POWER_CHANGED : Payload proxy power 1 channel 3
PAYLOAD_STATUS : Payload status state PSTATE_READY ... capture 0
PAYLOAD_CAPTURED : Payload captured still 1 path persistent-data/payload/camera/capture-1-1.jpg
PAYLOAD_STATUS : Payload status state PSTATE_READY ... capture 1 path persistent-data/payload/camera/capture-1-1.jpg
PAYLOAD_PROXY_POWER_CHANGED : Payload proxy power 0 channel 3
PAYLOAD_STATUS : Payload status state PSTATE_OFF ... capture 1 path persistent-data/payload/camera/capture-1-1.jpg
SEQUENCE_CONTEXT_UPDATED : Sequence context 1 state SUCCEEDED
```

Current rerun proves:

- the Pi-local direct target adapter path can consume the governed
  `PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_STILL -> PAYLOAD_SHUTDOWN` sequence
- proxy EPS channel `3` notification and deterministic artifact-path reporting
  under the governed persistent-data root
- probe cleanup hardening works on the Raspberry Pi verification workspace

Current rerun does **not** prove:

- target/lab node-`5` COMM ingress
- real OV5647 sensor capture
- a target build that actually enabled the `libcamera` backend
- a separately target-proven real-backend abort timing boundary
- payload downlink closure or target timing/WCET closure

Target probe hygiene note:

- the target probe now preflight-cleans stale
  `payload-ops-contract-v1-target.*` state plus verification-workspace GDS
  child processes before launch
- teardown terminates full spawned process groups, escalates to `SIGKILL`, and
  also removes detached `fprime_gds` `comm` and `tcpserver` children scoped to
  the verification workspace
- post-rerun process inspection confirmed the verification workspace no longer
  left orphaned `fprime_gds` Python children on the Raspberry Pi

## Verification Path Use

- Reused hosted path: registry entries 43 plus hosted authenticated
  envelope/session lifecycle evidence on the default CCSDS S-band node-`5`
  path
- Newly proven hosted behavior: payload contract and official sequencing
  consumption on that hosted node-`5` path
- Reused target path: registry entry 2 for the Raspberry Pi direct
  `OBC -> GDS` adapter path
- Newly proven target behavior in the currently recorded rerun: governed
  payload storage/output behavior and cleanup-hardened Pi-local sequencing on
  the Raspberry Pi direct adapter path

Observed backend caveat:

- the currently recorded `v0-hk-verify` target workspace rerun did not detect
  `libcamera` in its build configuration, so its `capture-1-1.jpg` artifact is
  a stub-generated JPEG rather than a real OV5647 sensor image
- after the later PR review fixes in `LibcameraPiCameraDriver.cpp`,
  `PiCameraManager.cpp`, and `PayloadOpsController.cpp`, the repository reran
  the fresh local gate plus the hosted CCSDS probe, but did not rerun the
  target path because the target hardware setup was not connected at that time

## Non-Claims

- No onboard scheduler or mission planner was added.
- No generic payload registry/framework was added.
- No payload-specific data product or downlink closure was added.
- No final physical EPS-switched payload rail was added.

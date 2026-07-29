# target-nonquiet-background-tm-stability-v1 Evidence

Current-note:

- this record is kept for oracle-rationale and degraded/non-quiet observability
  analysis
- any `SESSION_OPEN` wording in the retained artifacts is historical boundary
  language and must not be cited as current secure-auth truth for Chapter 5 or
  maintained target auth closure

Date:
- `2026-05-27`

OpenSpec change:
- `target-nonquiet-background-tm-stability-v1`

## Scope

This record diagnoses the remaining target/lab node-`6` background-telemetry
question on the active CAN-backed lab baseline.

Path under test:

- macOS `fprime-gds`
- macOS `ground_ttc_gateway` raw relay
- `subsystem.local` S-band node `5`, UHF node `6`, EPS, and ADCS
- `obc.local` OBC over shared SocketCAN
- target/lab UHF physical serial southbound on node `6`

This record does not claim:

- general target/lab non-quiet node-`6` closure
- simultaneous dual-link orchestration
- a productized second GDS or debug side channel
- RF or over-the-air behavior
- UHF reliable transfer

## Probe Changes In This Change

Branch-local diagnosis ownership now includes:

- [scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh]($REPO_ROOT/scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh)
- [scripts/comm_verification/lib/run_target_can_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_can_matrix_probe.py)
- [scripts/run_subsystem_csp_service.sh]($REPO_ROOT/scripts/run_subsystem_csp_service.sh)
- [scripts/comm_verification/lib/run_target_tcp_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_tcp_matrix_probe.py)
- [scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh]($REPO_ROOT/scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh)

What changed in the harness:

- added a dedicated target CAN node-`6` non-quiet diagnosis wrapper instead of
  widening the meaning of the existing quiet matrix cases
- narrowed the target CAN node-`6` control boundary from pre-switch
  `CSP ping node 6 success` to post-switch journal-first target command truth
- added branch-local summary artifacts for gateway byte captures, journal
  snapshots, channel snapshots, and beacon/debug capture
- enabled repo-owned ingress diagnostics on the subsystem UHF node so the
  non-quiet run can record whether background serial chunks are still reaching
  node `6`
- corrected the old non-quiet hard-fail oracle: post-command beacon growth is
  now a bounded observation, not a proxy for command success
- tightened cleanup so repository-owned probes remove owned beacon helpers and
  stale `pty_pair_bridge` processes instead of relying on ad hoc manual cleanup

No product runtime behavior was intentionally changed in this change. The
implementation stayed at probe/oracle/tooling scope.

## Governed Diagnosis Results

### Supporting Comparator: target TCP node-`6`

Verdict: `PASS`

| Field | Value |
|---|---|
| command | `bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` |
| artifact root | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/target-tcp-comparator` |
| path | `fprime-gds -> ground_ttc_gateway(raw relay) -> TCP stand-in southbound -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC` |

Observed PASS markers:

```text
target-tcp-matrix-probe: PASS
mode=command
profile=uhf-primary
sband-session-source=obc-log
switch-source=obc-log
uhf-session-source=obc-log
uhf-command-source=obc-log
```

This comparator proves the higher-level target session and command policy is
not generically broken across all target-bearing node-`6` paths.

### Quiet Control: target CAN node-`6`

Verdict: `PASS`

| Field | Value |
|---|---|
| command | `bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` |
| artifact root | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/quiet-control` |
| path | `fprime-gds -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC` |

Observed PASS markers:

```text
target-can-matrix-probe: PASS
mode=command
profile=uhf-primary
sband-session-source=target-journal
pre-switch-node6-ping=not-required
switch-source=target-journal
uhf-session-source=target-journal
uhf-command-source=target-journal
```

What this proves:

- the old pre-switch `CSP ping node 6 success` gate was not the correct proof
  boundary for this change
- with the narrower post-switch journal-first boundary, target CAN quiet
  node-`6` can still switch, open session, and complete a bounded command

### Non-Quiet Diagnosis: target CAN node-`6`

Verdict: `PASS`

| Field | Value |
|---|---|
| command | `bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` |
| artifact root | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/nonquiet-diagnosis` |
| summary artifact | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/nonquiet-diagnosis/diagnostics/nonquiet-diagnosis-summary.json` |
| checkpoint log | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/nonquiet-diagnosis/diagnostics/checkpoints.jsonl` |
| ground outbound capture | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/nonquiet-diagnosis/captures/gds-to-southbound.bin` |
| ground inbound capture | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/nonquiet-diagnosis/captures/southbound-to-gds.bin` |
| UHF raw command log | `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157/nonquiet-diagnosis/uhf-ground/raw-command.log` |

Observed PASS markers:

```text
target-can-matrix-probe: PASS
mode=nonquiet-diagnosis
profile=uhf-primary
baseline-beacon-count=1
quiet-override=disabled
sband-session-source=target-journal
pre-switch-node6-ping=not-required
switch-source=target-journal
uhf-session-source=target-journal
uhf-command-source=target-journal
post-command-beacon-count=5
post-command-beacon-growth=not-observed
```

What the four truth surfaces show together:

- target journal truth:
  - explicit switch to `UHF primary`
  - historical UHF `SESSION_OPEN(seq0)` boundary observed
  - follow-on `GET_RESET_CAUSE` envelope observed
  - `BOOT_RECOVERY_STATUS` completion observed
- ground relay truth:
  - outbound gateway capture is non-empty: `2462` bytes
  - inbound gateway capture is non-empty: `83908` bytes
  - UHF ground logs still report repeated checksum-validation warnings under
    background traffic
  - UHF ground status still never reaches a stable ground-owned
    `link-up-observed + quiet-window-observed` oracle during the command window
  - `GROUND_LINK_DOWN/UP` churn is still visible on the ground/event surfaces
- beacon/debug truth:
  - beacon frames continued, but post-command beacon growth was not required
    for command success
- subsystem ingress truth:
  - `COMM_NODE_INGRESS_DIAGNOSTICS=1` was actually applied
  - summary recorded `245` ingress-diagnostic lines
  - background serial chunks continued to arrive while the same-session command
    still completed

Important bounded observations from the summary artifact:

- `rootCauseHypothesis` is
  `nonquiet-command-roundtrip-succeeded-beacon-growth-not-required`
- `OBCApp.uhfGroundLinkDriver.GROUND_LINK_RX_ERRORS` was observed as `18`
- `OBCApp.uartDriver.UART_RX_ERRORS` was observed as `0`
- subsystem ingress diagnostics show accepted serial ingress during the noisy
  window rather than a silent target-side drop
- this fresh run did **not** produce `QueueOverflow` evidence, so current noise
  should not be restated as a fresh queue-overflow/runtime verdict

Ground-side noisy signals that still happened even though the target command
completed:

- repeated `Checksum validation failed` warnings in the UHF ground GDS view
- repeated `uhf-ground-link-up-not-observed-within-8s`
- repeated `uhf-ground-quiet-window-not-observed-within-8s`
- `GROUND_LINK_DOWN/UP` churn on the UHF-side event surfaces
- non-zero `OBCApp.uhfGroundLinkDriver.GROUND_LINK_RX_ERRORS`
- no matching evidence that `UART_RX_ERRORS` or `QueueOverflow` caused the
  command failure in this fresh PASS run

This proves at least one governed non-quiet target CAN node-`6` case can still
switch, open session, and complete a same-session UHF command while background
TM is present. The problem is therefore not a proven runtime ingress failure on
the target node-`6` path.

### Negative / Degraded Cases Kept Adjacent

#### Old Oracle Failure: non-quiet command succeeded but beacon oracle failed

Verdict: `diagnostic FAIL under old oracle`, not a runtime failure

| Field | Value |
|---|---|
| artifact root | `/private/tmp/target-can-node6-nonquiet-diagnosis.XQXSJl/nonquiet-diagnosis` |
| failure shape | command succeeded, but old probe logic failed on missing post-command beacon growth |

Observed shape:

- `SESSION_OPEN(seq0)` and `GET_RESET_CAUSE` both completed on the target path
- the historical session-open boundary completed on the target path, but this
  degraded case is not current secure-auth evidence
- the old probe still failed because it treated remote-beacon-count growth as a
  hard requirement after the command
- that oracle was wrong for this path because accepted UHF command activity can
  refresh the beacon-suppress window without forcing a new beacon immediately

This degraded case is kept to show the old false-negative acceptance surface
that this change removed.

#### Transport Baseline Blocker: environment failure separated from node-`6`

Verdict: `environment blocker`, excluded from root-cause classification

| Field | Value |
|---|---|
| artifact root | `/private/tmp/target-can-node6-nonquiet-diagnosis.2oMTeP` |
| failure shape | quiet-control blocked before diagnosis because the lab transport baseline was unhealthy |

Observed shape:

- `obc.local can0` fell to `BUS-OFF`
- subsystem CAN links showed degraded/passive state
- `obc-comm-csp-stack.service` exited `status=32`
- fresh non-quiet diagnosis was not trustworthy until the documented transport
  reset and `command-path` preflight passed again

This degraded case is kept to show that transport-baseline hygiene can
masquerade as node-`6` instability if not separated first.

#### Fresh-Build Closeout Rerun: degraded non-quiet session-open visibility

Verdict: `fresh-build degraded rerun`, not promoted to the formal PASS case

| Field | Value |
|---|---|
| command | `bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` |
| artifact root | `/private/tmp/target-can-node6-nonquiet-diagnosis.diGG2o` |
| failure shape | quiet-control PASS, but non-quiet retried UHF `SESSION_OPEN(seq0)` visibility repeatedly and never reached the same-session command step |

Observed shape:

- quiet-control still passed with:
  - `sband-session-source=target-journal`
  - `switch-source=target-journal`
  - `uhf-session-source=target-journal`
  - `uhf-command-source=target-journal`
- the following non-quiet rerun then degraded at repeated UHF
  `SESSION_OPEN(seq0)` visibility after the explicit switch
- the same rerun still showed:
  - `switch-to-uhf-primary-pass source=target-journal`
  - repeated `uhf-ground-link-up-not-observed-within-8s`
  - repeated `uhf-ground-quiet-window-not-observed-within-8s`
- after stopping the degraded rerun, the documented transport reset plus clean
  command-path preflight passed again at:
  - `/tmp/rpi-target-recovery-restart.2RXlNy`

This rerun is kept because it reinforces the bounded claim of this change:
there is now a governed healthy non-quiet PASS case, but the path is still not
stable enough to promote into a reusable general non-quiet node-`6` proof.

## Decision

### Root Cause Class

Current classification: `oracle`

Why:

- the target TCP comparator passes on the same higher-level target session and
  command policy
- target CAN quiet-control passes with the corrected post-switch boundary
- target CAN non-quiet now also passes on the physical serial plus CAN path
- ground-side observability remains noisy under background TM:
  - repeated checksum-validation warnings on the UHF ground view
  - repeated `link-up-not-observed` and `quiet-window-not-observed` timeouts on
    the UHF ground view
  - continued `GROUND_LINK_DOWN/UP` churn on the ground/event surfaces
  - non-zero `GROUND_LINK_RX_ERRORS` on the UHF ground-link driver
  - no reliable link-up or quiet-window oracle on the UHF ground side
  - beacon-count growth is not a stable acceptance proxy
- subsystem ingress diagnostics confirm background serial traffic continues to
  arrive during the successful same-session command window
- this fresh PASS run did not show `UART_RX_ERRORS` growth or `QueueOverflow`,
  so those older failure shapes are not the current accepted root cause here

What is explicitly *not* included in this classification:

- transport-baseline failures such as `BUS-OFF` are still real lab blockers,
  but they are separate environment hygiene issues, not proof that healthy
  non-quiet node-`6` command ingress is broken

### Chosen Isolation Boundary

Chosen boundary for this change:

- formal acceptance is isolated at post-switch journal-first target command
  truth
- debug/beacon capture stays diagnostics-only
- nominal runtime behavior stays unchanged

Why this is the correct current truth:

- GDS/gateway are not the owner of command success
- beacon growth is not equivalent to accepted command progress
- target-side journal truth is the only bounded surface that stays coherent
  across quiet and non-quiet node-`6` diagnosis runs
- healthy non-quiet target CAN node-`6` command completion is now proven at
  least once without any product runtime change

### Rejected Boundaries

- `GDS / ground side`
  - rejected because ground-only surfaces are exactly where the noisy oracle
    lives
- `ground_ttc_gateway`
  - rejected because the gateway remains a raw relay and byte capture already
    provides the needed diagnostic visibility
- `OBC egress mux / COMM runtime`
  - rejected because fresh target CAN non-quiet and target TCP comparator runs
    both complete the same-session command without any runtime fix
- `node-6 side-channel-only capture`
  - rejected because it is observability support only, not the formal command
    truth surface

## Alternatives / Trade-offs

### Second GDS / debug-only side channel

- unnecessary for the current diagnosis outcome
- still defer it unless a later change needs extra debug visibility
- keep it outside the formal flight-like TT&C claim if it is ever added

### Gateway-side split

- still useful for byte capture only
- not the behavioral fix boundary

### OBC egress suppression/isolation

- not justified as a product fix in this change
- probe-owned quieting remains a verification control, not nominal operator
  behavior

### Node-`6` side-channel-only capture

- useful as supporting debug evidence
- remains explicitly outside the current formal flight-like TT&C claim

## What Changed

This change makes probe/tooling and documentation changes only:

- corrected the target CAN diagnosis control boundary
- added a repository-owned non-quiet diagnosis wrapper
- added bounded artifact collection for ground logs, gateway byte captures,
  target journal snapshots, channel snapshots, and beacon/debug capture
- fixed repo-owned cleanup around remote beacon helpers and target TCP
  comparator stale `pty_pair_bridge` residue

This change does **not** land a product runtime fix.

## What Is Newly Proven

- the old pre-switch `CSP ping node 6 success` gate was too coarse for this
  diagnosis
- under a healthy transport baseline, target CAN non-quiet node-`6` can still
  switch, open session, and complete a same-session UHF command
- the main remaining issue is acceptance/oracle pollution on the ground-side
  observability surface, not a proven target runtime ingress failure
- target TCP comparator and target CAN non-quiet now agree on the higher-level
  session/command truth
- current evidence does not justify a second GDS or debug-only side channel as
  the next implementation step

## What Remains Unproven

- a generally reusable non-quiet target/lab node-`6` validation path
- simultaneous dual-link runtime behavior on target hardware
- RF behavior or over-the-air UHF claims
- a claim that fresh-build reruns always reproduce the governed non-quiet PASS
  result without transport reset or additional baseline hygiene

Current status for target non-quiet node-`6` proof: `partially closed`

Reason:

- the diagnosis question is now honestly resolved
- at least one governed non-quiet case passes
- but the result is still bounded to journal-first acceptance plus a healthy
  transport baseline, so it is not yet promoted to a reusable general
  validation path

Current status for `comm-dual-link-orchestration-v1` target-bearing start:
`not ready`

Reason:

- the old node-`6` runtime-failure suspicion is no longer the blocker
- but this change still does not prove simultaneous dual-link behavior, general
  non-quiet operational closure, or a reusable target-bearing orchestration
  baseline

## Local Verification

Focused local verification completed in this change:

| Step | Command | Result |
|---|---|---|
| target CAN helper syntax | `./fprime-venv/bin/python -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py` | PASS |
| target TCP helper syntax | `./fprime-venv/bin/python -m py_compile scripts/comm_verification/lib/run_target_tcp_matrix_probe.py` | PASS |
| subsystem CSP launcher syntax | `bash -n scripts/run_subsystem_csp_service.sh` | PASS |
| diagnosis wrapper syntax | `bash -n scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` | PASS |
| target TCP stack wrapper syntax | `bash -n scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh` | PASS |
| full local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| governed diagnosis wrapper | `bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` | PASS at `/private/tmp/target-can-node6-nonquiet-diagnosis.Z0C157` |
| fresh-build diagnosis rerun | `bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh` | degraded rerun at `/private/tmp/target-can-node6-nonquiet-diagnosis.diGG2o`: quiet-control PASS, non-quiet UHF session-open retries timed out |
| target baseline preflight after cleanup | `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh` | PASS at `/tmp/rpi-target-recovery-restart.zrKv9n` |
| target baseline preflight after fresh-build degraded rerun reset | `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh` | PASS at `/tmp/rpi-target-recovery-restart.2RXlNy` |
| change validation | `openspec validate target-nonquiet-background-tm-stability-v1` | PASS |
| spec validation | `openspec validate --specs` | PASS |

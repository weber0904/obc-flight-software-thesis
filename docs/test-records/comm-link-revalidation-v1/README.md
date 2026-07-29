# comm-link-revalidation-v1 Evidence

## Scope

This record is the evidence home for the formal communication verification
matrix introduced by `formal-comm-verification-matrix-v1`.

The suite groups the active communication surface into three environments:

1. pure macOS hosted
2. macOS GDS plus Pi OBC plus Pi subsystem over a TCP development carrier
3. macOS GDS plus Pi OBC plus Pi subsystem over CAN with physical quiet UHF
   UART southbound

It tracks nine case ids per environment:

- `direct-control`
- `sband-command`
- `sband-file`
- `sband-sequence-subsystem`
- `uhf-primary-command`
- `uhf-primary-file`
- `uhf-primary-sequence-subsystem`
- `failover-command`
- `csp-reachability`

## Purpose

This record is intentionally matrix-oriented. It is meant to answer:

- which formal communication paths are healthy today
- which paths still fail
- which cells remain blocked because the repo does not yet have a dedicated
  trustworthy harness

It does not collapse hosted stand-ins, target TCP development-carrier proofs,
and target CAN plus physical-UHF provenance into one undifferentiated claim.

## Evidence Storage Contract

Each environment run stores:

- `summary.json`
- `summary.md`
- one artifact directory per case under `cases/`

The canonical wrapper family is documented in
[docs/operator/formal-comm-verification-matrix-v1-runbook.md]($REPO_ROOT/docs/operator/formal-comm-verification-matrix-v1-runbook.md).

## Commands

Use one of:

```bash
bash scripts/run_comm_verification_hosted.sh
bash scripts/run_comm_verification_rpi_tcp.sh
bash scripts/run_comm_verification_rpi_can.sh
bash scripts/run_comm_verification_all.sh
```

## Initial Implementation Notes

The first implementation pass focuses on:

- a governed subtree under `scripts/comm_verification/`
- per-case metadata and environment summaries
- reuse of existing governed probe scripts where they already match a matrix
  cell
- explicit blocker reporting for cells that still need a dedicated probe

## Follow-On Change Train

This umbrella record remains the matrix dashboard while blocked cells are
closed in smaller follow-on changes:

- `comm-verification-matrix-foundation-v1`
- `comm-verification-sequence-subsystem-harness-v1`
- `target-can-node6-matrix-closure-v1`
- `target-tcp-southbound-parity-foundation-v1`
- `target-tcp-matrix-closure-v1`
- `target-direct-control-matrix-cases-v1`

Each follow-on change may add its own focused evidence README. This record is
responsible for linking those results back into the three-environment matrix.

## Runtime Results

This section records the current branch truth for the three-environment matrix
and its bounded cleanup-smoke contract.

### Initial Framework Smoke

Date:
- `2026-05-22`

Command:

```bash
COMM_VERIFICATION_CASES="direct-control sband-sequence-subsystem" \
  bash scripts/run_comm_verification_hosted.sh
```

Observed outcome:

- `direct-control`: PASS
- `sband-sequence-subsystem`: blocked as `environment-blocker`

Interpretation:

- the new matrix runner, per-case artifact layout, blocker classification, and
  summary rendering are active and reviewable
- the hosted direct `fprime-cli -> GDS -> OBC` comparator path has a new
  governed subtree entrypoint
- the dedicated sequence-subsystem proof required by this matrix still needs a
  purpose-built harness before it can become formal evidence

### Cleanup Smoke

Date:
- `2026-05-22`

Command:

```bash
COMM_VERIFICATION_CLEANUP_SMOKE_ENVS='hosted rpi_tcp rpi_can' \
  bash scripts/comm_verification/matrix/run_cleanup_smoke.sh
```

Observed outcome:

- cleanup-smoke root: `/tmp/commv-cleanup-smoke.fZhrba`
- `hosted`: rerun-result `pass`, second summary status `pass`
- `rpi_tcp`: rerun-result `pass`, second summary status `pass`
- `rpi_can`: rerun-result `pass`, second summary status `pass`

Interpretation:

- the bounded cleanup smoke exercised interrupt plus immediate rerun on all
  three wrapper families
- all three wrapper families completed the representative rerun with passing
  summaries
- the cleanup smoke now proves both wrapper-level rerun safety and current
  representative post-interrupt functional success for `hosted`, `rpi_tcp`,
  and `rpi_can`

### Hosted

- focused shared-sequence helper evidence:
  [docs/test-records/comm-verification-sequence-subsystem-harness-v1/README.md](../comm-verification-sequence-subsystem-harness-v1/README.md)
- current hosted sequence cell state:
  - `sband-sequence-subsystem`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/hosted/20260522T051636Z`
  - `uhf-primary-sequence-subsystem`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/hosted/20260523T065523Z`
- current interpretation:
  - the matrix-owned shared helper closes hosted node-`5` same-path sequence
    proof
  - the same helper now also closes hosted node-`6` same-path sequence proof
    on the governed UHF stand-in path
  - the product fix that enabled hosted node-`6` closure was multi-ingress
    file-return routing in `FileIngressAuthority` plus `TopCcsds` wiring that
    returns `FileHandling.fileUplink` buffers to the original ingress port
  - hosted sequence-subsystem proof is no longer a live blocker for the formal
    matrix

### Target TCP

- focused target TCP evidence:
  [docs/test-records/target-tcp-southbound-parity-foundation-v1/README.md](../target-tcp-southbound-parity-foundation-v1/README.md)
- focused target TCP high-level closure evidence:
  [docs/test-records/target-tcp-matrix-closure-v1/README.md](../target-tcp-matrix-closure-v1/README.md)
- focused target direct-control evidence:
  [docs/test-records/target-direct-control-matrix-cases-v1/README.md](../target-direct-control-matrix-cases-v1/README.md)
- current target TCP foundation cell state on this branch:
  - `direct-control`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T100016Z`
  - post-isolation rerun: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T100654Z`
  - `csp-reachability`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T081132Z`
  - `sband-command`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T072544Z`
  - `sband-file`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T072544Z`
  - `uhf-primary-command`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T072544Z`
  - `uhf-primary-file`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T072544Z`
  - `sband-sequence-subsystem`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T073659Z`
  - `uhf-primary-sequence-subsystem`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T081219Z`
  - `failover-command`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T075252Z`
- current interpretation:
  - the governed three-host target TCP parity launcher is now stable again on
    the current branch after two specific harness fixes:
    a bounded subsystem startup settle and explicit OBC-side comm-subsystem
    ping timeout / unavailable threshold tuning for the development-carrier
    environment
  - the final readiness false-negative on `20260523T080753Z` was a probe-oracle
    bug, not a product-path failure: the gate still matched the old
    `timeout 500 ms` fragment after the harness had moved to `1000 ms`
  - the matrix-owned target TCP direct wrapper is now formally closed
  - the resolved target direct blocker was not a surviving product ingress
    failure; it was a combination of helper contract drift
    (`fprime` framing reused from historical connectivity-only evidence) and a
    missing rebuilt remote workspace binary
  - target TCP node-`5` command/file proof, node-`5` same-path sequence proof,
    node-`6` command/file proof, node-`6` same-path sequence proof, and target
    TCP failover continuity are now formal matrix-owned development-carrier
    evidence
  - node `6` remains explicitly scoped to TCP-based southbound emulation and
    must not be read as physical UHF UART closure
  - target TCP no longer has an open formal matrix gap on this branch

### Target CAN

- focused target CAN evidence:
  [docs/test-records/target-can-node6-matrix-closure-v1/README.md](../target-can-node6-matrix-closure-v1/README.md)
- focused target direct-control evidence:
  [docs/test-records/target-direct-control-matrix-cases-v1/README.md](../target-direct-control-matrix-cases-v1/README.md)
- current target CAN cell state on this branch:
  - `direct-control`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260523T100103Z`
  - post-isolation rerun: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260523T100729Z`
  - `sband-sequence-subsystem`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T053648Z`
  - `uhf-primary-file`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T141125Z`
  - `uhf-primary-sequence-subsystem`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T141314Z`
  - `failover-command`: PASS via
    `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T141428Z`
- current interpretation:
  - the matrix-owned target CAN helper now formally closes same-path
    `sband-sequence-subsystem`
  - the same helper now also closes quiet node-`6` official `.fdp`
    file/downlink proof, quiet node-`6` same-path sequence-subsystem proof,
    and bounded `sband -> uhf-primary` failover continuity
  - the matrix-owned target CAN direct wrapper is now formally closed on the
    same corrected helper contract as target TCP
  - the branch-local diagnostics now snapshot installed service state,
    effective UHF baudrate, bootstrap readiness, node-`6` readiness, and
    session/file checkpoints under each target CAN probe root
  - target CAN no longer needs hardware-suspect quarantine for the quiet UHF
    family because the physical-UART matrix cells now pass on governed reruns

## Registry Update Rule

Do not update
[docs/verification-path-registry.md]($REPO_ROOT/docs/verification-path-registry.md)
from this record until a specific matrix cell has passing governed evidence and
its claim boundary is clear.

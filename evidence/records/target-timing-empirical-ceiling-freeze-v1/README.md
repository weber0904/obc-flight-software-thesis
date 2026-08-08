# Target Timing Empirical Ceiling Freeze V1

Status: final empirical timing-closure record for the active service-managed
Raspberry Pi node-`5` baseline.
Last updated: 2026-05-25.

## Scope

This change closes the numeric service-managed timing gap left open by
`target-timing-wcet-profile-proof-v1`.

What this record owns:

- the mandatory node-`5` control-partition preflight used to separate
  baseline-health truth from timing-measurement truth
- the three fresh restarted timing runs that freeze current empirical
  service-managed ceilings for the declared representative workload
- the final aggregated `RgMaxTime`, inter-arrival, and slip verdict numbers
  that now back `docs/interfaces.md` and registry entry `68`

What this record does not own:

- the original structural timing freeze from
  `target-timing-wcet-profile-proof-v1`
- the narrower blocker-classification investigation preserved under
  `target-node5-telemetry-backpressure-and-restart-stability-v1`
- any final flight-processor hard real-time claim

Current-note:

- the representative payload activity surface captured in this record used
  `PAYLOAD_PREPARE_SESSION(AUTO)` because that was the active payload branch
  surface at the time
- current payload semantics keep this timing evidence useful as workload
  context, but they no longer treat `PREPARE_SESSION(AUTO)` as a distinct
  prepared-state exclusivity requirement for normal still capture
- when translating this historical workload to the current maintained payload
  surface, use the shared non-RAW `PAYLOAD_SET_CAMERA_DEFAULTS ->
  PAYLOAD_SET_AUTO_DEFAULTS -> PAYLOAD_PREPARE` path instead of treating
  `PREPARE_SESSION(AUTO)` as still-current operator truth

## Active Baseline Under Test

- target OBC service:
  `obc-comm-csp-stack.service`
- target COMM profile:
  `TARGET_COMM_PROFILE=sband`
- target COMM node:
  `COMM_CSP_NODE=5`
- target authority profile:
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- representative activity surface:
  historical proof workload:
  `COMM_START_PASS -> MODE_SET PAYLOAD -> PAYLOAD_PREPARE_SESSION(AUTO) ->
  PAYLOAD_CAPTURE_AUTO -> PAYLOAD_GET_LAST_CAPTURE_METADATA ->
  PAYLOAD_SHUTDOWN -> MODE_SET IDLE -> COMM_STOP_PASS`

## Prerequisite Blocker Truth

Earlier timing reruns showed narrower blocker roots that were kept separate
instead of being folded back into broad timing `TBD` wording:

- `/tmp/target-timing-empirical-ceiling-freeze-v1.CQqgAA`
- `/tmp/target-timing-empirical-ceiling-freeze-v1.tA5n2O`

Those roots motivated
`target-node5-telemetry-backpressure-and-restart-stability-v1`, which then
revalidated the current rebuilt/install baseline and showed that:

- sustained `ComCcsds.comQueue.QueueOverflow` is not current node-`5` truth on
  a fresh rebuilt/install baseline
- restart-path `COMM_LINK_AVAILABILITY_CHANGED False` and
  `RateGroupCycleSlip` are not current node-`5` truth on that same baseline

This timing record therefore freezes numeric ceilings only from the fresh
clean reruns after that blocker revalidation.

## Fresh Verification Used For Final Freeze

### Local

```bash
./fprime-venv/bin/python3 -m py_compile \
  scripts/target_timing_empirical_ceiling_freeze_v1_probe.py
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut \
  --target OBC_Components_CommController_ut_exe -j4
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

### Target Build / Install

```bash
bash scripts/sync_rpi_workspace.sh
bash scripts/bootstrap_rpi_workspace.sh
bash scripts/package_rpi_bundle.sh
FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh
```

### Mandatory Control Preflight

```bash
TARGET_COMM_PROFILE=sband PROBE_MODE=command-path \
bash scripts/run_rpi_target_recovery_restart_probe.sh
```

Verdict: `PASS`

Artifact root:

- `/tmp/rpi-target-recovery-restart.WZIkd6`

Observed scope:

- authenticated `SESSION_OPEN`
- `GET_RESET_CAUSE`
- target journal acceptance on the active node-`5` path

### Final Three-Run Timing Closure

```bash
bash scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh
```

Verdict: `PASS`

Artifact root:

- `/tmp/target-timing-empirical-ceiling-freeze-v1.LxJ7X7`

Current-branch revalidation note:

- a local-helper cleanup fix alone was not enough; `/tmp/target-timing-empirical-ceiling-freeze-v1.vwkJKv`
  still failed in `run-2` after local cleanup with a fast-group slip during the
  representative-workload tail
- after the full manual transport baseline reset described in
  [docs/verification.md](../../../docs/verification.md),
  the same current branch returned to clean `3/3 PASS` on `LxJ7X7`

Per-run summary:

| Run | Restarted before run | Queue overflow events | Queue depth max `(events, telemetry, file)` | `RgCycleSlips` deltas | `RateGroupCycleSlip` events |
|---|---:|---:|---|---|---|
| `run-1` | `1` | `0` | `(7, 238, 0)` | `fast=0 slow=0 data=0` | `fast=0 slow=0 data=0` |
| `run-2` | `1` | `0` | `(8, 254, 0)` | `fast=0 slow=0 data=0` | `fast=0 slow=0 data=0` |
| `run-3` | `1` | `0` | `(7, 240, 0)` | `fast=0 slow=0 data=0` | `fast=0 slow=0 data=0` |

## Frozen Empirical Ceilings

The current service-managed node-`5` empirical ceilings are frozen from the
passing set above.

### `RgMaxTime`

- fast: `837381 us`
- slow: `87658 us`
- data: `9451 us`

### Governed GDS-Observed Inter-Arrival Bounds

- fast: `0.412 .. 2.823 s`
- slow: `4.998 .. 5.002 s`
- data: `0.995 .. 2.003 s`

### Passing Slip Verdict

Across all three fresh restarted runs:

- `RateGroupCycleSlip = 0`
- `RgCycleSlips delta = 0`
- `QueueOverflow = 0`

for fast, slow, and data groups across both:

- `steady-state`
- `steady-state-plus-representative-activity`

## What This Change Proved

1. The active service-managed node-`5` path can now freeze numeric empirical
   timing ceilings honestly for the declared representative workload.
2. The timing probe no longer depends on broad timing `TBD` prose; it now
   records:
   - control preflight verdict separately from timing measurement
   - installed service truth
   - queue-depth diagnostics
   - per-run slip verdicts
   - aggregated empirical ceilings from repeated clean runs
3. The declared node-`5` timing claim is now a bounded empirical contract, not
   merely a structural timing contract.

## Remaining Non-Claims

- no final flight-processor hard real-time closure
- no automatic transfer of these ceilings to other target ground paths,
  workloads, or future flight processors
- no radio-metrics, reliable-transfer, or scheduler-redesign claim
- no claim that the governed GDS observation path is equivalent to a pure
  onboard scheduler-only timing measurement

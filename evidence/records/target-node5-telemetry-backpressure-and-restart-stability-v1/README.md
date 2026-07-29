# Target Node-5 Telemetry Backpressure And Restart Stability V1

Status: blocker-closure revalidation record for the active service-managed
Raspberry Pi node-`5` timing baseline.
Last updated: 2026-05-25.

## Scope

This change started as a product-blocker follow-up for
`target-timing-empirical-ceiling-freeze-v1`.

The suspected blockers were:

1. representative payload plus COMM activity might be driving sustained
   `ComCcsds.comQueue.QueueOverflow` on telemetry queue index `1`
2. fresh `obc-comm-csp-stack.service` restarts might be dropping node-`5`
   availability or emitting early `RateGroupCycleSlip` before the first timing
   window is established

The final result of this change is narrower and more useful:

- the current fresh rebuilt/install baseline does **not** reproduce sustained
  telemetry queue overflow under the declared representative workload
- the same fresh rebuilt/install baseline does **not** reproduce restart-path
  `COMM_LINK_AVAILABILITY_CHANGED False` or `RateGroupCycleSlip`
- the blocker classification is therefore closed on the current active node-`5`
  path, and timing empirical closure can proceed on a clean baseline

This is not a generic COMM redesign, not a queue-resize exercise, and not a
new scheduler change.

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

## Earlier Suspected Blocker Baseline

The earlier blocker classification came from two fresh-but-older timing roots:

1. `/tmp/target-timing-empirical-ceiling-freeze-v1.CQqgAA`
   - first steady-state window passed
   - representative activity later showed sustained
     `ComCcsds.comQueue.QueueOverflow`
   - the post-activity timing window then lost clean fresh telemetry
2. `/tmp/target-timing-empirical-ceiling-freeze-v1.tA5n2O`
   - before the first timing window settled, the target journal showed:
     - `CSP ping node 5 success False`
     - `COMM_LINK_AVAILABILITY_CHANGED : SBAND available False`
     - `RateGroupCycleSlip`

Those roots were sufficient to justify opening this blocker-closure change,
but they were not sufficient to freeze the diagnosis without a fresh rebuilt
and reinstalled target baseline.

## Bounded Changes Applied In This Branch

### 1. Restart-path product-side stabilization

`CommController` now rate-limits and debounces primary-band subsystem probing
so single transient misses do not immediately churn the active service-managed
node-`5` availability verdict during restart/warmup.

Touched files:

- `OBC/Components/CommController/CommController.cpp`
- `OBC/Components/CommController/CommController.hpp`
- `OBC/Components/CommController/CommControllerRuntime.hpp`

### 2. Queue/backpressure observability

The timing proof probe now records queue truth for the active target downlink
path:

- `ComCcsds.comQueue.comQueueDepth`
- `ComCcsds.comQueue.buffQueueDepth`
- per-window `QueueOverflow` event counts

Touched file:

- `scripts/target_timing_empirical_ceiling_freeze_v1_probe.py`

This was the minimum extra observability needed to answer "is telemetry queue
index `1` actually saturating on the active node-`5` workload?" without
guessing from incomplete symptoms.

## Fresh Verification Performed On 2026-05-25

### Local

```bash
./fprime-venv/bin/python3 -m py_compile \
  scripts/target_timing_empirical_ceiling_freeze_v1_probe.py
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut \
  --target OBC_Components_CommController_ut_exe -j4
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

### Fresh Three-Run Timing Revalidation

```bash
bash scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh
```

Verdict: `PASS`

Artifact root:

- `/tmp/target-timing-empirical-ceiling-freeze-v1.LxJ7X7`

Follow-up note:

- later current-branch reruns proved that local-helper cleanup alone was not a
  sufficient reset boundary; `/tmp/target-timing-empirical-ceiling-freeze-v1.vwkJKv`
  still failed in `run-2`
- repeating the proof only after the full transport baseline reset restored the
  clean `3/3 PASS` shape on `LxJ7X7`

Per-run summary:

| Run | Restarted before run | Queue overflow events | Queue depth max `(events, telemetry, file)` | `RgCycleSlips` deltas | `RateGroupCycleSlip` events |
|---|---:|---:|---|---|---|
| `run-1` | `1` | `0` | `(6, 235, 0)` | `fast=0 slow=0 data=0` | `fast=0 slow=0 data=0` |
| `run-2` | `1` | `0` | `(5, 243, 0)` | `fast=0 slow=0 data=0` | `fast=0 slow=0 data=0` |
| `run-3` | `1` | `0` | `(5, 249, 0)` | `fast=0 slow=0 data=0` | `fast=0 slow=0 data=0` |

Aggregated empirical high-water ceilings from the same three fresh runs:

- `RgMaxTime`
  - fast: `837381 us`
  - slow: `87658 us`
  - data: `9451 us`
- GDS-observed inter-arrival bounds
  - fast: `0.412 .. 2.823 s`
  - slow: `4.998 .. 5.002 s`
  - data: `0.995 .. 2.003 s`

## What This Change Actually Proved

1. The active node-`5` telemetry queue does **not** currently revalidate as a
   sustained overflow blocker on a fresh rebuilt/install baseline.
   - telemetry queue high-water stayed at or below `256 / 500`
   - file queue stayed at `0`
   - event queue stayed at or below `6`
   - no `ComCcsds.comQueue.QueueOverflow` event reappeared in any of the three
     fresh restarted timing runs
2. The active node-`5` restart path does **not** currently revalidate as a
   slip/unavailable blocker on that same fresh baseline.
   - all three runs reopened the authenticated command session through the
     target journal
   - none of the three runs recorded restart-path
     `COMM_LINK_AVAILABILITY_CHANGED False`
   - none of the three runs recorded `RateGroupCycleSlip`
   - all three runs kept `RgCycleSlips` delta at `0`
3. The bounded `CommController` primary-band probe cadence/debounce change is
   sufficient for the current active baseline.
   - this change did **not** need a telemetry queue resize
   - this change did **not** need a generic downlink or scheduler redesign
4. The original blocker change therefore closes as a reclassification and
   baseline-cleanup record.
   - the older blocker artifacts remain useful as branch-local debugging
     history
   - they are no longer the current baseline truth

## Remaining Non-Claims

- no final flight-processor hard real-time claim
- no claim that other target ground paths inherit the same numeric timing
  ceilings automatically
- no claim that broader or noisier workloads than the declared representative
  activity surface will keep the same queue-depth bounds
- no radio-metrics, reliable-transfer, or scheduler-redesign claim

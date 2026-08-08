# Target Timing / WCET Profile Proof V1

Status: first reviewable target timing contract record for the active
service-managed Raspberry Pi baseline.
Last updated: 2026-05-24.

## Scope

This record merges two things into one reviewable change:

1. document catch-up for the 2026-05-21 payload convergence baseline and the
   2026-05-23 COMM verification matrix closure
2. the first formal target timing/WCET proof slice for the active
   `obc-comm-csp-stack.service` Raspberry Pi path

This is not final flight-processor real-time closure. It is a bounded
service-managed target timing contract update.

## Target Baseline Under Test

- target OBC service:
  `obc-comm-csp-stack.service`
- target COMM profile:
  `TARGET_COMM_PROFILE=sband`
- target COMM node:
  `COMM_CSP_NODE=5`
- target authority profile:
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- target runtime root:
  `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc`

## Frozen Timing Truth

These items are now frozen as current baseline truth for the service-managed
Raspberry Pi target path:

| Surface | Frozen truth | Source |
|---|---|---|
| base tick | `1000 ms` | `OBC/Runtime/HostedRuntime.hpp` plus installed `run_obc_comm_csp_stack.sh` not overriding `--tick-ms` |
| rate-group divisors | `{1, 5, 1}` | `OBC/TopCcsds/OBCAppTopology.cpp` |
| nominal rates | fast `1 Hz`, slow `0.2 Hz`, data `1 Hz` | derived from `1000 ms` plus `{1, 5, 1}` |
| missed-tick policy | F' `ActiveRateGroup` slip semantics: a new cycle arriving before the previous cycle completes emits `RateGroupCycleSlip` and increments `RgCycleSlips` | vendored F' `Svc::ActiveRateGroup` SDD and source |
| hard non-slip admissibility boundary | fast `< 1.0 s`, slow `< 5.0 s`, data `< 1.0 s` | implied by the active slip semantics and group periods |

## Probe Surface Added By This Change

New repository-owned probe entrypoints:

- `scripts/run_target_timing_wcet_profile_proof_v1_probe.sh`
- `scripts/target_timing_wcet_profile_proof_v1_probe.py`

Current-note:

- the representative payload activity surface captured in this record used
  `PAYLOAD_PREPARE_SESSION(AUTO)` because that was the active payload branch
  surface at the time
- current payload semantics keep this timing evidence useful as workload
  context, but they no longer treat `PREPARE_SESSION(AUTO)` as a distinct
  prepared-state exclusivity requirement for normal still capture
- when mapping this workload onto the current maintained payload surface, read
  that older step as the historical precursor of today's shared non-RAW
  `PAYLOAD_SET_CAMERA_DEFAULTS -> PAYLOAD_SET_AUTO_DEFAULTS -> PAYLOAD_PREPARE`
  path, not as a still-current command requirement

The probe is scoped to the current service-managed node-`5` baseline and
defines two observation windows:

1. `steady-state`
2. `steady-state-plus-representative-activity`

Representative activity is intentionally narrow. The sequence below is the
historical workload used when this proof was captured, not the current
maintained payload command surface:

- governed `SESSION_OPEN`
- bounded COMM activity on the active S-band path
- `MODE_SET PAYLOAD`
- `PAYLOAD_PREPARE_SESSION(AUTO)`
- `PAYLOAD_CAPTURE_AUTO`
- `PAYLOAD_GET_LAST_CAPTURE_METADATA`
- `PAYLOAD_SHUTDOWN`
- `MODE_SET IDLE`

Telemetry and events the probe is designed to capture:

- `OBCApp.rateGroup1Comp.RgMaxTime`
- `OBCApp.rateGroup2Comp.RgMaxTime`
- `OBCApp.rateGroup3Comp.RgMaxTime`
- `OBCApp.rateGroup1Comp.RgCycleSlips`
- `OBCApp.rateGroup2Comp.RgCycleSlips`
- `OBCApp.rateGroup3Comp.RgCycleSlips`
- `OBCApp.rateGroup{1,2,3}Comp.RateGroupCycleSlip`
- `OBCApp.commController.COMM_S_BAND_ACTIVITY_AGE_TICKS`
- `OBCApp.storageHealthBridge.STORAGE_SCAN_COUNT`
- `OBCApp.systemResources.CPU`

Probe implementation notes that now matter for review:

- command envelopes are signed from the resolved target authority profile and
  deployed service auth material (`COMMAND_AUTHORITY_PROFILE`,
  `COMMAND_AUTH_SOURCE_ID`, `COMMAND_AUTH_KEY_SLOT`, `COMMAND_AUTH_KEY_HEX`)
  when those values are present on the installed service
- `RgCycleSlips` is treated as a cumulative counter and the proof verdict uses
  per-window counter deltas, not the absolute lifetime value on a long-lived
  target runtime

## Fresh Verification Performed On 2026-05-24

### Local Gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

The closeout gate passed, including the repo consistency and OpenSpec spec
validation steps, before any target-side rerun evidence was reused.

### Target Packaging / Install

The change also fixes Raspberry Pi bundle contents so the helper-backed payload
contract is actually present in the installed release surface:

- `scripts/package_rpi_bundle.sh` now stages
  `bin/payload_camera_backend_helper`
- the installed target process list now includes both:
  - `$OBC_HOME/obc-deploy/current/bin/OBC`
  - `$OBC_HOME/obc-deploy/releases/.../bin/payload_camera_backend_helper`
- closeout rerun used:

```bash
bash scripts/package_rpi_bundle.sh
FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh
```

### Service-Managed Baseline Recovery

During timing proof bring-up, the target path initially failed because
`subsystem.local` was missing active EPS/ADCS/S-band CSP services. This was
corrected by reinstalling the subsystem COMM CSP services and then restarting
`obc-comm-csp-stack.service` after the subsystem stack was healthy again.

After the healthy restart, target journal evidence again showed:

- `EPS_STATUS_RECEIVED`
- `CSP_PING_RESULT : ... success 1 timeout 500 ms`
- `STATE_MONITOR_UPDATED : ... health=20 fault=0 quality=0`

This re-established the active target baseline before re-running timing probes.

## What Was Proven

1. The installed Raspberry Pi release needed the helper binary in the release
   bundle for the representative payload timing workload to be meaningful.
   This change fixes that packaging gap.
2. The service-managed target timing contract is no longer broad `TBD` prose.
   Base tick, divisors, nominal rates, and missed-tick policy are now explicit
   and reviewable.
3. The repository now has a governed target timing probe with explicit
   workload, channels, `RgMaxTime` / `RgCycleSlips` capture points, and
   residual non-claim handling.

## What Was Not Yet Proven

Numeric service-managed WCET/jitter ceilings were not frozen by this record.

Reason:

- repeated 2026-05-24 governed node-`5` GDS timing runs did not produce a
  clean enough end-to-end capture window to justify numeric ceiling claims
- after session-floor handling and helper packaging were fixed, both the new
  timing probe and the existing repository-owned target command-path control
  probe still intermittently timed out waiting for authenticated
  `SESSION_OPEN` on the same governed node-`5` path
- the fresh closeout rerun after `run_verification_ci.sh` plus forced target
  reinstall reproduced the same bounded failure, timing out at
  `SESSION_OPEN attempt 6`, so this record still reflects the current truth
- this means the remaining gap is narrower than the old broad timing `TBD`:
  the current instability is in the governed GDS command/timing observation
  path, not in the structural timing contract itself

## Required Evidence Boundary For The Next Narrow Follow-Up

The next timing-only follow-up must keep the same service-managed scope and
freeze numeric ceilings only after it records all of the following on a fresh,
healthy node-`5` run:

1. `steady-state` window with timestamped fast/slow/data telemetry samples
2. `steady-state-plus-representative-activity` window with the same telemetry
   plus the bounded payload/COMM workload above
3. `RateGroupCycleSlip = 0` events across both windows
4. `RgCycleSlips = 0` across both windows
5. per-group `RgMaxTime` high-water marks from the same windows
6. explicit residual non-claims if any one of the above still cannot be
   collected cleanly

## Current Non-Claims

- no frozen numeric service-managed WCET ceiling
- no frozen numeric service-managed jitter ceiling
- no final flight-processor hard real-time closure
- no RF, reliable-transfer, scheduler, or payload-feature expansion claim
- no claim that current node-`5` ground-software residency is itself timing-stable

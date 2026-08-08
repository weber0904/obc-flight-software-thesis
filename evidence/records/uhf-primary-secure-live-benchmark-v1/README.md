# UHF Primary Secure-Live Benchmark V1

Date:
- `2026-06-03`

## Summary

This record captures the governed target/lab secure-auth benchmark for the
switched `uhf-primary-after-failover` path under two runtime states:

- `quiet control`: current official UHF-primary packet quiet behavior intact
- `nonquiet experiment`: only `DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET=1`

Scope is intentionally **Command + Live Only**:
- S-band secure-auth bootstrap
- secure `COMM_SET_ACTIVE(UHF)`
- UHF primary re-auth
- bounded UHF secure command success
- UHF live observability and transport dirtiness evidence

This record does **not** prove:
- file downlink or file uplink
- staged upload
- sequence closure
- reliable transfer
- RF closure

## Current Baseline

This benchmark uses the current governed baseline:

- readiness gate:
  - `bash scripts/ensure_target_comm_lab_baseline.sh`
  - `bash scripts/ensure_ground_dual_gds_baseline.sh`
- secure-auth baseline references:
  - `bash scripts/run_target_secure_auth_command_path_probe.sh`
  - `bash scripts/run_target_secure_auth_proof.sh`
- `GDS keepalive interval = 0`
- `UHF serial preamble lines = 0`
- `UHF serial preamble delay = 0`
- benchmark postflight passes `TARGET_BASELINE_IGNORE_GPS_STATE_MISSING=1`
  because `GPS_STATE_UPDATED` in a short journal window is not a COMM-path
  claim and was previously causing false postflight blockers

This formal run also used probe-scoped diagnostics overlays for evidence
capture:

- `OBC_GROUNDLINK_DIAGNOSTICS=1`
- `SBAND/UHF COMM_NODE_INGRESS_DIAGNOSTICS=1`

These are not promoted into the governed target baseline. They are temporary
proof overlays and are removed before the repetition is counted.

## Benchmark Entry Points

- [scripts/run_target_uhf_primary_secure_live_benchmark.sh](../../../scripts/run_target_uhf_primary_secure_live_benchmark.sh)
- [scripts/comm_verification/lib/run_target_uhf_primary_secure_live_benchmark.py](../../../scripts/comm_verification/lib/run_target_uhf_primary_secure_live_benchmark.py)

## Artifact Ownership Model

Each ground path is split into three buckets:

- `native-cli/`
  - canonical passive `fprime-cli` native evidence
  - includes `event.log`, `channel.log`, `command.log`, `recv.bin`, `sent.bin`
- `process-control/`
  - benchmark wrapper control-plane logs only
  - launch command, stdout/stderr, timeout, return code, control queries
- `gds-runtime/`
  - `fprime-gds` / `ground_ttc_gateway` runtime logs and file store

Target-side evidence remains under:
- `diagnostics/journal-snapshots/`
- `diagnostics/service-snapshots/`

Canonical verdicts in this record use:
- `native-cli/*`
- gateway raw captures under `captures/`
- target journals and target-side diagnostics

They do **not** depend on wrapper stdout logs or product debug counters.

## Verdict Definitions

This benchmark carries two independent verdict families:

- `targetRuntimeVerdict`
- `groundObservabilityVerdict`

They answer different questions and use different evidence sources.

### `targetRuntimeVerdict`

`targetRuntimeVerdict = PASS` means the benchmark obtained the required
**target-journal-first command closure** for the full command path under test.

For `quiet-command`, this requires target-journal closure for:

- S-band secure `GET_RESET_CAUSE`
- secure `COMM_SET_ACTIVE(UHF)`
- UHF secure `GET_RESET_CAUSE`
- secure restore back to `SBAND`

For `nonquiet-command`, the same command boundaries are required.

Canonical evidence sources:

- primary oracle:
  - target OBC journal fragments captured through the probe
- corroborating but non-authoritative evidence:
  - ground raw captures under `captures/`
  - ground `native-cli/command.log`
  - secure-auth challenge / status evidence

What `targetRuntimeVerdict = FAIL` means:

- the benchmark did **not** obtain the required target-journal closure for one
  of the command boundaries above
- this does **not** automatically mean the ground command never uplinked
- this does **not** automatically mean the command definitely never executed
- it means the benchmark could not prove end-to-end command acceptance and
  completion at the required command boundary

In the current canonical run, the two `nonquiet` failures are examples of this:

- `paired-rerun-1`
  - S-band command path, UHF switch, and UHF secure-auth all completed
  - UHF live observability was already visible on the ground
  - but the benchmark never obtained target-journal closure for UHF secure
    `GET_RESET_CAUSE`
- `paired-rerun-3`
  - UHF live observability was already visible on the ground
  - but the benchmark never obtained target-journal closure for the secure
    restore back to `SBAND`

So a runtime failure here means:

- ground evidence may show the command uplink and later UHF live traffic
- but the target-side oracle still did not prove the required command boundary

### `groundObservabilityVerdict`

`groundObservabilityVerdict = PASS` means the benchmark obtained reviewable
ground-side observability on the **active ground path for that case**.

Case-specific active ground path:

- `quiet`:
  - active observability path is `S-band`
- `nonquiet`:
  - active observability path is `UHF`

Canonical evidence sources:

- `native-cli/event.log`
- `native-cli/channel.log`
- gateway raw downlink capture:
  - `captures/<path>/southbound-to-gds.bin`

The implemented rule is:

- `PASS`
  - active-path `event.log` contains reviewable live events
  - active-path `channel.log` contains reviewable live channels
  - active-path `southbound-to-gds.bin` is present and non-empty
- `DEGRADED`
  - some active-path transport is visible
  - but event or channel visibility is incomplete
- `FAIL`
  - no usable active-path transport visibility was captured

What `groundObservabilityVerdict = FAIL` means:

- this is a **ground evidence failure**
- it means the benchmark did not capture usable active-path event/channel
  observability
- it does **not** by itself prove that the target never executed the command

So the failure distinction is:

- runtime failure:
  - target-journal command closure missing
- observability failure:
  - command may still have executed, but ground-side event/channel evidence was
    not captured in a usable form

## Canonical Formal Run

Repository-owned entrypoint:

```bash
BENCHMARK_REPETITIONS=3 bash scripts/run_target_uhf_primary_secure_live_benchmark.sh
```

Reference run:

| Field | Value |
|---|---|
| wrapper verdict | `target-uhf-primary-secure-live-benchmark: PASS` |
| benchmark root | `$REPO_ROOT/build-artifacts/uhf-primary-secure-live-benchmark/20260603T-final-3-converged` |
| summary JSON | `$REPO_ROOT/build-artifacts/uhf-primary-secure-live-benchmark/20260603T-final-3-converged/campaign-summary.json` |
| summary MD | `$REPO_ROOT/build-artifacts/uhf-primary-secure-live-benchmark/20260603T-final-3-converged/summary.md` |
| counted repetitions | `3` |
| environment blockers | `0` |

Top-level result:

- `quiet-command`
  - `targetRuntimeVerdict = PASS`
  - `groundObservabilityVerdict = PASS`
  - `noiseVerdict = NOISY`
- `nonquiet-command`
  - `targetRuntimeVerdict = PASS` in `1/3` repetitions
  - `groundObservabilityVerdict = PASS`
  - `noiseVerdict = NOISY/SEVERE`

Counted repetition summary:

- `paired-rerun-1`
  - counted
  - `quiet = PASS`
  - `nonquiet = FAIL`
  - `observability = PASS`
  - `nonquiet` timed out waiting for UHF `GET_RESET_CAUSE` command closure after
    UHF secure-auth was already established
- `paired-rerun-2`
  - counted
  - `quiet = PASS`
  - `nonquiet = PASS`
  - `observability = PASS`
- `paired-rerun-3`
  - counted
  - `quiet = PASS`
  - `nonquiet = FAIL`
  - `observability = PASS`
  - `nonquiet` timed out waiting for secure restore back to `SBAND` after UHF
    live observability had already been demonstrated

Key artifacts:

- quiet result:
  [paired-rerun-2/quiet-command/result.json](../../README.md)
- nonquiet result:
  [paired-rerun-2/nonquiet-command/result.json](../../README.md)
- representative nonquiet failure:
  [paired-rerun-1/nonquiet-command/result.json](../../README.md)

## Quiet Vs Nonquiet Aggregate

`quiet` and `nonquiet` must be read against different active observability
surfaces:

- `quiet`
  - active ground path is `S-band`
  - `UHF primary` remains intentionally quiet, so `UHF` event/channel visibility
    is expected to stay absent
- `nonquiet`
  - active ground path is `UHF`
  - `UHF primary` packet quiet is explicitly disabled, so `UHF` event/channel
    visibility is the thing being measured

Aggregate comparison from `campaign-summary.json`:

| Field | `quiet` aggregate | `nonquiet` aggregate |
|---|---:|---:|
| active ground path | `sband` | `uhf` |
| runtime pass count | `3/3` | `1/3` |
| observability pass count | `3/3` | `3/3` |
| active event visible count | `3/3` | `3/3` |
| active channel visible count | `3/3` | `3/3` |
| S-band downlink median | `96640` bytes | `111616` bytes |
| UHF downlink median | `2048` bytes | `52556` bytes |
| quiet-path checksum warning median | `0` | `82` |
| quiet/active-path cycle slip median | `8` | `8` |
| active-path `GROUND_LINK_RX_ERRORS` median | `1` | `0` |
| active-path `GROUND_LINK_TX_ERRORS` median | `0` | `0` |

The important distinction is:

- `quiet` proves the secure command path with active observability still on
  `S-band`
- `nonquiet` proves that reviewable `UHF` live observability can appear, but
  command-path closure is not reliably preserved across all repetitions

## What Quiet Proved

`quiet-command` proved:

- S-band secure-auth establishment succeeded
- S-band secure `GET_RESET_CAUSE` succeeded
- secure `COMM_SET_ACTIVE(UHF)` succeeded
- UHF primary re-auth succeeded
- UHF primary secure `GET_RESET_CAUSE` succeeded
- cleanup restored S-band primary

But the active observability surface in `quiet` is still `S-band`, not `UHF`.
This is why the aggregate shows:

- `S-band` `event/channel` visible in `3/3`
- `UHF` `event/channel` visible in `0/3`
- `UHF` downlink present only as bounded transport traffic with median
  `2048 bytes`

Representative counted captures:

- `paired-rerun-1`
  - S-band uplink/downlink: `233 / 96640 bytes`
  - UHF uplink/downlink: `155 / 2048 bytes`
- `paired-rerun-2`
  - S-band uplink/downlink: `233 / 83328 bytes`
  - UHF uplink/downlink: `155 / 2048 bytes`
- `paired-rerun-3`
  - S-band uplink/downlink: `233 / 104832 bytes`
  - UHF uplink/downlink: `155 / 2048 bytes`

## What Nonquiet Measured

`nonquiet-command` demonstrated reviewable UHF live observability in all three
repetitions, but it did **not** preserve full runtime closure in all three.

What held in all `3/3` repetitions:

- S-band secure-auth establishment succeeded
- S-band secure `GET_RESET_CAUSE` succeeded
- secure `COMM_SET_ACTIVE(UHF)` succeeded
- UHF primary re-auth succeeded

What held only in `1/3` repetitions:

- UHF primary secure `GET_RESET_CAUSE` closure
- secure restore back to `SBAND`

But unlike earlier superseded benchmark attempts, the corrected run also
captured reviewable live UHF observability:

- canonical UHF event evidence:
  [paired-rerun-2/nonquiet-command/uhf-ground/native-cli/event.log](../../README.md)
- canonical UHF channel evidence:
  [paired-rerun-2/nonquiet-command/uhf-ground/native-cli/channel.log](../../README.md)
- UHF raw downlink capture:
  [paired-rerun-2/nonquiet-command/captures/uhf/southbound-to-gds.bin](../../README.md)

Observed live UHF event examples:

- `COMM_BAND_SWITCH`
- `COMM_PRIMARY_LINK_CHANGED`
- `COMMAND_SESSION_REVOKED`
- `SECURE_AUTH_REVOKED`
- repeated `EPS_STATUS_RECEIVED`
- repeated `STATE_MONITOR_UPDATED`
- `GROUND_LINK_DOWN / ERROR / UP`

Observed live UHF channel examples:

- `OBCApp.commController.COMM_PRIMARY_COMMAND_LINK = UHF`
- `OBCApp.commController.COMM_PRIMARY_TELEMETRY_LINK = UHF`
- `OBCApp.commController.COMM_PRIMARY_FILE_LINK = UHF`
- `OBCApp.uhfGroundLinkDriver.GROUND_LINK_RX_ERRORS = 1`
- `ComCcsds.comQueue.comQueueDepth = ['0', '0', '0']`
- `OBCComCcsds.comQueue.comQueueDepth = ['2', '0', '0']`
- `OBCApp.systemResources.*`

Representative counted captures:

- `paired-rerun-1`
  - S-band uplink/downlink: `233 / 111616 bytes`
  - UHF uplink/downlink: `178 / 52556 bytes`
- `paired-rerun-2`
  - S-band uplink/downlink: `233 / 76800 bytes`
  - UHF uplink/downlink: `233 / 27834 bytes`
- `paired-rerun-3`
  - S-band uplink/downlink: `233 / 145344 bytes`
  - UHF uplink/downlink: `155 / 67519 bytes`

## Noise Shape

Canonical aggregate noise evidence should also be read against the active
ground path.

`quiet` active-path noise (`S-band`):

- `checksumWarningCount`
  - aggregate median `0`, max `0`
- `GROUND_LINK_RX_ERRORS`
  - aggregate median `1`, max `1`
- `GROUND_LINK_TX_ERRORS`
  - aggregate median `0`, max `0`
- `groundLinkChurnCount`
  - aggregate median `0`, max `0`
- `rateGroupCycleSlipCount`
  - aggregate median `8`, max `10`

`nonquiet` active-path noise (`UHF`):

- `checksumWarningCount`
  - `paired-rerun-1 = 82`
  - `paired-rerun-2 = 0`
  - `paired-rerun-3 = 429`
  - aggregate median `82`, max `429`
- `GROUND_LINK_RX_ERRORS`
  - aggregate median `0`, max `1`
- `GROUND_LINK_TX_ERRORS`
  - aggregate median `0`, max `0`
- `groundLinkChurnCount`
  - aggregate median `0`, max `0`
- `rateGroupCycleSlipCount`
  - aggregate median `8`, max `9`

So the current canonical benchmark conclusion is:

- `quiet` preserves the secure command path and keeps active observability on
  `S-band`
- `quiet` is not strictly `CLEAN`; even the `S-band` active path still shows
  low-level dirtiness such as `GROUND_LINK_RX_ERRORS` and `RateGroupCycleSlip`
- `nonquiet` restores reviewable live `UHF` event/channel observability in `3/3`
- `nonquiet` increases `UHF` downlink volume substantially
- `nonquiet` increases `UHF` noise substantially
- `nonquiet` degrades command-path reliability: only `1/3` repetitions reached
  full runtime closure
- no repetition is now excluded by postflight `journal:gps-state-missing`

## Notes

- Earlier benchmark artifacts from `2026-06-02` and the one-shot convergence
  sample under `20260603T-artifact-converged` are superseded for canonical
  observability conclusions because they used earlier artifact layout and were
  not the final formal 3-repetition run.
- Existing `CommEgressMux` `UHF_ROUTED_* / UHF_SUPPRESSED_*` counters were used
  during debugging, but they are not required evidence for this record and are
  not the basis of the canonical verdicts above.

## Context

The active repo truth already distinguishes two different timing questions:

1. structural timing truth for the service-managed Raspberry Pi baseline
2. numeric empirical ceilings for that same baseline under a declared workload

The first question is already closed by `target-timing-wcet-profile-proof-v1`.
The second is still open because the old timing probe mixed baseline-health
questions with timing-harness questions. That makes every `SESSION_OPEN` timeout
ambiguous: it could mean the service-managed node-`5` path regressed, or it
could mean the timing harness itself still has stale oracle or readiness logic.

The repository already has a better discriminator: the proven target node-`5`
control probe
`TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`.
This change uses that discriminator as a formal gate before timing measurement.

## Goals / Non-Goals

**Goals**

- Keep one follow-up change focused only on the numeric service-managed timing
  gap from `target-timing-wcet-profile-proof-v1`.
- Separate baseline-health truth from timing-harness truth before freezing any
  empirical timing number.
- Reuse the active service-managed node-`5` baseline:
  `TARGET_COMM_PROFILE=sband`, `COMM_CSP_NODE=5`,
  `COMMAND_AUTHORITY_PROFILE=sband-primary`.
- Require three fresh timing runs, with one explicit service-restart rerun,
  before freezing empirical service-managed ceilings.
- Record either:
  - frozen empirical service-managed ceilings for the declared workload, or
  - one narrow blocker classification with reviewable evidence

**Non-Goals**

- Do not redesign target scheduling, rate-group membership, or mission
  autonomy.
- Do not widen scope into radio metrics, reliable transfer, secure boot,
  payload feature expansion, or legacy node-`4` rollback.
- Do not describe Raspberry Pi empirical timing as universal flight-processor
  WCET or final onboard scheduler jitter truth.

## Design

### 1. Two-Stage Probe Workflow

The top-level timing closure workflow will have two mandatory stages:

1. `control-partition preflight`
2. `timing measurement`

The new shell entrypoint will fail fast if the preflight fails. That failure is
classified as `baseline-regression`, not a timing-ceiling failure.

Only after the preflight passes may the timing measurement stage begin. This
lets the timing stage classify failures more narrowly as either:

- `oracle/harness-gap`
- `measurement-insufficient`

### 2. Control-Partition Preflight

The preflight is the existing repository-owned proven node-`5` control probe:

```bash
TARGET_COMM_PROFILE=sband PROBE_MODE=command-path \
bash scripts/run_rpi_target_recovery_restart_probe.sh
```

This stage is reused, not reimplemented. It already governs:

- installed service truth on `obc-comm-csp-stack.service`
- authenticated `SESSION_OPEN`
- target journal and `fprime-cli events` dual-source observation
- persisted session-floor-aware reopen behavior

The new timing closure workflow will store the preflight artifact root and
verdict alongside the timing evidence.

### 3. Timing Measurement Contract

The new per-run timing probe will still measure the same declared workload
surface, but it will change how readiness and verdict are determined.

#### Session / observation contract

- Installed service truth is snapshotted first:
  - `TARGET_COMM_PROFILE`
  - `COMM_CSP_NODE`
  - `COMMAND_AUTHORITY_PROFILE`
  - `COMMAND_AUTH_SOURCE_ID`
  - `COMMAND_AUTH_KEY_SLOT`
  - `COMMAND_AUTH_KEY_HEX` presence source
  - `COMM_SUBSYSTEM_PING_TIMEOUT_MS`
  - `COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD`
- Target readiness is checked from the installed service truth, not from baked
  probe constants. For the node-`5` path, readiness means the target journal
  shows the configured `CSP ping node 5 success 1 timeout <configured> ms`
  fragment.
- The original `STORAGE_SCAN_COUNT` slow oracle is explicitly retired for this
  probe. `StorageHealthBridge` only scans on the first slow tick and then every
  fourth slow tick, so it cannot prove the nominal `5s` rate-group cadence
  inside the declared `90s` count-gated window. The replacement slow oracle is
  a dedicated `StorageHealthBridge` telemetry counter emitted on every
  `schedIn_handler` call, keeping the measurement bound to repo-owned
  rate-group truth instead of live GPS or file-transfer activity.
- `SESSION_OPEN` acceptance is aligned with the proven control probe:
  `COMMAND_SESSION_OPENED` plus the resolved session id is the hard oracle.
- `COMMAND_ENVELOPE_OBSERVED` is kept only as diagnostics. Missing envelope
  observation must not by itself fail a passing command or session result.
- Retry budget matches the proven control path:
  - `12` attempts
  - fresh higher `session_id`
  - persisted-floor-aware reopen

#### Measurement contract

The old fixed-duration `22s` steady window is replaced with count-gated
windows. Each window starts only after the fast, slow, and data listeners have
already observed at least one initial sample.

For each window:

- fast requires at least `30` intervals
- data requires at least `30` intervals
- slow requires at least `10` intervals
- the window has `90s` to reach those counts

If the counts are not met in time, the run fails as
`measurement-insufficient`.

The two windows remain:

1. `steady-state`
2. `steady-state-plus-representative-activity`

Representative activity stays bounded to:

- governed `SESSION_OPEN`
- bounded COMM activity
- `MODE_SET PAYLOAD`
- `PAYLOAD_PREPARE_SESSION(AUTO)`
- `PAYLOAD_CAPTURE_AUTO`
- `PAYLOAD_GET_LAST_CAPTURE_METADATA`
- `PAYLOAD_SHUTDOWN`
- `MODE_SET IDLE`

#### Timing verdict

Each per-run window passes only when:

- `RateGroupCycleSlip` events are all `0`
- `RgCycleSlips` per-window channel deltas are all `0`

The run records:

- sample counts per window
- interval summaries for fast / slow / data
- per-window `RgMaxTime`
- per-window `RgCycleSlips` start / end / delta
- service snapshot and session-open source

### 4. Aggregated Empirical Ceiling Freeze

The top-level workflow will run the per-run timing probe three times:

1. fresh run
2. fresh run
3. fresh run after explicit `systemctl restart obc-comm-csp-stack.service`

If all three runs pass, the workflow freezes:

- empirical service-managed `RgMaxTime` ceilings as the maximum observed
  per-group value across all passing runs and both windows
- empirical inter-arrival bounds as the widest observed min / max intervals and
  the maximum absolute deviation from nominal across the same passing set

These numbers are documented as:

`empirical service-managed ceilings for the declared node-5 workload`

They are not described as universal final flight WCET or scheduler jitter
truth.

If any timing run fails after a passing control preflight, the change records a
narrow blocker instead of broad timing `TBD` language:

- `oracle/harness-gap`
- `measurement-insufficient`

If the control preflight itself fails, the change records:

- `baseline-regression`

### 5. Documentation Surfaces

The change will always add:

- `evidence/records/target-timing-empirical-ceiling-freeze-v1/README.md`

It will update:

- `evidence/verification-path-registry.md` entry `68`
- `docs/interfaces.md`

It will update current-baseline / next-work / architecture docs only if the
numeric target timing truth really changes or if the remaining blocker is
narrower than the current wording.

## Risks / Trade-offs

- [Risk] The service-managed baseline may still pass the control preflight but
  fail to collect enough slow-group samples inside the declared bound.
  - Mitigation: treat this as `measurement-insufficient` and record the exact
    sample shortfall rather than weakening the counting contract.
- [Risk] The timing harness may still rely on an outdated success oracle.
  - Mitigation: reuse the proven node-`5` session-open / readiness discipline
    and demote `COMMAND_ENVELOPE_OBSERVED` to diagnostics-only.
- [Risk] Reviewers may still over-read the frozen numbers.
  - Mitigation: keep the empirical scope wording explicit in evidence, the
    registry, and `docs/interfaces.md`.

## Migration Plan

1. Create proposal, design, tasks, and any minimal formal delta required by the
   active OpenSpec workflow.
2. Add the new top-level timing closure probe shell entrypoint and per-run
   Python helper.
3. Align the per-run timing probe with the proven node-`5` control-path
   readiness and session-open contract.
4. Run fresh local verification and the target control preflight.
5. Repackage / reinstall the target bundle, run three fresh timing runs, and
   restart the service before one of them.
6. Update evidence, registry, and interface docs with either frozen empirical
   ceilings or a narrowed blocker classification.
7. Run OpenSpec validation and keep the branch locally reviewable.

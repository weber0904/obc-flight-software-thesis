## Why

`target-timing-wcet-profile-proof-v1` already froze the active Raspberry Pi
service-managed structural timing contract, but it left the numeric
service-managed `RgMaxTime` and inter-arrival/jitter ceilings open because the
governed node-`5` observation path did not yet produce a clean enough record to
freeze them honestly.

That remaining gap is now narrow enough to handle as one follow-up change. The
next step is not to widen scope into scheduler work, radio metrics, or new
payload capabilities. It is to prove whether the active
`obc-comm-csp-stack.service` node-`5` path can produce repeatable empirical
timing ceilings, and if it cannot, to classify the blocker precisely instead of
falling back to broad timing `TBD` language.

## What Changes

- Add a new repository-owned target timing closure workflow for the active
  Raspberry Pi service-managed node-`5` baseline:
  `scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh` plus a
  bounded helper Python probe.
- Make the workflow mandatory two-stage:
  - control-partition preflight using the already proven node-`5`
    `command-path` probe
  - timing measurement only after that preflight passes
- Align the timing probe's session-open, target-readiness, and observation
  contract with the proven node-`5` control path instead of relying on the
  older looser timing-only harness rules.
- Capture per-run installed service truth, three fresh timing runs, and one
  explicit service-restart rerun so the repo can either freeze empirical
  service-managed ceilings or record a narrower blocker classification.
- Update evidence, the verification-path registry, and current timing docs so
  numeric timing closure is either frozen as an empirical service-managed
  contract for the declared node-`5` workload or left open with an explicit
  bounded blocker class.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- Repository-owned target timing verification now includes a governed
  control-partition preflight and multi-run empirical ceiling aggregation for
  the active node-`5` service-managed baseline.
- Current timing evidence and documentation may promote empirical
  service-managed ceilings from residual gap to frozen empirical boundary when
  the governed workload windows pass cleanly.

## Impact

- Affected code and verification:
  - new repository-owned timing closure probe entrypoint and helper
  - target node-`5` timing evidence aggregation for three fresh runs
- Affected docs:
  - `docs/test-records/target-timing-empirical-ceiling-freeze-v1/README.md`
  - `docs/verification-path-registry.md`
  - `docs/interfaces.md`
  - current-baseline / next-work / architecture docs only if the numeric
    service-managed truth actually changes
- Intended non-claims remain explicit:
  - no final flight-processor hard real-time closure
  - no scheduler redesign
  - no radio metrics or reliable-transfer expansion
  - no payload capability growth

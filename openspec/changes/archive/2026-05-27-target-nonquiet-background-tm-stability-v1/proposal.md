## Why

The active target/lab COMM baseline already proves:

- default node-`5` service-managed TT&C
- bounded quiet node-`6` command, file, sequence, failover, and beacon-suppress
  paths
- explicit non-claims around general non-quiet target node-`6` stability under
  background telemetry

What remains unresolved is narrower than dual-link orchestration and broader
than one failed probe run:

1. the repo does not yet have a governed target CAN node-`6` non-quiet
   diagnosis path
2. current truth does not yet say whether the residual node-`6` problem is
   mainly acceptance/oracle contamination, real runtime/egress instability, or
   a mixed boundary

This change closes that gap first so later target-bearing UHF or dual-link work
does not build on an unclassified node-`6` residual.

## What Changes

- Add a formal OpenSpec diagnosis slice for the target CAN node-`6` non-quiet
  path under background telemetry.
- Add a dedicated repository-owned non-quiet target CAN node-`6` diagnosis
  wrapper instead of widening the meaning of existing quiet matrix cases.
- Keep current quiet node-`6` proofs unchanged as the control path.
- Reuse target TCP node-`6` behavior as a supporting comparator only.
- Record four truth surfaces together for the diagnosis:
  - target journal
  - ground `fprime-cli events`
  - ground `fprime-cli channels`
  - gateway byte captures
  - node-`6` beacon/debug capture when relevant
- Classify the residual issue as `oracle`, `runtime`, or `mixed`.
- Apply only the smallest correct change for the proven boundary.
- Update specs/docs/evidence so current truth states exactly what is and is not
  now proven about target node-`6` non-quiet behavior.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: add a governed diagnosis boundary for target node-`6`
  non-quiet behavior and keep runtime ownership explicit if a real product bug
  is proven
- `verification-evidence`: require multi-oracle target node-`6` diagnosis
  evidence with quiet control, non-quiet case, and comparator
- `verification-path-registry`: keep quiet and non-quiet node-`6` truth
  separate and update the registry only if a genuinely reusable non-quiet
  boundary is proven

## Impact

- Affected scripts:
  - target CAN matrix helper and a new dedicated non-quiet diagnosis wrapper
- Affected docs/specs:
  - `openspec/specs/comm-subsystem/spec.md`
  - `openspec/specs/verification-evidence/spec.md`
  - `openspec/specs/verification-path-registry/spec.md`
  - current architecture/current-baseline/current COMM follow-up docs
  - target operator/runbook and the final evidence record if the diagnosis is
    executed
- Runtime/code impact is intentionally conditional:
  - none if the issue is oracle-only
  - narrow, owner-correct COMM runtime or egress changes only if non-quiet
    target evidence proves a real product-side fault

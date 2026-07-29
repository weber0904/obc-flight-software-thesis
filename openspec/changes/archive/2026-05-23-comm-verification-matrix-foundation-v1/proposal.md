## Why

The current `formal-comm-verification-matrix-v1` suite is now a usable
checkpoint, but several cells still depend on placeholder blocker wrappers,
loosely-composed composite probes, or metadata contracts that are only implied
by the implementation. Before adding more target TCP or target CAN behavior,
the matrix orchestration layer itself needs to become a trustworthy governed
surface.

The immediate need is to harden the matrix foundation so later cell-closure
changes can focus on communication behavior rather than reworking orchestration
details.

## What Changes

- Normalize the matrix case metadata contract, summary schema, carrier-kind
  naming, and blocker taxonomy.
- Harden cleanup smoke and rerun-safety verification for the matrix wrappers.
- Make wrapper intent explicit: reuse, narrowed proof, or bounded blocker.
- Update umbrella documentation so the matrix checkpoint and follow-on stack
  boundaries are easy to review.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require a reviewable and stable orchestration
  contract for matrix summaries, blocker reporting, and rerun-safety output.

## Impact

- Affected code:
  - `scripts/comm_verification/lib/`
  - `scripts/comm_verification/matrix/`
  - selected case wrappers that need clearer metadata semantics
  - umbrella matrix evidence and runbook references
- Public/operator impact:
  - no new communication path is claimed yet
  - operators gain a more trustworthy matrix summary and cleanup contract
- Non-goals:
  - no target TCP parity launcher yet
  - no new sequence-subsystem or failover closure yet

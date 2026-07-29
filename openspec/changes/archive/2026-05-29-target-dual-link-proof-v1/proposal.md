## Why

The repository already froze the exact future target-bearing dual-link claim in
`target-dual-link-claim-oracle-clarification-v1`, but that change
intentionally proved no new path. The active baseline therefore still lacks one
implementation-bearing, repository-owned target proof that closes a bounded
primary-led, switch-closed family on the physical target CAN + UHF UART path.

That missing step is now the main review gap. The repo has default target
node-`5` truth, quiet target node-`6` truth, target TCP comparator support,
and oracle-bounded non-quiet diagnosis, but it still does not have one
governed target proof that:

- keeps default node-`5` S-band truth as the governing primary surface
- exercises physical non-quiet node-`6` `uhf-backup` as a bounded adjunct
- closes formal UHF command truth only after explicit switch to
  `uhf-primary-after-failover`
- records target truth and operator observability as separate formal verdicts
- stays honest if the official run lands as `PASS/PASS` or `PASS/DEGRADED`

## What Changes

- Add a dedicated repository-owned target dual-link proof wrapper for the
  physical target CAN + UHF UART topology.
- Extend the existing target CAN governed probe with one new mode for this
  exact phase model instead of creating a separate proof stack.
- Implement one bounded proof family:
  - phase A: default node-`5` S-band primary command truth
  - phase B: non-quiet physical node-`6` `uhf-backup` adjunct with one minimal
    allowlisted read/status command
  - phase C: explicit switch to `uhf-primary-after-failover`, then formal
    non-quiet node-`6` UHF command truth
  - phase D: quiet node-`6` rescue only for a failed phase B adjunct, never as
    a substitute for phase C
- Record a dual-verdict outcome for the official run:
  - `target-claim`
  - `operator-observability`
- Register only the exact successful branch that the official governed run
  actually proves.
- Update current docs, evidence wording, and registry/spec references so the
  repo states what is now actually proven and what remains a non-claim.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: implement the first bounded target-bearing dual-link proof
  surface on the physical target path without widening the frozen claim.
- `ground-ttc-gateway`: keep ground-side gateway artifacts as operator
  observability support only, not the target-truth oracle.
- `interface-contract-index`: update `docs/interfaces.md` from a frozen future
  boundary to the exact implementation-bearing boundary and branch actually
  proven.
- `verification-evidence`: require the official evidence record to publish the
  exact branch exercised, dual verdicts, oracle precedence, rescue use, and
  residual non-claims.
- `verification-path-registry`: register a new target-bearing path only when
  fresh reviewed proof closes it, and describe only the exact proven branch.

## Impact

- Affected code:
  - `scripts/comm_verification/lib/run_target_can_matrix_probe.py`
  - one new target proof wrapper under `scripts/`
- Affected formal artifacts:
  - `proposal.md`, `design.md`, `tasks.md`
  - delta specs for the five capabilities above
- Affected current docs:
  - current architecture, current baseline, next-work, interfaces, target
    operator runbook, and verification-path registry
- This change does not claim:
  - symmetric dual-authority command closure
  - one-GDS multi-upstream behavior
  - one-gateway multiplexing
  - RF closure
  - UHF reliable-transfer redesign
  - packet-quiet or beacon-suppress redesign

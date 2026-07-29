## Why

The current maintained hosted near-term simultaneous baseline already closes
layer-1 operator truth: two distinct stock `fprime-gds` plus
`ground_ttc_gateway` stacks share one hosted `TopCcsds` runtime, and the
combined wrapper is explicitly composition-only. That baseline is reviewable,
but it does not provide a distinct layer-2 orchestration surface with its own
hosted lifecycle, failure, and cleanup contract.

This change is needed now because the repository wants a hosted-first answer to
the higher-level dual-link orchestration question without reopening COMM
runtime semantics or pretending the existing `43B` baseline already solved
orchestration ownership. The repo needs one honest layer-2 owner that can be
proven through orchestration-owned status and cleanup surfaces alone.

## What Changes

- Add a hosted-only thin lifecycle owner above the maintained per-band stock
  baseline.
- Keep layer-1 unchanged:
  - S-band stock `fprime-gds` + `ground_ttc_gateway` + node `5`
  - UHF stock `fprime-gds` + `ground_ttc_gateway` + node `6`
  - one shared hosted `TopCcsds` runtime
  - one combined wrapper that remains composition-only
- Add a distinct layer-2 orchestration entrypoint and helper that:
  - owns startup, ready, stopping, stopped, startup-failed, and cleanup-failed
    lifecycle state
  - records owned process set, shared-runtime interaction, and cleanup summary
  - keeps per-band TT&C semantics, gateway relay, stock GDS behavior, and
    COMM runtime policy explicitly delegated
- Add a dedicated hosted-first orchestration proof path that uses only
  orchestration-owned manifest/status/cleanup surfaces as its acceptance
  oracle.
- Add a dedicated hosted orchestration runbook and sync current-facing docs so
  they clearly separate:
  - layer-1 maintained per-band baseline
  - layer-2 hosted orchestration owner
  - deferred future target-bearing simultaneous work

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `ground-ttc-gateway`: add a distinct hosted orchestration owner surface above
  the maintained per-band stock-stack baseline and define its owner boundary.
- `verification-evidence`: require hosted orchestration evidence that proves
  lifecycle/failure/cleanup through orchestration-owned artifacts instead of
  older COMM semantic oracles.
- `verification-path-registry`: add a dedicated hosted orchestration-owner
  path that is hard-separated from `43B`.
- `planning-docs`: require roadmap/current-baseline wording to separate the
  maintained layer-1 baseline, the new layer-2 owner, and deferred
  target-bearing work.

## Impact

- Affected code:
  - one new hosted orchestration helper or launcher surface above the existing
    per-band helper
  - one new hosted orchestration proof wrapper
- Affected docs/specs:
  - OpenSpec change artifacts and the four modified main specs above
  - current architecture, roadmap, interface index, scripts inventory, a new
    hosted orchestration runbook, verification registry, and new evidence docs
- Affected operations:
  - hosted dual-link lifecycle coordination gains one explicit owner surface
  - this change does not claim command authority ownership, gateway
    multiplexing, one-GDS aggregation, COMM runtime ownership, target-bearing
    simultaneous proof, RF closure, or any reopening of frozen UHF semantics

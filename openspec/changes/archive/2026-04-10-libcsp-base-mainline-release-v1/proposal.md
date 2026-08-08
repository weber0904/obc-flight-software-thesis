## Why

The `feature/libcsp-internal-network-base` branch now contains the corrected libcsp-first internal subsystem baseline: CSP foundation, EPS node `2`, ADCS node `3`, and the legacy direct-ZMQ retirement guardrail. Before this branch can become the future development base, the repository needs a governed release-readiness change that records the verification gate and makes the mainline merge boundary explicit.

## What Changes

- Treat `feature/libcsp-internal-network-base` as the release candidate for the next formal mainline baseline.
- Re-run and record the local verification gate, OpenSpec validation, and legacy direct-ZMQ checker.
- Keep the ground path, external comm path, and GPS path clearly separate from the internal libcsp network.
- Update review docs and evidence so future developers know that the active EPS/ADCS internal path is libcsp over the hosted ZMQHUB-backed substrate with no direct-ZMQ fallback.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: Declare the libcsp integration branch as the candidate mainline baseline after verification.
- `verification-evidence`: Require release-readiness evidence before merging the libcsp integration base back to `main`.

## Impact

- Affected docs/evidence: README, verification matrix, reconciliation matrix, reporting package wording, and `evidence/records/libcsp-base-mainline-release-v1/`.
- Affected runtime code: none expected.
- Affected scripts: no required behavior change.

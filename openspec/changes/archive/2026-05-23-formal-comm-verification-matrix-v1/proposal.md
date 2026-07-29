## Why

The repository has accumulated multiple governed communication proofs, but the
active operator surface is still spread across many single-purpose scripts in
the flat `scripts/` tree. That makes it harder to answer a simple question
before new development starts: which formal communication paths are healthy
right now, and which ones are already broken or still missing a governed
probe?

The immediate need is a reviewable revalidation suite for the current
`TopCcsds` baseline that:

- groups formal communication checks by environment instead of by historical
  change name
- keeps `fprime-cli -> GDS -> OBC` direct control-path isolation separate from
  satcom-mediated S-band and UHF paths
- distinguishes development-carrier TCP proofs from target/lab CAN plus
  physical UHF UART provenance
- provides stable, rerunnable entrypoints that do not add more logic to the
  flat `scripts/` directory

## What Changes

- Add a new formal communication verification suite under
  `scripts/comm_verification/` with shared helpers, environment definitions,
  single-capability case wrappers, and matrix runners.
- Add thin root-level wrapper scripts that delegate into the new subtree.
- Create governed OpenSpec artifacts, operator runbook guidance, and evidence
  scaffolding for `formal-comm-verification-matrix-v1`.
- Reuse existing governed probe scripts where they already prove the required
  path, and classify unsupported or unstable paths explicitly instead of
  silently skipping them.
- Keep `formal-comm-verification-matrix-v1` as the umbrella matrix contract
  and evidence dashboard while follow-on changes close blocked cells in
  smaller, reviewable slices:
  - `comm-verification-matrix-foundation-v1`
  - `comm-verification-sequence-subsystem-harness-v1`
  - `target-can-node6-matrix-closure-v1`
  - `target-tcp-southbound-parity-foundation-v1`
  - `target-tcp-matrix-closure-v1`
  - `target-direct-control-matrix-cases-v1`

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require a reviewable matrix-oriented surface for the
  formal communication revalidation suite, including case verdicts, carrier
  kind metadata, rerun-safety output locations, and unresolved blockers.

## Impact

- Affected code:
  - new `scripts/comm_verification/` subtree
  - new root wrapper scripts for the comm-verification suite
  - new operator runbook and evidence record scaffold
  - OpenSpec artifacts for the new suite
- Public/operator impact:
  - operators and developers gain a single family of entrypoints for hosted,
    target TCP dev-carrier, and target CAN plus physical-UHF verification
- Non-goals:
  - no flight-runtime, topology, command schema, or wire-format changes
  - no immediate expansion of the verification-path registry for paths that
    still fail or remain blocked during revalidation
  - no unrelated cleanup sweep across historical flat `scripts/`

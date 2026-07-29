## Why

The target TCP matrix is currently blocked not because the capability family is
unknown, but because the repository lacks a governed three-host TCP parity
launcher for node `5`, node `6`, and the subsystem simulators. Existing remote
and split-host proofs are useful ingredients, but they do not yet express the
matrix cells with the same topology and carrier semantics.

This change establishes that target TCP parity foundation first.

## What Changes

- Define the official three-host target TCP parity topology.
- Add governed launchers and helpers for:
  - `macOS` ground stack
  - `obc.local` OBC-only target process
  - `subsystem.local` node `5`, node `6`, EPS, and ADCS services
- Close target TCP `csp-reachability`, `sband-command`, `sband-file`,
  `uhf-primary-command`, and `uhf-primary-file`.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require reviewable target TCP development-carrier
  evidence for node-`5` and node-`6` southbound parity without confusing it
  with target physical-UHF provenance.

## Impact

- Affected code:
  - new target TCP parity launcher or helper scripts
  - target TCP case wrappers and focused evidence
- Public/operator impact:
  - target TCP development-carrier matrix cells become explicit and governed
- Non-goals:
  - no target direct-control case yet
  - no target TCP sequence or failover closure yet

## Why

The current repository has maintained hosted and target/lab COMM baselines,
but the operator-facing surfaces are still fragmented across proof wrappers,
baseline managers, and lower-level launchers. That makes it awkward to bring
up a complete dual-GDS environment, keep the full hosted or target/lab stack
running, and then manually perform secure auth, secure commands, and governed
sequence/file actions from a stable operator-owned surface.

The repo now needs a formally maintained manual operator family that is
separate from probe/proof entrypoints, keeps hosted and target responsibilities
clear, and gives human operators a reviewable way to open dual GDS surfaces and
drive the current secure TT&C plus sequence/file workflow without reusing
probe-specific lifecycle assumptions.

## What Changes

- Add a dedicated `scripts/manual_ops/` subtree for the maintained manual
  operator family instead of scattering new wrappers under `scripts/`.
- Add separate hosted and target manual lifecycle surfaces:
  - hosted dual-GDS manual surface
  - target remote baseline manager
  - target local dual-GDS ground surface
- Add a repo-owned `manual_secure_ops.py` helper that performs:
  - secure auth establishment and clearing
  - secure-v2 command send
  - governed `.sequence-staging/<leaf>` upload
  - governed `SEQ_*` wrapper commands
- Extend the maintained launcher stack so manual surfaces can run in
  `GDS_UI_MODE=ui|headless` while preserving current headless proof defaults.
- Standardize a manifest/status contract that manual helpers and runbooks can
  consume instead of re-deriving ports, file stores, or southbound endpoints.
- Add current operator runbooks for the hosted and target manual surfaces and
  route repo entrypoints toward this new subtree.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `ground-ttc-gateway`: adds a maintained manual dual-GDS operator surface
  family that still keeps stock `fprime-gds` plus repo-owned
  `ground_ttc_gateway` as the current operator boundary.
- `comm-subsystem`: adds a maintained manual secure-ops helper surface for
  current secure auth, secure-v2 commands, and governed sequence/file actions
  without turning proof wrappers into operator entrypoints.
- `interface-contract-index`: records the manual operator manifest/status
  contract, session-state invalidation rules, and hosted/target role split.
- `documentation-governance`: requires the new manual operator subtree and its
  runbooks to be documented from current repo entrypoints.

## Impact

- Affected code:
  - `scripts/manual_ops/*`
  - `scripts/run_ground_gds_only_stack.sh`
  - `scripts/run_target_comm_csp_ground_stack.sh`
  - `scripts/per_band_stock_ground_stacks.py`
- Affected docs:
  - `docs/operator/hosted-manual-dual-gds-runbook.md`
  - `docs/operator/target-manual-dual-gds-runbook.md`
  - `README.md`
  - `scripts/README.md`
- Non-goals:
  - no GDS UI command interception or secure-command proxying
  - no one-GDS aggregation or one-gateway simultaneous multiplexer
  - no RF claim, payload preview/raw expansion, or broad mission-ops surface
  - no reopening of legacy `SESSION_OPEN` as the default operator flow

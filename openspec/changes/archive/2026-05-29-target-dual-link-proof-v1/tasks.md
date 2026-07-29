## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `comm-subsystem`, `ground-ttc-gateway`, `interface-contract-index`,
  `verification-evidence`, and `verification-path-registry`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-dual-link-proof-v1`.

## 2. Target Proof Implementation

- [x] 2.1 Add a dedicated repository-owned wrapper for the physical target
  dual-link proof.
- [x] 2.2 Extend
  `scripts/comm_verification/lib/run_target_can_matrix_probe.py` with one new
  `dual-link-proof` mode rather than creating a separate probe stack.
- [x] 2.3 Keep phase B bounded to one minimal allowlisted `uhf-backup`
  read/status command, and make quiet rescue phase-B-only.
- [x] 2.4 Keep phase C independently non-quiet and switch-closed even if phase
  B used quiet rescue.

## 3. Evidence, Registry, And Docs

- [x] 3.1 Add a formal evidence record for `target-dual-link-proof-v1` with
  exact path, phase sequence, oracle precedence, verdicts, rescue use, and
  residual non-claims.
- [x] 3.2 Add a new verification-path registry entry for the physical
  target-bearing proof path that describes only the exact branch the official
  run proved.
- [x] 3.3 Update current architecture, roadmap, interfaces, target runbook,
  and related main specs so they distinguish the newly proven path from the
  remaining non-claims.

## 4. Verification

- [x] 4.1 Run touched Python compile checks and shell syntax checks.
- [x] 4.2 Run fresh local verification for the touched proof surfaces.
- [x] 4.3 Run the governed target dual-link proof on the physical target path.
- [x] 4.4 Run the node-`5` command-path preflight after any failed or degraded
  attempt and at final cleanup confirmation.
- [x] 4.5 Run `openspec validate target-dual-link-proof-v1` and
  `openspec validate --specs`.

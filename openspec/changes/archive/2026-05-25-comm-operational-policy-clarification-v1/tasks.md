## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, and `tasks.md` for
  `comm-operational-policy-clarification-v1`.
- [x] 1.2 Freeze the clarification as a spec/design-first slice only and keep
  session-aware beacon suppress runtime implementation out of scope.

## 2. Formal Spec Updates

- [x] 2.1 Update `comm-subsystem` to freeze the three-layer boundary model,
  `SESSION_OPEN(seq0)` UHF policy-entry boundary, COMM-owned beacon policy,
  single-path relay truth, and retry/observability boundaries.
- [x] 2.2 Update `ground-ttc-gateway` to keep the gateway bounded as a raw
  single-link relay rather than an authority owner, multiplexer, or
  reliable-transfer engine.
- [x] 2.3 Update `target-comm-node56-migration` to distinguish `uhf-backup`
  from `uhf-primary-after-failover` while keeping the explicit-switch boundary.
- [x] 2.4 Update `live-beacon-broadcast` and
  `onboard-data-products-and-live-beacon` to preserve beacon and stored-history
  roles while keeping suppress runtime unclaimed.
- [x] 2.5 Update `interface-contract-index`,
  `verification-path-registry`, and `verification-evidence` so current policy
  wording cites existing evidence and keeps current non-claims explicit.

## 3. Current Doc Sync

- [x] 3.1 Update `docs/architecture/current-development-architecture.md`,
  `docs/interfaces.md`, and `docs/roadmap/current-baseline.md` so current
  baseline wording matches the clarified formal policy truth.
- [x] 3.2 Update `docs/roadmap/next-work.md` and
  `docs/architecture/comm-followup-directions.md` so future work picks up only
  the remaining implementation-bearing follow-ons.
- [x] 3.3 Update `evidence/verification-path-registry.md` and
  `docs/operator/formal-comm-verification-matrix-v1-runbook.md` to keep
  evidence, current non-claims, and matrix wording aligned with the formal
  clarification.

## 4. Validation

- [x] 4.1 Run `openspec validate comm-operational-policy-clarification-v1`.
- [x] 4.2 Run `openspec validate --specs`.
- [x] 4.3 Run `python3 scripts/check_documentation_governance.py`.
- [x] 4.4 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.5 Review the final diff and confirm the change remains a single formal
  clarification slice with no unintended runtime implementation scope growth.

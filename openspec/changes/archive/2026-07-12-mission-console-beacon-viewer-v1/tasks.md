## 1. Beacon Viewer Runtime

- [x] 1.1 Add hosted and target manual-surface beacon capability plumbing.
- [x] 1.2 Add Mission Console beacon manager, latest/history persistence, API,
  dashboard summary, and `/beacon` page.
- [x] 1.3 Keep beacon viewer state separate from readback, event, and channel
  caches.

## 2. Focused Verification

- [x] 2.1 Add focused Mission Console tests for beacon capability discovery,
  decode, short-frame reject, bounded history, dashboard summary, and beacon
  routes.
- [x] 2.2 Capture fresh hosted beacon viewer API evidence.
- [x] 2.3 Capture fresh target beacon viewer API evidence.

## 3. Demo-First Docs And Evidence

- [x] 3.1 Add a dedicated branch-local test record with repo-backed artifacts.
- [x] 3.2 Add a dedicated operator note for the beacon viewer demo flow.
- [x] 3.3 Update branch-local runbook/interface/architecture/registry wording
  to reflect the beacon viewer surface and its non-claims.

## 4. Post-Demo Follow-up

- [x] 4.1 Revisit manual-surface owner detach hardening if the launcher
  `owner-pid-dead` artifact still matters outside the Codex exec harness.
- [x] 4.2 Extract or rebase beacon-only commits onto the final parent/mainline
  path after the Route 1 dependency branch is formally settled.

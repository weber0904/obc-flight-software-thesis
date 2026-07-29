## 1. OpenSpec Artifacts

- [x] 1.1 Draft `proposal.md`, `design.md`, and `tasks.md` for
  `target-nonquiet-background-tm-stability-v1`.
- [x] 1.2 Add change-local spec deltas for `comm-subsystem`,
  `verification-evidence`, and `verification-path-registry`.

## 2. Diagnosis Harness

- [x] 2.1 Extend the target CAN matrix helper with a dedicated node-`6`
  command/non-quiet diagnosis path that preserves the current quiet control
  boundary.
- [x] 2.2 Add a repository-owned wrapper that runs:
  - quiet target CAN node-`6` control
  - non-quiet target CAN node-`6` diagnosis
  - target TCP node-`6` comparator
- [x] 2.3 Ensure the diagnosis captures target journal, ground events,
  ground channels, gateway byte captures, and beacon/debug capture when
  relevant.

## 3. Root-Cause Closure

- [x] 3.1 Run the governed diagnosis and classify the result as `oracle`,
  `runtime`, or `mixed`.
- [x] 3.2 The fresh result is oracle-only under a healthy transport baseline,
  so limit implementation to probes, acceptance logic, evidence wording, and
  current/formal docs.
- [x] 3.3 Record explicitly that no owner-correct product runtime fix is
  justified by the fresh evidence in this change.
- [x] 3.4 Keep residual non-claims explicit if full target node-`6`
  non-quiet closure is still not achieved.

## 4. Evidence And Docs

- [x] 4.1 Record a new evidence report under
  `docs/test-records/target-nonquiet-background-tm-stability-v1/README.md`
  with exact paths, oracle agreement/divergence, and final classification.
- [x] 4.2 Update current architecture/current baseline/current COMM follow-up
  docs so they state the resolved boundary without over-claiming closure.
- [x] 4.3 Update the operator runbook and keep
  `docs/verification-path-registry.md` unchanged because this change improves
  the diagnosis boundary but does not yet promote a reusable general non-quiet
  node-`6` path.

## 5. Verification

- [x] 5.1 Run fresh local verification for all touched code and scripts.
- [x] 5.2 Run the focused target/lab diagnosis wrapper and keep the quiet
  control and non-quiet case results adjacent.
- [x] 5.3 Run `openspec validate target-nonquiet-background-tm-stability-v1`.
- [x] 5.4 Run `openspec validate --specs`.

## 1. OpenSpec governance artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `document-realignment-governance-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate document-realignment-governance-v1`.

## 2. Current/canonical document realignment

- [x] 2.1 Reduce `README.md` and `AGENTS.md` to low-churn onboarding entrypoints
  while preserving current scope, authority boundaries, and workflow pointers.
- [x] 2.2 Reconcile current/canonical narrative docs for active deployment,
  COMM path identity, mission-history boundary, recovery ownership, workflow
  handoff, and roadmap direction.
- [x] 2.3 Add or normalize required freshness metadata on current-facing
  narrative docs that are meant to describe current truth.

## 3. Snapshot and package boundary cleanup

- [x] 3.1 Refresh `docs/reporting/README.md`,
  `docs/architecture-review/current/**`, and `docs/thesis/**` so their
  freshness boundary and non-canonical or snapshot status are explicit.
- [x] 3.2 Repair only the misleading archive/index wording or moved-path
  references needed to keep historical layers from being misread as current.

## 4. Governance checker hardening

- [x] 4.1 Add a dedicated documentation governance checker covering required
  metadata, snapshot-boundary markers, and current-entrypoint routing rules.
- [x] 4.2 Wire the checker into the repository consistency script and shared
  verification gate.
- [x] 4.3 Update any supporting docs/spec wording needed so the checker matches
  the formal rules.

## 5. Verification

- [x] 5.1 Run `openspec validate document-realignment-governance-v1` and
  `openspec validate --specs`.
- [x] 5.2 Run repository consistency checks including the new documentation
  governance checker.
- [x] 5.3 Run `git diff --check`.
- [x] 5.4 Run the shared verification gate in full mode for the final branch.

## Why

The repository already separates formal specs, evidence, roadmap notes, review
packages, reporting snapshots, and thesis/reference materials, but its
current-facing narrative docs still drift too easily. `README.md` and
`AGENTS.md` carry high-churn baseline detail, several current or snapshot
entrypoints lack consistent freshness metadata, and the existing repo checks
verify spec/matrix integrity more reliably than they verify narrative-doc
governance.

This change aligns the checked-in documentation layers with the current
baseline, lowers drift in the repo-root entrypoints, and adds an explicit
governance checker so future documentation refreshes fail fast when freshness
or boundary markers are missing.

## What Changes

- Add a governed documentation-governance capability covering repo-root
  onboarding entrypoints, current-facing narrative-doc metadata, snapshot
  package boundaries, and the repo-local documentation governance checker.
- Reduce `README.md` and `AGENTS.md` to lower-churn onboarding surfaces that
  point to the existing canonical current-truth documents instead of repeating
  large portions of active baseline detail.
- Refresh current/canonical narrative docs so active deployment, COMM path,
  mission-history, recovery/watchdog, workflow handoff, and roadmap wording
  stay aligned with code, current specs, the verification-path registry, and
  evidence.
- Add consistent freshness and scope markers to current non-canonical packages
  such as `docs/thesis/`, `docs/reporting/`, and
  `docs/architecture-review/current/`.
- Add a dedicated documentation governance checker and wire it into the shared
  repository consistency and verification gate flow.

## Capabilities

### New Capabilities

- `documentation-governance`: governs repo-root onboarding docs, current-facing
  narrative-doc freshness metadata, snapshot package boundaries, and the
  documentation governance checker

### Modified Capabilities

- `delivery-workflow`: add the documentation-governance checker to the required
  repository gate and keep documentation governance enforcement on the formal
  workflow path

## Impact

- Affected docs: `README.md`, `AGENTS.md`, `docs/architecture/**`,
  `docs/roadmap/**`, `docs/interfaces.md`, `docs/operator/**`,
  `docs/reporting/README.md`, `docs/architecture-review/current/**`,
  `docs/thesis/**`, and selected archive/index surfaces where boundary wording
  is misleading
- Affected scripts: repository consistency checks, shared verification gate, and
  a new documentation governance checker
- Affected formal specs: new `documentation-governance` capability plus a
  `delivery-workflow` delta for gate enforcement
- Runtime/product impact: none; this is documentation and governance hardening

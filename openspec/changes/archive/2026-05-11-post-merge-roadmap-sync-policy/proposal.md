## Why

The repository currently treats some narrative updates, especially `docs/roadmap/*`, as truthful only after a feature branch has merged and `main` has been synced locally. Without an explicit rule for that timing, agents either stop at `local-ready` and leave roadmap reconciliation ambiguous or open a low-value follow-on documentation PR solely to restate the already-merged mainline state.

## What Changes

- Clarify that branch-verifiable documentation stays in the original feature or documentation branch before review.
- Add a narrow post-merge mainline reconciliation rule for `docs/roadmap/*` when those files need to describe the already-merged `main` state, updated ordering, or refreshed handoff status.
- Allow the maintainer to apply that narrow `docs/roadmap/*` reconciliation directly on `main` after merge instead of forcing a second PR.
- Explicitly forbid using that post-merge shortcut for formal specs, verification evidence, architecture truth, operator runbooks, or any document whose truth should have been reviewed with the original change.
- Update the onboarding and narrative documentation so future agents know which layer to edit before review and which layer, if any, may wait until after merge.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: defines when documentation must ship in the original branch versus when `docs/roadmap/*` may be reconciled after merge on `main`.

## Impact

- Affected formal workflow spec: `openspec/specs/delivery-workflow/spec.md`.
- Affected onboarding and narrative docs: `AGENTS.md`, `README.md`, and `docs/architecture/current-development-architecture.md`.
- Future agents should stop creating follow-on docs-only PRs solely to restate merged roadmap progress on `main`, while still keeping formal, evidence, and architecture updates inside the originating branch/PR.

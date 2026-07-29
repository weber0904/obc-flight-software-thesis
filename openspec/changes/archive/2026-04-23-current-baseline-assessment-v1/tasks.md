## 1. Change Artifacts

- [x] 1.1 Create proposal, design, and a delta spec for the new `architecture-review` capability.
- [x] 1.2 Define the report shape, truth-priority rules, and release-readiness/governance scope of the package.

## 2. Architecture Review Package

- [x] 2.1 Add a documentation index for `docs/architecture-review/`.
- [x] 2.2 Add `docs/architecture-review/current-baseline-assessment-v1/README.md` with the executive summary, architecture overview, one-page classification, and reading order.
- [x] 2.3 Add `subsystem-and-capability-dossiers.md` with fixed-section dossiers for platform, CSP, EPS, ADCS, comm, GPS, boot/update, archive, storage health, mission autonomy, and scenario-driven validation.
- [x] 2.4 Add `verification-and-release-readiness.md` with explicit path separation, verification-layer summary, and release recommendation.
- [x] 2.5 Add `workflow-governance-audit.md` with repo-only change/release workflow conclusions.
- [x] 2.6 Add a checked-in `thesis-summary-export.md` that can be copied to repo-external thesis notes without changing the technical conclusions.

## 3. Evidence And Discoverability

- [x] 3.1 Add `docs/test-records/current-baseline-assessment-v1/README.md` documenting the checks, consulted truth sources, and bounded conclusions for this slice.
- [x] 3.2 Update top-level documentation indices so the technical architecture-review package is discoverable alongside the professor/PM reporting package.

## 4. Validation And Finalization

- [x] 4.1 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.2 Run `python3 scripts/check_agent_entrypoint.py`.
- [x] 4.3 Run `python3 scripts/report_verification_inventory.py --json`.
- [x] 4.4 Run `git describe --tags --always --dirty` and `openspec list --json` to capture current baseline state.
- [x] 4.5 Run `openspec validate current-baseline-assessment-v1` and `openspec validate --specs`.
- [x] 4.6 Sync and archive `current-baseline-assessment-v1`.
- [x] 4.7 Update the baseline reconciliation matrix and machine-readable source after archive so the new capability and archived change remain reviewable.
- [x] 4.8 Collapse the change into a reviewable Conventional Commit and stop at `local-ready` until push is explicitly approved.

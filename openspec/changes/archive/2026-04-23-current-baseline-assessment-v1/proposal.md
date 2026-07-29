## Why

The repository now has enough validated scope and archived history that release planning depends on a technical baseline assessment rather than on ad hoc chat summaries. That assessment does not yet exist as a formal checked-in package, which makes release judgement, architecture review, and future agent continuity weaker than they should be.

## What Changes

- Add a formal `architecture-review` capability for a checked-in technical baseline assessment package.
- Check in a current-baseline assessment package under `docs/architecture-review/current-baseline-assessment-v1/` covering architecture, subsystem dossiers, release readiness, workflow governance, and an export-ready thesis summary.
- Add an evidence record for the assessment slice and update top-level documentation indices so future reviewers can find both the technical audit package and the professor/PM-facing reporting package.

## Capabilities

### New Capabilities
- `architecture-review`: checked-in technical baseline assessment and release-readiness reporting grounded in repository truth

### Modified Capabilities
- None

## Impact

- Affected code: repository documentation, one new main spec capability after archive, and one evidence record.
- Affected systems: release judgement, architecture review, future agent onboarding, and thesis/report export preparation.
- No change to flight runtime behavior, target behavior, public F' contracts, or verification-path semantics.

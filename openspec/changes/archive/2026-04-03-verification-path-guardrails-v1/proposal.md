## Why

Recent work showed that the repository's actual delivery discipline was stronger than what a new conversation could reliably infer from generic F' knowledge. In particular, developers could confuse `OBC -> GDS` TCP connectivity with `fprime-cli -> GDS` command/uplink behavior, or skip the documented branch / push / CI sequence and still believe they were following the established workflow.

## What Changes

- Add an explicit verification-path registry that records which command, telemetry, event, and ground-link paths have been formally proven in this repository, and under what scope.
- Clarify that generic upstream F' knowledge is not a substitute for repository-specific evidence when choosing a validation path.
- Strengthen the delivery workflow so new work must start from a dedicated `feature/*`, `fix/*`, `docs/*`, or `hotfix/*` branch instead of direct development on `main`.
- Document that push, CI-green, and release/tag timing are mandatory completion gates, and that new conversations must re-check archived evidence before assuming a path is already proven.

## Capabilities

### New Capabilities

- `verification-path-registry`: A repo-owned reference describing which validation paths are formally proven, their ports/transports, and their current evidence scope.

### Modified Capabilities

- `delivery-workflow`: Clarify branch naming, push/CI/release timing, and the requirement to consult repository evidence before reusing a validation path.
- `verification-evidence`: Require evidence and debugging guidance to name the exact path being validated, distinguish adjacent paths, and cite the governing baseline when a path is reused.

## Impact

- Affected narrative and formal workflow documents
- New repo documentation under `docs/`
- Future debugging and verification practice for GDS / CLI / target validation

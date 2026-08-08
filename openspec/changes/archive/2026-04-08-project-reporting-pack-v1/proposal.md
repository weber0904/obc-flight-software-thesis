## Why

The repository has reached a stage where the validated baseline is substantial, but there is no checked-in reporting package that explains the architecture, completed scope, verification status, and safest live-demo path in a form a non-domain professor or PM can review quickly. That gap now matters because the next milestone is a short in-person report with a live demonstration rather than another implementation slice.

## What Changes

- Add a formal `project-reporting` capability for a checked-in professor/PM reporting package grounded in repository truth.
- Create a reporting package under `docs/reporting/` containing a main briefing document, three Mermaid architecture/workflow diagrams, a capability status matrix, and a demo runbook.
- Record the governed evidence for the reporting package and update the review surfaces so the new capability and archived change are visible in the repository reconciliation trail.

## Capabilities

### New Capabilities
- `project-reporting`: checked-in reporting and demo-package materials for professor/PM-facing project review

### Modified Capabilities
- `verification-evidence`: add reviewable evidence expectations for the checked-in reporting package and demo-runbook maintenance slice

## Impact

- Affected code: repository documentation, OpenSpec specs, reporting-package docs, evidence records, and reconciliation/verification review surfaces.
- Affected systems: professor/demo preparation, project communication, and reviewability of current completed scope.
- No change to flight runtime behavior, target integration behavior, or hardware scope.

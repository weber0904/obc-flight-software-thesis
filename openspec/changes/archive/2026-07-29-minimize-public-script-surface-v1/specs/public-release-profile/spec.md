## MODIFIED Requirements

### Requirement: Public Distribution Excludes Local And Historical Entry Points
The public Git tree SHALL exclude local workspace state, agent-specific
automation, thesis body drafts, stale reporting/review packages, retired
redirects, historical executable wrappers, narrow development diagnostics,
and scripts outside the declared public workflow allowlist.

#### Scenario: Clean public checkout is audited
- **WHEN** a clean public checkout is scanned
- **THEN** every tracked script SHALL belong to a named public workflow or its
  direct dependency closure
- **AND** no ignored cache, platform metadata, redundant empty-directory
  placeholder, compatibility wrapper, or unlisted script SHALL be present

## ADDED Requirements

### Requirement: Public Script Surface Is Explicitly Allowlisted
The public release SHALL maintain a reviewable script allowlist covering
repository verification, principal hosted operation, governed target
baselines, integrated thesis routes, Mission Console, and their direct support
code.

#### Scenario: Reviewer inspects public automation
- **WHEN** a reviewer opens the script catalog and allowlist
- **THEN** each retained workflow group and representative verification entry
  SHALL be identifiable
- **AND** the release checker SHALL fail if a tracked script is not allowlisted

## MODIFIED Requirements

### Requirement: Public Script Surface Is Explicitly Allowlisted
The public release SHALL maintain a reviewable script allowlist and file-level
catalog covering repository verification, principal hosted operation,
governed target baselines, integrated thesis routes, Mission Console, and
their direct support code. Each retained file SHALL identify its purpose,
role, parent workflow, and public invocation status.

#### Scenario: Reviewer inspects public automation
- **WHEN** a reviewer opens the script catalog and allowlist
- **THEN** each retained file SHALL have a concrete purpose and workflow owner
- **AND** hosted and target/laboratory operator entrypoints SHALL both be
  represented
- **AND** the release checker SHALL fail if a tracked script is unlisted,
  undocumented, or lacks a retained dependency path

#### Scenario: Incremental development probe is superseded
- **WHEN** an integrated retained workflow covers the same public capability as
  a narrower development-stage probe
- **THEN** the narrower probe SHALL be excluded from the public script surface
- **AND** its historical result MAY remain in OpenSpec and evidence

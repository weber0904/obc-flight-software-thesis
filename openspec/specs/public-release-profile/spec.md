# public-release-profile Specification

## Purpose
Define the provenance, publication-boundary, credential, evidence, hardware-
claim, and immutable-tag requirements for the curated public thesis release.
## Requirements
### Requirement: Public Snapshot Has Immutable Provenance
The public release SHALL identify one development source commit, one public
commit, and exact submodule commits without importing development Git history.

#### Scenario: Reviewer inspects release provenance
- **WHEN** a reviewer opens the tagged release provenance
- **THEN** the development source, public commit, F Prime commit, and libcsp
  commit SHALL be explicit and resolvable
- **AND** the public Git history SHALL contain no development-repository commits

### Requirement: Every Source Path Has One Publication Disposition
Every tracked path at the declared source commit SHALL be classified exactly
once as included, transformed, externalized, or excluded.

#### Scenario: Publication manifest is validated
- **WHEN** the publication checker compares the source Git tree with the
  publication manifest
- **THEN** no tracked path SHALL be unclassified
- **AND** no tracked path SHALL match more than one disposition

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

### Requirement: OpenSpec History Is Preserved
The public release SHALL retain all main OpenSpec capability directories and
all archived changes present at the source commit.

#### Scenario: OpenSpec inventory is compared
- **WHEN** the source and public OpenSpec inventories are compared
- **THEN** every source main spec and archived change SHALL be present
- **AND** the release change SHALL be synced and archived
- **AND** no active change SHALL remain at tag time

### Requirement: Public Credentials Are Non-Deployable Examples
The repository SHALL track only an example command-auth keystore and SHALL
require an ignored local copy for runtime use.

#### Scenario: Developer bootstraps hosted configuration
- **WHEN** the developer runs the documented bootstrap command in a clean clone
- **THEN** an ignored mode-`0600` local keystore SHALL be created without
  overwriting an existing file

#### Scenario: Target bundle is packaged
- **WHEN** a target bundle is requested
- **THEN** an explicit packaging-time keystore path SHALL be required
- **AND** the public example fingerprint SHALL be rejected
- **AND** the installed bundle manifest SHALL record the copied keystore digest

### Requirement: Raw Evidence Is Externally Checksummed
All test-record summaries SHALL remain in Git while raw artifact trees SHALL be
stored in a versioned release asset bound by committed checksums.

#### Scenario: Reviewer follows externalized evidence
- **WHEN** a test record references externalized artifacts
- **THEN** an `ARTIFACTS.json` descriptor SHALL identify the release asset,
  archive prefix, file count, and checksum
- **AND** the release tag SHALL contain the asset SHA-256 and redaction ledger

### Requirement: Hardware Claims Are Commit Scoped

Hardware and laboratory result records SHALL identify their execution commit,
date, environment, commands, and artifact digests. Current reader documents
SHALL link those records by capability and environment.

#### Scenario: Reviewer follows a hardware result
- **WHEN** a reviewer selects a Raspberry Pi, UART, SocketCAN, subsystem, or
  watchdog result
- **THEN** the linked evidence SHALL expose the exact execution provenance and
  observations

### Requirement: Public Tag Is CI Gated And Immutable
The `thesis-submission-v1` annotated tag SHALL be created only from merged
public `main` after required CI succeeds and SHALL not subsequently move.

#### Scenario: Release is published
- **WHEN** the public PR is merged and the main commit passes required CI
- **THEN** the annotated tag MAY be created after explicit approval
- **AND** release assets, checksums, SBOM, and provenance SHALL be attached

#### Scenario: Post-release correction is needed
- **WHEN** a non-security defect is found after publication
- **THEN** the existing tag SHALL remain unchanged
- **AND** the correction SHALL use a successor version

### Requirement: Portfolio Documentation Is Capability First

The portfolio-facing README and current documents SHALL explain implemented
capabilities, architecture, operation, and verification directly.

#### Scenario: Reviewer reads a current document
- **WHEN** a reader opens README, architecture, interfaces, verification,
  operator, thesis, security, or release documentation
- **THEN** the prose SHALL focus on the software and reproducible engineering
  results
- **AND** publication-process commentary SHALL remain in machine-readable
  provenance or formal change history

### Requirement: Public Submodule Revisions Are Recursively Obtainable
Every submodule Gitlink declared by the public release SHALL resolve from its
configured public remote in a fresh recursive checkout of the release commit.

#### Scenario: Reviewer clones the release recursively
- **WHEN** a reviewer clones the candidate release with recursive submodules
- **THEN** every declared submodule SHALL checkout to the Gitlink revision
- **AND** the resulting checkout SHALL not contain modified or uninitialized
  submodules

### Requirement: Public Automation Has No Local Thesis Workspace Exception
The public script surface SHALL not allow, synchronize, or test an ignored
local thesis-writing workspace as an exception to provenance or release rules.

#### Scenario: Reviewer inspects public provenance automation
- **WHEN** a reviewer scans the public Route 1 provenance and target-sync
  entrypoints
- **THEN** no `.codex_thesis_work` exception or exclusion SHALL be present
- **AND** campaign identity SHALL still reject undeclared untracked roots

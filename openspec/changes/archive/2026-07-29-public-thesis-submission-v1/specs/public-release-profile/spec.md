## ADDED Requirements

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
redirects, and historical executable wrappers.

#### Scenario: Clean public checkout is audited
- **WHEN** a clean public checkout is scanned
- **THEN** it SHALL contain only the release-profile document families and
  maintained or support/internal executables

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
Hardware evidence not rerun on the public commit SHALL NOT be described as
fresh verification of the public tag.

#### Scenario: Public README summarizes target results
- **WHEN** prior Raspberry Pi or lab evidence is cited
- **THEN** it SHALL be labeled previously demonstrated or historical
- **AND** its source commit, date, environment, and release delta SHALL be
  visible

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

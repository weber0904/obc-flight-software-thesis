## ADDED Requirements

### Requirement: Evidence Governance SHALL Permit Manifest-Backed Canonicalization

Repository evidence governance SHALL permit omission of byte-identical
derived decode outputs when a checked manifest identifies one retained
canonical artifact and every removed equivalent by repository-relative path
and SHA-256. Canonicalization SHALL NOT remove original transport bytes,
source or received product bytes, provenance, attempt history, verdict
oracles, or time-distinct observations. The manifest SHALL provide the source
and received product hashes and a repository-owned reconstruction command.

#### Scenario: Exact decoded JSON copies are canonicalized

- **WHEN** multiple derived decoded JSON artifacts have the same SHA-256
- **THEN** the evidence bundle MAY retain one canonical `decode-primary`
  artifact and omit the byte-identical equivalents
- **AND** the manifest SHALL record the canonical path, content hash, every
  omitted equivalent path, raw source and received product hashes, and the
  reconstruction command.

#### Scenario: Non-derived and provenance evidence stays retained

- **WHEN** an evidence bundle is canonicalized
- **THEN** original transport bytes, source and received FDP products, verdict
  summaries, attempt provenance, exact sequence source and compiled execution
  inputs, journals, and required observation logs SHALL remain present
- **AND** content identity alone SHALL NOT justify removing time-distinct
  snapshots or observations.

#### Scenario: Distinct ground consumers remain independently reviewable

- **WHEN** GDS runtime and an independently connected StandardPipeline store
  byte-identical received FDP products
- **THEN** both received-product observations SHALL remain retained and
  SHA-256-checked
- **AND** byte identity SHALL NOT canonicalize away either ground consumer.

#### Scenario: Pipeline observations remain hash-locked

- **WHEN** the frozen Route 1 bundle retains StandardPipeline channel and
  event observations
- **THEN** both logs SHALL be enumerated and SHA-256-checked
- **AND** deleting or altering either log SHALL fail evidence validation.

#### Scenario: Contrasting authority clauses remain independent

- **WHEN** a dated non-authority statement is followed by a `while`,
  `although`, or equivalent clause asserting that it remains authoritative
- **THEN** the later positive clause SHALL be evaluated independently
- **AND** the governing-document check SHALL fail.

#### Scenario: Frozen sequence execution inputs remain hash-locked

- **WHEN** a frozen Route 1 bundle claims a sequence-driven observation
- **THEN** its exact retained sequence source and compiled execution binary
  SHALL be enumerated and SHA-256-checked
- **AND** deleting or altering either input SHALL fail evidence validation.

#### Scenario: Governing evidence authority remains explicit

- **WHEN** a frozen Route 1 bundle identifies governing documents
- **THEN** its manifest and checker SHALL require each document to classify
  the provenance-incomplete 2026-07-12 proof as historical functional
  evidence, not current target authority
- **AND** each document SHALL explicitly classify the 2026-07-20 bundle as a
  non-authoritative functional observation and target requalification as
  pending rather than relying on date or link presence alone
- **AND** coordinated document and manifest edits SHALL fail when they replace
  an explicit non-authority phrase with an authority-promoting phrase that
  merely contains the character sequence `not`
- **AND** retaining the approved assertion while adding a contradictory
  authority-promoting statement SHALL also fail
- **AND** double-negated non-authority wording and a later positive authority
  clause, including one joined by a coordinating conjunction, SHALL fail
  independently
- **AND** common numeric, English month-name, or Chinese spellings of the two
  governed dates SHALL receive the same authority-promotion checks.

#### Scenario: Frozen campaign README classification remains hash-locked

- **WHEN** the frozen Route 1 campaign README states the bundle's evidence
  classification
- **THEN** the README SHALL be enumerated and SHA-256-checked as a retained
  artifact
- **AND** the checker SHALL require an explicit non-authoritative
  classification and pending target requalification
- **AND** adding an authority-promoting statement SHALL fail validation even
  if the required non-authoritative substrings are still present.

#### Scenario: Resume does not rebind a retained target PASS

- **WHEN** a campaign resumes with an existing target PASS
- **THEN** that attempt's authoritative status SHALL remain bound to its
  retained target revision provenance by an attempt-recorded SHA-256
- **AND** the importer manifest SHALL be bound by an attempt-recorded SHA-256,
  enumerate the exact retained file set with sizes and SHA-256 values, and be
  revalidated before hosted or target execution is skipped
- **AND** a current install, rebuild, or provenance refresh SHALL NOT replace
  the retained provenance unless a new target attempt is executed
- **AND** missing, corrupt, changed, or unlisted retained attempt evidence
  SHALL prevent authoritative campaign PASS.

#### Scenario: Campaign identity remains stable between surfaces

- **WHEN** hosted execution completes before a target attempt
- **THEN** the runner SHALL compare the current checkout with the recorded
  campaign identity again before target restart or provenance validation
- **AND** a mismatch SHALL block target execution rather than combine hosted
  and target evidence from different revisions.

#### Scenario: Resume identity matches retained campaign history

- **WHEN** a campaign resumes with existing attempt records
- **THEN** the branch/head/version in its campaign identity file SHALL match
  the identity already recorded in the retained campaign manifest before any
  attempt is loaded
- **AND** replacing either identity surface SHALL block resume rather than
  relabel retained attempts.

#### Scenario: Target workspace bytes remain bound to campaign source

- **WHEN** formal target evidence uses a workspace without `.git`
- **THEN** its provenance SHALL retain a deterministic manifest and SHA-256
  over every serialized Git-indexed path, normalized mode, and content
- **AND** those serialized bytes and modes SHALL be compared with the
  committed superproject/submodule trees before synchronization so index
  stat hints, skip-worktree flags, or file-mode configuration cannot relabel
  uncommitted inputs as campaign-HEAD content
- **AND** the target gate SHALL recompute that digest before C and reject a
  changed, missing, unsafe, duplicated, or omitted path
- **AND** the workspace marker SHALL bind a package-path-keyed build-time hash
  for every remote-build input copied into the target bundle so a later
  mutually consistent build/package/install hash chain cannot relabel any
  replaced executable, dictionary, or build metadata as campaign-HEAD
  evidence
- **AND** the live remote build-input audit and installed manifest entries
  SHALL match those marker-time hashes before target C.

### Requirement: Formal Evidence Attempts SHALL Preserve Failure Lineage

The formal artifact importer SHALL support attempt labels, retry lineage, and
failure classification so a failed or blocked attempt remains distinguishable
from the later governing PASS.

#### Scenario: Retry metadata is imported

- **WHEN** formal evidence is imported with `--attempt-label`, `--retry-of`,
  or `--failure-class`
- **THEN** the generated evidence metadata SHALL retain the supplied values
- **AND** a later PASS SHALL NOT erase or relabel the earlier attempt lineage
- **AND** every target attempt SHALL retain an attempt-specific revision
  provenance path and SHA-256 that remains independently verifiable after a
  retry.

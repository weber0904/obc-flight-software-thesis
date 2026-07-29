## ADDED Requirements

### Requirement: Thesis-Backed Route 1 Evidence SHALL Remain Frozen And Checkable

The maintained Route 1 sequence evidence SHALL classify the 2026-07-12 proof
as historical functional evidence, not current target authority, because its
remote workspace identity and build/install provenance artifacts were not
retained. The 2026-07-20 bundle SHALL remain frozen as a thesis-backed
functional observation, not as A/B/C authority, because its C stage restarted
the shared OBC service and its contemporaneous revision provenance is
incomplete. Current Route 1 target requalification SHALL remain pending until
a fresh campaign satisfies the current provenance and ownership requirements.
A repository-owned checker SHALL verify the frozen bundle's canonical
artifacts, governance classification, documentation links, and the Chapter 5
facts: secure authentication at `02:15:07`, sequence starts at `02:17:46`,
sequence success at `02:18:57`, AUTO index `48`, DETERMINISTIC index `49`, FDP
size `48,506` bytes, JPEG size `48,187` bytes, and SoC transitions
`PAYLOAD -> IDLE` at `59` and `IDLE -> SAFE` at `39`.

#### Scenario: Frozen target functional observation is checked

- **WHEN** the Route 1 evidence checker runs against the maintained campaign
- **THEN** it SHALL verify the functional PASS summary, retained A/B snapshots,
  provenance artifacts, the recorded ownership/provenance limitations,
  canonical AUTO and DETERMINISTIC artifacts, the exact retained sequence
  source and compiled execution binary, retained raw products, and all
  thesis-critical timestamps, sizes, indices, and SoC results
- **AND** the independently stored StandardPipeline received FDP SHALL remain
  retained and SHA-256-checked separately from the byte-identical GDS-runtime
  received FDP
- **AND** it SHALL fail when a required artifact, value, or governed
  documentation link is missing or changed
- **AND** every governing document SHALL explicitly classify both the
  2026-07-12 proof and 2026-07-20 bundle as non-authoritative functional
  evidence and state that target requalification remains pending.
- **AND** the checker SHALL validate explicit non-authority phrases rather than
  accepting `not` embedded inside an authority-promoting word or statement.
- **AND** it SHALL reject a contradictory authority-promoting statement even
  when the approved non-authority assertions remain elsewhere in the same
  governing document
- **AND** a double-negated classification or later positive authority clause
  SHALL fail independently
- **AND** common numeric, English month-name, or Chinese spellings of either
  governed date SHALL receive the same authority-promotion checks.
- **AND** the campaign README itself SHALL remain SHA-256-checked and SHALL
  reject an authority-promoting statement even when the required historical
  strings remain present.

#### Scenario: SoC fallback restores testcase state before PASS

- **WHEN** the target SoC-fallback C scenario changes the EPS simulator from
  the Route 1 baseline to the `59` and `39` threshold cases
- **THEN** it SHALL restore the testcase-owned EPS simulator SoC to the Route 1
  baseline on success and failure before reporting PASS
- **AND** it SHALL NOT restart, override, or otherwise assume lifecycle
  ownership of the shared EPS service.

#### Scenario: Automatic target retry re-enters revision gates

- **WHEN** a target attempt fails with a retryable classification and a second
  attempt is permitted
- **THEN** before the second C attempt the formal runner SHALL revalidate
  campaign source identity, request the A-owned forced OBC restart, and
  generate fresh target revision provenance
- **AND** the retry SHALL bind to the fresh attempt-specific provenance path
  and file SHA-256
- **AND** hosted retries SHALL remain independent of target lifecycle gates
- **AND** retained authoritative target PASS evidence SHALL preserve its
  existing behavior only after its per-attempt importer manifest and retained
  artifact bytes revalidate.

#### Scenario: Target retry preserves each provenance artifact

- **WHEN** a retryable target failure is followed by another target attempt
- **THEN** the failed and retried attempts SHALL retain distinct
  attempt-specific target-provenance paths and SHA-256 bindings
- **AND** validating campaign history SHALL fail if either retained provenance
  artifact is missing or changed.

#### Scenario: Resume identity matches retained campaign history

- **WHEN** a campaign resumes with existing attempt records
- **THEN** the branch/head/version in its campaign identity file SHALL match
  the identity already recorded in the retained campaign manifest before any
  attempt is loaded
- **AND** replacing either identity surface SHALL block resume rather than
  relabel retained attempts.

#### Scenario: Hosted attempts rebuild independently of deployment

- **WHEN** a hosted attempt executes under a fresh campaign, `SKIP_DEPLOY`, a
  pending resume, or an automatic retry
- **THEN** the formal runner SHALL revalidate campaign identity and complete a
  fresh native build of OBC and every executable used by the hosted wrapper
  before starting it
- **AND** target deployment controls SHALL NOT bypass or satisfy that hosted
  build requirement
- **AND** a retained authoritative hosted PASS SHALL remain skipped on resume
  only after its attempt-manifest SHA-256, metadata, exact retained-file set,
  sizes, and artifact SHA-256 values revalidate.

#### Scenario: Remote workspace or build mutation fails provenance

- **WHEN** a synchronized Git-indexed remote path is changed or deleted after
  the workspace marker is written
- **THEN** the formal target gate SHALL reject the recomputed workspace digest
- **AND** before synchronization, every serialized tracked byte and
  Git-normalized mode SHALL match the committed superproject/submodule trees
  even when Git status is suppressed by index flags or file-mode configuration
- **AND** the marker SHALL bind a package-path-keyed build-time hash for every
  remote-build input copied into the target bundle so a later package/install
  with mutually consistent hashes still rejects any replaced executable,
  dictionary, or build metadata
- **AND** the live remote build-input audit and installed manifest entries
  SHALL cover the same governed input set and match those marker-time hashes
- **AND** `SKIP_DEPLOY` and pending resume SHALL NOT bypass either check.

#### Scenario: Installed runtime input mutation fails provenance

- **WHEN** any installed manifest-listed launcher, helper, configuration,
  dictionary, binary, or other release file changes after installation
- **THEN** the pre-C gate SHALL reject its live size or SHA-256 mismatch
- **AND** the live manifest SHA-256 and content SHALL match a local trusted
  package/install receipt, so coordinated file-plus-manifest edits fail
- **AND** retained provenance SHALL include the trusted receipt, installed
  manifest SHA-256, and complete live audit rather than binding `bin/OBC`
  alone.

#### Scenario: Active systemd launch inputs remain bound

- **WHEN** A has restarted the OBC service before target C
- **THEN** the pre-C gate SHALL compare the active service fragment with the
  trusted rendered-unit install receipt
- **AND** it SHALL require exactly the governed A-owned CAN-FD and Beacon
  drop-ins, rejecting changed or unknown drop-ins
- **AND** it SHALL bind the effective launch path and service process tree to
  the installed release.
- **AND** the formal runner, A, C, and provenance checker SHALL use
  `obc-comm-csp-stack.service`; a caller-selected alternate service SHALL fail
  before target execution.

#### Scenario: Caller manual timeouts cannot alter formal A

- **WHEN** a caller exports manual-auth timeout values or a timeout drop-in
  name before a Route 1 target wrapper starts
- **THEN** the wrapper SHALL clear those inputs before requesting A's
  manual-auth preflight
- **AND** C SHALL retain verification-only ownership of the service profile.

#### Scenario: Fresh deployment initializes submodules before identity

- **WHEN** a fresh deployment run starts with uninitialized or
  revision-drifted recursive submodules
- **THEN** the runner SHALL initialize and align them before recording its
  first campaign identity
- **AND** `SKIP_DEPLOY` and resume behavior SHALL remain unchanged.

#### Scenario: Superseded campaign is not governing evidence

- **WHEN** reviewers follow the Route 1 test record and verification registry
- **THEN** they SHALL be directed to the historical 2026-07-12 proof and the
  frozen 2026-07-20 functional observation without treating either as current
  target authority
- **AND** the removed 2026-07-19 campaign SHALL NOT be presented as current
  maintained PASS authority.

### Requirement: Route 1 Evidence Freeze SHALL Not Expand Operational Claims

Canonicalizing stored evidence SHALL preserve the existing Route 1 validation
boundary and SHALL NOT represent a new hosted, target, RF, OTA, scheduler, or
throughput qualification.

#### Scenario: Constrained local verification exception is reported

- **WHEN** the evidence freeze is closed locally
- **THEN** its verification record SHALL state that no local full gate and no
  new hosted or target campaign were run by explicit developer decision
- **AND** it SHALL retain the 2026-07-20 functional observations without
  promoting them to operational authority while requiring the pull request's
  hosted `baseline-gate`.

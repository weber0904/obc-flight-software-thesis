# Route 1 Sequence Verification Specification

## Purpose

Define the bounded hosted and governed-target Route 1 sequence-driven payload
verification contract without expanding it into scheduler, throughput, RF, or
general sequence-control claims.

## Requirements

### Requirement: Route 1 SHALL Prove The Governed Sequence-Driven Payload Flow

Route 1 verification SHALL use the active official sequence compiler, governed
`.sequence-staging/<leaf>` upload destination, and repo-owned sequence
admission/control surface. A PASS SHALL require successful compile, upload,
validation, and run results; the configured SoC and mode admission conditions;
and fresh payload-completion evidence from the same invocation.

#### Scenario: Hosted Route 1 completes through the governed sequence path
- **WHEN** the hosted Route 1 wrapper runs from an isolated runtime root after
  a fresh build
- **THEN** it SHALL compile the checked-in Route 1 sequence, upload it only to
  governed sequence staging, validate and run it through the repo-owned
  wrapper, and record fresh payload-completion evidence
- **AND** it SHALL reject a command error, failed sequence, wrong destination,
  stale artifact, or missing completion evidence as PASS.

#### Scenario: Target Route 1 preserves baseline and probe ownership
- **WHEN** the target Route 1 functional scenario is run
- **THEN** A and B SHALL establish the shared target and ground baseline before
  C executes the Route 1 sequence scenario
- **AND** C SHALL not restart or stop shared target baseline services
- **AND** when the baseline is externally managed, C SHALL verify every
  requested OBC service-profile field and fail on mismatch without applying a
  service override
- **AND** C SHALL only own probe-local helpers, temporary overlays, and
  evidence capture
- **AND** the SoC-fallback C scenario SHALL restore its testcase-owned EPS
  simulator SoC to the Route 1 baseline before reporting PASS, including on a
  failed functional attempt, without restarting or overriding the shared EPS
  service
- **AND** A and B SHALL be rerun after C to establish postflight readiness.

### Requirement: Route 1 SHALL Keep Scope And Provenance Bounded

Hosted and target Route 1 evidence SHALL identify their respective execution
surfaces. Target evidence SHALL record local branch/head, remote workspace
revision identity (a Git head when repository metadata is retained, otherwise
a provenance marker binding the exact synchronized source head), remote build
metadata, and installed release pointer before functional interpretation. When
the synchronized workspace excludes `.git`, that marker SHALL enumerate every
serialized Git-indexed superproject and initialized-submodule path, record a
deterministic SHA-256 over path, Git-normalized mode, and content, and record a
package-path-keyed SHA-256 map for every remote-build input copied into the
target bundle, including executables, dictionary, and build metadata. Before
synchronization or marker creation, the exact serialized bytes and
Git-normalized modes SHALL match the committed superproject and submodule
trees, independent of index stat hints, skip-worktree flags, or
`core.filemode`. Before target C, the provenance gate SHALL recompute the
remote workspace digest and every mapped remote-build input hash, then compare
each hash with both the marker-time value and the installed manifest entry.
The same gate SHALL compare the live installed manifest's SHA-256 and content
with a local trusted package/install receipt, then recompute size and SHA-256
for every manifest-listed installed file before target C; a missing, changed,
unsafe, non-regular, or unlisted runtime input SHALL fail provenance even when
`bin/OBC` still matches. Fresh
`SKIP_DEPLOY=1` authority SHALL require an explicit local trusted manifest,
while resume SHALL retain the campaign's copied receipt.
The same pre-C gate SHALL compare the active OBC systemd fragment with the
trusted rendered-unit install receipt, require exactly the A-owned CAN-FD and
Beacon drop-ins with their governed bytes, reject unknown drop-ins, and bind
the effective launch path plus service process tree to the installed release.
The formal runner, A, C, and provenance checker SHALL use the canonical
`obc-comm-csp-stack.service` identity; a caller-provided alternate OBC service
name SHALL be rejected before baseline restart or functional execution.
Fresh `SKIP_DEPLOY=1` authority SHALL also require an explicit trusted rendered
unit receipt, while resume SHALL retain the campaign's copied unit receipt.
Before every hosted attempt that executes, including an automatic retry or a
pending resumed attempt, the runner SHALL revalidate campaign identity and run
a fresh native build of OBC and every executable used by the hosted wrapper.
`SKIP_DEPLOY` SHALL skip only target synchronization, packaging, and
installation; it SHALL NOT permit a hosted verdict to reuse an unvalidated
native build. A retained authoritative hosted PASS SHALL remain skipped on
resume only after the runner revalidates its per-attempt importer manifest
SHA-256, metadata, artifact roots, exact retained-file set, sizes, and SHA-256
values.
Before every target C attempt that has no retained authoritative target PASS,
including an automatic retry, a fresh invocation, or a pending resumed
invocation, whether the runner installed the release or `SKIP_DEPLOY` accepted
an externally installed release, the runner SHALL revalidate campaign identity
and the A-layer baseline manager SHALL force-restart and revalidate the shared
OBC service immediately before generating fresh target provenance. When
Route 1 requests A's manual-auth preflight, caller-provided manual timeout
values and timeout-drop-in names SHALL be cleared before A so they cannot
change the formal service profile after deployment provenance is established.
When
multiple target attempts execute, each attempt SHALL retain its provenance
under an attempt-specific path and bind that path plus SHA-256 on its attempt
record; a later retry SHALL NOT overwrite the failed attempt's provenance.
When resume retains an authoritative target PASS, the runner SHALL preserve and
revalidate that attempt's original provenance, including its
attempt-recorded provenance path and file SHA-256, rather than overwrite it
with the currently installed binary. The same retained attempt SHALL pass the
importer manifest and artifact integrity checks required for hosted resume;
missing, invalid, mismatched, or non-authoritative retained provenance or
attempt evidence SHALL fail closed.
Formal target synchronization SHALL serialize only
Git-indexed superproject and initialized-submodule files into a replacement
workspace that cannot retain stale non-indexed inputs. A formal campaign SHALL
bind one source branch/head/version identity before its first attempt, and
resume SHALL reject a different identity before retaining prior attempts.
Every invocation SHALL revalidate that identity after hosted execution, and
every target attempt SHALL revalidate it before target restart, provenance, or
target-attempt retention.
Route 1 SHALL state that it does not prove a mission scheduler, persistent
onboard scheduling, generic payload throughput, RF closure, or OTA receipt
closure.

#### Scenario: Target provenance failure precedes product diagnosis
- **WHEN** the local branch/head, remote workspace, remote build metadata, or
  installed release pointer do not identify the intended Route 1 revision
- **THEN** the result SHALL be classified as provenance failure
- **AND** the repository SHALL not modify product logic until provenance,
  baseline readiness, and probe-oracle causes have been excluded.

#### Scenario: Formal sync and resume preserve revision identity
- **WHEN** a Route 1 formal target workspace is synchronized or an existing
  campaign is resumed
- **THEN** a fresh deployment run SHALL initialize and align recursive
  submodules before recording its first campaign identity
- **AND** ignored and untracked working-tree files SHALL NOT enter the formal
  target sync archive or remain from the replaced remote workspace
- **AND** every serialized tracked byte and Git-normalized mode SHALL match the
  committed HEAD tree before synchronization, even when Git status reports a
  path clean because of index flags or file-mode configuration
- **AND** the workspace marker SHALL bind every serialized Git-indexed path
  and its deterministic aggregate digest to the local checkout
- **AND** resume SHALL compare the campaign branch/head/version identity with
  the current checkout before importing or retaining prior attempts
- **AND** before loading retained attempts, resume SHALL require the existing
  campaign manifest's recorded branch/head/version to match the campaign
  identity file
- **AND** any mismatch SHALL block authoritative campaign continuation.

#### Scenario: Remote workspace or build mutation fails provenance

- **WHEN** a synchronized Git-indexed remote path is changed or deleted after
  the workspace marker is written
- **THEN** the recomputed remote workspace digest SHALL fail target provenance
- **AND** when any remote-build input copied into the target bundle is
  replaced after marking, a later package/install with internally consistent
  hashes SHALL still fail its marker-time hash binding
- **AND** the marker-time build-input map, live remote build-input audit, and
  installed-manifest package paths SHALL cover the same governed input set
- **AND** `SKIP_DEPLOY` and pending resume SHALL NOT bypass either check.

#### Scenario: Installed runtime input mutation fails provenance

- **WHEN** any installed manifest-listed file, including a launcher, helper,
  configuration, dictionary, or OBC binary, changes after installation
- **THEN** the pre-C provenance gate SHALL detect its size or SHA-256 mismatch
- **AND** coordinated changes to both a release file and its live manifest
  SHALL fail comparison with the local trusted package/install receipt
- **AND** the retained provenance SHALL bind that receipt, the installed
  manifest SHA-256, and the complete live file audit rather than validating
  `bin/OBC` alone.

#### Scenario: Resume preserves retained target PASS provenance

- **WHEN** a resumed campaign already contains an authoritative target PASS
- **THEN** the runner SHALL revalidate and preserve the provenance that made
  that attempt authoritative, and SHALL require its file SHA-256 to match the
  digest recorded on the target attempt, without restarting target services
  or writing current-install provenance over the historical record
- **AND** before skipping C, the runner SHALL validate the attempt-recorded
  importer-manifest SHA-256 and every retained artifact byte
- **AND** a retained PASS whose authority or provenance cannot be validated
  SHALL block resume rather than become authoritative under new provenance.

#### Scenario: Automatic target retry re-enters revision gates

- **WHEN** a target attempt fails with a retryable classification and a second
  attempt is permitted
- **THEN** before the second C attempt the runner SHALL revalidate campaign
  source identity, request the A-owned forced OBC restart, and generate fresh
  target revision provenance
- **AND** the retry SHALL bind to the fresh attempt-specific provenance path
  and file SHA-256
- **AND** hosted retries SHALL remain independent of target lifecycle gates.

#### Scenario: Target retry preserves each provenance artifact

- **WHEN** a retryable target failure is followed by another target attempt
- **THEN** the failed and retried attempts SHALL retain distinct
  attempt-specific target-provenance paths and SHA-256 bindings
- **AND** validating campaign history SHALL fail if either retained provenance
  artifact is missing or changed.

#### Scenario: Hosted attempts rebuild independently of deployment

- **WHEN** a hosted attempt executes under a fresh campaign, `SKIP_DEPLOY`, a
  pending resume, or an automatic retry
- **THEN** the runner SHALL revalidate campaign identity and complete a fresh
  native build of OBC and every executable used by the hosted wrapper before
  starting it
- **AND** target deployment controls SHALL NOT bypass or satisfy that hosted
  build requirement
- **AND** a retained authoritative hosted PASS SHALL remain skipped on resume.

#### Scenario: Route 1 non-claims stay explicit
- **WHEN** Route 1 hosted or target evidence is reviewed
- **THEN** it SHALL identify the exact sequence/payload boundary proven
- **AND** it SHALL not describe the result as generic scheduler, payload
  throughput, RF, or OTA closure.

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
  source and compiled execution binary, the DETERMINISTIC source and
  GDS-runtime ground-received FDP hashes, the independently stored
  StandardPipeline received FDP, directional gateway captures, native-CLI and
  pipeline raw receive captures, the SoC-fallback CLI raw receive capture,
  other retained raw products, and all
  thesis-critical timestamps, sizes, indices, and SoC results
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
- **AND** a double-negated classification or a later authority-promoting
  semicolon/colon clause SHALL fail rather than borrow negation from an earlier
  clause
- **AND** common numeric, English month-name, or Chinese spellings of either
  governed date SHALL receive the same authority-promotion checks.
- **AND** the campaign README itself SHALL remain SHA-256-checked and SHALL
  reject an authority-promoting statement even when the required historical
  strings remain present.

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

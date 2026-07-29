## ADDED Requirements

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
- **AND** C SHALL only own probe-local helpers, temporary overlays, and
  evidence capture
- **AND** A and B SHALL be rerun after C to establish postflight readiness.

### Requirement: Route 1 SHALL Keep Scope And Provenance Bounded

Hosted and target Route 1 evidence SHALL identify their respective execution
surfaces. Target evidence SHALL record local branch/head, remote workspace
head, remote build metadata, and installed release pointer before functional
interpretation. Route 1 SHALL state that it does not prove a mission scheduler,
persistent onboard scheduling, generic payload throughput, RF closure, or OTA
receipt closure.

#### Scenario: Target provenance failure precedes product diagnosis
- **WHEN** the local branch/head, remote workspace, remote build metadata, or
  installed release pointer do not identify the intended Route 1 revision
- **THEN** the result SHALL be classified as provenance failure
- **AND** the repository SHALL not modify product logic until provenance,
  baseline readiness, and probe-oracle causes have been excluded.

#### Scenario: Route 1 non-claims stay explicit
- **WHEN** Route 1 hosted or target evidence is reviewed
- **THEN** it SHALL identify the exact sequence/payload boundary proven
- **AND** it SHALL not describe the result as generic scheduler, payload
  throughput, RF, or OTA closure.

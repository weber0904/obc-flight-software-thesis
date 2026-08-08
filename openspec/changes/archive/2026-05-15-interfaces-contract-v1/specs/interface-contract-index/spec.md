## ADDED Requirements

### Requirement: Current Interface Contract Index
The repository SHALL provide a checked-in `docs/interfaces.md` file as a
non-normative current interface contract index for the active baseline, and the
document SHALL state that code, topology, archived evidence, the verification
path registry, and current main specs outrank that index when conflicts exist.

#### Scenario: Reviewers have one checked-in current-contract entrypoint
- **WHEN** a reviewer or future agent needs the current command, transport,
  timing, storage, or sequencing interface boundary
- **THEN** the repository SHALL provide `docs/interfaces.md` as one checked-in
  starting point instead of requiring them to reconstruct the answer from
  multiple roadmap notes alone

#### Scenario: Interface index does not override higher-priority truth
- **WHEN** `docs/interfaces.md` disagrees with code, topology, archived
  evidence, the verification-path registry, or current main specs
- **THEN** the repository SHALL treat the higher-priority source as current
  truth
- **AND** the index SHALL be updated in a later governed change instead of
  silently redefining the baseline

### Requirement: Interface Index Uses Explicit Fact Status
`docs/interfaces.md` SHALL classify its current-fact entries as `verified`,
`configured-hosted`, or `TBD`, and it SHALL NOT present unknown target timing
or MTU values as proven current baseline facts.

#### Scenario: Hosted-only timing and framing facts stay hosted-scoped
- **WHEN** the index records the current hosted CCSDS frame size or hosted
  rate-group timing profile
- **THEN** it SHALL mark those entries as `configured-hosted` or otherwise make
  the hosted scope explicit
- **AND** it SHALL NOT imply that those values are already frozen target-flight
  truth

#### Scenario: Unknown target values remain intentionally open
- **WHEN** the repository has not yet proven a target flightlike MTU, WCET,
  jitter limit, or rate-group divisor
- **THEN** the corresponding interface-index entry SHALL remain `TBD`
- **AND** the document SHALL NOT substitute a guessed or Raspberry-Pi-only
  number as final target truth

### Requirement: Interface Index Covers Current Baseline Boundaries
`docs/interfaces.md` SHALL summarize the active baseline's command-envelope
surface, ingress roles and admission boundary, CCSDS framing identifiers and
APID mappings, rate-group timing profiles, official `.fdp` history boundary,
persistent fault readback surface, and governed sequencing/resource surfaces.

#### Scenario: Hosted CCSDS framing identifiers are reviewable in one place
- **WHEN** a reviewer inspects the CCSDS section of `docs/interfaces.md`
- **THEN** it SHALL identify the current hosted-proven SCID `0x44`, VCID `1`,
  and APID flows for command `0`, telemetry `1`, event/log `2`, and file `3`
- **AND** it SHALL describe those values as current hosted CCSDS proven
  contract rather than as a frozen target-wide allocation scheme

#### Scenario: Current history and recovery boundaries stay explicit
- **WHEN** a reviewer inspects the storage and recovery sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that official `.fdp` products plus
  `DpCatalog` are the active mission-history path for onboard state
- **AND** it SHALL state that `PersistentFaultManager` is a separate bounded
  recovery breadcrumb surface
- **AND** it SHALL NOT describe `HousekeepingArchive`, `runtime/hk`,
  `hk/index.csv`, or `HK_*` commands as active current-baseline history paths

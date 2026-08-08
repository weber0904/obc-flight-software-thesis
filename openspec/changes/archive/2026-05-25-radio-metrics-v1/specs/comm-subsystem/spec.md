## ADDED Requirements

### Requirement: Raw Ground-Link Observation Fields Are Reviewable

The comm subsystem SHALL expose a complete raw ground-link observation contract
 from `GroundLinkDriver` for each active band on the current baseline.

#### Scenario: Raw ground-link observation includes byte counters
- **WHEN** code or reviewable runtime status reads a band-specific ground-link
  observation
- **THEN** it SHALL include `mode`, `healthSemantics`, `connected`,
  `txChunks`, `rxChunks`, `txBytes`, `rxBytes`, `txErrors`, `rxErrors`, and
  `successfulStatusObservations`

### Requirement: Radio Observation Freshness Is Reviewable

The comm subsystem SHALL expose cached radio observation freshness and
unavailable-result semantics separately from derived link health.

#### Scenario: Cached radio observation reports age and last result
- **WHEN** operators inspect the current radio observability surface
- **THEN** they SHALL be able to see whether a cached radio sample exists
- **AND** they SHALL be able to see the cached-sample age in fast-group ticks
- **AND** they SHALL be able to distinguish successful observation from timeout,
  transport error, invalid response, or unsupported observation result

### Requirement: Hosted Runtime Status Separates Raw Observation From Policy

The comm subsystem SHALL keep hosted runtime status readback explicit about raw
ground-link observation, derived health, policy-facing COMM runtime state, and
cached radio observation.

#### Scenario: Hosted runtime status does not collapse link layers
- **WHEN** hosted runtime status is printed for the active baseline
- **THEN** it SHALL show per-band raw ground-link observation and cached radio
  observation separately from `CommRuntimeState`
- **AND** it SHALL NOT present raw RSSI or raw transport counters as if they
  were provider-owned availability verdicts

## MODIFIED Requirements

### Requirement: COMM Runtime State Exposes Per-Band Activity Age And Reasons
The comm subsystem SHALL expose per-band ground-link activity age and
availability reason through reviewable COMM runtime state on the active
baseline, and it SHALL keep reviewable raw ground-link observation and cached
radio observation available through separate runtime contracts. Radio
signal-quality metrics such as RSSI are intentionally limited in this change to
raw `RadioController` observation with explicit freshness and unavailable-value
semantics, while SNR and any signal-quality contribution to link-health policy
remain deferred to a future governed change.

#### Scenario: Hosted status shows new health fields
- **WHEN** operators inspect hosted COMM runtime status
- **THEN** they SHALL be able to see S-band and UHF activity-age values
- **AND** they SHALL be able to distinguish healthy activity,
  connected-only fallback, explicit disconnect, and stale-activity reasons

#### Scenario: Raw radio metrics remain outside link-health policy
- **WHEN** reviewers inspect the current radio and link-health surfaces
- **THEN** RSSI SHALL be available only on the raw `RadioController`
  observation and readback surface with explicit freshness and unavailable
  semantics
- **AND** RSSI and SNR SHALL NOT be required fields in the current
  `CommLinkHealthView`
- **AND** no current `CommController` availability or failover verdict SHALL be
  derived from signal-quality fields

### Requirement: Ground Link Observation Contract Is OBC-Owned

The comm subsystem SHALL keep provider-facing ground-link observation types in
the OBC-owned runtime surface rather than in the simulator/backend-owned
implementation header, and that OBC-owned contract SHALL be the complete raw
ground-link observability source for current runtime readback.

#### Scenario: Provider consumes OBC-owned observation types

- **WHEN** `GroundLinkHealthProvider` reads a ground-link observation
- **THEN** the observation type SHALL come from the OBC-owned ground-link
  runtime contract
- **AND** simulator/backend code SHALL only implement and populate that
  contract
- **AND** the provider-facing observation contract SHALL NOT expose raw CSP
  target-node identity

### Requirement: CommController Has No Driver Type Coupling

The comm subsystem SHALL keep `CommController` policy evaluation dependent on
the ground-link health provider rather than on direct `GroundLinkDriver`
pointers, and the COMM-owned overflow-adjacent observability surface SHALL stay
limited to owner, pending-owner, and reject-count runtime state rather than
copying `ComCcsds` queue internals into `CommController`.

#### Scenario: COMM policy evaluates provider state only

- **WHEN** `CommController` is configured for runtime operation
- **THEN** its ground-link policy dependency SHALL be the health provider
  surface
- **AND** it SHALL NOT store direct S-band or UHF `GroundLinkDriver` pointers
- **AND** queue depth, queue overflow, and other `ComCcsds` queue-owned fields
  SHALL remain outside the `CommController` runtime contract

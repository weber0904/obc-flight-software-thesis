## MODIFIED Requirements

### Requirement: Mission History Path Roles
The onboard state data system SHALL treat official F' HK `.fdp` products as the
only active HK/state mission-history path, SHALL treat command responses plus
bounded telemetry, events, and live beacon as nominal live operational
visibility rather than stored history, and SHALL keep logs, journal output,
captures, `.fdp` staging or file-store artifacts, and hosted status dumps as
diagnostic or operator-review surfaces unless a later governed requirement
promotes them.

#### Scenario: Data path roles remain separate
- **WHEN** onboard state data is produced during hosted runtime
- **THEN** command responses, telemetry, and events SHALL provide immediate
  operator visibility
- **AND** live beacon SHALL remain a no-ACK current-health broadcast
- **AND** official HK `.fdp` files SHALL be the only active stored HK/state
  mission-history target
- **AND** the runtime SHALL NOT require `runtime/hk`, `hk/index.csv`, or
  `HK_*` command surfaces

#### Scenario: Diagnostic captures do not become nominal downlinked truth
- **WHEN** logs, journal excerpts, beacon capture files, hosted status dumps,
  or ad hoc local inspection artifacts are used during verification
- **THEN** those artifacts SHALL remain diagnostic or review-support surfaces
- **AND** the repository SHALL NOT describe them as the nominal stored-history
  or flight-like live-downlink truth of the current baseline

#### Scenario: Live beacon wire format is unchanged
- **WHEN** storage health adds `data-products` root visibility
- **THEN** the live beacon wire format SHALL remain unchanged
- **AND** any `data-products` warning or degraded condition SHALL be reflected
  only through the existing storage masks

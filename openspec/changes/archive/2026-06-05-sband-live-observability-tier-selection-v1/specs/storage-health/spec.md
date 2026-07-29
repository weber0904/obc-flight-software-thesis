## ADDED Requirements

### Requirement: Storage Status Readback Is Fresh-By-Default

The storage-health capability SHALL treat `STORAGE_GET_STATUS` as the fresh
operator readback command and SHALL NOT keep a second operator-visible scan
command that duplicates the same fresh readback semantics.

#### Scenario: Storage status command performs a fresh scan
- **WHEN** an operator runs `STORAGE_GET_STATUS`
- **THEN** `StorageHealthBridge` SHALL perform a fresh storage scan before
  replying
- **AND** it SHALL make the detailed storage-root telemetry reviewable as
  bounded readback for that command.

#### Scenario: Redundant scan-now command is retired
- **WHEN** the current baseline defines operator-facing storage status commands
- **THEN** it SHALL keep `STORAGE_GET_STATUS` as the fresh readback surface
- **AND** it SHALL retire `STORAGE_SCAN_NOW` from the current command
  contract, authority catalog, and operator documentation.

### Requirement: Scheduled Storage Visibility Is Summary-Oriented

The storage-health capability SHALL keep scheduled storage refresh focused on
cache maintenance, selected summary telemetry, and threshold/failure events
rather than broad repeated detailed root telemetry as live baseline truth.

#### Scenario: Scheduled storage scan keeps summary and critical events
- **WHEN** `StorageHealthBridge` performs its scheduled refresh
- **THEN** it SHALL keep only selected warning/degraded/quota summary telemetry
  as baseline live visibility
- **AND** it SHALL keep root-missing, scan-failed, and warning-threshold
  events reviewable.

## ADDED Requirements

### Requirement: Storage Cache Maintenance And Operator Readback Stay Distinct

The current resource-storage baseline SHALL distinguish scheduled storage cache
maintenance from explicit operator readback and SHALL keep the latter
fresh-by-default.

#### Scenario: Scheduled storage refresh remains background maintenance
- **WHEN** current runtime-root storage visibility is described
- **THEN** the baseline SHALL allow `StorageHealthBridge` to keep a scheduled
  scan cadence for onboard cache maintenance, reduced-state consumers, and
  threshold detection
- **AND** it SHALL NOT restate that background cadence as the operator's only
  status-readback mechanism.

#### Scenario: Operator storage readback is explicitly fresh
- **WHEN** the current baseline describes operator storage status behavior
- **THEN** it SHALL identify `STORAGE_GET_STATUS` as the fresh scan readback
  command
- **AND** it SHALL NOT require a separate current `STORAGE_SCAN_NOW` operator
  command to obtain a fresh scan.

## ADDED Requirements

### Requirement: Scheduled GPS Live Visibility Is Summary-Oriented

The GPS subsystem SHALL keep scheduled current live observability summary-only
while preserving fresh detailed bounded readback on explicit GPS state
requests.

#### Scenario: Scheduled GPS refresh keeps fix summary and critical transitions
- **WHEN** `GpsBridge` performs a scheduled sentence poll
- **THEN** it SHALL keep only source-mode, sample-validity, fix-validity, and
  satellite-count telemetry as baseline live summary
- **AND** it SHALL keep fix-acquired, fix-lost, parse-error, and source-error
  events reviewable.

#### Scenario: Explicit GPS state readback remains fresh and detailed
- **WHEN** `GPS_GET_STATE` performs a fresh GPS poll
- **THEN** the subsystem SHALL apply the update through the same state logic
  used for scheduled refresh
- **AND** it SHALL make the detailed GPS telemetry reviewable as bounded
  readback for that explicit command.

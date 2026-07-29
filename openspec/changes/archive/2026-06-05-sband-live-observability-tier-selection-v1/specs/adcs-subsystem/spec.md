## ADDED Requirements

### Requirement: Scheduled ADCS Live Visibility Is Summary-Oriented

The ADCS subsystem SHALL keep scheduled current live observability summary-only
while preserving fresh detailed bounded readback on explicit ADCS state or
control commands.

#### Scenario: Scheduled ADCS refresh keeps summary telemetry and transition events
- **WHEN** `AdcsBridge` performs a scheduled ADCS state refresh
- **THEN** it SHALL keep only the selected ADCS summary telemetry as baseline
  live visibility
- **AND** it SHALL keep mode-change, detumble-complete, pointing-acquired,
  sensor-fault, and comm-error events reviewable.

#### Scenario: Explicit ADCS readback remains fresh and detailed
- **WHEN** `ADCS_GET_ATTITUDE`, `ADCS_SET_MODE`, `ADCS_SET_TARGET`, or
  `ADCS_CALIBRATE` obtains a fresh ADCS reply
- **THEN** the subsystem SHALL update the owned ADCS cache and transition
  logic through the normal apply path
- **AND** it SHALL make the detailed ADCS telemetry reviewable as bounded
  readback for that explicit interaction.

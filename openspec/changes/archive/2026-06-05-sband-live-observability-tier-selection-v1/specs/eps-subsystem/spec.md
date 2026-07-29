## ADDED Requirements

### Requirement: Scheduled EPS Live Visibility Is Summary-Oriented

The EPS subsystem SHALL keep scheduled current live observability summary-only
while preserving fresh detailed bounded readback on explicit EPS status or
control commands.

#### Scenario: Scheduled EPS refresh keeps only summary telemetry live
- **WHEN** `EpsBridge` performs a scheduled status refresh
- **THEN** it SHALL keep only the selected EPS summary telemetry as baseline
  live visibility
- **AND** it SHALL keep low-battery, critical-battery, over-temperature,
  PDU-change, and comm-error events reviewable on that same path.

#### Scenario: Explicit EPS readback remains fresh and detailed
- **WHEN** `EPS_GET_STATUS`, `EPS_SET_PDU`, `EPS_SET_HEATER`, or `EPS_RESET`
  obtains a fresh EPS reply
- **THEN** the subsystem SHALL update the owned EPS cache and threshold/latch
  state through the normal apply path
- **AND** it SHALL make the detailed EPS telemetry reviewable as bounded
  readback for that explicit interaction.

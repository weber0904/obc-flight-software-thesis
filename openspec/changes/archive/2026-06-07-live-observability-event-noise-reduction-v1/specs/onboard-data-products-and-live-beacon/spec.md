## ADDED Requirements

### Requirement: Reduced-State Update Event Is Mask-Change-Oriented

The onboard data-products and live-beacon capability SHALL treat
`STATE_MONITOR_UPDATED` as a reduced-state mask-transition event rather than a
periodic heartbeat.

#### Scenario: Successful reduction without mask change stays event-quiet
- **WHEN** `OnboardStateMonitor` produces a new successful reduced-state sample
  whose `healthMask`, `faultMask`, and `qualityMask` match the previous
  successful reduced-state sample
- **THEN** it SHALL continue updating its cached reduced state and telemetry
- **AND** it SHALL NOT emit `STATE_MONITOR_UPDATED`.

#### Scenario: Successful reduction with mask change emits one update event
- **WHEN** `OnboardStateMonitor` produces a new successful reduced-state sample
  whose `healthMask`, `faultMask`, or `qualityMask` differs from the previous
  successful reduced-state sample
- **THEN** it SHALL emit `STATE_MONITOR_UPDATED` with the new three mask
  values.

### Requirement: HK Product Success Event Remains The Operator-Facing Product Completion Surface

The onboard data-products and live-beacon capability SHALL keep
`HK_TREND_PRODUCT_WRITTEN` as the current operator-facing HK product success
event and SHALL NOT require low-level writer success events to remain on the
default packetized ground event surface.

#### Scenario: HK product success remains visible even when writer success is packet-quiet
- **WHEN** the HK trend producer successfully emits an official HK data
  product
- **THEN** `HK_TREND_PRODUCT_WRITTEN` SHALL remain part of the current
  packetized operator event surface
- **AND** lower-level `DpWriter.FileWritten` success visibility MAY remain
  local/debug-only.

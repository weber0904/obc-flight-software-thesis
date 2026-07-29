## ADDED Requirements

### Requirement: TTC Auto-Entry Can Trigger ADCS Pointing

The ADCS subsystem SHALL support a best-effort internal runtime mode-set path
that `TtcPassManager` can use to request `POINTING` when TTC auto-entry occurs.

#### Scenario: TTC entry requests ADCS pointing through existing runtime mode surface
- **WHEN** `TtcPassManager` completes a TTC auto-entry request on the active
  runtime
- **THEN** it SHALL be able to request ADCS mode `POINTING` through the
  existing runtime mode-set path owned by `AdcsBridge`

#### Scenario: ADCS request failure does not redefine TTC entry success
- **WHEN** the TTC-triggered ADCS `POINTING` request cannot be completed
- **THEN** the active runtime SHALL keep TTC entry ownership and verdict on the
  TTC policy path
- **AND** the ADCS failure SHALL remain reviewable as a bounded warning or
  degraded condition rather than forcing TTC entry failure by itself

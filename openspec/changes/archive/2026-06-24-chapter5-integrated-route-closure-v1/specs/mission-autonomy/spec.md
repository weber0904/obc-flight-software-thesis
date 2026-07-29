## ADDED Requirements

### Requirement: TTC Policy Owns One-Way ADCS Entry Trigger Only

The mission-autonomy capability SHALL allow `TtcPassManager` to issue one
best-effort internal ADCS entry trigger on TTC auto-entry while keeping TTC
pass policy separate from broader ADCS or mission-execution ownership.

#### Scenario: TTC auto-entry sends one ADCS entry trigger
- **WHEN** `TtcPassManager` transitions from non-`TTC` into `TTC` through its
  pass-window policy path
- **THEN** it SHALL send at most one internal ADCS entry trigger for that TTC
  entry edge
- **AND** repeated scheduler ticks while already in `TTC` SHALL NOT resend that
  trigger

#### Scenario: ADCS failure does not block TTC policy entry
- **WHEN** `TtcPassManager` determines that TTC entry should occur
- **AND** the ADCS entry trigger fails
- **THEN** `TtcPassManager` SHALL still request TTC through its owned internal
  mode-control path
- **AND** it SHALL keep the ADCS failure as best-effort observability only

#### Scenario: TTC exit does not restore prior ADCS mode in this slice
- **WHEN** `TtcPassManager` later exits `TTC`
- **THEN** it SHALL NOT automatically restore a previous ADCS mode as part of
  this change
- **AND** any later ADCS restore policy SHALL require a separate governed
  design

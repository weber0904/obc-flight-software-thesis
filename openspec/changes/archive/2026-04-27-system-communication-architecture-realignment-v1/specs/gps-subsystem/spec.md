## ADDED Requirements

### Requirement: GPS Direct OBC UART Baseline Remains The Primary Hardware Direction
After the first governed live GPS UART slice, the GPS subsystem SHALL continue to treat direct OBC-attached UART as the primary hardware baseline unless a later governed architecture change explicitly redefines GPS as something else.

#### Scenario: Future GPS work extends the direct OBC baseline by default
- **WHEN** a later change extends live GPS behavior beyond the first UART slice
- **THEN** that change SHALL build on the existing direct OBC UART path instead of implicitly moving GPS into the CSP subsystem baseline

### Requirement: GPS Remains Outside The Planned Shared Subsystem CAN FD Bus
The GPS subsystem SHALL remain outside the planned shared `CAN FD` bus direction for `EPS`, `ADCS`, and `COMM` unless a later governed change explicitly redefines GPS architecture.

#### Scenario: Shared CAN FD migration does not implicitly absorb GPS
- **WHEN** the repository migrates subsystem traffic toward shared `CAN FD`
- **THEN** GPS SHALL remain a separate direct sensor path unless a later governed architecture change explicitly states otherwise

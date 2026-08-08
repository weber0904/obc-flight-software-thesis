## ADDED Requirements

### Requirement: Lab Serial Acquisition For Subsystem-Origin Bytes
The comm subsystem SHALL provide a governed focused probe that validates bounded subsystem-origin serial acquisition from `subsystem.local` to a passive macOS receiver over the physical lab serial wiring.

#### Scenario: Passive macOS receiver acquires subsystem-origin frames
- **WHEN** macOS opens the explicit host serial endpoint before `subsystem.local` transmits
- **AND** `subsystem.local` sends the governed preamble and bounded acquisition frames over `/dev/serial0`
- **THEN** the macOS receiver SHALL extract the exact framed payload sequence from the serial stream

#### Scenario: Acquisition proof stays below TT&C
- **WHEN** the lab serial acquisition probe passes
- **THEN** the repository SHALL describe the result as subsystem-origin serial acquisition only
- **AND** it SHALL NOT describe that result as gateway-backed TT&C, RF, file/downlink, target OBC, or COMM shared CAN FD validation

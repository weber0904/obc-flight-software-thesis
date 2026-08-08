## ADDED Requirements

### Requirement: Lab Serial Acquisition Evidence
The verification evidence tree SHALL record the commands, endpoints, baudrate, acquisition framing settings, observed decoded frames, raw byte counts, and final verdict for the subsystem-origin lab serial acquisition probe.

#### Scenario: Acquisition evidence is reviewable
- **WHEN** the lab serial acquisition probe completes
- **THEN** reviewers SHALL be able to inspect the macOS receiver command, subsystem sender command, selected serial devices, baudrate, frame count, decoded payload count, and any raw-byte diagnostic summary

#### Scenario: Failed TT&C attempt remains diagnostic
- **WHEN** the acquisition change records the prior guarded TT&C attempt
- **THEN** the evidence SHALL label that attempt as diagnostic context only
- **AND** it SHALL NOT register or imply a successful physical lab serial TT&C path

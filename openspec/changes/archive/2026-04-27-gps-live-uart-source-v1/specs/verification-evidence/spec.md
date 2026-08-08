## MODIFIED Requirements

### Requirement: Live GPS UART Evidence Is Reviewable
The verification evidence tree SHALL record the commands, target serial device, baudrate, observed source mode, observed sentence-ingestion state change, and final verdict for the governed `obc.local` live GPS UART path.

#### Scenario: First hardware GPS proof stays bounded
- **WHEN** the governed live GPS probe passes
- **THEN** the evidence SHALL show that real hardware sentences entered `GpsBridge`, advanced accepted-sentence state, and updated cached runtime fields without claiming live-sky fix quality, PPS behavior, RF behavior, or comm migration coverage

### Requirement: Historical OBC-Side Serial Comm Evidence Is Not Reused As Current GPS Or Comm Baseline
When the repository realigns `obc.local:/dev/serial0` to GPS, the evidence tree SHALL keep the older OBC-side serial comm records as historical proof and SHALL not reuse them as if they still represent the current target baseline.

#### Scenario: New GPS evidence does not erase old comm history
- **WHEN** the new GPS live UART evidence is added
- **THEN** reviewers SHALL still be able to inspect the old comm evidence, but the new record and related docs SHALL state that those OBC-side serial comm results are historical rather than the active target baseline

## MODIFIED Requirements

### Requirement: Live OBC GPS UART Path Is Registered Separately
The verification-path registry SHALL include a distinct entry for the governed `GY-GPS6MV2 -> obc.local:/dev/serial0 -> GpsBridge` live UART path.

#### Scenario: GPS live UART registry entry stays separate from direct GDS and historical comm paths
- **WHEN** a later change needs to cite target-side live GPS behavior
- **THEN** the registry SHALL direct it to the governed GPS live UART entry rather than to direct `GDS -> TCP -> OBC` records or the older OBC-side serial comm entries

### Requirement: Historical OBC-Side Comm UART Entries Remain Historical
After `obc.local:/dev/serial0` is reassigned to GPS, the verification-path registry SHALL preserve the old OBC-side serial comm entries as historical paths and SHALL describe active comm development as proceeding through other governed paths until a later hardware migration change re-establishes a new target baseline.

#### Scenario: Reviewers can distinguish current GPS ownership from historical comm ownership
- **WHEN** reviewers compare current GPS target validation against older OBC-side serial comm validation
- **THEN** the registry SHALL make clear which path is current and which is historical

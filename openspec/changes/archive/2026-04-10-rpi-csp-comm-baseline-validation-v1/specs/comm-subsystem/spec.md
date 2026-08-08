## ADDED Requirements

### Requirement: Raspberry Pi Comm UART Is Included In Libcsp Base Target Readiness
The comm subsystem SHALL include the existing Raspberry Pi `/dev/serial0` external comm path in the target-side libcsp-base readiness validation, while keeping that UART path separate from the internal CSP network and from the GDS ground path.

#### Scenario: Target comm validation proves external UART only
- **WHEN** the Raspberry Pi CSP + comm baseline probe passes
- **THEN** reviewers SHALL be able to see that `CommController`, `RadioController`, and `UartDriver` exchanged data over the explicit target UART device against a host-side mock peer
- **AND** the evidence SHALL NOT describe that UART path as internal CSP, GDS, GPS, RF, or vendor control-plane validation

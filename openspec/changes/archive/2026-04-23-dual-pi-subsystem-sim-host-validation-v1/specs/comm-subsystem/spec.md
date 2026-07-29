## ADDED Requirements

### Requirement: Dual-Pi Split-Host Comm Coexistence Path Is Governed
The comm subsystem SHALL provide a governed validation path where `obc.local` runs the external comm stack on `/dev/serial0` while the split-host internal CSP topology continues to use `macOS` as the CSP hub host and `subsystem.local` as the EPS/ADCS simulator host.

#### Scenario: External comm coexists with split-host subsystem traffic
- **WHEN** the repository runs the governed dual-Pi coexistence probe
- **THEN** `CommController`, `RadioController`, and `UartDriver` SHALL be able to exchange data over `/dev/serial0` against a macOS host-side mock peer while the OBC can still reach remote EPS node `2` and ADCS node `3` through the split-host CSP topology

#### Scenario: Coexistence proof keeps adjacent claims separate
- **WHEN** the governed dual-Pi coexistence probe passes
- **THEN** the result SHALL prove only the coexistence of external comm with the split-host subsystem simulator path and SHALL NOT be described as RF validation, vendor control-plane validation, or future physical-bus equivalence

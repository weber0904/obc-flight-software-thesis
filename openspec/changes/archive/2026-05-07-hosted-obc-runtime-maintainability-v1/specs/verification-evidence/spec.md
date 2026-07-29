## ADDED Requirements

### Requirement: Runtime Maintainability Evidence Proves Behavior Preservation
The verification evidence SHALL record reviewable local evidence for hosted runtime maintainability refactors, including focused helper tests, build coverage, reused hosted probe coverage, OpenSpec validation, and explicit out-of-scope boundaries.

#### Scenario: Helper tests cover extracted dispatch behavior
- **WHEN** hosted command parsing or dispatch logic is extracted into helper code
- **THEN** the change SHALL include focused helper tests for command routing, stop commands, unknown commands, parser failure messages, help text, and launch argument validation

#### Scenario: Existing hosted paths are rerun as behavior-preservation evidence
- **WHEN** the runtime refactor is complete
- **THEN** repository-owned hosted probes SHALL be rerun after a fresh build for the relevant default hosted runtime and CCSDS spike runtime behavior
- **AND** the evidence SHALL identify those probes as reused behavior-preservation paths rather than newly proven validation paths

#### Scenario: Evidence avoids new path claims
- **WHEN** runtime maintainability evidence is recorded
- **THEN** it SHALL state that the refactor does not create new direct-GDS, S-band-through-COMM, UHF serial backup, CCSDS adoption, target/Pi, RF, reliable-transfer, command-authority, failover, or pass-scheduler claims
- **AND** it SHALL cite existing registry entries only for the paths actually reused during verification

## ADDED Requirements

### Requirement: Hosted COMM Simulator Has A Model Layer
The comm subsystem SHALL implement the hosted `COMM` CSP stand-in as a simulator model plus server shell, where the model owns business state and behavior while the server shell owns libcsp and serial I/O integration.

#### Scenario: COMM model preserves existing CSP service contract
- **WHEN** the hosted `COMM` node handles `UPLINK_POLL`, `DOWNLINK_WRITE`, or `LINK_STATUS`
- **THEN** the node SHALL preserve the existing service ports `30`, `31`, and `32` and their request/reply wire layouts
- **AND** the simulator model SHALL provide the service semantics without adding a new CSP debug or status service in this change

#### Scenario: COMM model tracks bounded uplink state
- **WHEN** serial ingress provides uplink bytes to the hosted `COMM` simulator
- **THEN** the simulator model SHALL store those bytes in a bounded queue with deterministic drop-new overflow behavior
- **AND** overflow SHALL preserve already queued bytes while incrementing reviewable model status counters

#### Scenario: COMM model tracks downlink acceptance state
- **WHEN** OBC writes a bounded downlink chunk through the hosted `COMM` CSP service
- **THEN** the simulator model SHALL accept the chunk only when the link is available and the model is not applying downlink backpressure or injected error state
- **AND** rejected downlink writes SHALL map to the existing CSP result vocabulary rather than changing the wire contract

#### Scenario: Model status remains internal for the first foundation slice
- **WHEN** tests or future hosted simulator code inspect `COMM` simulator state
- **THEN** queue depth, overflow, backpressure, disconnect, and error-injection state SHALL be available through C++ model-facing status APIs
- **AND** the existing `LINK_STATUS` wire reply SHALL remain compatible with the previously validated gateway-backed path

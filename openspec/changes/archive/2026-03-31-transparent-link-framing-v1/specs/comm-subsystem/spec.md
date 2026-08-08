## ADDED Requirements

### Requirement: Transparent Link Framing Above UART Transport
The comm subsystem SHALL provide a repository-owned transparent link framing mode above the existing serial byte-stream transport so that the legacy transparent-UART path can exchange binary-safe framed payloads without replacing the current TCP/GDS baseline or `mock-text` radio baseline.

#### Scenario: Framed transparent mode reuses the current serial link
- **WHEN** the project selects the framed transparent-UART validation path
- **THEN** the OBC SHALL reuse the existing serial transport and SHALL add framing above that transport rather than redefining the hardware link itself

#### Scenario: Direct TCP ground baseline remains unchanged
- **WHEN** the project runs the direct `fprime-gds` integration path
- **THEN** that path SHALL continue to use the existing documented TCP connection and SHALL NOT depend on the transparent framing mode

### Requirement: Framed Transparent Path Is Binary-Safe
The transparent link framing mode SHALL support payload bytes that include delimiter and escape values, SHALL define explicit frame boundaries, and SHALL detect payload corruption using CRC-32.

#### Scenario: Payload includes reserved bytes
- **WHEN** the framed transparent path carries payload bytes that include the framing delimiter, escape byte, or `0x00`
- **THEN** the link layer SHALL escape and recover those bytes without truncation or ambiguity

#### Scenario: Corrupted frame is rejected
- **WHEN** the host or target receives a framed payload whose CRC-32 does not match
- **THEN** the frame SHALL be rejected and the exchange SHALL report a framing or transport error instead of silently accepting corrupted data

### Requirement: Host Transparent Peer Can Deframe Governed Frames
The host-side transparent peer SHALL support a governed mode that deframes the repository-owned transparent link format, validates the frame, and returns a framed response over the same serial link.

#### Scenario: Framed transparent peer echoes decoded payload
- **WHEN** the host-side peer receives a valid framed payload
- **THEN** it SHALL decode the payload, preserve the recovered bytes, and return a framed response using the same link format

## ADDED Requirements

### Requirement: Framed Transparent Path Supports Sustained Binary-Safe Exchange
The comm subsystem SHALL provide a governed validation path where the repository-owned transparent frame v1 carries repeated binary-safe payloads over the existing Raspberry Pi to host UART/RS485 path without replacing the current direct TCP/GDS or `mock-text` baselines.

#### Scenario: Repeated framed payloads remain reviewable
- **WHEN** the project runs the governed framed transparent robustness probe
- **THEN** the host and target SHALL be able to exchange multiple framed payloads, including representative binary-safe payloads, over the same serial session and expose a reviewable success summary

### Requirement: Framed Transparent Path Recovers After Host-Peer Restart
The comm subsystem SHALL provide a governed validation path where the framed transparent UART flow detects a host-side peer interruption, allows the peer to be restarted, and supports a subsequent successful framed exchange without redefining the frame format or replacing the existing serial transport contract.

#### Scenario: Framed exchange succeeds after peer restart
- **WHEN** the host-side framed transparent peer is interrupted and then restarted during governed validation
- **THEN** a later framed exchange over the same governed hardware path SHALL succeed and SHALL preserve the same frame v1 encode/decode behavior

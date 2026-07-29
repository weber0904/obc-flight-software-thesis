## MODIFIED Requirements

### Requirement: Verification Baseline Records Physical-Bus Scope Carefully
When the repository adds a physical internal CSP carrier proof, the governing evidence SHALL distinguish bus capability, logical-node behavior, and physical-controller limits instead of collapsing those into one claim.

#### Scenario: First SocketCAN evidence distinguishes bus capability from frame usage
- **WHEN** the first governed CAN FD-capable SocketCAN evidence is recorded
- **THEN** that evidence SHALL distinguish between the bus being configured as CAN FD-capable and the actual observed libcsp frame behavior on the wire

#### Scenario: First SocketCAN evidence records CAN health and mapping
- **WHEN** the first governed CAN FD-capable SocketCAN evidence is recorded
- **THEN** it SHALL include `parentdev` mapping, active/reserved channel identification, pre/post CAN statistics, and enough traffic capture or idle capture to support both the active-bus path and reserved-channel isolation claims

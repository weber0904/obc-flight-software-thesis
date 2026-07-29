## MODIFIED Requirements

### Requirement: Comm Participates In The Future CSP Subsystem Topology
The future comm architecture SHALL allow `COMM` to participate as a CSP-facing subsystem alongside `EPS` and `ADCS`, and the first governed implementation SHALL assign `COMM` to node `4` with reserved COMM-owned application service ports `30` through `39`.

#### Scenario: Deployed COMM clients use owner-managed runtime access
- **WHEN** `CommController`, `GroundLinkDriver`, or `CspBridge` uses the CSP client path in the deployed topology
- **THEN** those COMM-facing runtime calls SHALL execute through the topology-owned runtime owner boundary
- **AND** the existing COMM public commands, events, telemetry, and service-port contracts SHALL remain unchanged

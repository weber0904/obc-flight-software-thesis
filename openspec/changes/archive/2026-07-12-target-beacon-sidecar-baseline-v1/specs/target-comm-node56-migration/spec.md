## ADDED Requirements

### Requirement: Target UHF Beacon Egress Is A Baseline-Owned Auxiliary Path

The governed target baseline SHALL configure UHF node-`6` Beacon egress as an
auxiliary A-owned sidecar path while preserving node-`6` serial ingress and
authority-role semantics.

#### Scenario: Auxiliary Beacon egress preserves UHF ingress ownership
- **WHEN** A enables or repairs the target Beacon sidecar
- **THEN** it SHALL configure the UHF service's Beacon output separately from
  its configured physical serial ingress and shared SocketCAN carrier
- **AND** it SHALL keep `uhf-backup` and `uhf-primary-after-failover` authority
  semantics unchanged.

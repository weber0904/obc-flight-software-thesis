## ADDED Requirements

### Requirement: Hosted Node-5 Observability-Governance Proof Is Registered As A Distinct S-band Path

The verification-path registry SHALL register the hosted node-`5` S-band
observability-governance proof as a path distinct from UHF quiet, UHF beacon
suppress, and unrelated telemetry-heavy adjunct records.

#### Scenario: Registry identifies hosted node-5 auth-gated observability boundary
- **WHEN** the hosted node-`5` observability-governance proof passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> internal CSP -> hosted OBC CommController -> CommEgressMux S-band live-packet gate`
- **AND** it SHALL state that the proof covers pre-auth packet quiet,
  post-auth live packet visibility, bounded `GET_*` summary readback, and
  session-close suppression on the maintained hosted node-`5` path.

### Requirement: Target Node-5 Observability-Governance Proof Is Registered As A Distinct S-band Path

The verification-path registry SHALL register the service-managed target
node-`5` S-band observability-governance proof as a path distinct from quiet
node-`6`, target beacon suppress, and target non-quiet UHF diagnosis records.

#### Scenario: Registry identifies target node-5 auth-gated observability boundary
- **WHEN** the target node-`5` observability-governance proof passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> subsystem.local sband_comm_csp_node(node 5) -> shared SocketCAN -> obc.local OBC CommController -> CommEgressMux S-band live-packet gate`
- **AND** it SHALL state that the proof covers pre-auth packet quiet,
  post-auth live packet visibility, bounded `GET_*` summary readback, and
  session-close suppression on the maintained target node-`5` path.

#### Scenario: Registry keeps node-5 observability-governance scope bounded
- **WHEN** reviewers inspect either new node-`5` observability-governance entry
- **THEN** the entry SHALL keep its scope bounded to observability governance on
  the maintained S-band path
- **AND** it SHALL NOT widen that result into UHF command-paced summary
  closure, beacon suppress proof, generic telemetry-schema redesign, or
  one-gateway simultaneous aggregation.

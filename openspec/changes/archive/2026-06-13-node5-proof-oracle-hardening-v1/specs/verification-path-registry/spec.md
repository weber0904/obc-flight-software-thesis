## ADDED Requirements

### Requirement: Registry Keeps Node-5 Observability Proof Packet-Path Oracles Explicit
The verification-path registry SHALL describe the maintained hosted and target
node-`5` observability-governance entries as packet-path proofs whose bounded
readback and switch-close checkpoints stay packet-path grounded, not just
passive observer silence.

#### Scenario: Hosted node-5 observability entry states bounded readback and close semantics
- **WHEN** reviewers inspect the maintained hosted node-`5`
  observability-governance entry after the oracle-hardening follow-up
- **THEN** the entry SHALL state that bounded detailed `GET_*` requalification
  stays tied to a bounded command-specific packet-path artifact
- **AND** session-close suppression SHALL still keep gateway/downlink capture
  quiet reviewable alongside any passive observer surfaces.

#### Scenario: Target node-5 observability entry states bounded readback and close semantics
- **WHEN** reviewers inspect the maintained target node-`5`
  observability-governance entry after the oracle-hardening follow-up
- **THEN** the entry SHALL state that bounded detailed `GET_*` requalification
  stays tied to a bounded command-specific packet-path artifact
- **AND** session-close suppression SHALL still keep gateway/downlink capture
  quiet reviewable alongside any passive observer surfaces.

### Requirement: Registry Keeps Target Secure-Auth Path Identity While Allowing Source-Aware Handshake Observation
The verification-path registry SHALL keep the maintained target secure-auth path
identity unchanged when the proof oracle must recover from handshake-source
drift between wire capture and native packet logs.

#### Scenario: Target secure-auth entry keeps source-aware oracle bounded
- **WHEN** reviewers inspect the maintained target secure-auth entry after the
  oracle-hardening follow-up
- **THEN** the entry SHALL allow source-aware handshake confirmation across the
  existing wire-capture and native packet-log surfaces
- **AND** it SHALL keep that result bounded to the existing target secure-auth
  path instead of describing it as a new target path or new command plane.

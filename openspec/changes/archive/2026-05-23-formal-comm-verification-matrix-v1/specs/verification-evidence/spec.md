## ADDED Requirements

### Requirement: Formal Comm Verification Matrix Evidence Is Reviewable

The verification evidence tree SHALL preserve reviewable matrix-oriented
evidence for `formal-comm-verification-matrix-v1`, including the environment
run command, per-case verdicts, carrier-kind metadata, artifact roots, and any
explicit blocker classification.

#### Scenario: Hosted, target TCP, and target CAN matrix outputs are reviewable
- **WHEN** `formal-comm-verification-matrix-v1` records evidence
- **THEN** reviewers SHALL be able to inspect a machine-readable summary and a
  human-readable summary for each of the three governed environments
- **AND** each summary SHALL identify the nine formal communication cases
  explicitly rather than relying on historical script names alone

#### Scenario: Carrier provenance remains explicit in matrix evidence
- **WHEN** a matrix case records UHF or S-band evidence
- **THEN** the evidence SHALL distinguish at least direct control, hosted
  serial stand-in, target TCP southbound, and target physical UHF UART carrier
  kinds where applicable
- **AND** it SHALL NOT describe target TCP UHF proof as physical UART closure

#### Scenario: Failed or blocked cells stay truthful
- **WHEN** a matrix case fails or remains blocked
- **THEN** the evidence SHALL record that case as failed or blocked with a
  bounded blocker classification
- **AND** it SHALL NOT promote the corresponding verification-path claim into
  the registry until passing governed evidence exists

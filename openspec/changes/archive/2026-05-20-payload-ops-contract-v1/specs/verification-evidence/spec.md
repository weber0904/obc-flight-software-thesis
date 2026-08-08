## ADDED Requirements

### Requirement: Payload Operation Evidence Is Split By Contract And Hardware Truth

The first payload operation slice SHALL record separate hosted contract proof
and Raspberry Pi target camera proof.

#### Scenario: Hosted proof is explicit about its limit

- **WHEN** the repository records hosted payload verification for this change
- **THEN** the evidence SHALL state that the hosted path proves the governed
  payload contract, sequencing integration, proxy power semantics, and storage
  boundary
- **AND** it SHALL state that hosted proof does not prove real `libcamera`
  sensor interaction

#### Scenario: Target proof is explicit about the real camera path

- **WHEN** the repository records Raspberry Pi target verification for this
  change
- **THEN** the evidence SHALL state the actual camera hardware path, the target
  runtime root, the real capture output, and the proxy EPS channel behavior

### Requirement: Closeout-Ready Status Requires Target Payload Proof

The change SHALL not be treated as closeout-ready on hosted proof alone.

#### Scenario: Missing target proof remains a bounded verification gap

- **WHEN** the change has hosted contract proof but lacks Raspberry Pi target
  camera proof
- **THEN** the evidence SHALL record that gap honestly using the repository's
  constrained-validation language
- **AND** the change SHALL NOT claim full closeout-ready payload verification

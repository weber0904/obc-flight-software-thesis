# verification-evidence Specification Delta

## ADDED Requirements

### Requirement: Capture Mode V2 Evidence Distinguishes Auto And Deterministic Proof

The verification evidence for payload capture modes v2 SHALL record separate
hosted proof for auto and deterministic capture behavior, capability readback,
and sidecar metadata.

#### Scenario: Hosted capture-mode proof is reviewable

- **WHEN** payload-capture-modes-v2 completes hosted verification
- **THEN** the evidence SHALL record the commands, session kind, capture
  artifact path, sidecar metadata path, and final verdict

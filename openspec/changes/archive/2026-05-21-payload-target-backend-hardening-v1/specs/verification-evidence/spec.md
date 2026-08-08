# verification-evidence Specification Delta

## ADDED Requirements

### Requirement: Target Hardening Evidence Records Real Camera And Power-Investigation Truth

The verification evidence for target backend hardening SHALL record real target
camera closure and the Raspberry Pi camera power-control investigation result.

#### Scenario: Target camera proof is explicit about the achieved boundary

- **WHEN** payload-target-backend-hardening-v1 records target proof
- **THEN** the evidence SHALL state whether the proof achieved real JPEG
  capture, real register round-trip, and any maintainable power-control path

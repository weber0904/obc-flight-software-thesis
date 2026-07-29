# verification-evidence Specification Delta

## ADDED Requirements

### Requirement: Raw Sensor Evidence Distinguishes Hosted Fake And Target Truth

The verification evidence for raw sensor register control SHALL distinguish
hosted contract proof from target real-register proof.

#### Scenario: Hosted proof does not claim real sensor effects

- **WHEN** hosted verification records raw-register command behavior
- **THEN** the evidence SHALL state that it proves only contract, authority, and
  sequence behavior

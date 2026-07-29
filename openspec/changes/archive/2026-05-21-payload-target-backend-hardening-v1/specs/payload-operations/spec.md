# payload-operations Specification Delta

## ADDED Requirements

### Requirement: Target Payload Backend Distinguishes Hosted Contract Proof From Real Camera Closure

The payload contract SHALL keep the public surface stable across hosted and
target backends while allowing target-only hardening behind the backend boundary.

#### Scenario: Target backend hardening does not change the operator contract

- **WHEN** the real target backend moves behind helper-process isolation
- **THEN** the public payload command contract SHALL remain stable across hosted
  and target paths

# platform-baseline Specification Delta

## ADDED Requirements

### Requirement: Target Camera Backend Uses Helper Isolation For Hard-Hang Recovery

The real target camera backend SHALL execute behind a helper-process boundary
when the active baseline claims bounded timeout and recovery from camera backend
stall behavior.

#### Scenario: Timeout kills the helper rather than hanging the OBC process

- **WHEN** a target camera operation exceeds its bounded deadline
- **THEN** the payload runtime SHALL be able to terminate the target helper and
  return the payload controller to a governed fault or cleanup state

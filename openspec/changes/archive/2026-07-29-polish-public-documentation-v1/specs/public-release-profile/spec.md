## ADDED Requirements

### Requirement: Portfolio Documentation Is Capability First

The portfolio-facing README and current documents SHALL explain implemented
capabilities, architecture, operation, and verification directly.

#### Scenario: Reviewer reads a current document
- **WHEN** a reader opens README, architecture, interfaces, verification,
  operator, thesis, security, or release documentation
- **THEN** the prose SHALL focus on the software and reproducible engineering
  results
- **AND** publication-process commentary SHALL remain in machine-readable
  provenance or formal change history

## MODIFIED Requirements

### Requirement: Hardware Claims Are Commit Scoped

Hardware and laboratory result records SHALL identify their execution commit,
date, environment, commands, and artifact digests. Current reader documents
SHALL link those records by capability and environment.

#### Scenario: Reviewer follows a hardware result
- **WHEN** a reviewer selects a Raspberry Pi, UART, SocketCAN, subsystem, or
  watchdog result
- **THEN** the linked evidence SHALL expose the exact execution provenance and
  observations

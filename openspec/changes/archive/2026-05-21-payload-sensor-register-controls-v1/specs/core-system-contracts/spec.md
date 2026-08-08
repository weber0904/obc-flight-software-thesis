# core-system-contracts Specification Delta

## ADDED Requirements

### Requirement: Raw Sensor Register Writes Stay On High-Authority Payload Surfaces

Raw sensor register writes SHALL remain bounded to the high-authority payload
control surface.

#### Scenario: Read-only or backup ingress cannot write registers

- **WHEN** a read-only or backup ingress surface requests a raw payload sensor
  register write
- **THEN** the system SHALL reject the request before payload execution

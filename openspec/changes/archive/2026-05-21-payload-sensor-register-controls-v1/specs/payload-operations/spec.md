# payload-operations Specification Delta

## ADDED Requirements

### Requirement: Raw Sensor Register Control Is Session-Bounded

The active payload contract SHALL expose raw sensor register control only while
the payload is in a prepared `RAW_SENSOR` session.

#### Scenario: Raw register control requires RAW_SENSOR session

- **WHEN** an operator requests a sensor register read or write outside a
  prepared `RAW_SENSOR` session
- **THEN** the payload contract SHALL reject the request

#### Scenario: Raw register control does not create a generic raw tunnel

- **WHEN** the payload contract exposes sensor register control
- **THEN** it SHALL expose register-oriented read/write semantics rather than an
  arbitrary byte-stream tunnel

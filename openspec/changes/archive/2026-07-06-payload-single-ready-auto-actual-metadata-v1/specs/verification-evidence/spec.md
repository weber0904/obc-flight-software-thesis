## MODIFIED Requirements

### Requirement: Payload evidence distinguishes request settings from actual target AUTO results

Current maintained payload target evidence SHALL separate request/controller
truth from actual camera-runtime truth.

#### Scenario: AUTO to deterministic operator workflow evidence is fresh

- **GIVEN** a governed target payload proof for the current real-camera path
- **WHEN** the proof performs one prepare, an `AUTO` capture, actual metadata
  readback, and a later `DETERMINISTIC` capture without re-prepare
- **THEN** the evidence SHALL record both:
  - the request/controller-resolved settings
  - the actual target auto result fields
- **AND** the record SHALL state that the proof demonstrates the operator
  workflow of using `AUTO` to inspect actual values before deterministic replay

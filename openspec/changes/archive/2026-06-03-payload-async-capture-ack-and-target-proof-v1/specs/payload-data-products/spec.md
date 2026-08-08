## MODIFIED Requirements

### Requirement: Canonical Publication Is Part Of Capture Success

Final payload success SHALL require canonical payload `.fdp` family
publication in addition to local JPEG creation.

#### Scenario: Final payload success remains gated on canonical publication even after async ack

- **WHEN** a payload capture request is accepted and acknowledged immediately
- **THEN** canonical payload `.fdp` family publication SHALL still determine
  the final payload success or failure result
- **AND** publication failure SHALL be surfaced on payload status/metadata
  surfaces even though the original capture command already returned success

### Requirement: Payload FDP Family Ceiling Is Bounded

The active payload data-product family SHALL use a governed default total
serialized payload-data budget of `2 MiB` while allowing multi-slice family
publication within that bound.

#### Scenario: Oversize payload capture becomes a deferred payload failure above the governed family budget

- **WHEN** the serialized canonical payload `.fdp` family data region would
  exceed `2 MiB`
- **THEN** the payload runtime SHALL reject canonical publication
- **AND** the payload completion surfaces SHALL report final failure
- **AND** payload captures that fit within that bound MAY span multiple
  official `.fdp` family members
